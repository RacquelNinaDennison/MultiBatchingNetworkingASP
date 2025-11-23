#!/usr/bin/env python3
"""
RMP in PySCIPOpt driven by ASP facts 

Usage:
  python rmp.py instances.lp --M 1e6 --scale 1000
"""
import re, sys, argparse
from collections import defaultdict, namedtuple
from pyscipopt import Model, quicksum


# regular expressions for parsing ASP encodings - this is needed for when we parse the data
FACT = re.compile(r'^\s*([a-zA-Z_][a-zA-Z0-9_]*)\((.*?)\)\.\s*$')

def parse_terms(argstr):
    out, cur, depth = [], [], 0
    for ch in argstr:
        if ch == '(':
            depth += 1; cur.append(ch)
        elif ch == ')':
            depth -= 1; cur.append(ch)
        elif ch == ',' and depth == 0:
                out.append(''.join(cur).strip()); cur = []
        else:
            cur.append(ch)
    if cur: out.append(''.join(cur).strip())
    return out

def atom(line):
    m = FACT.match(line)
    if not m: return None
    return m.group(1), [t.strip() for t in parse_terms(m.group(2))]

def sym(x):
    x = str(x)
    if len(x)>=2 and ((x[0]=="'" and x[-1]=="'") or (x[0]=='"' and x[-1]=='"')):
        return x[1:-1]
    return x

def as_num(x):
    try:
        if isinstance(x,str) and ('.' in x or 'e' in x.lower()):
            return float(x)
        return int(x)
    except Exception:
        return None

def read_facts(path):
    # set of locations
    locations = set()
    # set of products 
    products  = set()
    offer = defaultdict(lambda: defaultdict(int))
    demand = defaultdict(lambda: defaultdict(int))
    # list of tuples
    routes = []  # (from,to,tr,dist)

    with open(path,'r') as f:
        for raw in f:
            s = raw.strip()
            if not s or s.startswith('%'): continue
            a = atom(s)
            if not a: continue
            pred, t = a
            print(pred)
            print(t)
            if pred == 'location' and len(t)==1:
                locations.add(sym(t[0]))
            elif pred == 'part' and len(t)==1:
                products.add(sym(t[0]))
            elif pred == 'offer' and len(t)==3:
                p,l,q = sym(t[0]), sym(t[1]), as_num(t[2])
                offer[p][l] += q; locations.add(l); products.add(p)
            elif pred == 'demand' and len(t)==3:
                p,l,q = sym(t[0]), sym(t[1]), as_num(t[2])
                demand[p][l] += q; locations.add(l); products.add(p)
            elif pred == 'route' and len(t)==5:
                frm,to,tr,dist,_ = sym(t[0]), sym(t[1]), sym(t[2]), as_num(t[3]), as_num(t[4])
                locations.add(frm); locations.add(to)
                routes.append((frm,to,tr,dist))

    # deduplicate per (from,to,tr) by min distance
    best = {}
    for frm,to,tr,dist in routes:
        key=(frm,to,tr)
        if key not in best or dist < best[key][3]:
            best[key]=(frm,to,tr,dist)
    routes=list(best.values())

    return {
        'locations': sorted(locations),
        'products': sorted(products),
        'routes': routes,
        'offer': offer,
        'demand': demand,
    }

def solve_rmp(data, M=1e6, scale=1000):
    locs  = data['locations']
    prods = data['products']
    routes_raw = data['routes']  # tuples: (from,to,tr,dist)

    # name each route distinctly using TR
    Route = namedtuple('Route', 'frm to tr dist name')
    routes = []
    for frm,to,tr,dist in routes_raw:
        name = f"{frm}->{to}[{tr}]"
        routes.append(Route(frm,to,tr,dist,name))

    # Net supply-demand per (l,p)
    nsd = defaultdict(lambda: defaultdict(int))
    for p in prods:
        for l in locs:
            nsd[p][l] = 0
        for l,q in data['offer'].get(p,{}).items():
            nsd[p][l] += q
        for l,q in data['demand'].get(p,{}).items():
            nsd[p][l] -= q

    m = Model("RMP")

    # Variables
    f = {}  # flow
    y = {}  # Big-M slack
    for r in routes:
        for p in prods:
            f[(r,p)] = m.addVar(lb=0.0, ub=None, name=f"f[{r.name},{p}]")
            y[(r,p)] = m.addVar(lb=0.0, ub=None, obj=M, name=f"y[{r.name},{p}]")

    # Flow conservation
    flow_con = {}
    for l in locs:
        for p in prods:
            expr = quicksum(f[(r,p)] for r in routes if r.frm==l) \
                 - quicksum(f[(r,p)] for r in routes if r.to==l)
            c = m.addCons(expr == nsd[p][l], name=f"flow[{l},{p}]")
            flow_con[(l,p)] = c

    # Coverage (no columns yet): y - f >= 0
    cover_con = {}
    for r in routes:
        for p in prods:
            c = m.addCons(y[(r,p)] - f[(r,p)] >= 0, name=f"cover[{r.name},{p}]")
            cover_con[(r,p)] = c

    m.setMinimize()
    m.optimize()

    # Objective
    obj = m.getObjVal()

    # Duals
    phi = { (l,p): m.getDualsolLinear(c) for (l,p),c in flow_con.items() }
    pi  = { (r.name,p): m.getDualsolLinear(c) for (r,p),c in cover_con.items() }

    print(f"RMP objective (minimal Big-M cost): {obj:.6f}")
    print("\n# Duals for flow conservation (phi(l,p))")
    for (l,p),v in sorted(phi.items()):
        print(f"phi({l},{p}) = {v:.6f}")

    print("\n# Duals for coverage (dualCover(route,p))")
    for (rname,p),v in sorted(pi.items()):
        print(f"dualCover({rname},{p}) = {v:.6f}")

    # Emit ASP facts for pricing (scaled integers)
    asp_lines = []
    for (l,p),v in phi.items():
        asp_lines.append(f"phi({l},{p},{int(round(v*scale))}).")
    for (rname,p),v in pi.items():
        asp_lines.append(f"dualCover({rname},{p},{int(round(v*scale))}).")

    with open("duals_out.lp","w") as g:
        g.write("\n".join(asp_lines) + "\n")
    print("\nWrote duals_out.lp (scaled by", scale, ")")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("facts", help="ASP facts .lp file")
    ap.add_argument("--M", type=float, default=1e6, help="Big-M cost for y")
    ap.add_argument("--scale", type=int, default=1000, help="scale factor for integer duals")
    args = ap.parse_args()

    data = read_facts(args.facts)
    print(data)
    solve_rmp(data, M=args.M, scale=args.scale)

if __name__ == "__main__":
    main()

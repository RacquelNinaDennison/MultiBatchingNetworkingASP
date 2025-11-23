import re, json, subprocess, sys
from collections import defaultdict

FACT = re.compile(r'^\s*([a-zA-Z_][a-zA-Z0-9_]*)\((.*?)\)\.\s*$')

def parse_terms(s):
    out, cur, d = [], [], 0
    for ch in s:
        if ch=='(':
            d+=1; cur.append(ch)
        elif ch==')':
            d-=1; cur.append(ch)
        elif ch==',' and d==0:
            out.append(''.join(cur).strip()); cur=[]
        else:
            cur.append(ch)
    if cur: out.append(''.join(cur).strip())
    return out

def atom(line):
    m = FACT.match(line)
    return (m.group(1), [t.strip() for t in parse_terms(m.group(2))]) if m else None

def sym(x):
    if x and x[0] in "'\"" and x[-1]==x[0]: return x[1:-1]
    return x

def as_num(x):
    try:
        if '.' in x or 'e' in x.lower(): return float(x)
        return int(x)
    except: return x

def facts_to_json(facts_lines, co2Costs=50.0, capitalCosts=0.1):
    locations=set(); trs=set()
    trcap={}; trco2={}; trcost={}; trspeed={}
    prods=set(); psz={}, {}; pval={}
    psize={}; pval={}
    validTR=defaultdict(set)
    offer=defaultdict(lambda: defaultdict(int))
    demand=defaultdict(lambda: defaultdict(int))
    routes=defaultdict(dict)

    for raw in facts_lines:
        s=raw.strip()
        if not s or s.startswith('%'): continue
        a=atom(s)
        if not a: continue
        pred,t=a
        if pred=='location' and len(t)==1: locations.add(sym(t[0]))
        elif pred=='transportResource' and len(t)==1: trs.add(sym(t[0]))
        elif pred=='transportCapacity' and len(t)==2: trcap[sym(t[0])]=as_num(t[1])
        elif pred=='transportCO2' and len(t)==2: trco2[sym(t[0])]=as_num(t[1])
        elif pred=='transportCost' and len(t)==2: trcost[sym(t[0])]=as_num(t[1])
        elif pred=='transportSpeed' and len(t)==2: trspeed[sym(t[0])]=as_num(t[1])
        elif pred=='part' and len(t)==1: prods.add(sym(t[0]))
        elif pred=='partSize' and len(t)==2: psize[sym(t[0])]=as_num(t[1])
        elif pred=='partVal' and len(t)==2: pval[sym(t[0])]=as_num(t[1])
        elif pred=='partTR' and len(t)==2: validTR[sym(t[0])].add(sym(t[1]))
        elif pred=='offer' and len(t)==3:
            p,l,q=sym(t[0]),sym(t[1]),as_num(t[2]); locations.add(l); prods.add(p); offer[p][l]+=q
        elif pred=='demand' and len(t)==3:
            p,l,q=sym(t[0]),sym(t[1]),as_num(t[2]); locations.add(l); prods.add(p); demand[p][l]+=q
        elif pred=='route' and len(t)==5:
            frm,to,tr,dist,_c = sym(t[0]),sym(t[1]),sym(t[2]),as_num(t[3]),as_num(t[4])
            locations.add(frm); locations.add(to); trs.add(tr)
            # keep min distance per (frm,to,tr)
            if tr not in routes[(frm,to)] or dist < routes[(frm,to)][tr]:
                routes[(frm,to)][tr] = dist

    # build ids
    loc_ids={name:f"L{i+1}" for i,name in enumerate(sorted(locations))}
    tr_ids ={name:f"TR{i+1}" for i,name in enumerate(sorted(trs))}
    prod_ids={name:f"P{i+1}" for i,name in enumerate(sorted(prods))}

    J={"settings":{"co2Costs":co2Costs,"capitalCosts":capitalCosts},
       "locations":{}, "transportResources":{}, "products":{}, "routes":{}}

    for name,lid in loc_ids.items(): J["locations"][lid]={"name":name}
    for name,tid in tr_ids.items():
        J["transportResources"][tid]={
            "name":name,
            "capacity":trcap.get(name,0),
            "co2Emissions":trco2.get(name,0.0),
            "cost":trcost.get(name,0.0),
            "speed":trspeed.get(name,1.0),
        }
    for name,pid in prod_ids.items():
        nsd={ lid:0 for lid in J["locations"].keys() }
        for l,q in offer.get(name,{}).items(): nsd[loc_ids[l]] += int(q)
        for l,q in demand.get(name,{}).items(): nsd[loc_ids[l]] -= int(q)
        J["products"][pid]={
            "name":name,
            "validTR":[tr_ids[t] for t in sorted(validTR.get(name,set())) if t in tr_ids],
            "size":psize.get(name,0),
            "value":pval.get(name,0),
            "netSupplyDemand": nsd
        }
    ridx=1
    for (frm,to), pertr in sorted(routes.items()):
        J["routes"][f"R{ridx}"]={
            "from": loc_ids[frm],
            "to":   loc_ids[to],
            "transportResources": { tr_ids[t]:{"distance":d} for t,d in pertr.items() if t in tr_ids }
        }
        ridx+=1
    return J

DUAL_FLOW = re.compile(r'^phi\(([^,]+),([^)\s]+)\)=([+-]?\d+(?:\.\d+)?)$')
DUAL_COV  = re.compile(r'^dualCover\(([^,>]+)->([^,]+),([^)\s]+)\)=([+-]?\d+(?:\.\d+)?)$')

def run_rmp(stdin_json: dict, exe="./lno_rmp_stdin", scale=1000):
    p = subprocess.run(
        [exe],
        input=json.dumps(stdin_json).encode(),
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True
    )
    out = p.stdout.decode().splitlines()
    phi=[]; pi=[]
    in_flow=in_cov=False
    for line in out:
        line=line.strip()
        if line=="DUALS_FLOW_BEGIN": in_flow=True; continue
        if line=="DUALS_FLOW_END":   in_flow=False; continue
        if line=="DUALS_COVER_BEGIN": in_cov=True; continue
        if line=="DUALS_COVER_END":   in_cov=False; continue
        if in_flow:
            m=DUAL_FLOW.match(line)
            if m: phi.append(("phi", m.group(1), m.group(2), float(m.group(3))))
        elif in_cov:
            m=DUAL_COV.match(line)
            if m: pi.append(("dualCover", m.group(1), m.group(2), m.group(3), float(m.group(4))))
    # emit ASP facts
    asp = []
    for _,l,p,v in phi:
        asp.append(f"phi({l},{p},{int(round(v*scale))}).")
    for _,frm,to,p,v in pi:
        # route labelled by endpoints; if you split by TR later, include it here
        asp.append(f"dualCover({frm}->{to},{p},{int(round(v*scale))}).")
    return "\n".join(asp) + "\n"

if __name__=="__main__":
    facts_path = sys.argv[1]
    exe        = sys.argv[2] if len(sys.argv)>2 else "./lno_rmp_stdin"
    with open(facts_path,'r') as f:
        J = facts_to_json(f.readlines())
    asp_duals = run_rmp(J, exe=exe, scale=1000)
    # write the duals for the pricing ASP
    with open("duals_out.lp","w") as g: g.write(asp_duals)
    print("Wrote duals_out.lp")

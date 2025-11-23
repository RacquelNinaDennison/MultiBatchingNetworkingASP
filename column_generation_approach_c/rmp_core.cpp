#include "rmp_core.h"
#include "objscip/objscip.h"
#include "objscip/objscipdefplugins.h"
#include <map>
#include <tuple>
#include <cmath>
#include <iostream>
using namespace scip;

SCIP_RETCODE solve_rmp_from_data(
    const Settings& settings,
    const std::vector<Location*>& locations,
    const std::vector<TransportResource*>& transportResources,
    const std::vector<Product*>& products,
    const std::vector<Route*>& routes)
{
  SCIP* scip = nullptr;
  SCIP_CALL( SCIPcreate(&scip) );
  SCIPprintVersion(scip, nullptr); SCIPinfoMessage(scip,nullptr,"\n");
  SCIP_CALL( SCIPincludeDefaultPlugins(scip) );
  SCIP_CALL( SCIPcreateProbBasic(scip, "RMP") );

  // Vars f and Big-M y
  std::map<std::tuple<Route*,Product*>, SCIP_VAR*> fvar;
  for (auto* route: routes) for (auto* prod: products) {
    SCIP_VAR* v=nullptr; char nm[255];
    (void)SCIPsnprintf(nm,255,"f_%s->%s_%s",
      route->from->name.c_str(), route->to->name.c_str(), prod->name.c_str());
    SCIP_CALL(SCIPcreateVarBasic(scip,&v,nm,0.0,SCIPinfinity(scip),0.0,SCIP_VARTYPE_CONTINUOUS));
    SCIP_CALL(SCIPaddVar(scip,v));
    fvar[{route,prod}] = v;
  }

  // flow conservation == nsd
  std::map<std::pair<Location*,Product*>, SCIP_CONS*> flow_con;
  for (auto* loc: locations) for (auto* prod: products) {
    const int nsd = prod->netSupplyDemand.at(loc);
    SCIP_CONS* c=nullptr;
    SCIP_CALL(SCIPcreateConsBasicLinear(scip,&c,"flow",0,nullptr,nullptr, nsd, nsd));
    SCIP_CALL(SCIPaddCons(scip,c));
    for (auto* r: routes) {
      if (r->from==loc) SCIP_CALL(SCIPaddCoefLinear(scip,c,fvar[{r,prod}], 1.0));
      else if (r->to==loc) SCIP_CALL(SCIPaddCoefLinear(scip,c,fvar[{r,prod}],-1.0));
    }
    flow_con[{loc,prod}] = c;
  }

  // cover constraints: y - f >= 0  (no columns yet)
  std::map<std::tuple<Route*,Product*>, SCIP_CONS*> cover_con;
  for (auto* r: routes) for (auto* p: products) {
    // Big-M y var
    SCIP_VAR* y=nullptr; char ynm[255];
    (void)SCIPsnprintf(ynm,255,"y_%s->%s_%s",
      r->from->name.c_str(), r->to->name.c_str(), p->name.c_str());
    SCIP_CALL(SCIPcreateVarBasic(scip,&y,ynm,0.0,SCIPinfinity(scip),1e6,SCIP_VARTYPE_CONTINUOUS));
    SCIP_CALL(SCIPaddVar(scip,y));

    SCIP_CONS* c=nullptr; char cnm[255];
    (void)SCIPsnprintf(cnm,255,"cover_%s->%s_%s",
      r->from->name.c_str(), r->to->name.c_str(), p->name.c_str());
    SCIP_CALL(SCIPcreateConsBasicLinear(scip,&c,cnm,0,nullptr,nullptr, 0.0, SCIPinfinity(scip)));
    SCIP_CALL(SCIPaddCons(scip,c));
    SCIP_CALL(SCIPaddCoefLinear(scip,c,y,  1.0));
    SCIP_CALL(SCIPaddCoefLinear(scip,c,fvar[{r,p}], -1.0));
    cover_con[{r,p}] = c;
  }

  SCIP_CALL(SCIPsolve(scip));

  // Print duals (so the controller can capture them)
  std::cout << "\nDUALS_FLOW_BEGIN\n";
  for (auto& kv: flow_con) {
    auto* loc = kv.first.first;
    auto* pr  = kv.first.second;
    SCIP_Real dual = SCIPgetDualsolLinear(scip, kv.second);
    std::cout << "phi(" << loc->name << "," << pr->name << ")=" << dual << "\n";
  }
  std::cout << "DUALS_FLOW_END\n";

  std::cout << "DUALS_COVER_BEGIN\n";
  for (auto& kv: cover_con) {
    auto* r = std::get<0>(kv.first);
    auto* p = std::get<1>(kv.first);
    SCIP_Real dual = SCIPgetDualsolLinear(scip, kv.second);
    // Label route by endpoints (and you can extend with TR if you split routes by TR)
    std::cout << "dualCover(" << r->from->name << "->" << r->to->name << "," << p->name << ")=" << dual << "\n";
  }
  std::cout << "DUALS_COVER_END\n";

  // cleanup
  for (auto& kv: cover_con) SCIP_CALL(SCIPreleaseCons(scip,&kv.second));
  for (auto& kv: flow_con)  SCIP_CALL(SCIPreleaseCons(scip,&kv.second));
  for (auto& kv: fvar)      SCIP_CALL(SCIPreleaseVar(scip,&kv.second));
  SCIP_CALL(SCIPfree(&scip));
  BMScheckEmptyMemory();
  return SCIP_OKAY;
}

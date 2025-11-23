//
// Created by Karl Henning on 25.06.23.
//

/* standard library includes */
#include <fstream>
#include <iostream>

/* scip includes */
#include "objscip/objscip.h"
#include "objscip/objscipdefplugins.h"

/* user defined includes */
#include "main.h"
#include "pricer_knapsack.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/* namespace usage */
using namespace std;
using namespace scip;

/** read LNO problem */
static int read_problem(const char *filename, Settings &settings,
                        vector<Location *> &locations,
                        vector<TransportResource *> &transportResources,
                        vector<Product *> &products, vector<Route *> &routes) {
  ifstream file(filename);

  if (!file) {
    cerr << "Cannot open file " << filename << endl;
    return 1;
  }

  json data = json::parse(file);
  // cout << data.dump(4) << endl;

  struct Linkings {
    unordered_map<string, Location *> locations;
    unordered_map<string, TransportResource *> transportResources;
    unordered_map<string, Product *> products;
    unordered_map<string, Route *> routes;
  };
  Linkings linkings;

  settings.co2Costs = data.at("settings").at("co2Costs");
  settings.capitalCosts = data.at("settings").at("capitalCosts");

  for (auto i = data.at("locations").begin(); i != data.at("locations").end();
       ++i) {
    auto *location = new Location(i.value().at("name"));
    linkings.locations[i.key()] = location;
    locations.push_back(location);
  }

  for (auto i = data.at("transportResources").begin();
       i != data.at("transportResources").end(); ++i) {
    auto *transportResource =
        new TransportResource(i.value().at("name"), i.value().at("capacity"),
                              i.value().at("co2Emissions"),
                              i.value().at("cost"), i.value().at("speed"));
    linkings.transportResources[i.key()] = transportResource;
    transportResources.push_back(transportResource);
  }

  for (auto i = data.at("products").begin(); i != data.at("products").end();
       ++i) {
    vector<TransportResource *> validTR;
    for (auto &tr : i.value().at("validTR")) {
      validTR.push_back(linkings.transportResources[tr]);
    }

    map<Location *, int> netSupplyDemand;
    for (auto j = i.value().at("netSupplyDemand").begin(); j != i.value().at("netSupplyDemand").end(); ++j) {
        netSupplyDemand[linkings.locations[j.key()]] = j.value();
    }

    auto *product = new Product(i.value().at("name"), validTR, i.value().at("size"), i.value().at("value"), netSupplyDemand);

    linkings.products[i.key()] = product;
    products.push_back(product);
  }

  for (auto i = data.at("routes").begin(); i != data.at("routes").end(); ++i) {
    /*
    "transportResources": {
        "TR1": {
          "distance": 2
        },
        "TR2": {
          "distance": 4
        }
      }
      */
    vector<tuple<TransportResource*, double>> transportResources;
    for (auto tr = i.value().at("transportResources").begin(); tr != i.value().at("transportResources").end(); ++tr) {
        transportResources.push_back(make_tuple(linkings.transportResources[tr.key()], tr.value().at("distance")));
    }
    
    auto *route = new Route(
        linkings.locations[i.value().at("to")],
        linkings.locations[i.value().at("from")],
        transportResources);
    linkings.routes[i.key()] = route;
    routes.push_back(route);
  }

  return 0;
}

//------------------------------------------------------------
static SCIP_RETCODE execmain(int argc, char **argv) {
  // measure runtime
  clock_t start = clock();

  SCIP *scip = nullptr;

  cout << "Solving the logistics network optimization problem using SCIP."
       << endl;

  if (argc != 2) {
    cerr << "Usage: lno datafile" << endl;
    return SCIP_INVALIDDATA;
  }

  /**********************
   * Setup problem data *
   **********************/

  Settings settings;
  vector<Location *> locations;
  vector<TransportResource *> transportResources;
  vector<Product *> products;

  vector<Route *> routes;

  if (read_problem(argv[argc - 1], settings, locations, transportResources,
                   products, routes)) {
    cerr << "Error reading data file " << argv[argc - 1] << endl;
    return SCIP_READERROR;
  }

  /**************
   * Setup SCIP *
   **************/

  /* initialize SCIP environment */
  SCIP_CALL(SCIPcreate(&scip));

  /***********************
   * Version information *
   ***********************/

  SCIPprintVersion(scip, nullptr);
  SCIPinfoMessage(scip, nullptr, "\n");

  /* include default plugins */
  SCIP_CALL(SCIPincludeDefaultPlugins(scip));

  /* set verbosity parameter */
  // SCIP_CALL( SCIPsetIntParam(scip, "display/verblevel", 0) );
  /* SCIP_CALL( SCIPsetBoolParam(scip, "display/lpinfo", TRUE) ); */

  /* create empty problem */
  SCIP_CALL(SCIPcreateProbBasic(scip, "LNO"));

  // flow variables
  map<tuple<Route *, Product *>, SCIP_VAR *> flow_vars;
  for (const auto &route : routes) {
    for (const auto &product : products) {
      SCIP_VAR *var;
      char flow_var_name[255];
      (void)SCIPsnprintf(flow_var_name, 255, "flow_%s->%s_%s",
                         route->from->name.c_str(), route->to->name.c_str(),
                         product->name.c_str());

      SCIP_CALL(SCIPcreateVarBasic(
          scip, &var,         // returns new index
          flow_var_name,      // name
          0,                  // lower bound
          SCIPinfinity(scip), // upper bound
          0,                   // objective
          SCIP_VARTYPE_CONTINUOUS)); // variable type
      SCIP_CALL(SCIPaddVar(scip, var));
      flow_vars[make_tuple(route, product)] = var;
    }
  }

  // add flow conservation constraints
  for (const auto &location : locations) {
    for (const auto &product : products) {
      int flow_amount = product->netSupplyDemand[location];

      SCIP_CONS *cons;
      SCIP_CALL(SCIPcreateConsBasicLinear(scip, &cons, "flow conservation", 0,
                                          nullptr, nullptr,
                                          flow_amount,   /* lhs */
                                          flow_amount)); /* rhs */
      SCIP_CALL(SCIPaddCons(scip, cons));

      for (const auto &route : routes) {
        if (route->from == location)
          SCIP_CALL(SCIPaddCoefLinear(
              scip, cons, flow_vars[make_tuple(route, product)], 1));

        else if (route->to == location)
          SCIP_CALL(SCIPaddCoefLinear(
              scip, cons, flow_vars[make_tuple(route, product)], -1));
      }

      // release constraint
      SCIP_CALL(SCIPreleaseCons(scip, &cons));
    }
  }

  /* add flow amount constraints */
  map<tuple<Route *, Product *>, SCIP_CONS *> demand_con;
  for (const auto &route : routes) {
    for (const auto &product : products) {
      SCIP_CONS *con;
      char demand_con_name[255];
      (void)SCIPsnprintf(demand_con_name, 255, "demand_%s->%s_%s",
                         route->from->name.c_str(), route->to->name.c_str(),
                         product->name.c_str());
      SCIP_CALL(SCIPcreateConsBasicLinear(scip, &con, demand_con_name, 0, nullptr, nullptr, 
                                          0.0, /* lhs */
                                          SCIPinfinity(scip)));                 /* rhs */
      SCIP_CALL(SCIPsetConsModifiable(scip, con, true));
      SCIP_CALL(SCIPaddCons(scip, con));
      demand_con[make_tuple(route, product)] = con;

      SCIP_CALL(SCIPaddCoefLinear(scip, con, flow_vars[make_tuple(route, product)], -1));
      SCIP_CALL(SCIPreleaseVar(scip, &flow_vars[make_tuple(route, product)]));

      SCIP_VAR *var;
      char y_var_name[255];
      (void)SCIPsnprintf(y_var_name, 255, "initial-y_%s->%s_%s",
                         route->from->name.c_str(), route->to->name.c_str(),
                         product->name.c_str());

      SCIP_CALL(SCIPcreateVarBasic(
          scip, &var, y_var_name, // name
          0.0,                     // lower bound
          SCIPinfinity(scip),      // upper bound
          pow(10,6),   // objective
          SCIP_VARTYPE_CONTINUOUS)); // variable type
      SCIP_CALL(SCIPaddVar(scip, var));

      SCIP_CALL(SCIPaddCoefLinear(scip, con, var, 1));
      SCIP_CALL(SCIPreleaseVar(scip, &var));
    }
  }

  static const char *PRICER_KNAPSACK_NAME = "Knapsack Pricer";

  /* include LNO pricer */
  auto *lno_pricer_ptr = new PricerKnapsack(scip, PRICER_KNAPSACK_NAME, routes, products, settings, demand_con);

  SCIP_CALL(SCIPincludeObjPricer(scip, lno_pricer_ptr, true));

  /* activate pricer */
  SCIP_CALL(
      SCIPactivatePricer(scip, SCIPfindPricer(scip, PRICER_KNAPSACK_NAME)));

  // SCIP_CALL( SCIPwriteOrigProblem(scip, "lno_init.lp", "lp", FALSE) );

  /*************
   *  Solve    *
   *************/

  SCIP_CALL(SCIPsolve(scip));

  /**************
   * Statistics *
   *************/
  SCIP_CALL(SCIPprintStatistics(scip, nullptr));

  SCIP_CALL(SCIPprintBestSol(scip, nullptr, FALSE));

  /********************
   * Deinitialization *
   ********************/

  /* release variables */
  for (const auto &route : routes) {
    for (const auto &product : products) {
      SCIP_CALL(SCIPreleaseCons(scip, &demand_con[make_tuple(route, product)]));
    }
  }

  SCIP_CALL(SCIPfree(&scip));

  BMScheckEmptyMemory();

  // measure runtime
  clock_t end = clock();
  double elapsed_secs = double(end - start) / CLOCKS_PER_SEC;
  cout << "Runtime: " << elapsed_secs << "s" << endl;

  return SCIP_OKAY;
}

int main(int argc, char **argv) {
  cout << "starting..." << endl;
  return execmain(argc, argv) != SCIP_OKAY ? 1 : 0;
}


// dependency bug fix required for Windows compilation
/*
 * Data structures
 */

/** variable pricer data */
struct SCIP_PricerData
{
   scip::ObjPricer*      objpricer;          /**< variable pricer object */
   SCIP_Bool             deleteobject;       /**< should the pricer object be deleted when pricer is freed? */
};




/*
 * Callback methods of variable pricer
 */

extern "C"
{

/** copy method for pricer plugins (called when SCIP copies plugins) */
static
SCIP_DECL_PRICERCOPY(pricerCopyObj)
{  /*lint --e{715}*/
   SCIP_PRICERDATA* pricerdata;
   
   assert(scip != NULL);
   
   pricerdata = SCIPpricerGetData(pricer);
   assert(pricerdata != NULL);
   assert(pricerdata->objpricer != NULL);
   assert(pricerdata->objpricer->scip_ != scip);

   if( pricerdata->objpricer->iscloneable() )
   {
      scip::ObjPricer* newobjpricer;
      newobjpricer = dynamic_cast<scip::ObjPricer*> (pricerdata->objpricer->clone(scip, valid));

      /* call include method of pricer object */
      SCIP_CALL( SCIPincludeObjPricer(scip, newobjpricer, TRUE) );
   }

   return SCIP_OKAY;
}

/** destructor of variable pricer to free user data (called when SCIP is exiting) */
static
SCIP_DECL_PRICERFREE(pricerFreeObj)
{  /*lint --e{715}*/
   SCIP_PRICERDATA* pricerdata;

   pricerdata = SCIPpricerGetData(pricer);
   assert(pricerdata != NULL);
   assert(pricerdata->objpricer != NULL);
   assert(pricerdata->objpricer->scip_ == scip);

   /* call virtual method of pricer object */
   SCIP_CALL( pricerdata->objpricer->scip_free(scip, pricer) );

   /* free pricer object */
   if( pricerdata->deleteobject )
      delete pricerdata->objpricer;

   /* free pricer data */
   delete pricerdata;
   SCIPpricerSetData(pricer, NULL); /*lint !e64*/
   
   return SCIP_OKAY;
}


/** initialization method of variable pricer (called after problem was transformed) */
static
SCIP_DECL_PRICERINIT(pricerInitObj)
{  /*lint --e{715}*/
   SCIP_PRICERDATA* pricerdata;

   pricerdata = SCIPpricerGetData(pricer);
   assert(pricerdata != NULL);
   assert(pricerdata->objpricer != NULL);
   assert(pricerdata->objpricer->scip_ == scip);

   /* call virtual method of pricer object */
   SCIP_CALL( pricerdata->objpricer->scip_init(scip, pricer) );

   return SCIP_OKAY;
}


/** deinitialization method of variable pricer (called before transformed problem is freed) */
static
SCIP_DECL_PRICEREXIT(pricerExitObj)
{  /*lint --e{715}*/
   SCIP_PRICERDATA* pricerdata;

   pricerdata = SCIPpricerGetData(pricer);
   assert(pricerdata != NULL);
   assert(pricerdata->objpricer != NULL);

   /* call virtual method of pricer object */
   SCIP_CALL( pricerdata->objpricer->scip_exit(scip, pricer) );

   return SCIP_OKAY;
}


/** solving process initialization method of variable pricer (called when branch and bound process is about to begin) */
static
SCIP_DECL_PRICERINITSOL(pricerInitsolObj)
{  /*lint --e{715}*/
   SCIP_PRICERDATA* pricerdata;

   pricerdata = SCIPpricerGetData(pricer);
   assert(pricerdata != NULL);
   assert(pricerdata->objpricer != NULL);

   /* call virtual method of pricer object */
   SCIP_CALL( pricerdata->objpricer->scip_initsol(scip, pricer) );

   return SCIP_OKAY;
}


/** solving process deinitialization method of variable pricer (called before branch and bound process data is freed) */
static
SCIP_DECL_PRICEREXITSOL(pricerExitsolObj)
{  /*lint --e{715}*/
   SCIP_PRICERDATA* pricerdata;

   pricerdata = SCIPpricerGetData(pricer);
   assert(pricerdata != NULL);
   assert(pricerdata->objpricer != NULL);

   /* call virtual method of pricer object */
   SCIP_CALL( pricerdata->objpricer->scip_exitsol(scip, pricer) );

   return SCIP_OKAY;
}


/** reduced cost pricing method of variable pricer for feasible LPs */
static
SCIP_DECL_PRICERREDCOST(pricerRedcostObj)
{  /*lint --e{715}*/
   SCIP_PRICERDATA* pricerdata;

   pricerdata = SCIPpricerGetData(pricer);
   assert(pricerdata != NULL);
   assert(pricerdata->objpricer != NULL);

   /* call virtual method of pricer object */
   SCIP_CALL( pricerdata->objpricer->scip_redcost(scip, pricer, lowerbound, stopearly, result) );

   return SCIP_OKAY;
}


/** farkas pricing method of variable pricer for infeasible LPs */
static
SCIP_DECL_PRICERFARKAS(pricerFarkasObj)
{  /*lint --e{715}*/
   SCIP_PRICERDATA* pricerdata;

   pricerdata = SCIPpricerGetData(pricer);
   assert(pricerdata != NULL);
   assert(pricerdata->objpricer != NULL);

   /* call virtual method of pricer object */
   SCIP_CALL( pricerdata->objpricer->scip_farkas(scip, pricer, result) );

   return SCIP_OKAY;
}
}



/*
 * variable pricer specific interface methods
 */

/** creates the variable pricer for the given variable pricer object and includes it in SCIP */
SCIP_RETCODE SCIPincludeObjPricer(
   SCIP*                 scip,               /**< SCIP data structure */
   scip::ObjPricer*      objpricer,          /**< variable pricer object */
   SCIP_Bool             deleteobject        /**< should the pricer object be deleted when pricer is freed? */
   )
{
   SCIP_PRICERDATA* pricerdata;

   assert(scip != NULL);
   assert(objpricer != NULL);

   /* create variable pricer data */
   pricerdata = new SCIP_PRICERDATA;
   pricerdata->objpricer = objpricer;
   pricerdata->deleteobject = deleteobject;

   /* include variable pricer */
   SCIP_CALL( SCIPincludePricer(scip, objpricer->scip_name_, objpricer->scip_desc_, objpricer->scip_priority_,
         objpricer->scip_delay_,
         pricerCopyObj,
         pricerFreeObj, pricerInitObj, pricerExitObj,
         pricerInitsolObj, pricerExitsolObj, pricerRedcostObj, pricerFarkasObj,
         pricerdata) ); /*lint !e429*/

   return SCIP_OKAY; /*lint !e429*/
}

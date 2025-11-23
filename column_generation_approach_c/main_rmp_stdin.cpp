#include <iostream>
#include <sstream>
#include <unordered_map>
#include "rmp_core.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;
using namespace std;

// same object wiring as your old read_problem(), but fed by an in-memory json
static void build_from_json(
  const json& data,
  Settings& settings,
  vector<Location*>& locations,
  vector<TransportResource*>& trs,
  vector<Product*>& products,
  vector<Route*>& routes)
{
  struct Lk { unordered_map<string,Location*> loc;
              unordered_map<string,TransportResource*> tr;
              unordered_map<string,Product*> prod;
              unordered_map<string,Route*> route; } lk;

  settings.co2Costs     = data.at("settings").at("co2Costs");
  settings.capitalCosts = data.at("settings").at("capitalCosts");

  for (auto it=data.at("locations").begin(); it!=data.at("locations").end(); ++it){
    auto* L = new Location(it.value().at("name"));
    lk.loc[it.key()] = L; locations.push_back(L);
  }
  for (auto it=data.at("transportResources").begin(); it!=data.at("transportResources").end(); ++it){
    auto* T = new TransportResource(it.value().at("name"),
                                    it.value().at("capacity"),
                                    it.value().at("co2Emissions"),
                                    it.value().at("cost"),
                                    it.value().at("speed"));
    lk.tr[it.key()] = T; trs.push_back(T);
  }
  for (auto it=data.at("products").begin(); it!=data.at("products").end(); ++it){
    vector<TransportResource*> valid;
    for (auto& trId: it.value().at("validTR")) valid.push_back(lk.tr.at(trId));

    map<Location*,int> nsd;
    for (auto jt=it.value().at("netSupplyDemand").begin(); jt!=it.value().at("netSupplyDemand").end(); ++jt)
      nsd[ lk.loc.at(jt.key()) ] = jt.value();

    auto* P = new Product(it.value().at("name"), valid,
                          it.value().at("size"), it.value().at("value"), nsd);
    lk.prod[it.key()] = P; products.push_back(P);
  }
  for (auto it=data.at("routes").begin(); it!=data.at("routes").end(); ++it){
    vector<tuple<TransportResource*, double>> perTR;
    for (auto jt=it.value().at("transportResources").begin(); jt!=it.value().at("transportResources").end(); ++jt)
      perTR.push_back( make_tuple(lk.tr.at(jt.key()), jt.value().at("distance")) );
    auto* R = new Route( lk.loc.at( it.value().at("to") ),
                         lk.loc.at( it.value().at("from") ),
                         perTR );
    lk.route[it.key()] = R; routes.push_back(R);
  }
}

int main(){
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  // read whole stdin into a string
  std::ostringstream buf;
  buf << cin.rdbuf();
  json data = json::parse(buf.str());

  Settings settings;
  vector<Location*> locations;
  vector<TransportResource*> trs;
  vector<Product*> products;
  vector<Route*> routes;

  build_from_json(data, settings, locations, trs, products, routes);

  auto rc = solve_rmp_from_data(settings, locations, trs, products, routes);
  return rc==SCIP_OKAY ? 0 : 1;
}

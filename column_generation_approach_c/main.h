//
// Created by Karl Henning on 25.06.23.
//

#ifndef LNO_MAIN_H
#define LNO_MAIN_H

#include <iostream>
#include <vector>
#include <map>

using namespace std;

struct Settings {
  double co2Costs;
  double capitalCosts;

  explicit Settings(double co2Costs = 0, double capitalCosts = 0) {
    this->capitalCosts = capitalCosts;
    this->co2Costs = co2Costs;
  };
};

struct Location {
  string name;

  explicit Location(string name) { this->name = std::move(name); };
};

struct TransportResource {
  string name;
  double capacity;
  double co2Emissions;
  double cost;
  double speed;

  TransportResource(string name, double capacity, double co2Emissions,
                    double cost, double speed) {
    this->name = std::move(name);
    this->capacity = capacity;
    this->co2Emissions = co2Emissions;
    this->cost = cost;
    this->speed = speed;
  };
};

struct Product {
  string name;
  vector<TransportResource *> validTR;
  double size;
  double value;
  map<Location *, int> netSupplyDemand;

  Product(string name, vector<TransportResource *> validTR, double size, double value, map<Location *, int> netSupplyDemand) {
        this->name = std::move(name);
        this->size = size;
        this->value = value;
        this->validTR = std::move(validTR);
        this->netSupplyDemand = netSupplyDemand;
    };
};

struct Route {
  Location *to;
  Location *from;
  vector<tuple<TransportResource*, double>> transportResources;

  Route(Location *to, Location *from, vector<tuple<TransportResource*, double>> transportResources) {
    this->to = to;
    this->from = from;
    this->transportResources = transportResources;
  };
};

#endif // LNO_MAIN_H

#pragma once
#include <vector>
#include <map>
#include <tuple>
#include "main.h"  // where Settings, Location, TransportResource, Product, Route are declared
#include "objscip/objscip.h"

scip::SCIP_RETCODE solve_rmp_from_data(
    const Settings& settings,
    const std::vector<Location*>& locations,
    const std::vector<TransportResource*>& transportResources,
    const std::vector<Product*>& products,
    const std::vector<Route*>& routes);

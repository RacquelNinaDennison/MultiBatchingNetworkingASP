routeCost(RouteId,Cost) :- route(From,To,_,_,Cost), RouteId=(From,To).
routeDistance(RouteId,Distance) :- route(From,To,_,Distance,_), RouteId=(From,To).
routeTransportResource(RouteId,TR) :- route(From,To,TR,_,_), RouteId=(From,To).
routeID(RouteId) :- route(From,To,_,_,_), RouteId=(From,To).


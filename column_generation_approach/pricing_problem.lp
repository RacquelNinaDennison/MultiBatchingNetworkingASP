% data format

% generate approach: where for every product p which can be transported on the route r with the transport resource tr(r) associated with the route r, we introduce an integer variable xrp ∈ Z+ which represents how many units of product p are packed into the packing.

% #const N = 10. 
% represent the number of products which can be transported
numberOfTransportedProducts(0..20). 

% reduce the predicate sizes

transportedOnRoute(P,RouteId) :- routeID(RouteId), part(P), routeTransportResource(RouteId,TR), partTR(P,TR). 

{ packedOnRoute(P,R,N) : numberOfTransportedProducts(N) } :- transportedOnRoute(P,R).
:- packedOnRoute(P,R,N1), packedOnRoute(P,R,N2), N1 != N2.

packageSize(RouteId,Size,P) :- packedOnRoute(P,RouteId,N) , partSize(P,S), Size= N*S.

:- routeTransportResource(RouteId,TR), transportCapacity(TR,Cap), 
    #sum{Size,P:packageSize(RouteId,Size,P)} > Cap.

costOfRoute(RouteId,Cost) :- routeDistance(RouteId,Distance), transportCost(TR,Cost), Cost = Distance * Cost. 

% duals describe what would be one more extra cost on this route link of a specified product - dual(RouteId,Product,DualCost)

costOfProductPerRoute(P,RouteId,TotalCost) :- packedOnRoute(P,RouteId,N), dual(RouteId,P,DualCost), TotalCost = N * DualCost.
totalCostOfProductsPerRoute(RouteId,TotalCost) :- routeID(RouteId),
    TotalCost = #sum{Cost,P:costOfProductPerRoute(P,RouteId,Cost)}.

minimizeFunction(RouteId,C):-costOfRoute(RouteId,Cost), totalCostOfProductsPerRoute(RouteId,TotalCost), C = Cost - TotalCost.

#minimize{C : minimizeFunction(RouteId,C)}.

#show packedOnRoute/3.
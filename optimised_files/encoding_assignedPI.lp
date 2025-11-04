% location: name
location(l1).
location(l2).
location(l3).
location(l4).
location(l5).
location(l6).

% transportResource: name, capacity, co2emissions, cost, speed
transportResource(tr1).
transportResource(tr2).

transportCapacity(tr1,10).
transportCapacity(tr2,15).

transportCO2(tr1,60).
transportCO2(tr2,45).

transportCost(tr1,56).
transportCost(tr2,42).

transportSpeed(tr1,3).
transportSpeed(tr2,2).

% Part: name, validTR, size, value
part(p1).
part(p2).
partSize(p1,4).
partSize(p2,3).
partTR(p1,tr1).
partTR(p1,tr2).
partTR(p2,tr1).
partTR(p2,tr2).
partVal(p1,1000).
partVal(p2,600).


% netSupplyDemand: part name, location, offer/ produced/able
offer(p1,l1,10).
offer(p2,l1,40).
offer(p1,l2,20).

% netSupplyDemand: part name, location, demand/ consumed/able
demand(p2,l2,10).
demand(p1,l5,10).
demand(p2,l5,20).
demand(p1,l6,20).
demand(p2,l6,10).

% 'real' simple example including production plan constraints
% productionPlan(fullFuselageShort,section34).
%productionPlan(fullFuselageShort,section1).
%productionPlan(fullFuselageShort,section34).
%productionPlan(fullFuselageShort,section67).
%productionPlan(section34,section3).
%productionPlan(section34,section4).
%productionPlan(section67,section7).
%productionPlan(section67,section6).

%offer(l1,fullFuselageShort,10).
%demand(l1,section1,10).
%demand(l1,section34,10).
%demand(l1,section67,10).

%offer(l2,section1,10).

%offer(l4,section34,10).
%demand(l4,section3,10).
%demand(l4,section4,10).

%offer(l6,section67,10).
%demand(l6,section6,10).
%demand(l6,section7,10).

% route: from, to, transport mean, distance, cost (per 1)
route(l1,l2,tr1,1,56).
route(l1,l2,tr2,2,84).
route(l1,l3,tr1,3,168).
route(l1,l3,tr2,4,168).
route(l1,l4,tr1,4,224).
route(l1,l4,tr2,8,336).
route(l2,l3,tr1,1,56).
route(l2,l3,tr2,2,84).
route(l2,l4,tr1,2,112).
route(l2,l4,tr2,6,252).
route(l2,l6,tr1,2,112).
route(l3,l5,tr1,1,56).
route(l3,l5,tr2,3,126).
route(l3,l6,tr1,1,56).
route(l3,l6,tr2,2,84).
route(l4,l5,tr1,2,112).
route(l4,l5,tr2,4,168).
route(l4,l6,tr1,3,168).
route(l4,l6,tr2,5,210).


% these can be externally set based on the number of parts declared or frequency of products in the network
#const maxNumberOfParts = 20.
#const maxFrequency = 20. 
#const maxPackingIDs = 20. % maximum packing lists id could be at maximum each item in a package 

itemsTranspotedOnARoute(1..maxNumberOfParts).
num(0..maxFrequency).
{flow(From,To,ProductType,NumberOfProductsOnRoute) : part(ProductType), itemsTranspotedOnARoute(NumberOfProductsOnRoute)} :- route(From,To,_,_,_).
:- flow(From,To,ProductType,NumberOfProductsOnRoute1), flow(From,To,ProductType,NumberOfProductsOnRoute2), NumberOfProductsOnRoute1!=NumberOfProductsOnRoute2.
% if there is a demand for a product, we should not remove it from the site as it is being used
:- flow(From,To,ProductType,NumberOfProductsOnRoute), demand(ProductType, From, D).
% if there is a supply for the product, we should not bring products to the site 
:- flow(From,To,ProductType,NumberOfProductsOnRoute), offer(ProductType, To, D).


% flow conservation; this ensures that the products leaving a location l minus the products entering a location is equal to the net supply or demand of product at location l 
% netSupplyDemand is defined as netSupplyDemand(l,p) where this is positive if the product is produced at the location and - if the product is demanded
demandSupply(P,L,0) :- not offer(P,L,_), not demand(P,L,_), part(P), location(L).
demandSupply(P,L,O) :- offer(P,L,O).
demandSupply(P,L,D) :- demand(P,L,M), D=M*-1.


:- #sum{ N,To  : flow(Loc,To,Part,N); 
         -N,From: flow(From,Loc,Part,N) } != DS,
   demandSupply(Part,Loc,DS).


packing_id(1..maxPackingIDs).

% For each transport resource (TR) and packing ID, we can choose to include
% a quantity 'Qty' of each part 'P'. We only generate items with Qty > 0.
0 { packing_item(TR, PackingID, P, Qty) : num(Qty), Qty > 0 } :-
    transportResource(TR), packing_id(PackingID), part(P).

% Any packing list whose total size exceeds the TR's capacity is invalid.
:- #sum{ Size*Qty, P : packing_item(TR, PackingID, P, Qty), partSize(P, Size) } > Capacity,
   transportCapacity(TR, Capacity),
   packing_id(PackingID).

valid_packing(TR, PackingID) :- packing_item(TR, PackingID, _, _).


% % For a given flow, we choose one or more valid packings (TR, PackingID) and a frequency.
1 { transportLink(From, To, TR, PackingID, Freq) : valid_packing(TR, PackingID), num(Freq) } :- 
    flow(From, To, Part, T).

:- transportLink(From,To, L, TR, Freq1), transportLink(From,To, L, TR, Freq2), Freq1!=Freq2.
:- packingList((P1,P2),TR,N), numPart(P1,V1), numPart(P2,V2), V1>V2.
:- packingList((P1,(P2,L)),TR,N), numPart(P1,V1), numPart(P2,V2), V1>V2.

% % The sum of parts transported is calculated by looking up the quantity
:- T != #sum{ Qty*Freq, PackingID : transportLink(From, To, TR, PackingID, Freq), packing_item(TR, PackingID, Part, Qty) },
   flow(From, To, Part, T).

:- 0 != #sum{ Qty*Freq, PackingID : transportLink(From, To, TR, PackingID, Freq), packing_item(TR, PackingID, Part, Qty) }, 
   not flow(From, To, Part, _), 
   location(From), location(To), part(Part).


#minimize{ Freq*D*C, From, To, TR, PackingID : transportLink(From, To, TR, PackingID, Freq), route(From, To, TR, D, C) }.


#show flow/4.
#show transportLink/5.
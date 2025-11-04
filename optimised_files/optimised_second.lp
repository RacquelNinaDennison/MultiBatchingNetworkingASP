1 <= { flow(From,From,Part,N): offer(Part,From,N) } <= 1 :- offer(Part,From,N); root(Part).
1 <= { transportLink(From,From,intrasite,N) } <= 1 :- flow(From,From,Part,N); root(Part).
1 <= { assign(Part,intrasite,N) } <= 1 :- flow(From,From,Part,N); root(Part).
demandSupply(P,L,0) :- not offer(P,L,_); not demand(P,L,_); part(P); location(L).
demandSupply(P,L,O) :- offer(P,L,O).
demandSupply(P,L,(M*-1)) :- demand(P,L,M).
#const maxNrParts = 20.
#const maxFreq = 20.
numFlow((1..maxNrParts)).
num((0..maxFreq)).
% constraints
% Specify possible flow
{ flow(From,To,Part,N): numFlow(N), part(Part) } :- route(From,To,_,_,_).
__dom_flow(From,From,Part) :- offer(Part,From,_); root(Part).
__dom_flow(From,To,Part) :- numFlow(_); part(Part); route(From,To,_,_,_).
#false :- __dom_flow(From,To,Part); 2 <= #count { N1: flow(From,To,Part,N1) }.
#false :- flow(From,_,Part,_); demand(Part,From,_).
#false :- flow(_,To,Part,_); offer(Part,To,_).
% make sure that flow constraints hold
#false :- demandSupply(Part,Loc,DS); 0 != #sum { (N*-1),To,__agg(0): flow(Loc,To,Part,N); N,From,__agg(1): flow(From,Loc,Part,N); DS,__agg(2) }.
% Specify possible packing lists
%% NOTE TO SELF: This can be restructured as at the moment we are using recursion, we can instead use ID assignment and deconstruction 
{ packingList(P,TR,N) } :- transportCapacity(TR,Cap); Cap >= N; partSize(P,N).
{ packingList((P,L),TR,(T+N)) } :- packingList(L,TR,T); transportCapacity(TR,Cap); Cap >= (T+N); partSize(P,N).
countPartsInL(Part,Part,1) :- packingList(Part,_,_); part(Part).
countPartsInL(Part1,Part2,0) :- packingList(Part1,_,_); part(Part2); part(Part1); Part1 != Part2.
countPartsInL((Head,L),Part,(V1+V2)) :- countPartsInL(L,Part,V2); countPartsInL(Head,Part,V1); packingList((Head,L),_,_).
% based on flow, define transport link with corresponding packing list 
1 <= { transportLink(From,To,L,TR,Freq): packingList(L,TR,_), countPartsInL(L,Part,V), num(V), V > 0, num(Freq) } :- flow(From,To,Part,_).
% make sure that transport links conform with flow
#false :- T != #sum { (N*Freq),L,TR: transportLink(From,To,L,TR,Freq), countPartsInL(L,Part,N) }; flow(From,To,Part,T).
#false :- 0 != #sum { (N*Freq),L,TR: transportLink(From,To,L,TR,Freq), countPartsInL(L,Part,N) }; not flow(From,To,Part,_); location(From); location(To); part(Part).
#false :- transportLink(From,To,L,TR,_); 2 <= #count { Freq1: transportLink(From,To,L,TR,Freq1) }.
% symmetry breaking
%numPart(p1,1).
%numPart(p2,2).
#false :- packingList((P1,P2),_,_); numPart(P1,V1); numPart(P2,V2); V1 > V2.
#false :- packingList((P1,(P2,_)),_,_); numPart(P1,V1); numPart(P2,V2); V1 > V2.
% optimization on transportation costs
:~ transportLink(From,To,_,TR,Freq); route(From,To,TR,D,C). [((Freq*D)*C)@0,From,To,TR]
#show flow/4.
#show transportLink/5.

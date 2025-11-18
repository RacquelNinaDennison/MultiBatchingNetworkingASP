1{flow(From,From,Part,N):offer(Part,From,N)}1 :- offer(Part,From,N), root(Part).
1{transportLink(From,From,intrasite,N)}1 :- flow(From,From,Part,N), root(Part).
1{assign(Part,intrasite,N)}1 :- flow(From,From,Part,N),root(Part).



demandSupply(P,L,0) :- not offer(P,L,_), not demand(P,L,_), part(P), location(L).
demandSupply(P,L,O) :- offer(P,L,O).
demandSupply(P,L,D) :- demand(P,L,M), D=M*-1.

% Extract arcs from routes
arc(Aid,From,To) :- route(From,To,TR,D,C), Aid = (From,To). 


#const maxNrParts = 20.
#const maxFlow    = 20.
numFlow(1..maxFlow).
num(0..maxFlow).


{flow(Aid,Part,N) : numFlow(N), part(Part) } :- arc(Aid,From,To).

% Ensure at most one N per (Aid,Part)
:- flow(Aid,Part,N1), flow(Aid,Part,N2), N1 != N2.
:- flow(Aid,Part,N), arc(Aid,From,To), demand(Part,From,_).
:- flow(Aid,Part,N), arc(Aid,From,To), offer(Part,To,_).


% incoming/outgoing per (Loc,Part,N)
inFlow(Loc,Part,N)  :- flow(Aid,Part,N), arc(Aid,From,Loc).
outFlow(Loc,Part,N) :- flow(Aid,Part,N), arc(Aid,Loc,To).

% conservation constraint
:- #sum{ -N : inFlow(Loc,Part,N); N : outFlow(Loc,Part,N) } != DS,
   demandSupply(Part,Loc,DS).



#show flow/3.
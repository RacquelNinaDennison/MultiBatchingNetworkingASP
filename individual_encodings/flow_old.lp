

1{flow(From,From,Part,N):offer(Part,From,N)}1 :- offer(Part,From,N), root(Part).
1{transportLink(From,From,intrasite,N)}1 :- flow(From,From,Part,N), root(Part).
1{assign(Part,intrasite,N)}1 :- flow(From,From,Part,N),root(Part).



demandSupply(P,L,0) :- not offer(P,L,_), not demand(P,L,_), part(P), location(L).
demandSupply(P,L,O) :- offer(P,L,O).
demandSupply(P,L,D) :- demand(P,L,M), D=M*-1.

#const maxNrParts = 20.
#const maxFreq = 20.

numFlow(1..maxNrParts).
num(0..maxFreq).

% constraints

% Specify possible flow

{flow(From,To,Part,N):numFlow(N),part(Part)} :-route(From,To,_,_,_).

:- flow(From,To,Part,N1), flow(From,To,Part,N2), N1!=N2.
:- flow(From,To,Part,N), demand(Part,From,D).
:- flow(From,To,Part,N), offer(Part,To,D).

% make sure that flow constraints hold
:- DS!=Out-In, In=#sum{N,From:flow(From,Loc,Part,N)}, Out=#sum{N,To:flow(Loc,To,Part,N)}, demandSupply(Part,Loc,DS).

from clingo import Control 
from clingo.ast import parse_files, ProgramBuilder
from ngo import optimize, Predicate,  auto_detect_input, auto_detect_output
import json

def runNgoFunction(input_encoding, input_instances,
                   input_predicates: list[Predicate],
                   output_predicates: list[Predicate]):
    ctl = Control()  
    prg = []
    parse_files([input_encoding], prg.append)

    prg = optimize(
        prg, auto_detect_input(prg), auto_detect_output(prg),
        cleanup=False, unused=False, duplication=False,
        symmetry=True, minmax_chains=False, sum_chains=False,
        math=True, inline=True, projection=True
    )

    with ProgramBuilder(ctl) as bld:
        for stm in prg:
            bld.add(stm)

    ctl.load(input_instances)
    ctl.ground([('base', [])])
    best = [None]
    def on_model(model):
        if model.optimality_proven:
            best[0] = model

    ctl.solve(on_model=on_model)
    optimal_model = best[0]
    # print(optimal_model)  # SAT/UNSAT/OPTIMUM
    print(json.dumps(ctl.statistics,
                     sort_keys=True, indent=4, separators=(',', ': ')))
    time_take_optimised = ctl.statistics['summary']['times']['total']
    return time_take_optimised, optimal_model

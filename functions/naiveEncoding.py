from clingo import Control 
from clingo.ast import parse_files, ProgramBuilder
from ngo import optimize, Predicate
import json

def runNaiveEncoding(input_encoding, input_instances):
    ctl = Control()  
    ctl.load(input_encoding)   
    ctl.load(input_instances)  
    ctl.ground([('base', [])])
    best = [None]
    def on_model(model):
        if model.optimality_proven:
            best[0] = model

    ctl.solve(on_model=on_model)
    print(json.dumps(ctl.statistics,
                     sort_keys=True, indent=4, separators=(',', ': ')))
    optimal_model = best[0]
    time_take_optimised = ctl.statistics['summary']['times']['total']
    return time_take_optimised, optimal_model


'''
This script consists of means of running the ngo file and testing the encoding. 
This is an optimised encoding technique that rewrites encodings. 

1. Run the ngo through the terminal?
2. Test with the python API - there are serveral optmisation techniques that can be used here.
'''
import functions.ngoFunction as ngoFunction
import functions.naiveEncoding as naiveEncoding 
from ngo import Predicate

def pythonAPICall():
    file_encoding= "naive-encoding/encoding.lp"
    file_instances  = "naive-encoding/instance.lp"
    file_optimised_encoding = "optimised_files/optimised_second.lp"
    file_optimised_instances = "optimised_files/optimised_instances.lp"
    input_predicates = [Predicate("location",1), Predicate("transportResource",1),Predicate("transportCapacity",2),Predicate("transportCO2",2) , Predicate("transportCost",2), Predicate("transportSpeed", 2),
                        Predicate("part",1), Predicate("partSize",2), Predicate("partTR",2), Predicate("partVal",2),
                        Predicate("offer",3), Predicate("demand",3), Predicate("route",5)]  # Specify input predicates if needed
    output_predicates = [Predicate("flow",4), Predicate("transportLink",5)] # Specify output predicates if needed
    
    #time_taken_optimised , optimal_model = ngoFunction.runNgoFunction(file_encoding, file_instances, input_predicates, output_predicates)
    time_taken_optimised , optimal_model = naiveEncoding.runNaiveEncoding(file_optimised_encoding, file_optimised_instances)
    time_taken_naive , optimal_model_naive = naiveEncoding.runNaiveEncoding(file_encoding, file_instances)
    print(f"Time taken for naive encoding: {time_taken_naive} seconds")
    print(f"Time taken for optimised encoding: {time_taken_optimised} seconds")


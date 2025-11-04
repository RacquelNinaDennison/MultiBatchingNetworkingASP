import os 

def runTerminalEncodings():
    print("Calling naive encoding via terminal...")
    os.system("clingo --outf=2 --quiet=1 naive-encoding/encoding.lp naive-encoding/instance.lp")
    print("Calling optimised encoding via terminal...")
    os.system("clingo --outf=2 --quiet=1 optimised_files/optimised_second.lp optimised_files/optimised_instances.lp")
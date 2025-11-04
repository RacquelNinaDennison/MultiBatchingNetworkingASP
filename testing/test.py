import json
import glob
import os

def average_call_time(folder: str):
    times = []
    for path in sorted(glob.glob(os.path.join(folder, "test*.json"))):
        with open(path, "r") as f:
            data = json.load(f)
        call_time = data["Time"]["Total"]
        times.append(call_time)

    avg = sum(times) / len(times)
    print(f"\nAverage over {len(times)} files in '{folder}': {avg:.3f} s")
    return avg

def main():
    average_call_time("naive")
    average_call_time("optimised")
    
if __name__ == "__main__":
    main()

  

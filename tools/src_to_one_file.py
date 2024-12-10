import glob
import os

# Gather al header and implementation files from the src dir into one .txt file
script_dir = os.path.dirname(os.path.realpath(__file__))
output_path = os.path.join(script_dir, "result.txt")

read_files = glob.glob(os.path.join(script_dir, "../src/*.cpp")) + glob.glob(os.path.join(script_dir, "../src/*.h"))

with open(output_path, "wb") as outfile:
    for f in read_files:
        with open(f, "rb") as infile:
            outfile.write(infile.read())
            outfile.write(b"\n") 

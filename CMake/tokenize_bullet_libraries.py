import os, sys

tokens = sys.argv[1:]
debug_libs = []
release_libs = []
for i in range(0, len(tokens), 2):
    config = tokens[i].strip()
    path = tokens[i+1].strip()
    if config == "debug":
        debug_libs.append('"' + path + '"')
    elif config == "optimized":
        release_libs.append('"' + path + '"')

print(" ".join(debug_libs) + ";" + " ".join(release_libs) + ";")

#
# Converts a SPIR-V format binary file to a bytes array to be imported in Wasabi
#
import os, sys

INPUT_FILE = sys.argv[1]
OUTPUT_FILE = sys.argv[2]

def formatCode(code):
	return ", ".join(list(map(lambda c: hex(c), code))).encode('utf-8')

with open(INPUT_FILE, "rb") as F:
    spirvCode = F.read()

formattedCode = formatCode(spirvCode)

with open(OUTPUT_FILE, "wb") as F:
    F.write(formattedCode)

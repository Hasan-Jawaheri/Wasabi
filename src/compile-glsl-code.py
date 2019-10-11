import os, sys
import traceback
from subprocess import Popen, PIPE
import json

if len(sys.argv) != 3:
	print("Usage: {} <VULKAN_SDK_PATH> <SOURCES_PATH>".format(sys.argv[0]))
	exit(1)

VULKAN_SDK = sys.argv[1]
SOURCES_PATH = sys.argv[2]

if not os.path.isdir(VULKAN_SDK):
	print("INVALID VULKAN_SDK environment variable (VULKAN_SDK: {})".format(VULKAN_SDK))
	exit(1)

GLSLANG_VALIDATOR_PATHS = [
	"{}/Bin/glslangValidator".format(VULKAN_SDK),
	"{}/Bin/glslangValidator.exe".format(VULKAN_SDK),
	"{}/bin/glslangValidator".format(VULKAN_SDK),
	"{}/bin/glslangValidator.exe".format(VULKAN_SDK)
]

GLSLANG_VALIDATOR_PATH = None
for validatorPath in GLSLANG_VALIDATOR_PATHS:
	if os.path.isfile(validatorPath):
		GLSLANG_VALIDATOR_PATH = validatorPath
		break
if GLSLANG_VALIDATOR_PATH is None:
	print("CANNOT FIND glslangValidator executable (SDK path: {})".format(VULKAN_SDK))
	exit(1)

FILE_MODIFIED_TIMES = {}
try:
	with open("glsl-files-cache.json", "r") as F:
		FILE_MODIFIED_TIMES = json.load(F)
except: pass

def formatCode(code):
	return ", ".join(list(map(lambda c: hex(c), code))).encode('utf-8')

def compileShader(filename):
	tmp_output_file = filename + ".tmp.spv"
	real_output_file = filename + ".spv"

	# if the output file exists, check if we don't need to compile
	if os.path.isfile(real_output_file):
		if str(os.stat(filename).st_mtime) == FILE_MODIFIED_TIMES.get(filename, None):
			# file was compiled already
			return 0
	
	print("Compiling {}".format(filename))
	try:
		entry_point = "main"
		command = [GLSLANG_VALIDATOR_PATH, "-V", "--entry-point", entry_point, filename, "-o", tmp_output_file]
		print(" ".join(command))
		p = Popen(command, stdout=PIPE, stderr=PIPE)
		out = p.stdout.read().decode('utf-8').replace('\r', '')
		err = p.stderr.read().decode('utf-8').replace('\r', '')
		p.wait()
		if p.returncode != 0:
			print(out)
			print(err)
			return p.returncode
		with open(tmp_output_file, "rb") as F:
			spirv_code = F.read()
		formatted_code = formatCode(spirv_code)
		try:
			with open(real_output_file, "rb") as F:
				previously_compiled_spirv = F.read()
		except:
			previously_compiled_spirv = None

		if formatted_code != previously_compiled_spirv:
			with open(real_output_file, "wb") as F:
				F.write(formatted_code)
		
		# cache the result
		FILE_MODIFIED_TIMES[filename] = str(os.stat(filename).st_mtime)
		with open("glsl-files-cache.json", "w") as F:
			json.dump(FILE_MODIFIED_TIMES, F)
		
		os.remove(tmp_output_file)

		return 0
	except:
		print(traceback.format_exc())
		return 1

ret = 0
for root, dirs, files in os.walk(SOURCES_PATH):
	for file in files:
		if file.endswith(".vert.glsl") or file.endswith(".geom.glsl") or file.endswith(".frag.glsl") or file.endswith(".tesc.glsl") or file.endswith(".tese.glsl") or file.endswith(".comp.glsl"):
			fullFilename = os.path.join(root, file)
			ret += compileShader(fullFilename)
exit(ret)
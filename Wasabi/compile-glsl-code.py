import os, sys
import traceback
from subprocess import Popen, PIPE

VULKAN_SDK = os.environ['VULKAN_SDK']

def formatCode(code):
	return ", ".join(list(map(lambda c: hex(c), code))).encode('utf-8')

def compileShader(filename):
	print("======================== Compiling %s ========================" % (filename))
	try:
		output_file = filename + ".spv"
		entry_point = "main"
		command = ["%s/Bin/glslangValidator" % (VULKAN_SDK), "-V", "--entry-point", entry_point, filename, "-o", output_file]
		print (command)
		p = Popen(command, stdout=PIPE, stderr=PIPE)
		out = p.stdout.read().decode('utf-8').replace('\r', '')
		err = p.stderr.read().decode('utf-8').replace('\r', '')
		p.wait()
		if p.returncode != 0:
			print(out)
			print(err)
			return p.returncode
		with open(output_file, "rb") as F:
			spirv_code = F.read()
		with open(output_file, "wb") as F:
			F.write(formatCode(spirv_code))
		return 0
	except:
		print(traceback.format_exc())
		return 1

ret = 0
for root, dirs, files in os.walk("./"):
    for file in files:
        if file.endswith(".glsl"):
             ret += compileShader(os.path.join(root, file))
sys.exit(ret)

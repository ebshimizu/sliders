import sys
import json
import os
from shutil import copyfile

def runOpts(settings, iters, outDir):
	dataFile = open(settings, 'r')
	data = json.load(dataFile)

	ceresOutputDir = data["saveTo"]

	for x in range(0, iters):
		# run the optimization, copy the file to specified location after run
		os.system("../ceres_harness/x64/Release/ceresHarness.exe config " + settings)
		copyfile(ceresOutputDir + "evo_log.json", outDir + "evo_log_" + str(x) + ".json")

if __name__ == "__main__":
	runOpts(sys.argv[1], int(sys.argv[2]), sys.argv[3])
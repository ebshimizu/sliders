import sys
import json
import os
from shutil import copyfile

def runOpts(settings, iters, outDir):
	dataFile = open(settings, 'r')
	data = json.load(dataFile)

	ceresOutputDir = data["saveTo"]
	analysisCmd = "python ..\\..\\scripts\\evoPlotCompare.py"

	for x in range(0, iters):
		# run the optimization, copy the file to specified location after run
		#os.system('"..\\..\\ceres_harness\\x64\\Release\\ceresHarness.exe" config ' + settings)

		#print "copy " + ceresOutputDir + "evo_log.json" + " to " + outDir + "evo_log_" + str(x) + ".json"

		#copyfile(ceresOutputDir + "evo_log.json", outDir + "evo_log_" + str(x) + ".json")
		analysisCmd = analysisCmd + " " + outDir + "evo_log_" + str(x) + ".json"

	analysisCmd = analysisCmd + " " + outDir + "summary"
	os.system(analysisCmd)

if __name__ == "__main__":
	runOpts(sys.argv[1], int(sys.argv[2]), sys.argv[3])
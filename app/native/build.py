#!/usr/bin/env python
import sys
import os
import getopt

def build():
	try:
		opts, args = getopt.getopt(sys.argv[1:], "cdnr", ['configure','debug','node-only','rebuild'])
	except getopt.GetoptError as err:
		print str(err)
		sys.exit(-1)

	# node gyp options

	# run in config mode
	configure = False

	# build configuration
	buildRelease = True

	# build with electron or no
	withElectron = True

	# rebuild application
	rebuild = False

	for o, a in opts:
		if o in ("-c", "--configure"):
			configure = True
		elif o in ("-d", "--debug"):
			buildRelease = False
		elif o in ("-n", "--node-only"):
			withElectron = False
		elif o in ("-r", "--rebuild"):
			rebuild = True

	if (configure):
		# run configuration
		print("node-gyp configure")
		os.system("node-gyp configure")
	else:
		cmd = "node-gyp"

		if (rebuild):
			cmd = cmd + " rebuild"
		else:
			cmd = cmd + " build"

		if (not buildRelease):
			cmd = cmd + " --debug"

		if (withElectron):
			cmd = cmd + " --target=1.6.8 --arch=x64 --dist-url=https://atom.io/download/electron"

		print(cmd)
		os.system(cmd)

if __name__ == "__main__":
	build()
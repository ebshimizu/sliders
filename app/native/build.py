#!/usr/bin/env python
import sys
import os
import getopt
import shutil

def build():
	try:
		opts, args = getopt.getopt(sys.argv[1:], "cdnr", ['configure','debug','node-only','rebuild', 'move-debug-dir'])
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

	# move debug dir
	moveDebugDir = False

	for o, a in opts:
		if o in ("-c", "--configure"):
			configure = True
		elif o in ("-d", "--debug"):
			buildRelease = False
		elif o in ("-n", "--node-only"):
			withElectron = False
		elif o in ("-r", "--rebuild"):
			rebuild = True
		elif o in ("--move-debug-dir"):
			moveDebugDir = True

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
			#cmd = cmd + " --target=1.8.4 --arch=x64 --dist-url=https://atom.io/download/electron"
			cmd = cmd + " --target=2.0.9 --arch=x64 --dist-url=https://atom.io/download/electron"

		print(cmd)
		os.system(cmd)

		if (not buildRelease):
			# saved a local settings file that has debug setup, copy to build folder
			shutil.copyfile("./debug/compositor.vcxproj.user", "./build/compositor.vcxproj.user")

			print("Copied windows debug startup settings to build folder.")

		if (moveDebugDir):
			print("Deleting existing debug_build (if it exists)...")
			if (os.path.isdir("./debug_build")):
				shutil.rmtree("./debug_build")

			print("Moving build folder to 'debug_build'...")
			os.rename("./build", "./debug_build")

			print("Build folder moved")


if __name__ == "__main__":
	build()

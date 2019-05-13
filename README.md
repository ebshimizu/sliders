# Hover Visualization Interface

Prototype interface implementing a hover visualization layer selection paradigm.

## Setup

Prereq: Python 2, your preferred flavor of node.js.

Clone and init submodules recursively.

In both `src/app` and `src/node-compositor`, run `npm install`. In `src/node-compositor`, also
run `python ./build.py -r`. If this step fails, you should check line 60 and verify that the version number
matches the installed version of electron.

To launch the app, after verifying that the native module compilation
succeeded, launch electron from the node modules path in `src/app`: `./node_modules/electron/dist/electron.exe ./`.

## Usage

To use the application, you will need to load a document in a specific format. A set of
sample compositions can be found here (Link forthcoming). Open them with `File > Open`.

If you'd like to try loading your own compositions, you will first need to export them from Adobe Photoshop
by using the `extractLayers.jsx` Photoshop script. This script requires `Get.jsx` and `Getter.jsx` to
also be in the Photoshop scripts folder in order to work. To execute properly, you will need to make sure
each layer is given a unique name. If the script executes properly, you will found a `.json` file that can be
imported into the interface (`File > Import`) and saved to the interface's specific format (`.dark`).

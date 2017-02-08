/*********************************************************************
extractLayers.jsx
author: Evan Shimizu

Exports layers as PNGs along with compositing information in JSON
format.
**********************************************************************/

/*
// BEGIN__HARVEST_EXCEPTION_ZSTRING

<javascriptresource>
<name>Export Layers To...</name>
<menu>export</menu>
</javascriptresource>

// END__HARVEST_EXCEPTION_ZSTRING
*/

// returns a list of all the artLayers in the specified layerSet.
function getAllArtLayers(layers) {
	var ret = []

	for( var i = 0; i < layers.artLayers.length; i++) {
        ret.push(layers.artLayers[i])
    }

    for( var i = 0; i < layers.layerSets.length; i++) {
        ret = ret.concat(getAllArtLayers(layers.layerSets[i]))
    }

    return ret
}

function turnOnParents(obj) {
	if (obj.parent) {
		obj.parent.visible = true
		turnOnParents(obj.parent)
	}
}

function turnOffAll(obj) {
	for(var i = 0; i < obj.artLayers.length; i++) {
        obj.artLayers[i].visible = false
    }

    for(var i = 0; i < obj.layerSets.length; i++) {
    	obj.layerSets[i].visible = false
        turnOffAll(obj.layerSets[i])
    }
}

var doc = app.activeDocument

var outDir = Folder.selectDialog("Layer Export Location")

// save out layers one at a time
var layers = getAllArtLayers(doc)

var metadata = {}

// progress window setup
var userCanceled = false
var progressWindow = new Window("palette { text:'Exporting Layers', \
    statusText: StaticText { text: 'Exporting Layers...', preferredSize: [350,20] }, \
    progressGroup: Group { \
        progress: Progressbar { minvalue: 0, maxvalue: 100, value: 0, preferredSize: [300,20] }, \
        cancelButton: Button { text: 'Cancel' } \
    } \
}");
var statusText = progressWindow.statusText;
var progress = progressWindow.progressGroup.progress;
progressWindow.progressGroup.cancelButton.onClick = function() { userCanceled = true; }
progressWindow.show();

for (var i = 0; i < layers.length; i++) {
	if (userCanceled) {
		break;
	}

	// get current layer being exported
	var activeLayer = layers[i]

	statusText.text = "Exporting layer " + activeLayer.name + " (" + (i + 1) + "/" + layers.length + ")"
	progress.value = (i / layers.length) * 100;

	// save compositing information
	// Note that if the layer is an adjustment layer, we don't currently have a
	// way to extract the settings for that later in a script.
	// TODO: Ask Photoshop team about accessing adjustment layer properties
	metadata[activeLayer.name] = {}
	metadata[activeLayer.name]["visible"] = activeLayer.visible
	metadata[activeLayer.name]["opacity"] = activeLayer.opacity
	metadata[activeLayer.name]["blendMode"] = activeLayer.blendMode
	metadata[activeLayer.name]["kind"] = activeLayer.kind
	metadata[activeLayer.name]["parent"] = activeLayer.parent.name

	// Toggle visibility for all other layers
	turnOffAll(doc)

	// Enable visibility for current layer
	activeLayer.visible = true

	// Background layers don't have these options editable
	if (!activeLayer.isBackgroundLayer) {
		activeLayer.opacity = 100
		activeLayer.blendMode = BlendMode.NORMAL
	}

	// Ensure parent containers are also visible
	turnOnParents(activeLayer)

	// save layer
	var pngOpts = new PNGSaveOptions()
	pngOpts.compression = 0
	pngOpts.interlaced = false

	// sanitize filename
	var filename = activeLayer.name.replace(/\//, '-')
	metadata[activeLayer.name]["filename"] = filename + ".png"

	doc.saveAs(new File(outDir.absoluteURI + "/" + filename),
		pngOpts, true, Extension.LOWERCASE)

	// restore settings
	activeLayer.visible = metadata[activeLayer.name]["visible"]

	if (!activeLayer.isBackgroundLayer && activeLayer.kind == LayerKind.NORMAL) {
		activeLayer.opacity = metadata[activeLayer.name]["opacity"]
		activeLayer.blendMode = metadata[activeLayer.name]["blendMode"]
	}
}

statusText.text = "Exporting layer info"
progress.value = 100

// save metadata
var str = "{\n"
str += '\t"filename" : "' + doc.name + '",\n';
var jsonObjs = []
// we'll make our own JSON for this yay
for (var key in metadata) {
	var json = '\t"' + key + '" : {\n'

	var dictEntries = []
	for (var subKey in metadata[key]) {
		dictEntries.push('\t\t"' + subKey + '" : "' + metadata[key][subKey] + '"')
	}

	json += dictEntries.join(',\n')
	json += "\n\t}"

	jsonObjs.push(json)
}

str += jsonObjs.join(',\n')
str += "\n}"

// save JSON file
var metaFile = new File(outDir.absoluteURI + "/" + doc.name + ".meta")
metaFile.open('w')
metaFile.write(str)
metaFile.close()

progressWindow.hide()
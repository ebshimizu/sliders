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

#include "json2.js"

// some globals to make the code easier to read
var classActionSet = app.stringIDToTypeID("actionSet");
var classAction = app.stringIDToTypeID("action");
var classApplication = app.stringIDToTypeID("application");
var classBackgroundLayer = app.stringIDToTypeID("backgroundLayer");
var classChannel = app.stringIDToTypeID("channel");
var classDocument = app.stringIDToTypeID("document");
var classHistoryState = app.stringIDToTypeID("historyState");
var classLayer = app.stringIDToTypeID("layer");
var classPath = app.stringIDToTypeID("path");
var classProperty = app.stringIDToTypeID("property");
var enumTarget = app.stringIDToTypeID("targetEnum");
var enumZoom = app.stringIDToTypeID("zoom");
var eventSet = app.stringIDToTypeID( "set" );
var keyBackground = app.stringIDToTypeID("background");
var keyCount = app.stringIDToTypeID("count");
var keyTo = app.stringIDToTypeID( "to" );
var kcachePrefs = app.stringIDToTypeID("cachePrefs");
var kgeneratorEnabledStrID = app.stringIDToTypeID("generatorEnabled");
var kgeneratorSettingsStrID = app.stringIDToTypeID("generatorSettings");
var kgeneratorStatusStrID = app.stringIDToTypeID("generatorStatus");
var kLayerID = app.stringIDToTypeID("layerID");
var kLayerName = app.stringIDToTypeID("name");
var kopenglEnabled = app.stringIDToTypeID("openglEnabled");
var kpluginPickerStrID = app.stringIDToTypeID("pluginPicker");
var kreadBytesStrID = app.stringIDToTypeID("readBytes");
var kreadMessagesStrID = app.stringIDToTypeID("readMessages");
var kreadStatusStrID = app.stringIDToTypeID("readStatus");
var ksmartObjectMoreStr = app.stringIDToTypeID("smartObjectMore");
var kwriteBytesStrID = app.stringIDToTypeID("writeBytes");
var kwriteMessagesStrID = app.stringIDToTypeID("writeMessages");
var kwriteStatusStrID = app.stringIDToTypeID("writeStatus");
var kzoomStr = app.stringIDToTypeID("zoom");
var typeOrdinal = app.stringIDToTypeID("ordinal");
var typeNULL = app.stringIDToTypeID("null");
var unitPercent = app.stringIDToTypeID("percentUnit");

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

function getAdjustmentLayerInfo(doc) {
	var ref = new ActionReference();
	ref.putEnumerated(classDocument, typeOrdinal, enumTarget);
    activeDocument = doc;

    desc = executeActionGet(ref);
    var docData = new Object();
    docData.objectName = "Photoshop Document";
    DescriptorToObject(docData, desc);
    
    // the following have DescriptorToObject and ObjectToFile calls included
    docData.flatLayers = new Array();
    GetBackgroundInfo(docData.flatLayers);
    GetLayerInfo(docData.flatLayers, docData.numberOfLayers);

    // extract required data
    var adj = {}
    for (var i = 0; i < docData.flatLayers.length; i++) {
    	if (docData.flatLayers[i].objectName == "Photoshop Background Layer") {
    		continue;
    	}

    	var name = docData.flatLayers[i].name

    	// search through raw json for proper adjustment object
    	var json = docData.flatLayers[i].json
    	var layers = json.layers

    	// find layer with matching name, must be recursive, some layers have
    	// nested layers
		findAdjProps(name, layers, adj);
    }

    return adj
}

function findAdjProps(name, layers, adj) {
	for (var i = 0; i < layers.length; i++) {
		if (layers[i].name == name) {
			if ("adjustment" in layers[i]) {
				adj[name] = layers[i]["adjustment"] 
			}
		}
		else {
			if ("layers" in layers[i]) {
				findAdjProps(name, layers[i]["layers"], adj)
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Function: GetBackgroundInfo
// Usage: Get all info about the current documents background layer
// Input: JavaScript Array (inOutArray), current Array of layers found
//        JavaScript File (inLogFile), file to append to information to
// Return: Nothing, inOutArray is updated and log file is appended
///////////////////////////////////////////////////////////////////////////////
function GetBackgroundInfo(inOutArray) {
    try {
        var ref = new ActionReference();
        ref.putProperty(classBackgroundLayer, keyBackground);
        ref.putEnumerated(classDocument, typeOrdinal, enumTarget);
        desc = executeActionGet(ref);
        var backgroundData = new Object();
        backgroundData.objectName = "Photoshop Background Layer";
        DescriptorToObject(backgroundData, desc);
        inOutArray.push(backgroundData);
   }
   catch(e) {
       ; // current document might not have a background or no document open
   }
}

///////////////////////////////////////////////////////////////////////////////
// Function: DescriptorToObject
// Usage: update a JavaScript Object from an ActionDescriptor
// Input: JavaScript Object (o), current object to update (output)
//  Photoshop ActionDescriptor (d), descriptor to pull new params for object from
//  JavaScript Function (f), post process converter utility to convert
// Return: Nothing, update is applied to passed in JavaScript Object (o)
///////////////////////////////////////////////////////////////////////////////
function DescriptorToObject(o, d, f) {
	var l = d.count;
	for (var i = 0; i < l; i++) {
		var k = d.getKey(i);
		var t = d.getType(k);
		var strk = app.typeIDToStringID(k);
        if (strk.length == 0) {
            strk = app.typeIDToCharID(k);
        }
		switch (t) {
			case DescValueType.BOOLEANTYPE:
				o[strk] = d.getBoolean(k);
				break;
			case DescValueType.STRINGTYPE:
				o[strk] = d.getString(k);
				break;
			case DescValueType.DOUBLETYPE:
				o[strk] = d.getDouble(k);
				break;
			case DescValueType.INTEGERTYPE:
                o[strk] = d.getInteger(k);
                break;
            case DescValueType.LARGEINTEGERTYPE:
            	o[strk] = d.getLargeInteger(k);
            	break;
			case DescValueType.OBJECTTYPE:
                var newT = d.getObjectType(k);
                var newV = d.getObjectValue(k);
                o[strk] = new Object();
                DescriptorToObject(o[strk], newV, f);
                break;
			case DescValueType.UNITDOUBLE:
                var newT = d.getUnitDoubleType(k);
                var newV = d.getUnitDoubleValue(k);
                o[strk] = new Object();
                o[strk].type = typeIDToCharID(newT);
                o[strk].typeString = typeIDToStringID(newT);
                o[strk].value = newV;
                break;
			case DescValueType.ENUMERATEDTYPE:
                var newT = d.getEnumerationType(k);
                var newV = d.getEnumerationValue(k);
                o[strk] = new Object();
                o[strk].type = typeIDToCharID(newT);
                o[strk].typeString = typeIDToStringID(newT);
                o[strk].value = typeIDToCharID(newV);
                o[strk].valueString = typeIDToStringID(newV);
                break;
			case DescValueType.CLASSTYPE:
                o[strk] = d.getClass(k);
                break;
			case DescValueType.ALIASTYPE:
                o[strk] = d.getPath(k);
                break;
			case DescValueType.RAWTYPE:
                var tempStr = d.getData(k);
                o[strk] = new Array();
                for (var tempi = 0; tempi < tempStr.length; tempi++) { 
                    o[strk][tempi] = tempStr.charCodeAt(tempi); 
                }
                break;
			case DescValueType.REFERENCETYPE:
                var ref = d.getReference(k);
                o[strk] = new Object();
                ReferenceToObject(o[strk], ref, f);
                break;
			case DescValueType.LISTTYPE:
                var list = d.getList(k);
                o[strk] = new Array();
                ListToObject(o[strk], list, f);
                break;
			default:
				myLogging.LogIt("Unsupported type in descriptorToObject " + t);
		}
	}
	if (undefined != f) {
		o = f(o);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Function: ReferenceToObject
// Usage: update a JavaScript Object from an ActionReference
// Input: JavaScript Object (o), current object to update (output)
//  Photoshop ActionReference (r), reference to pull new params for object from
//  JavaScript Function (f), post process converter utility to convert
// Return: Nothing, update is applied to passed in JavaScript Object (o)
///////////////////////////////////////////////////////////////////////////////
function ReferenceToObject(o, r, f) {
 // TODO : seems like I should output the desiredClass information here
 // maybe all references have a name, index, etc. and then the are either undefined
 // what about clobber, can I get an index in an index and then that would not work
 // should the second loop be doing something different?
    var originalRef = r;
	while (r != null) {
		var refForm = r.getForm();
        var refClass = r.getDesiredClass();
        var strk = app.typeIDToStringID(refClass);
        if (strk.length == 0) {
            strk = app.typeIDToCharID(refClass);
        }
		switch (refForm) {
			case ReferenceFormType.NAME:
				o["name"] = r.getName();
				break;
			case ReferenceFormType.INDEX:
				o["index"] = r.getIndex();
				break;
			case ReferenceFormType.IDENTIFIER:
                o["indentifier"] = r.getIdentifier();
                break;
			case ReferenceFormType.OFFSET:
                o["offset"] = r.getOffset();
                break;
			case ReferenceFormType.ENUMERATED:
                var newT = r.getEnumeratedType();
                var newV = r.getEnumeratedValue();
                o["enumerated"] = new Object();
                o["enumerated"].type = typeIDToCharID(newT);
                o["enumerated"].typeString = typeIDToStringID(newT);
                o["enumerated"].value = typeIDToCharID(newV);
                o["enumerated"].valueString = typeIDToStringID(newV);
                break;
			case ReferenceFormType.PROPERTY:
                o["property"] = app.typeIDToStringID(r.getProperty());
                if (o["property"].length == 0) {
                    o["property"] = app.typeIDToCharID(r.getProperty());
                }
                break;
			case ReferenceFormType.CLASSTYPE:
                o["class"] = refClass; // i already got that r.getDesiredClass(k);
                break;
			default:
				myLogging.LogIt("Unsupported type in referenceToObject " + t);
		}
        r = r.getContainer();
        try {
            r.getDesiredClass();
        } catch(e) {
            r = null;
        }
	}
	if (undefined != f) {
		o = f(o);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Function: ListToObject
// Usage: update a JavaScript Object from an ActionList
// Input: JavaScript Array (a), current array to update (output)
//        Photoshop ActionList (l), list to pull new params for object from
//        JavaScript Function (f), post process converter utility to convert
// Return: Nothing, update is applied to passed in JavaScript Object (o)
///////////////////////////////////////////////////////////////////////////////
function ListToObject(a, l, f) {
	var c = l.count;
	for (var i = 0; i < c; i++) {
		var t = l.getType(i);
		switch (t) {
			case DescValueType.BOOLEANTYPE:
				a.push(l.getBoolean(i));
				break;
			case DescValueType.STRINGTYPE:
				a.push(l.getString(i));
				break;
			case DescValueType.DOUBLETYPE:
				a.push(l.getDouble(i));
				break;
			case DescValueType.INTEGERTYPE:
                a.push(l.getInteger(i));
                break;
            case DescValueType.LARGEINTEGERTYPE:
            	a.push(l.getLargeInteger(i));
            	break;
			case DescValueType.OBJECTTYPE:
                var newT = l.getObjectType(i);
                var newV = l.getObjectValue(i);
                var newO = new Object();
                a.push(newO);
                DescriptorToObject(newO, newV, f);
                break;
			case DescValueType.UNITDOUBLE:
                var newT = l.getUnitDoubleType(i);
                var newV = l.getUnitDoubleValue(i);
                var newO = new Object();
                a.push(newO);
                newO.type = typeIDToCharID(newT);
                newO.typeString = typeIDToStringID(newT);
                newO.value = newV;
                break;
			case DescValueType.ENUMERATEDTYPE:
                var newT = l.getEnumerationType(i);
                var newV = l.getEnumerationValue(i);
                var newO = new Object();
                a.push(newO);
                newO.type = typeIDToCharID(newT);
                newO.typeString = typeIDToStringID(newT);
                newO.value = typeIDToCharID(newV);
                newO.valueString = typeIDToStringID(newV);
                break;
			case DescValueType.CLASSTYPE:
                a.push(l.getClass(i));
                break;
			case DescValueType.ALIASTYPE:
                a.push(l.getPath(i));
                break;
			case DescValueType.RAWTYPE:
                var tempStr = l.getData(i);
                tempArray = new Array();
                for (var tempi = 0; tempi < tempStr.length; tempi++) { 
                    tempArray[tempi] = tempStr.charCodeAt(tempi); 
                }
                a.push(tempArray);
                break;
			case DescValueType.REFERENCETYPE:
                var ref = l.getReference(i);
                var newO = new Object();
                a.push(newO);
                ReferenceToObject(newO, ref, f);
                break;
			case DescValueType.LISTTYPE:
                var list = l.getList(i);
                var newO = new Object();
                a.push(newO);
                ListToObject(newO, list, f);
                break;
			default:
				myLogging.LogIt("Unsupported type in descriptorToObject " + t);
		}
	}
	if (undefined != f) {
		o = f(o);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Function: GetLayerInfo
// Usage: Get all info about the current documents layers
// Input: JavaScript Array (inOutArray), current Array of layers found
//        inFlatLayerCount, the number of layers in the document, not counting background layer
//        JavaScript File (inLogFile), file to append to information to
// Return: Nothing, inOutArray is updated and log file is appended
///////////////////////////////////////////////////////////////////////////////
function GetLayerInfo(inOutArray, inFlatLayerCount) {
    for (var i = inFlatLayerCount; i > 0; i--) {
        var ref = new ActionReference();
        ref.putIndex(classLayer, i);
        ref.putEnumerated(classDocument, typeOrdinal, enumTarget);
        desc = executeActionGet(ref);
        var layerData = new Object();
        layerData.objectName = "Photoshop Layer";
        DescriptorToObject(layerData, desc);
        var ref = new ActionReference();
        ref.putProperty(classProperty, stringIDToTypeID("json"));
        ref.putIndex(classLayer, i);
        ref.putEnumerated(classDocument, typeOrdinal, enumTarget);
        desc = executeActionGet(ref);
        DescriptorToObject(layerData, desc);
        var ref = new ActionReference();
        ref.putProperty(classProperty, stringIDToTypeID("json"));
        ref.putIndex(classLayer, i);
        ref.putEnumerated(classDocument, typeOrdinal, enumTarget);
        desc = executeActionGet(ref);
        DescriptorToObject(layerData, desc);
        layerData.json = eval("a = " + layerData.json);
        inOutArray.push(layerData);
   }
}

var doc = app.activeDocument

var outDir = Folder.selectDialog("Layer Export Location")

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

statusText.text = "Gathering Layer Data..."

// save out layers one at a time
var layers = getAllArtLayers(doc)

// we need the adjustment layer data and the way to get that is pretty gross.
// The function called here will return a map from layer name to adjustment properties
var adjLayerParams = getAdjustmentLayerInfo(doc)

var metadata = {}

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
	metadata[activeLayer.name] = {}
	metadata[activeLayer.name]["visible"] = activeLayer.visible
	metadata[activeLayer.name]["opacity"] = activeLayer.opacity
	metadata[activeLayer.name]["blendMode"] = activeLayer.blendMode
	metadata[activeLayer.name]["kind"] = activeLayer.kind
	metadata[activeLayer.name]["parent"] = activeLayer.parent.name

	if (activeLayer.name in adjLayerParams) {
		// dump adjustment layer info
		metadata[activeLayer.name]["adjustment"] = adjLayerParams[activeLayer.name].toSource()
	}

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
    if (!activeLayer.isBackgroundLayer && activeLayer.kind == LayerKind.NORMAL) {
        activeLayer.opacity = metadata[activeLayer.name]["opacity"]
        activeLayer.blendMode = metadata[activeLayer.name]["blendMode"]
    }

	activeLayer.visible = metadata[activeLayer.name]["visible"]

	// stringify for later export
	metadata[activeLayer.name]["blendMode"] = activeLayer.blendMode.toString()
	metadata[activeLayer.name]["kind"] = activeLayer.kind.toString()
}

statusText.text = "Exporting layer info"
progress.value = 100

// save metadata file
var metaFile = new File(outDir.absoluteURI + "/" + doc.name + ".meta")
metaFile.open('w')
metaFile.write(JSON.stringify(metadata))
metaFile.close()

progressWindow.hide()
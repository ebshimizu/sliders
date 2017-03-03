const comp = require('../native/build/Release/compositor')
var {dialog, app} = require('electron').remote
var fs = require('fs')

comp.setLogLevel(1)

// initializes a global compositor object to operate on
var c = new comp.Compositor()

// global settings vars
var g_renderSize;

const blendModes = {
    "BlendMode.NORMAL" : 0,
    "BlendMode.MULTIPLY" : 1,
    "BlendMode.SCREEN" : 2,
    "BlendMode.OVERLAY" : 3,
    "BlendMode.HARDLIGHT" : 4,
    "BlendMode.SOFTLIGHT" : 5,
    "BlendMode.LINEARDODGE" : 6,
    "BlendMode.COLORDODGE" : 7,
    "BlendMode.LINEARBURN" : 8,
    "BlendMode.LINEARLIGHT" : 9,
    "BlendMode.COLOR" : 10,
    "BlendMode.LIGHTEN" : 11,
    "BlendMode.DARKEN" : 12,
    "BlendMode.PINLIGHT" : 13
}

const adjType = {
    "HSL" : 0,
    "LEVELS" : 1,
    "CURVES" : 2,
    "EXPOSURE" : 3,
    "GRADIENTMAP" : 4,
    "SELECTIVE_COLOR" : 5,
    "COLOR_BALANCE" : 6,
    "PHOTO_FILTER" : 7,
    "COLORIZE" : 8,
    "LIGHTER_COLORIZE" : 9
}

/*===========================================================================*/
/* Initialization                                                            */
/*===========================================================================*/

// Initializes html element listeners on document load
function init() {
    $("#renderCmd").on("click", function() {
        renderImage()
    });

    $("#openCmd").click(function() { loadFile(); });
    $("#exitCmd").click(function() { app.quit(); })

    $(".ui.dropdown").dropdown();

    $('.paramSlider').slider({
        orientation: "horizontal",
        range: "min",
        max: 100,
        min: 0
    });

    $('#renderSize a.item').click(function() {
        // reset icon status
        var links = $('#renderSize a.item')
        for (var i = 0; i < links.length; i++) {
            $(links[i]).html($(links[i]).attr("name"));
        }

        // add icon to selected
        $(this).prepend('<i class="checkmark icon"></i>');

        g_renderSize = $(this).attr("internal");
        renderImage();
    })

    // autoload
    //openLayers("C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/shapes/shapes.json", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/shapes")

    initUI()
}

// Renders an image from the compositor module and
// then puts it in an image tag
function renderImage() {
    var dat = 'data:image/png;base64,';

    if (g_renderSize === "full") {
        dat += c.render().base64();
    }
    else {
        dat += c.render(g_renderSize).base64();
    }

    $("#render").html('<img src="' + dat + '"" />')
}

function initUI() {
    // for now we assume the compositor is already initialized for testing purposes
    var layers = c.getLayerNames()

    for (var layer in layers) {
        createLayerControl(layers[layer])
    }

    initGlobals();
}

// default global variable settings
function initGlobals() {
    g_renderSize = "full";
}

function createLayerControl(name, pre, kind) {
    var controls = $('#layerControls')
    var layer = c.getLayer(name)

    // controls are created based on what adjustments each layer has

    // create the html
    var html = '<div class="layer" layerName="' + name + '">'
    html += '<h3 class="ui grey inverted header">' + name + '</h3>'

    if (layer.visible()) {
        html += '<button class="ui icon button mini white visibleButton" layerName="' + name + '">'
        html += '<i class="unhide icon"></i>'
    }
    else {
        html += '<button class="ui icon button mini black visibleButton" layerName="' + name + '">'
        html += '<i class="hide icon"></i>'
    }

    html += '</button>'

    // blend mode options
    html += genBlendModeMenu(name)

    // generate parameters
    html += createLayerParam(name, "opacity")

    // separate handlers for each adjustment type
    var adjustments = layer.getAdjustments();

    for (var i = 0; i < adjustments.length; i++) {
        if (i === 0) {
            // hue sat
            html += createLayerParam(name, "hue");
            html += createLayerParam(name, "saturation");
            html += createLayerParam(name, "lightness");
        }
        else if (i === 1) {
            // levels
            // TODO: Turn some of these into range sliders
            html += createLayerParam(name, "inMin");
            html += createLayerParam(name, "inMax");
            html += createLayerParam(name, "gamma");
            html += createLayerParam(name, "outMin");
            html += createLayerParam(name, "outMax");
        }
        else if (i === 2) {
            // curves
        }
        else if (i === 3) {
            // exposure
            html += createLayerParam(name, "exposure");
            html += createLayerParam(name, "offset");
            html += createLayerParam(name, "gamma");
        }
        else if (i === 4) {
            // gradient
        }
        else if (i === 5) {
            // selective color
        }
        else if (i === 6) {
            // color balance
            html += createLayerParam(name, "shadow R");
            html += createLayerParam(name, "shadow G");
            html += createLayerParam(name, "shadow B");
            html += createLayerParam(name, "mid R");
            html += createLayerParam(name, "mid G");
            html += createLayerParam(name, "mid B");
            html += createLayerParam(name, "highlight R");
            html += createLayerParam(name, "highlight G");
            html += createLayerParam(name, "highlight B");
        }
        else if (i === 7) {
            // photo filter
            html += createLayerParam(name, "red");
            html += createLayerParam(name, "green");
            html += createLayerParam(name, "blue");
            html += createLayerParam(name, "density");
        }
        else if (i === 8) {
            // colorize
            html += createLayerParam(name, "red");
            html += createLayerParam(name, "green");
            html += createLayerParam(name, "blue");
            html += createLayerParam(name, "alpha");
        }
        else if (i === 9) {
            // lighter colorize
            // not name conflicts with previous params
            html += createLayerParam(name, "red");
            html += createLayerParam(name, "green");
            html += createLayerParam(name, "blue");
            html += createLayerParam(name, "alpha");
        }
    }

    html += '</div>'

    if (pre) {
        controls.prepend(html)
    }
    else {
        controls.append(html);
    }

    // connect events
    bindStandardEvents(name, layer);

    // param events
    for (var i = 0; i < adjustments.length; i++) {
        if (i === 0) {
            // hue sat
            bindHSLEvents(name, layer);
        }
        else if (i === 1) {
            // levels
            // TODO: Turn some of these into range sliders
            bindLevelsEvents(name, layer);
        }
        else if (i === 2) {
            // curves
            // TODO: CONTROLS
        }
        else if (i === 3) {
            // exposure
            bindExposureEvents(name, layer);
        }
        else if (i === 4) {
            // gradient
            // TODO: CONTROLS
        }
        else if (i === 5) {
            // selective color
            // TODO: CONTROLS
        }
        else if (i === 6) {
            // color balance
            bindColorBalanceEvents(name, layer);
        }
        else if (i === 7) {
            // photo filter
            bindPhotoFilterEvents(name, layer);
        }
        else if (i === 8) {
            // colorize
            bindColorizeEvents(name, layer);
        }
        else if (i === 9) {
            // lighter colorize
            // not name conflicts with previous params
            bindLighterColorizeEvents(name, layer);
        }
    }
}

function bindStandardEvents(name, layer) {
    // visibility
    $('button[layerName="' + name + '"]').on('click', function() {
        // check status of button
        var visible = layer.visible()

        layer.visible(!visible)

        var button = $('button[layerName="' + name + '"]')
        
        if (layer.visible()) {
            button.html('<i class="unhide icon"></i>')
            button.removeClass("black")
            button.addClass("white")
        }
        else {
            button.html('<i class="hide icon"></i>')
            button.removeClass("white")
            button.addClass("black")
        }

        // trigger render after adjusting settings
        renderImage()
    })

    // blend mode
    $('.dropdown[layerName="' + name + '"]').dropdown({
        action: 'activate',
        onChange: function(value, text) {
            layer.blendMode(parseInt(value));
            renderImage();
        },
        'set selected': layer.blendMode()
    });

    $('.dropdown[layerName="' + name + '"]').dropdown('set selected', layer.blendMode());

    bindLayerParamControl(name, layer, "opacity", layer.opacity(), { "uiHandler" : handleParamChange });
}

function bindHSLEvents(name, layer) {
    bindLayerParamControl(name, layer, "hue", layer.getAdjustment(adjType["HSL"])["hue"],
        { "range" : false, "max" : 180, "min" : -180, "step" : 0.1, "uiHandler" : handleHSLParamChange });

    bindLayerParamControl(name, layer, "saturation", layer.getAdjustment(adjType["HSL"])["sat"],
        { "range" : false, "max" : 100, "min" : -100, "step" : 0.1, "uiHandler" : handleHSLParamChange });

    bindLayerParamControl(name, layer, "lightness", layer.getAdjustment(adjType["HSL"])["light"],
        { "range" : false, "max" : 100, "min" : -100, "step" : 0.1, "uiHandler" : handleHSLParamChange });
}

function bindLevelsEvents(name, layer) {
    bindLayerParamControl(name, layer, "inMin", layer.getAdjustment(adjType["LEVELS"])["inMin"],
        { "range" : "min", "max" : 255, "min" : 0, "step" : 1, "uiHandler" : handleLevelsParamChange });

    bindLayerParamControl(name, layer, "inMax", layer.getAdjustment(adjType["LEVELS"])["inMax"],
        { "range" : "max", "max" : 255, "min" : 0, "step" : 1, "uiHandler" : handleLevelsParamChange });

    bindLayerParamControl(name, layer, "gamma", layer.getAdjustment(adjType["LEVELS"])["gamma"],
        { "range" : false, "max" : 10, "min" : 0, "step" : 0.01, "uiHandler" : handleLevelsParamChange });

    bindLayerParamControl(name, layer, "outMin", layer.getAdjustment(adjType["LEVELS"])["outMin"],
        { "range" : "min", "max" : 255, "min" : 0, "step" : 1, "uiHandler" : handleLevelsParamChange });

    bindLayerParamControl(name, layer, "outMax", layer.getAdjustment(adjType["LEVELS"])["outMax"],
        { "range" : "max", "max" : 255, "min" : 0, "step" : 1, "uiHandler" : handleLevelsParamChange });
}

function bindExposureEvents(name, layer) {
    bindLayerParamControl(name, layer, "exposure", layer.getAdjustment(adjType["EXPOSURE"])["exposure"],
        { "range" : false, "max" : 20, "min" : -20, "step" : 0.1, "uiHandler" : handleExposureParamChange });
    bindLayerParamControl(name, layer, "offset", layer.getAdjustment(adjType["EXPOSURE"])["offset"],
        { "range" : false, "max" : 0.5, "min" : -0.5, "step" : 0.01, "uiHandler" : handleExposureParamChange });
    bindLayerParamControl(name, layer, "gamma", layer.getAdjustment(adjType["EXPOSURE"])["gamma"],
        { "range" : false, "max" : 9.99, "min" : 0.01, "step" : 0.01, "uiHandler" : handleExposureParamChange });
}

function bindColorBalanceEvents(name, layer) {
    bindLayerParamControl(name, layer, "shadow R", layer.getAdjustment(adjType["COLOR_BALANCE"])["shadowR"],
        { "range" : false, "max" : 1, "min" : -1, "step" : .01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "shadow G", layer.getAdjustment(adjType["COLOR_BALANCE"])["shadowG"],
        { "range" : false, "max" : 1, "min" : -1, "step" : .01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "shadow B", layer.getAdjustment(adjType["COLOR_BALANCE"])["shadowB"],
        { "range" : false, "max" : 1, "min" : -1, "step" : .01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "mid R", layer.getAdjustment(adjType["COLOR_BALANCE"])["midR"],
        { "range" : false, "max" : 1, "min" : -1, "step" : .01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "mid G", layer.getAdjustment(adjType["COLOR_BALANCE"])["midG"],
        { "range" : false, "max" : 1, "min" : -1, "step" : .01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "mid B", layer.getAdjustment(adjType["COLOR_BALANCE"])["midB"],
        { "range" : false, "max" : 1, "min" : -1, "step" : .01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "highlight R", layer.getAdjustment(adjType["COLOR_BALANCE"])["highR"],
        { "range" : false, "max" : 1, "min" : -1, "step" : .01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "highlight G", layer.getAdjustment(adjType["COLOR_BALANCE"])["highG"],
        { "range" : false, "max" : 1, "min" : -1, "step" : .01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "highlight B", layer.getAdjustment(adjType["COLOR_BALANCE"])["highB"],
        { "range" : false, "max" : 1, "min" : -1, "step" : .01, "uiHandler" : handleColorBalanceParamChange });
    // preserve luma?
}

function bindPhotoFilterEvents(name, layer) {
    bindLayerParamControl(name, layer, "red", layer.getAdjustment(adjType["PHOTO_FILTER"])["r"],
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handlePhotoFilterParamChange });
    bindLayerParamControl(name, layer, "green", layer.getAdjustment(adjType["PHOTO_FILTER"])["g"],
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handlePhotoFilterParamChange });
    bindLayerParamControl(name, layer, "blue", layer.getAdjustment(adjType["PHOTO_FILTER"])["b"],
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handlePhotoFilterParamChange });
    bindLayerParamControl(name, layer, "density", layer.getAdjustment(adjType["PHOTO_FILTER"])["density"],
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handlePhotoFilterParamChange });
    // preserve luma?
}

function bindColorizeEvents(name, layer) {
    bindLayerParamControl(name, layer, "red", layer.getAdjustment(adjType["COLORIZE"])["r"],
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handleColorizeParamChange });
    bindLayerParamControl(name, layer, "green", layer.getAdjustment(adjType["COLORIZE"])["g"],
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handleColorizeParamChange });
    bindLayerParamControl(name, layer, "blue", layer.getAdjustment(adjType["COLORIZE"])["b"],
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handleColorizeParamChange });
    bindLayerParamControl(name, layer, "a", layer.getAdjustment(adjType["COLORIZE"])["a"],
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handleColorizeParamChange });
}

function bindLighterColorizeEvents(name, layer) {
    bindLayerParamControl(name, layer, "red", layer.getAdjustment(adjType["COLORIZE"])["r"],
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handleLighterColorizeParamChange });
    bindLayerParamControl(name, layer, "green", layer.getAdjustment(adjType["COLORIZE"])["g"],
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handleLighterColorizeParamChange });
    bindLayerParamControl(name, layer, "blue", layer.getAdjustment(adjType["COLORIZE"])["b"],
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handleLighterColorizeParamChange });
    bindLayerParamControl(name, layer, "a", layer.getAdjustment(adjType["COLORIZE"])["a"],
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handleLighterColorizeParamChange });
}

function bindLayerParamControl(name, layer, paramName, initVal, settings = {}) {
    var s = '.paramSlider[layerName="' + name + '"][paramName="' + paramName +  '"]';
    var i = '.paramInput[layerName="' + name + '"][paramName="' + paramName +  '"] input';

    // defaults
    if (!("range" in settings)) {
        settings["range"] = "min";
    }
    if (!("max" in settings)) {
        settings["max"] = 100;
    }
    if (!("min" in settings)) {
        settings["min"] = 0;
    }
    if (!("step" in settings)) {
        settings["step"] = 0.1;
    }
    if (!("uiHandler" in settings)) {
        settings["uiHandler"] = handleParamChange;
    }

    $(s).slider({
        orientation: "horizontal",
        range: settings["range"],
        max: settings["max"],
        min: settings["min"],
        step: settings["step"],
        value: initVal,
        stop: function(event, ui) {
            settings["uiHandler"](name, ui)
            renderImage()
        },
        slide: function(event, ui) { settings["uiHandler"](name, ui) },
        change: function(event, ui) { settings["uiHandler"](name, ui) },
    });

    $(i).val(String(initVal.toFixed(1)));

    // input box events
    $(i).blur(function() {
        var data = parseFloat($(this).val());
        $(s).slider("value", data);
        renderImage();
    });
    $(i).keydown(function(event) {
        if (event.which != 13)
            return;

        var data = parseFloat($(this).val());
        $(s).slider("value", data);
        renderImage();
    });  
}

function createLayerParam(layerName, param) {
    var html = '<div class="parameter" layerName="' + layerName + '" paramName="' + param + '">'

    html += '<div class="paramLabel">' + param + '</div>'
    html += '<div class="paramSlider" layerName="' + layerName + '" paramName="' + param + '"></div>'
    html += '<div class="paramInput ui inverted transparent input" layerName="' + layerName + '" paramName="' + param + '"><input type="text"></div>'

    html += '</div>'

    return html
}

function genBlendModeMenu(name) {
    var menu = '<div class="ui scrolling selection dropdown" layerName="' + name + '">'
    menu += '<input type="hidden" name="Blend Mode" value="' + c.getLayer(name).blendMode() + '">'
    menu += '<i class="dropdown icon"></i>'
    menu += '<div class="default text">Blend Mode</div>'
    menu += '<div class="menu">'
    menu += '<div class="item" data-value="0">Normal</div>'
    menu += '<div class="item" data-value="1">Multiply</div>'
    menu += '<div class="item" data-value="2">Screen</div>'
    menu += '<div class="item" data-value="3">Overlay</div>'
    menu += '<div class="item" data-value="4">Hard Light</div>'
    menu += '<div class="item" data-value="5">Soft Light</div>'
    menu += '<div class="item" data-value="6">Linear Dodge (Add)</div>'
    menu += '<div class="item" data-value="7">Color Dodge</div>'
    menu += '<div class="item" data-value="8">Linear Burn</div>'
    menu += '<div class="item" data-value="9">Linear Light</div>'
    menu += '<div class="item" data-value="10">Color</div>'
    menu += '<div class="item" data-value="11">Lighten</div>'
    menu += '<div class="item" data-value="12">Darken</div>'
    menu += '<div class="item" data-value="13">Pin Light</div>'
    menu += '</div>'
    menu += '</div>'

    return menu
}

/*===========================================================================*/
/* File System                                                               */
/*===========================================================================*/

function loadFile() {
    dialog.showOpenDialog({
        filters: [ { name: 'JSON Files', extensions: ['json'] } ],
        title: "Load Layers"
    }, function(filePaths) {
        if (filePaths === undefined) {
            return;
        }
        
        var file = filePaths[0];

        // need to split out the directory path from the file path
        var splitChar = '/';

        if (file.includes('\\')) {
            splitChar = '\\'
        }

        var folder = file.split(splitChar).slice(0, -1).join('/');

        var filename = file.split(splitChar);
        filename = filename[filename.length - 1];
        $('#fileNameText').html(filename);

        openLayers(file, folder)
    });
}

function openLayers(file, folder) {
   fs.readFile(file, function(err, data) {
        if (err) {
            throw err;
        }

        // load the data
        loadLayers(JSON.parse(data), folder);
    }); 
}

function loadLayers(data, path) {
    // create new compositor object
    deleteAllControls();
    c = new comp.Compositor();
    var order = [];
    var movebg = false

    // the import function should be rewritten as follows:
    // - group information should be obtained in a first pass.

    // gather data about adjustments and groups and add layers as needed
    var metadata = {}
    for (var layerName in data) {
        var layer = data[layerName];
        var group = layerName;
        
        // check groups
        if ("group" in layer) {
            // do not create a layer for this, layer is clipping mask
            // clipping masks typically come before their target layer, so create metadata fields
            // if they don't exist
            group = layer["group"];

            if (!(group in metadata)) {
                metadata[group] ={}
                metadata[group]["adjustments"] = []
            }
        }
        else {
            if (!(group in metadata)) {
                metadata[layerName] ={}
                metadata[layerName]["adjustments"] = []
            }

            metadata[layerName]["type"] = layer["kind"]

            if ("filename" in layer) {
                // standard layer, create layer
                c.addLayer(layerName, path + "/" + layer["filename"]);
            }
            else {
                // adjustment layer, create layer
                c.addLayer(layerName);
            }
        }
        
        // adjustment layer information
        // layer kind
        var type = layer["kind"];
        if (type === "LayerKind.HUESATURATION") {
            metadata[group]["adjustments"].push("HSL");
            metadata[group]["HSL"] = layer["adjustment"];
        }
        else if (type === "LayerKind.LEVELS") {
            metadata[group]["adjustments"].push("LEVELS");
            metadata[group]["LEVELS"] = layer["adjustment"];
        }
        else if (type === "LayerKind.CURVES") {
            metadata[group]["adjustments"].push("CURVES");
            metadata[group]["CURVES"] = layer["adjustment"];
        }
        else if (type === "LayerKind.EXPOSURE") {
            metadata[group]["adjustments"].push("EXPOSURE");
            metadata[group]["EXPOSURE"] = layer["adjustment"];
        }
        else if (type === "LayerKind.GRADIENTMAP") {
            metadata[group]["adjustments"].push("GRADIENTMAP");
            metadata[group]["GRADIENTMAP"] = layer["adjustment"];
        }
        else if (type === "LayerKind.SELECTIVECOLOR") {
            metadata[group]["adjustments"].push("SELECTIVECOLOR");
            metadata[group]["SELECTIVECOLOR"] = layer["adjustment"];
        }
        else if (type === "LayerKind.COLORBALANCE") {
            metadata[group]["adjustments"].push("COLORBALANCE");
            metadata[group]["COLORBALANCE"] = layer["adjustment"];
        }
        else if (type === "LayerKind.PHOTOFILTER") {
            metadata[group]["adjustments"].push("PHOTOFILTER");
            metadata[group]["PHOTOFILTER"] = layer["adjustment"];
        }
        else if (type === "LayerKind.SOLIDFILL" && ("group" in layer)) {
            // special case, clipping mask that's using a color blend mode
            if (layer["blendMode"] === "BlendMode.COLORBLEND") {
                // unknown default color, will check if that data can be extracted
                metadata[group]["adjustments"].push("COLORIZE");
            }
            else if (layer["blendMode"] === "BlendMode.LIGHTERCOLOR") {
                metadata[group]["adjustments"].push("LIGHTER_COLORIZE");
            }
        }

        console.log("Pre-processed layer " + layerName + " of type " + layer["kind"]);
    }

    // create controls and set initial values
    for (var layerName in data) {
        // if no metadata exists skip
        if (!(layerName in metadata)) {
            continue;
        }

        var layer = data[layerName];

        // at this point the proper layers have been added, we need to order
        // and add adjustments to them

        var adjustmentList = metadata[layerName]["adjustments"]

        for (var type in adjustmentList) {
            if (type === "HSL") {
                var adjustment = metadata[layerName]["HSL"];
                var hslData = {"hue" : 0, "saturation" : 0, "lightness" : 0}
                
                if ("adjustment" in adjustment) {
                    hslData = adjustment["adjustment"][0];
                }

                // need to extract adjustment params here
                c.getLayer(layerName).addHSLAdjustment(hslData["hue"], hslData["saturation"], hslData["lightness"]);
            }
            if (type === "LEVELS") {
                var adjustment = metadata[layerName]["LEVELS"];

                var levelsData = {};

                if ("adjustment" in adjustment) {
                    levelsData = adjustment["adjustment"][0];
                }

                var inMin = ("input" in levelsData) ? levelsData["input"][0] : 0;
                var inMax = ("input" in levelsData) ? levelsData["input"][1] : 255;
                var gamma = ("gamma" in levelsData) ? levelsData["gamma"] : 1;
                var outMin = ("output" in levelsData) ? levelsData["output"][0] : 0;
                var outMax = ("output" in levelsData) ? levelsData["output"][1] : 255;

                c.getLayer(layerName).addLevelsAdjustment(inMin, inMax, gamma, outMin, outMax);
            }
            else if (type === "CURVES") {
                var adjustment = metadata[layerName]["CURVES"];

                for (var channel in adjustment) {
                    if (channel === "class") {
                        continue;
                    }

                    // normalize values, my system uses floats
                    var curve = adjustment[channel];
                    for (var i = 0; i < curve.length; i++) {
                        curve[i]["x"] = curve[i]["x"] / 255;
                        curve[i]["y"] = curve[i]["y"] / 255;
                    }

                    c.getLayer(layerName).addCurve(channel, curve);
                }
            }
            else if (type === "EXPOSURE") {
                var adjustment = metadata[layerName]["EXPOSURE"];

                c.getLayer(layerName).addExposureAdjustment(adjustment["exposure"], adjustment["offset"], adjustment["gammaCorrection"]);
            }
            else if (type === "GRADIENTMAP") {
                var adjustment = metadata[layerName]["GRADIENTMAP"];

                var colors = adjustment["colors"];
                var pts = []
                var gc = []
                var stops = adjustment["interfaceIconFrameDimmed"];

                for (var i = 0; i < colors.length; i++) {
                    pts.push(colors[i]["location"] / stops);
                    gc.push({"r" : colors[i]["color"]["red"] / 255, "g" : colors[i]["color"]["green"] / 255, "b" : colors[i]["color"]["blue"] / 255 });
                }
                c.getLayer(layerName).addGradient(pts, gc);
            }
            else if (type === "SELEVTIVECOLOR") {
                var adjustment = metadata[layerName]["GRADIENTMAP"];
 
                var relative = (adjustment["method"] === "relative") ? true : false;
                var colors = adjustment["colorCorrection"]
                var sc = {}

                for (var i = 0; i < colors.length; i++) {
                    var name;
                    var adjust = {}

                    for (var id in colors[i]) {
                        if (id === "colors") {
                            name = colors[i][id]
                        }
                        else if (id === "yellowColor") {
                            adjust["yellow"] = colors[i][id]["value"] / 100;
                        }
                        else {
                            adjust[id] = colors[i][id]["value"] / 100;
                        }
                    }

                    sc[name] = adjust;
                }

                c.getLayer(layerName).selectiveColor(relative, sc);
            }
            else if (type === "COLORBALANCE") {
                var adjustment = metadata[layerName]["COLORBALANCE"];

                c.getLayer(layerName).colorBalance(adjustment["preserveLuminosity"], adjustment["shadowLevels"][0] / 100,
                    adjustment["shadowLevels"][1] / 100, adjustment["shadowLevels"][2] / 100,
                    adjustment["midtoneLevels"][0] / 100, adjustment["midtoneLevels"][1] / 100, adjustment["midtoneLevels"][2] / 100,
                    adjustment["highlightLevels"][0] / 100, adjustment["highlightLevels"][1] / 100, adjustment["highlightLevels"][2] / 100);
            }
            else if (type === "PHOTOFILTER") {
                var adjustment = metadata[layerName]["PHOTOFILTER"];
                var color = adjustment["color"];

                var dat = { "preserveLuma" : adjustment["preserveLuminosity"], "density" : adjustment["density"] / 100 };

                if ("luminance" in color) {
                    dat["luminance"] = color["luminance"];
                    dat["a"] = color["a"];
                    dat["b"] = color["b"];
                }
                else if ("hue" in color) {
                    dat["hue"] = color["hue"]["value"];
                    dat["saturation"] = color["saturation"];
                    dat["brightness"] = color["brightness"];
                }

                c.getLayer(layerName).photoFilter(dat);
            }
        }

        // update properties
        var cLayer = c.getLayer(layerName)
        cLayer.blendMode(blendModes[layer["blendMode"]]);
        cLayer.opacity(layer["opacity"]);
        cLayer.visible(layer["visible"]);

        // photoshop exports layers top-down, the compositor adds layers bottom-up
        // track the actual order layers should be in here. order[0] is the bottom.
        if (layerName !== "Background") {
            order.unshift(layerName);
            createLayerControl(layerName, false, layer["kind"]);
        }
        else {
            movebg = true;
        }

        console.log("Added layer " + layerName);
    }

    if (movebg) {
        order.unshift("Background");
        createLayerControl("Background", false, "LayerKind.NORMAL");
    }

    c.setLayerOrder(order);
    renderImage();
}

/*===========================================================================*/
/* UI Callbacks                                                              */
/*===========================================================================*/

function handleParamChange(layerName, ui) {
    // hopefully ui has the name of the param somewhere
    var paramName = $(ui.handle).parent().attr("paramName")

    if (paramName == "opacity") {
        c.getLayer(layerName).opacity(ui.value);

        // find associated value box and dump the value there
        $(ui.handle).parent().next().find("input").val(String(ui.value));
    }
}

function handleHSLParamChange(layerName, ui) {
    var paramName = $(ui.handle).parent().attr("paramName")

    if (paramName == "hue") {
        c.getLayer(layerName).addAdjustment(adjType["HSL"], "hue", ui.value);
    }
    else if (paramName == "saturation") {
        c.getLayer(layerName).addAdjustment(adjType["HSL"], "sat", ui.value);
    }
    else if (paramName == "lightness") {
        c.getLayer(layerName).addAdjustment(adjType["HSL"], "light", ui.value);

    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleLevelsParamChange(layerName, ui) {
    var paramName = $(ui.handle).parent().attr("paramName")

    if (paramName == "inMin") {
        c.getLayer(layerName).addAdjustment(adjType["LEVELS"], "inMin", ui.value);
    }
    else if (paramName == "inMax") {
        c.getLayer(layerName).addAdjustment(adjType["LEVELS"], "inMax", ui.value);
    }
    else if (paramName == "gamma") {
        c.getLayer(layerName).addAdjustment(adjType["LEVELS"], "gamma", ui.value);
    }
    else if (paramName == "outMin") {
        c.getLayer(layerName).addAdjustment(adjType["LEVELS"], "outMin", ui.value);
    }
    else if (paramName == "outMax") {
        c.getLayer(layerName).addAdjustment(adjType["LEVELS"], "outMax", ui.value);
    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleExposureParamChange(layerName, ui) {
    var paramName = $(ui.handle).parent().attr("paramName")

    if (paramName === "exposure") {
        c.getLayer(layerName).addAdjustment(adjType["EXPOSURE"], "exposure", ui.value);
    }
    else if (paramName === "offset") {
        c.getLayer(layerName).addAdjustment(adjType["EXPOSURE"], "offset", ui.value);
    }
    else if (paramName === "gamma") {
        c.getLayer(layerName).addAdjustment(adjType["EXPOSURE"], "gamma", ui.value);
    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleColorBalanceParamChange(layerName, ui) {
    var paramName = $(ui.handle).parent().attr("paramName")

    if (paramName === "shadow R") {
        c.getLayer(layerName).addAdjustment(adjType["COLOR_BALANCE"], "shadowR", ui.value);
    }
    else if (paramName === "shadow G") {
        c.getLayer(layerName).addAdjustment(adjType["COLOR_BALANCE"], "shadowG", ui.value);
    }
    else if (paramName === "shadow B") {
        c.getLayer(layerName).addAdjustment(adjType["COLOR_BALANCE"], "shadowB", ui.value);
    }
    else if (paramName === "mid R") {
        c.getLayer(layerName).addAdjustment(adjType["COLOR_BALANCE"], "midR", ui.value);
    }
    else if (paramName === "mid G") {
        c.getLayer(layerName).addAdjustment(adjType["COLOR_BALANCE"], "midG", ui.value);
    }
    else if (paramName === "mid B") {
        c.getLayer(layerName).addAdjustment(adjType["COLOR_BALANCE"], "midB", ui.value);
    }
    else if (paramName === "highlight R") {
        c.getLayer(layerName).addAdjustment(adjType["COLOR_BALANCE"], "highR", ui.value);
    }
    else if (paramName === "highlight G") {
        c.getLayer(layerName).addAdjustment(adjType["COLOR_BALANCE"], "highG", ui.value);
    }
    else if (paramName === "highlight B") {
        c.getLayer(layerName).addAdjustment(adjType["COLOR_BALANCE"], "highB", ui.value);
    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handlePhotoFilterParamChange(layerName, ui) {
    var paramName = $(ui.handle).parent().attr("paramName")

    if (paramName === "red") {
        c.getLayer(layerName).addAdjustment(adjType["PHOTO_FILTER"], "r", ui.value);
    }
    else if (paramName === "green") {
        c.getLayer(layerName).addAdjustment(adjType["PHOTO_FILTER"], "g", ui.value);
    }
    else if (paramName === "blue") {
        c.getLayer(layerName).addAdjustment(adjType["PHOTO_FILTER"], "b", ui.value);
    }
    else if (paramName === "density") {
        c.getLayer(layerName).addAdjustment(adjType["PHOTO_FILTER"], "density", ui.value);
    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleColorizeParamChange(layerName, ui) {
    var paramName = $(ui.handle).parent().attr("paramName")

    if (paramName === "red") {
        c.getLayer(layerName).addAdjustment(adjType["COLORIZE"], "r", ui.value);
    }
    else if (paramName === "green") {
        c.getLayer(layerName).addAdjustment(adjType["COLORIZE"], "g", ui.value);
    }
    else if (paramName === "blue") {
        c.getLayer(layerName).addAdjustment(adjType["COLORIZE"], "b", ui.value);
    }
    else if (paramName === "alpha") {
        c.getLayer(layerName).addAdjustment(adjType["COLORIZE"], "a", ui.value);
    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleLighterColorizeParamChange(layerName, ui) {
     var paramName = $(ui.handle).parent().attr("paramName")

    if (paramName === "red") {
        c.getLayer(layerName).addAdjustment(adjType["LIGHTER_COLORIZE"], "r", ui.value);
    }
    else if (paramName === "green") {
        c.getLayer(layerName).addAdjustment(adjType["LIGHTER_COLORIZE"], "g", ui.value);
    }
    else if (paramName === "blue") {
        c.getLayer(layerName).addAdjustment(adjType["LIGHTER_COLORIZE"], "b", ui.value);
    }
    else if (paramName === "alpha") {
        c.getLayer(layerName).addAdjustment(adjType["LIGHTER_COLORIZE"], "a", ui.value);
    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));   
}

function deleteAllControls() {
    // may need to unlink callbacks
    $('#layerControls').html('')
}

/*===========================================================================*/
/* Utility                                                                   */
/*===========================================================================*/

function quoteMatches(match, p1, offset, string) {
    return '"' + p1 + '":';
}

function PSObjectToJSON(str) {
    // quote property names
    var s = str.replace(/(\w+):/g, quoteMatches);

    // remove parentheses
    s = s.replace(/([\(\)])/g, '');

    return JSON.parse(s);
}
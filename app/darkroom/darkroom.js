const comp = require('../native/build/Release/compositor')
var {dialog, app} = require('electron').remote
var fs = require('fs')

// initializes a global compositor object to operate on
var c = new comp.Compositor()

// global settings vars
var g_renderSize;

const blendModes = {
    "BlendMode.NORMAL" : 0,
    "BlendMode.MULTIPLY" : 1,
    "BlendMode.SCREEN" : 2,
    "BlendMode.OVERLAY" : 3,
    "BlendMode.SOFTLIGHT" : 5,
    "BlendMode.LINEARDODGE" : 6,
    "BlendMode.LINEARLIGHT" : 9
}

// for testing we load up three solid color test images
//c.addLayer("Elilipse 1", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/sliders/app/native/debug_images/Ellipse 1.png")
//c.addLayer("Rectangle 1", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/sliders/app/native/debug_images/Rectangle 1.png")
//c.addLayer("Ellipse 2", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/sliders/app/native/debug_images/Ellipse 2.png")

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

        g_renderSize = $(this).attr("internal")
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

function createLayerControl(name, pre) {
    var controls = $('#layerControls')
    var layer = c.getLayer(name)

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

    html += '</div>'

    if (pre) {
        controls.prepend(html)
    }
    else {
        controls.append(html);
    }

    // connect events
    // update input
    $('.paramInput[layerName="' + name + '"][paramName="opacity"] input').val(String(layer.opacity()))

    // input box events
    $('.paramInput[layerName="' + name + '"][paramName="opacity"] input').blur(function() {
        var data = parseFloat($(this).val());
        $('.paramSlider[layerName="' + name + '"][paramName="opacity"]').slider("value", data);
        renderImage();
    });
    $('.paramInput[layerName="' + name + '"][paramName="opacity"] input').keydown(function(event) {
        if (event.which != 13)
            return;

        var data = parseFloat($(this).val());
        $('.paramSlider[layerName="' + name + '"][paramName="opacity"]').slider("value", data);
        renderImage();
    });

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

    // param events
    $('.paramSlider[layerName="' + name + '"][paramName="opacity"]').slider({
        orientation: "horizontal",
        range: "min",
        max: 100,
        min: 0,
        step: 0.1,
        value: layer.opacity(),
        stop: function(event, ui) {
            handleParamChange(name, ui)
            renderImage()
        },
        slide: function(event, ui) { handleParamChange(name, ui) },
        change: function(event, ui) { handleParamChange(name, ui) },
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

    for (var layerName in data) {
        // top level
        var layer = data[layerName];

        if (layer["kind"] !== "LayerKind.NORMAL") {
            console.log("No handler for layer kind " + layer["kind"])
            console.log(layer)
            continue;
        }

        // photoshop exports layers top-down, the compositor adds layers bottom-up
        // track the actual order layers should be in here. order[0] is the bottom.
        order.unshift(layerName);

        c.addLayer(layerName, path + "/" + layer["filename"]);

        // update properties
        var cLayer = c.getLayer(layerName)
        cLayer.blendMode(blendModes[layer["blendMode"]]);
        cLayer.opacity(layer["opacity"]);
        cLayer.visible(layer["visible"]);

        createLayerControl(layerName, false);
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

function deleteAllControls() {
    // may need to unlink callbacks
    $('#layerControls').html('')
}
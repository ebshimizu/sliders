/* jshint esversion: 6, maxerr: 1000, node: true */

const comp = require('../native/build/Release/compositor');
var events = require('events');
var {dialog, app} = require('electron').remote;
var fs = require('fs');
const saveVersion = 0.1
const versionString = "0.1"

function inherits(target, source) {
  for (var k in source.prototype)
    target.prototype[k] = source.prototype[k];
}

inherits(comp.Compositor, events);

comp.setLogLevel(1);

// initializes a global compositor object to operate on
var c, docTree, modifiers;
var currentFile = "";
var cp;
var msgId = 0, sampleId = 0;
var sampleIndex = {};
var maxThreads = comp.hardware_concurrency();
var settings = {
    "showSampleId" : true,
    "sampleRows" : 6,
    "sampleThreads" : parseInt(maxThreads / 2),
    "sampleRenderSize" : "thumb",
    "renderSize" : "full",
    "maskMode" : "mask",
    "maskTool": "paint",
    "search": {
        "useVisibleLayersOnly": 1,
        "mode" : 1
    },
    "maxResults" : 100
};

// global settings vars
var g_renderPause = false;
var g_isPainting = false;
var g_ctx;  // drawing context for mask canvas
var g_drawReady = false;
var g_renderID = 0;

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
    "BlendMode.COLORBLEND" : 10,
    "BlendMode.LIGHTEN" : 11,
    "BlendMode.DARKEN" : 12,
    "BlendMode.PINLIGHT" : 13
};

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
    "LIGHTER_COLORIZE" : 9,
    "OVERWRITE_COLOR" : 10
};

const num2Str = {
    1 : "one",
    2 : "two",
    3 : "three",
    4 : "four",
    5 : "five",
    6 : "six",
    7 : "seven",
    8 : "eight",
    9 : "nine",
    10 : "ten"
};

const searchModeStrings = {
    0: "Debug",
    1: "Random",
    2: "Directed Random",
    3: "MCMC",
    4: "Non-Linear Least Squares"
}

/*===========================================================================*/
/* Recurring Events                                                         */
/*===========================================================================*/

// kickoff the render loop.
window.requestAnimationFrame(repaint);

/*===========================================================================*/
/* Initialization                                                            */
/*===========================================================================*/

// Initializes html element listeners on document load
function init() {
    // menu commands
    $("#renderCmd").on("click", function() {
        renderImage("#renderCmd click callback");
    });
    $("#importCmd").click(function() { importFile(); });
    $("#openCmd").click(function() { openFile(); });
    $("#exitCmd").click(function() { app.quit(); });
    $("#saveCmd").click(() => { saveCmd(); });
    $("#saveAsCmd").click(() => { saveAsCmd(); });
    $("#saveImgCmd").click(() => { saveImgCmd(); });
    $("#expandAllCmd").click(() => {
        $('.layerSetContainer').show();
        $(".setName").find('i').removeClass("right");
        $(".setName").find('i').addClass("down");
    });
    $("#collapseAllCmd").click(() => {
        $(".layerSetContainer").hide();
        $(".setName").find('i').removeClass("down");
        $(".setName").find('i').addClass("right");
    });
    $("#clearCanvasCmd").click(() => { clearCanvas(); });
    $('#toggleMaskTools').click(() => { $("#mask-tools").toggle(); });

    // render size options
    $('#renderSize a.item').click(function() {
        // reset icon status
        var links = $('#renderSize a.item');
        for (var i = 0; i < links.length; i++) {
            $(links[i]).html($(links[i]).attr("name"));
        }

        // add icon to selected
        $(this).prepend('<i class="checkmark icon"></i>');

        settings.renderSize = $(this).attr("internal");
        renderImage("#renderSize click callback");
    });

    $('#sampleRenderSize a.item').click(function() {
        // reset icon status
        var links = $('#sampleRenderSize a.item');
        for (var i = 0; i < links.length; i++) {
            $(links[i]).html($(links[i]).attr("name"));
        }

        // add icon to selected
        $(this).prepend('<i class="checkmark icon"></i>');

        settings.sampleRenderSize = $(this).attr("internal");
    });

    // threading options
    // add additional options.
    for (var i = 0; i < maxThreads; i++) {
        var str = '<a class="item" name="' + (i + 1) + '">' + (i + 1) + '</a>';
        $("#sampleThreads").append(str);
    }

    $('#sampleThreads a.item').click(function() {
        // reset icon status
        var links = $('#sampleThreads a.item');
        for (var i = 0; i < links.length; i++) {
            $(links[i]).html($(links[i]).attr("name"));
        }

        // add icon to selected
        $(this).prepend('<i class="checkmark icon"></i>');

        settings.sampleThreads = parseInt($(this).attr("name"));
    });

    // dropdown components
    $(".ui.dropdown").dropdown();

    // popup components
    $("#sampleContainer .settings").popup({
        inline: true,
        hoverable: true,
        delay: {
            show: 50,
            hide: 500
        }
    });

    // settings
    $("#showSampleId").checkbox({
        onChecked: () => { settings.showSampleId = true; $("#sampleContainer").removeClass("noIds"); },
        onUnchecked: () => { settings.showSampleId = false; $("#sampleContainer").addClass("noIds"); }
    });

    $('#sampleRows').dropdown({
        action: 'activate',
        onChange: function(value, text) {
            settings.sampleRows = parseInt(text);
            $('#sampleContainer .sampleWrapper').removeClass("one two three four five six seven eight nine ten");
            $('#sampleContainer .sampleWrapper').addClass(value);
        }
    });

    $('#maxSamples input').change(function () {
        settings.maxResults = parseInt($(this).val());
    });
    $('#maxSamples input').val(settings.maxResults);

    // search settings
    $('#sampleControls .secondary.menu .item').tab();

    $('#useVisibleLayersOnly').checkbox({
        onChecked: () => { settings.search.useVisibleLayersOnly = 1; },
        onUnchecked: () => { settings.search.useVisibleLayersOnly = 0; }
    });

    $('#searchModeSelector').dropdown({
        action: 'activate',
        onChange: function (value, text) {
            settings.search.mode = parseInt(value);
            $('#searchModeSelector .text').html(text);
        }
    })

    $('#useVisibleLayersOnly').checkbox('set checked');
    $('#searchModeSelector .text').html(searchModeStrings[settings.search.mode]);

    // sample event bindings
    $('#samples').on('mouseover', '.sample', function() {
        showPreview(this);
    });

    $('#samples').on('mouseout', '.sample', function() {
        hidePreview();
    });

    $('#samples').on('click', '.pickSampleCmd', function() {
        pickSample(this);
    });

    // debug: if any sliders exist on load, initialize them with no listeners
    $('.paramSlider').slider({
        orientation: "horizontal",
        range: "min",
        max: 100,
        min: 0
    });

    // buttons
    $("#runSearchBtn").click(function() {
        runSearch(this);
    });

    $("#maskCanvas").mousedown(function(e) { canvasMousedown(e, this); });
    $("#maskCanvas").mouseup(function(e) { canvasMouseup(e, this); });
    $("#maskCanvas").mousemove(function(e) { canvasMousemove(e, this); });
    $("#maskCanvas").mouseout(function(e) { canvasMouseup(e, this); });

    // mask tools
    $('#mask-paint').click(function () {
        settings.maskTool = "paint";
        $('#mask-paint').addClass("active");
        $('#mask-rect').removeClass("active");
    });

    $('#mask-rect').click(function () {
        settings.maskTool = "rect";
        $('#mask-paint').removeClass("active");
        $('#mask-rect').addClass("active");
    });

    $('#mode-paint').click(function () {
        settings.maskMode = "mask";
        $('#mode-paint').addClass("active");
        $('#mode-erase').removeClass("active");
    });

    $('#mode-erase').click(function () {
        settings.maskMode = "erase";
        $('#mode-paint').removeClass("active");
        $('#mode-erase').addClass("active");
    });

    $('#mask-tools').hide();

    // autoload
    //openLayers("C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/shapes/shapes.json", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/shapes")

    initCompositor();
    initUI();
    loadSettings();
}

// initialize and rebind events for the compositor. Should always be called
// instead of manually re-creating compositor object.
function initCompositor() {
    c = new comp.Compositor();

    c.on('sample', function(img, ctx, meta) {
        processNewSample(img, ctx, meta);
    });
}

function initUI() {
    cp = new ColorPicker({
        noAlpha: true,
        appendTo: document.getElementById('colorPicker')
    });

    $('.cp-exit').click(() => {
        $('#colorPicker').addClass('hidden');
        $('#colorPicker').removeClass('visible');
    });

    // for now we assume the compositor is already initialized for testing purposes
    var layers = c.getLayerNames();

    for (var layer in layers) {
        createLayerControl(layers[layer]);
    }
}

function loadSettings() {
    if (settings.showSampleId) {
        $("#showSampleId").checkbox('check');
        $("#sampleContainer").removeClass("noIds");
    }
    else {
        $("#showSampleId").checkbox('unchecked');
        $("#sampleContainer").addClass("noIds");
    }

    $("#sampleRows").dropdown('set selected', num2Str[settings.sampleRows]);

    // select max threads / 2
    $("#sampleThreads a.item").removeClass("selected");
    $('#sampleThreads a.item[name="' + settings.sampleThreads + '"]').addClass("selected").prepend('<i class="checkmark icon"></i>');
}

// Inserts a layer into the hierarchy. Later, this hierarchy will be used
// to create the proper groups and html elements for those groups in the interface
// Also returns the html of the element created if needed
function insertLayerElem(name, doc) {
    var layer = c.getLayer(name);

    // controls are created based on what adjustments each layer has
    // create the html
    var html = '<div class="layer" layerName="' + name + '">';
    html += '<h3 class="ui grey inverted header">' + name + '</h3>';

    if (layer.visible()) {
        html += '<button class="ui icon button mini white visibleButton" layerName="' + name + '">';
        html += '<i class="unhide icon"></i>';
    }
    else {
        html += '<button class="ui icon button mini black visibleButton" layerName="' + name + '">';
        html += '<i class="hide icon"></i>';
    }

    html += '</button>';

    // blend mode options
    html += genBlendModeMenu(name);

    // add adjustment button
    html += genAddAdjustmentButton(name);

    // generate parameters
    html += createLayerParam(name, "opacity");

    // separate handlers for each adjustment type
    var adjustments = layer.getAdjustments();

    for (var i = 0; i < adjustments.length; i++) {
        var type = adjustments[i];

        if (type === 0) {
            // hue sat
            html += startParamSection(name, "Hue/Saturation", type);
            html += addSliders(name, "Hue/Saturation", ["hue", "saturation", "lightness"]);
            html += endParamSection(name, type);
        }
        else if (type === 1) {
            // levels
            // TODO: Turn some of these into range sliders
            html += startParamSection(name, "Levels");
            html += addSliders(name, "Levels", ["inMin", "inMax", "gamma", "outMin", "outMax"]);
            html += endParamSection(name, type);
        }
        else if (type === 2) {
            // curves
            html += startParamSection(name, "Curves");
            html += addCurves(name, "Curves");
            html += endParamSection(name, type);
        }
        else if (type === 3) {
            // exposure
            html += startParamSection(name, "Exposure");
            html += addSliders(name, "Exposure", ["exposure", "offset", "gamma"]);
            html += endParamSection(name, type);
        }
        else if (type === 4) {
            // gradient
            html += startParamSection(name, "Gradient Map");
            html += addGradient(name);
            html += endParamSection(name, type);
        }
        else if (type === 5) {
            // selective color
            html += startParamSection(name, "Selective Color");
            html += addTabbedParamSection(name, "Selective Color", "Channel", ["reds", "yellows", "greens", "cyans", "blues", "magentas", "whites", "neutrals", "blacks"], ["cyan", "magenta", "yellow", "black"]);
            html += addToggle(name, "Selective Color", "relative", "Relative");
            html += endParamSection(name, type);
        }
        else if (type === 6) {
            // color balance
            html += startParamSection(name, "Color Balance");
            html += addSliders(name, "Color Balance", ["shadow R", "shadow G", "shadow B", "mid R", "mid G", "mid B", "highlight R", "highlight G", "highlight B"]);
            html += addToggle(name, "Color Balance", "preserveLuma", "Preserve Luma");
            html += endParamSection(name, type);
        }
        else if (type === 7) {
            // photo filter
            html += startParamSection(name, "Photo Filter");
            html += addColorSelector(name, "Photo Filter");
            html += addSliders(name, "Photo Filter", ["density"]);
            html += addToggle(name, "Photo Filter", "preserveLuma", "Preserve Luma");
            html += endParamSection(name, type);
        }
        else if (type === 8) {
            // colorize
            html += startParamSection(name, "Colorize");
            html += addColorSelector(name, "Colorize");
            html += addSliders(name, "Colorize", ["alpha"]);
            html += endParamSection(name, type);
        }
        else if (type === 9) {
            // lighter colorize
            // note name conflicts with previous params
            html += startParamSection(name, "Lighter Colorize");
            html += addColorSelector(name, "Lighter Colorize");
            html += addSliders(name, "Lighter Colorize", ["alpha"]);
            html += endParamSection(name, type);
        }
        else if (type === 10) {
            html += startParamSection(name, "Overwrite Color");
            html += addColorSelector(name, "Overwrite Color");
            html += addSliders(name, "Overwrite Color", ["alpha"]);
            html += endParamSection(name, type);
       }
    }

    html += '</div>';

    // place the html element in the proper location in the heirarchy.
    // basically we actually have an element placeholder already in the heirarchy, and just need to insert the
    // html in the proper position so that when we eventually iterate through it, we can just drop the html
    // in the proper position.
    placeLayer(name, html, doc);

    return html;
}

function placeLayer(name, html, doc) {
    if (typeof(doc) !== "object")
        return;

    for (var key in doc) {
        if (key === name) {
            doc[key].html = html;
            return;
        }
        else {
            // everything in doc is an object, check it
            placeLayer(name, html, doc[key]);
        }
    }
}

function regenLayerControls(name) {
    // remove handlers and elements
    $('.layer[layerName="' + name + '"]').empty();

    // update the doc tree
    var html = insertLayerElem(name, docTree);

    // replace the element
    $('.layer[layerName="' + name + '"]').replaceWith(html);

    // rebind the events
    bindLayerEvents(name);

    console.log("Updated controls for Layer " + name);
}

// Order gets written to
function generateControlHTML(doc, order, setName = "") {
    // if the document contains literally nothing skip, it's an empty group
    if (Object.keys(doc).length === 0)
        return '';

    // place the controls generated earlier. iterate through doc adding proper section dividers.
    var html = '';

    if (setName !== "") {
        html += '<div class="layerSet">';
        html += '<div class="setName ui header"><i class="caret down icon"></i>' + setName + "</div>";

        html += '<button class="ui icon button mini white groupButton" setName="' + setName + '">';
        html += '<i class="unhide icon"></i>';
        html += '</button>';

        html += '<div class="layerSetContainer">';
        html += '<div class="parameter" setName="' + setName + '" paramName="opacity">';

        html += '<div class="paramLabel">Group Opacity</div>';
        html += '<div class="paramSlider groupSlider" setName="' + setName + '" paramName="opacity"></div>';
        html += '<div class="paramInput groupInput ui inverted transparent input" setName="' + setName + '" paramName="opacity"><input type="text"></div>';
        html += '</div>';
    }

    // place layers
    for (var key in doc) {
        // doc[key] may not be an object. If it's not, well, continue
        if (Object.keys(doc[key]).length === 0) {
            continue;
        }

        if (!("html" in doc[key])) {
            // element is a set
            html += generateControlHTML(doc[key], order, key);
        }
        else {
            html += doc[key].html;
            order.unshift(key);
        }
    }

    html += '</div></div>';
    
    return html;
}

function bindGlobalEvents() {
    // group controls
    $('.setName').click(function() {
        // collapse first child
        $(this).next().next('.layerSetContainer').toggle();

        // change icon
        var icon = $(this).find('i');
        if (icon.hasClass("down")) {
            // we are closing
            icon.removeClass("down");
            icon.addClass("right");
        }
        else {
            // we are opening
            icon.removeClass("right");
            icon.addClass("down");
        }
    });

    // group visibility
    $('.layerSet .groupButton').click(function() {
        // get group name
        var group = $(this).attr("setName");

        // toggle shadow doc visibility
        var visible = toggleGroupVisibility(group, docTree);

        if (visible) {
            $(this).html('<i class="unhide icon"></i>');
            $(this).removeClass("black");
            $(this).addClass("white");
        }
        else {
            $(this).html('<i class="hide icon"></i>');
            $(this).removeClass("white");
            $(this).addClass("black");
        }

        renderImage(".layerSet visibility change callback");
    });

    // group opacity
    $('.groupSlider').slider({
        orientation: "horizontal",
        range: "min",
        max: 100,
        min: 0,
        step: 0.1,
        value: 100,
        stop: function(event, ui) {
            groupOpacityChange($(this).attr("setName"), ui.value, docTree);
            renderImage(".layerSet opacity slider change callback");
        },
        slide: function(event, ui) { groupOpacityChange($(this).attr("setName"), ui.value, docTree); },
        change: function(event, ui) { groupOpacityChange($(this).attr("setName"), ui.value, docTree); }
    });

    $('.groupInput input').val("100");

    // input box events
    $('.groupInput input').blur(function() {
        var data = parseFloat($(this).val());
        var group = $(this).attr("setName");
        $('.groupSlider[setName="' + group + '"]').slider("value", data);
        renderImage(".layerSet opacity input change callback");
    });
    $('.groupInput input').keydown(function(event) {
        if (event.which != 13)
            return;

        var data = parseFloat($(this).val());
        var group = $(this).attr("setName");
        $('.groupSlider[setName="' + group + '"]').slider("value", data);
        renderImage(".layerSet opacity input change callback");
    });
}

// Given a layer name, this function will add the proper bindings
// for the interactive controls. This function assumes the controls exist.
function bindLayerEvents(name) {
    var layer = c.getLayer(name);

    // connect events
    bindStandardEvents(name, layer);
    bindSectionEvents(name, layer);

    var adjustments = layer.getAdjustments();

    // param events
    for (var i = 0; i < adjustments.length; i++) {
        var type = adjustments[i];

        if (type === 0) {
            // hue sat
            bindHSLEvents(name, "Hue/Saturation", layer);
        }
        else if (type === 1) {
            // levels
            // TODO: Turn some of these into range sliders
            bindLevelsEvents(name, "Levels", layer);
        }
        else if (type === 2) {
            // curves
            updateCurve(name);
        }
        else if (type === 3) {
            // exposure
            bindExposureEvents(name, "Exposure", layer);
        }
        else if (type === 4) {
            // gradient
            updateGradient(name);
        }
        else if (type === 5) {
            // selective color
            bindSelectiveColorEvents(name, "Selective Color", layer);
        }
        else if (type === 6) {
            // color balance
            bindColorBalanceEvents(name, "Color Balance", layer);
        }
        else if (type === 7) {
            // photo filter
            bindPhotoFilterEvents(name, "Photo Filter", layer);
        }
        else if (type === 8) {
            // colorize
            bindColorizeEvents(name, "Colorize", layer);
        }
        else if (type === 9) {
            // lighter colorize
            // not name conflicts with previous params
            bindLighterColorizeEvents(name, "Lighter Colorize", layer);
        }
        else if (type === 10) {
            bindOverwriteColorEvents(name, "Overwrite Color", layer);
        }
    }
}

function bindStandardEvents(name, layer) {
    // visibility
    $('button[layerName="' + name + '"]').on('click', function() {
        // check status of button
        var visible = layer.visible();

        layer.visible(!visible && modifiers[name].groupVisible);
        modifiers[name].visible = !visible;

        var button = $('button[layerName="' + name + '"]');
        
        if (modifiers[name].visible) {
            button.html('<i class="unhide icon"></i>');
            button.removeClass("black");
            button.addClass("white");
        }
        else {
            button.html('<i class="hide icon"></i>');
            button.removeClass("white");
            button.addClass("black");
        }

        // trigger render after adjusting settings
        renderImage('layer ' + name + ' visibility change');
    });

    var button = $('button[layerName="' + name + '"]');
    // set starting visibility
    if (modifiers[name].visible) {
        button.html('<i class="unhide icon"></i>');
        button.removeClass("black");
        button.addClass("white");
    }
    else {
        button.html('<i class="hide icon"></i>');
        button.removeClass("white");
        button.addClass("black");
    }

    // blend mode
    $('.blendModeMenu[layerName="' + name + '"]').dropdown({
        action: 'activate',
        onChange: function(value, text) {
            layer.blendMode(parseInt(value));
            renderImage('layer ' + name + ' blend mode change');
        },
        'set selected': layer.blendMode()
    });

    $('.blendModeMenu[layerName="' + name + '"]').dropdown('set selected', layer.blendMode());

    // add adjustment menu
    $('.addAdjustment[layerName="' + name + '"]').dropdown({
        action: 'hide',
        onChange: function (value, text) {
            // add the adjustment or something
            addAdjustmentToLayer(name, parseInt(value));
            regenLayerControls(name);
            renderImage('layer ' + name + ' adjustment added');
        }
    });

    bindLayerParamControl(name, layer, "opacity", layer.opacity(), "", { "uiHandler" : handleParamChange });
}

function bindSectionEvents(name, layer) {
    // simple hide function for sections
    $('.layer[layerName="' + name + '"] .divider').click(function() {
        var section = $(this).html();
        $(this).siblings('.paramSection[sectionName="' + section + '"]').transition('fade down');
    });

    // there's a delete button
    $('.layer[layerName="' + name + '"] .deleteAdj').click(function () {
        var adjType = parseInt($(this).attr("adjType"));
        c.getLayer(name).deleteAdjustment(adjType);
        regenLayerControls(name);
        renderImage('layer ' + name + ' adjustment deleted');
    });
}

function bindHSLEvents(name, sectionName, layer) {
    bindLayerParamControl(name, layer, "hue", layer.getAdjustment(adjType.HSL).hue, sectionName,
        { "range" : false, "max" : 180, "min" : -180, "step" : 0.1, "uiHandler" : handleHSLParamChange });
    bindLayerParamControl(name, layer, "saturation", layer.getAdjustment(adjType.HSL).sat, sectionName,
        { "range" : false, "max" : 100, "min" : -100, "step" : 0.1, "uiHandler" : handleHSLParamChange });
    bindLayerParamControl(name, layer, "lightness", layer.getAdjustment(adjType.HSL).light, sectionName,
        { "range" : false, "max" : 100, "min" : -100, "step" : 0.1, "uiHandler" : handleHSLParamChange });
}

function bindLevelsEvents(name, sectionName, layer) {
    bindLayerParamControl(name, layer, "inMin", layer.getAdjustment(adjType.LEVELS).inMin, sectionName,
        { "range" : "min", "max" : 255, "min" : 0, "step" : 1, "uiHandler" : handleLevelsParamChange });
    bindLayerParamControl(name, layer, "inMax", layer.getAdjustment(adjType.LEVELS).inMax, sectionName,
        { "range" : "max", "max" : 255, "min" : 0, "step" : 1, "uiHandler" : handleLevelsParamChange });
    bindLayerParamControl(name, layer, "gamma", layer.getAdjustment(adjType.LEVELS).gamma, sectionName,
        { "range" : false, "max" : 10, "min" : 0, "step" : 0.01, "uiHandler" : handleLevelsParamChange });
    bindLayerParamControl(name, layer, "outMin", layer.getAdjustment(adjType.LEVELS).outMin, sectionName,
        { "range" : "min", "max" : 255, "min" : 0, "step" : 1, "uiHandler" : handleLevelsParamChange });
    bindLayerParamControl(name, layer, "outMax", layer.getAdjustment(adjType.LEVELS).outMax, sectionName,
        { "range" : "max", "max" : 255, "min" : 0, "step" : 1, "uiHandler" : handleLevelsParamChange });
}

function bindExposureEvents(name, sectionName, layer) {
    bindLayerParamControl(name, layer, "exposure", layer.getAdjustment(adjType.EXPOSURE).exposure, sectionName,
        { "range" : false, "max" : 20, "min" : -20, "step" : 0.1, "uiHandler" : handleExposureParamChange });
    bindLayerParamControl(name, layer, "offset", layer.getAdjustment(adjType.EXPOSURE).offset, sectionName,
        { "range" : false, "max" : 0.5, "min" : -0.5, "step" : 0.01, "uiHandler" : handleExposureParamChange });
    bindLayerParamControl(name, layer, "gamma", layer.getAdjustment(adjType.EXPOSURE).gamma, sectionName,
        { "range" : false, "max" : 9.99, "min" : 0.01, "step" : 0.01, "uiHandler" : handleExposureParamChange });
}

function bindColorBalanceEvents(name, sectionName, layer) {
    bindLayerParamControl(name, layer, "shadow R", layer.getAdjustment(adjType.COLOR_BALANCE).shadowR, sectionName,
        { "range" : false, "max" : 1, "min" : -1, "step" : 0.01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "shadow G", layer.getAdjustment(adjType.COLOR_BALANCE).shadowG, sectionName,
        { "range" : false, "max" : 1, "min" : -1, "step" : 0.01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "shadow B", layer.getAdjustment(adjType.COLOR_BALANCE).shadowB, sectionName,
        { "range" : false, "max" : 1, "min" : -1, "step" : 0.01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "mid R", layer.getAdjustment(adjType.COLOR_BALANCE).midR, sectionName,
        { "range" : false, "max" : 1, "min" : -1, "step" : 0.01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "mid G", layer.getAdjustment(adjType.COLOR_BALANCE).midG, sectionName,
        { "range" : false, "max" : 1, "min" : -1, "step" : 0.01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "mid B", layer.getAdjustment(adjType.COLOR_BALANCE).midB, sectionName,
        { "range" : false, "max" : 1, "min" : -1, "step" : 0.01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "highlight R", layer.getAdjustment(adjType.COLOR_BALANCE).highR, sectionName,
        { "range" : false, "max" : 1, "min" : -1, "step" : 0.01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "highlight G", layer.getAdjustment(adjType.COLOR_BALANCE).highG, sectionName,
        { "range" : false, "max" : 1, "min" : -1, "step" : 0.01, "uiHandler" : handleColorBalanceParamChange });
    bindLayerParamControl(name, layer, "highlight B", layer.getAdjustment(adjType.COLOR_BALANCE).highB, sectionName,
        { "range" : false, "max" : 1, "min" : -1, "step" : 0.01, "uiHandler" : handleColorBalanceParamChange });
    
    bindToggleControl(name, sectionName, layer, "preserveLuma", "COLOR_BALANCE");
}

function bindPhotoFilterEvents(name, sectionName, layer) {
    bindColorPickerControl('.paramColor[layerName="' + layer.name() + '"][sectionName="' + sectionName + '"]', "PHOTO_FILTER", layer);

    bindLayerParamControl(name, layer, "density", layer.getAdjustment(adjType.PHOTO_FILTER).density, sectionName,
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handlePhotoFilterParamChange });
    
    bindToggleControl(name, sectionName, layer, "preserveLuma", "PHOTO_FILTER");
}

function bindColorizeEvents(name, sectionName, layer) {
    bindColorPickerControl('.paramColor[layerName="' + layer.name() + '"][sectionName="' + sectionName + '"]', "COLORIZE", layer);

    bindLayerParamControl(name, layer, "alpha", layer.getAdjustment(adjType.COLORIZE).a, sectionName,
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handleColorizeParamChange });
}

function bindLighterColorizeEvents(name, sectionName, layer) {
    bindColorPickerControl('.paramColor[layerName="' + layer.name() + '"][sectionName="' + sectionName + '"]', "LIGHTER_COLORIZE", layer);

    bindLayerParamControl(name, layer, "alpha", layer.getAdjustment(adjType.LIGHTER_COLORIZE).a, sectionName,
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handleLighterColorizeParamChange });
}

function bindOverwriteColorEvents(name, sectionName, layer) {
    bindColorPickerControl('.paramColor[layerName="' + layer.name() + '"][sectionName="' + sectionName + '"]', "OVERWRITE_COLOR", layer);

    bindLayerParamControl(name, layer, "alpha", layer.getAdjustment(adjType.OVERWRITE_COLOR).a, sectionName,
        { "range" : "min", "max" : 1, "min" : 0, "step" : 0.01, "uiHandler" : handleOverwriteColorizeParamChange });
}

function bindSelectiveColorEvents(name, sectionName, layer) {
    // parameter controls
    bindLayerParamControl(name, layer, "cyan", layer.selectiveColorChannel("reds", "cyan"), sectionName,
        { "range" : false, "max" : 100, "min" : -100, "step" : 0.01, "uiHandler" : handleSelectiveColorParamChange });
    bindLayerParamControl(name, layer, "magenta", layer.selectiveColorChannel("reds", "magenta"), sectionName,
        { "range" : false, "max" : 100, "min" : -100, "step" : 0.01, "uiHandler" : handleSelectiveColorParamChange });
    bindLayerParamControl(name, layer, "yellow", layer.selectiveColorChannel("reds", "yellow"), sectionName,
        { "range" : false, "max" : 100, "min" : -100, "step" : 0.01, "uiHandler" : handleSelectiveColorParamChange });
    bindLayerParamControl(name, layer, "black", layer.selectiveColorChannel("reds", "black"), sectionName,
        { "range" : false, "max" : 100, "min" : -100, "step" : 0.01, "uiHandler" : handleSelectiveColorParamChange });

    $('.tabMenu[layerName="' + name + '"][sectionName="' + sectionName + '"]').dropdown('set selected', 0);
    $('.tabMenu[layerName="' + name + '"][sectionName="' + sectionName + '"]').dropdown({
        action: 'activate',
        onChange: function(value, text) {
            // update sliders
            var params = ["cyan", "magenta", "yellow", "black"];
            for (var j = 0; j < params.length; j++) {
                s = 'div[sectionName="' + sectionName +'"] .paramSlider[layerName="' + name + '"][paramName="' + params[j] +  '"]';
                i = 'div[sectionName="' + sectionName +'"] .paramInput[layerName="' + name + '"][paramName="' + params[j] +  '"] input';

                var val = layer.selectiveColorChannel(text, params[j]) * 100;
                $(s).slider({value: val });
                $(i).val(String(val.toFixed(2)));
            }
        }
    });

    bindToggleControl(name, sectionName, layer, "relative", "SELECTIVE_COLOR");
}

function bindLayerParamControl(name, layer, paramName, initVal, sectionName, psettings = {}) {
    var s, i;

    if (sectionName !== "") {
        s = 'div[sectionName="' + sectionName +'"] .paramSlider[layerName="' + name + '"][paramName="' + paramName +  '"]';
        i = 'div[sectionName="' + sectionName +'"] .paramInput[layerName="' + name + '"][paramName="' + paramName +  '"] input';
    }
    else {
        s = '.paramSlider[layerName="' + name + '"][paramName="' + paramName +  '"]';
        i = '.paramInput[layerName="' + name + '"][paramName="' + paramName +  '"] input';
    }

    // defaults
    if (!("range" in psettings)) {
        psettings.range = "min";
    }
    if (!("max" in psettings)) {
        psettings.max = 100;
    }
    if (!("min" in psettings)) {
        psettings.min = 0;
    }
    if (!("step" in psettings)) {
        psettings.step = 0.1;
    }
    if (!("uiHandler" in psettings)) {
        psettings.uiHandler = handleParamChange;
    }

    $(s).slider({
        orientation: "horizontal",
        range: psettings.range,
        max: psettings.max,
        min: psettings.min,
        step: psettings.step,
        value: initVal,
        stop: function(event, ui) {
            psettings.uiHandler(name, ui);
            renderImage('layer ' + name + ' parameter ' + paramName + ' change');
        },
        slide: function(event, ui) { psettings.uiHandler(name, ui); },
        change: function(event, ui) { psettings.uiHandler(name, ui); },
    });

    $(i).val(String(initVal.toFixed(2)));

    // input box events
    $(i).blur(function() {
        var data = parseFloat($(this).val());
        $(s).slider("value", data);
        renderImage('layer ' + name + ' parameter ' + paramName + ' change');
    });
    $(i).keydown(function(event) {
        if (event.which != 13)
            return;

        var data = parseFloat($(this).val());
        $(s).slider("value", data);
        renderImage('layer ' + name + ' parameter ' + paramName + ' change');
    });  
}

function bindColorPickerControl(selector, adjustmentType, layer) {
    $(selector).click(function() {
        if ($('#colorPicker').hasClass('hidden')) {
            // move color picker to spot
            var thisElem = $(selector);
            var offset = thisElem.offset();

            var adj = layer.getAdjustment(adjType[adjustmentType]);
            cp.setColor({"r" : adj.r * 255, "g" : adj.g * 255, "b" : adj.b * 255}, 'rgb');
            cp.startRender();

            if (offset.top + thisElem.height() + $('#colorPicker').height() > $('body').height()) {
                $('#colorPicker').css({"right": "10px", top: offset.top - $('#colorPicker').height()});
            } 
            else {
                $('#colorPicker').css({"right": "10px", top: offset.top + thisElem.height()});
            }

            // assign callbacks to update proper color
            cp.color.options.actionCallback = function(e, action) {
                console.log(action);
                if (action === "changeXYValue" || action === "changeZValue" || action === "changeInputValue") {
                    var color = cp.color.colors.rgb;
                    updateColor(layer, adjType[adjustmentType], color);
                    $(thisElem).css({"background-color": "#" + cp.color.colors.HEX});

                    if (layer.visible()) {
                        // no point rendering an invisible layer
                        renderImage('layer ' + name + ' color change');
                    }
                }
            };

            $('#colorPicker').addClass('visible');
            $('#colorPicker').removeClass('hidden');
        }
        else {
            $('#colorPicker').addClass('hidden');
            $('#colorPicker').removeClass('visible');
        }
    });

    var adj = layer.getAdjustment(adjType[adjustmentType]);
    var colorStr = "rgb(" + parseInt(adj.r * 255) + ","+ parseInt(adj.g * 255) + ","+ parseInt(adj.b * 255) + ")";
    $(selector).css({"background-color" : colorStr });
}

function bindToggleControl(name, sectionName, layer, param, adjustment) {
    var elem = '.checkbox[paramName="' + param + '"][layerName="' + name + '"][sectionName="' + sectionName + '"]';
    $(elem).checkbox({
        onChecked: function() {
            layer.addAdjustment(adjType[adjustment], param, 1);
            if (layer.visible()) { renderImage('layer ' + name + ' parameter ' + param + ' toggle'); }
        },
        onUnchecked: function() {
            layer.addAdjustment(adjType[adjustment], param, 0);
            if (layer.visible()) { renderImage('layer ' + name + ' parameter ' + param + ' toggle'); }
        }
    });

    // set initial state
    var paramVal = layer.getAdjustment(adjType[adjustment], param);
    if (paramVal === 0) {
        $(elem).checkbox('set unchecked');
    }
    else {
        $(elem).checkbox('set checked');
    }
}

function updateGradient(layer) {
    var canvas = $('.gradientDisplay[layerName="' + layer + '"] canvas');
    canvas.attr({width:canvas.width(),height:canvas.height()});
    var ctx = canvas[0].getContext("2d");
    var grad = ctx.createLinearGradient(0, 0, canvas.width(), canvas.height());
    
    // get gradient data
    var g = c.getLayer(layer).getGradient();
    for (var i = 0; i < g.length; i++) {
        var color = 'rgb(' + g[i].color.r * 255 + "," + g[i].color.g * 255 + "," + g[i].color.b * 255 + ")";

        grad.addColorStop(g[i].x, color);
    }

    ctx.fillStyle = grad;
    ctx.fillRect(0, 0, canvas.width(), canvas.height());
}

function updateCurve(layer) {
    var w = 400;
    var h = 400;

    var canvas = $('.curveDisplay[layerName="' + layer + '"] canvas');
    canvas.attr({width:w, height:h});
    var ctx = canvas[0].getContext("2d");
    ctx.clearRect(0, 0, w, h);
    ctx.lineWeight = w / 100;

    // draw lines
    ctx.strokeStyle = "rgba(255,255,255,0.6)";
    ctx.strokeRect(0, 0, w, h);
    ctx.beginPath();
    ctx.moveTo(0, h * 0.25);
    ctx.lineTo(w, h * 0.25);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(0, h * 0.5);
    ctx.lineTo(w, h * 0.5);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(0, h * 0.75);
    ctx.lineTo(w, h * 0.75);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(w * 0.25, 0);
    ctx.lineTo(w * 0.25, h);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(w * 0.5, 0);
    ctx.lineTo(w * 0.5, h);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(w * 0.75, 0);
    ctx.lineTo(w * 0.75, h);
    ctx.stroke();

    var l = c.getLayer(layer);
    var types = ["green", "blue", "red", "RGB"];
    for (var i = 0; i < types.length; i++) {
        // set line style
        strokeStyle = types[i];
        if (strokeStyle == "RGB") {
            strokeStyle = "black";
        }
        ctx.strokeStyle = strokeStyle;
        ctx.fillStyle = strokeStyle;
        ctx.beginPath();
        ctx.moveTo(0, h);

        // sample the curves and stuff from 0 - 100
        for (var x = 0; x <= w; x++) {
            var y = l.evalCurve(types[i], x / w);
            ctx.lineTo(x, h - y * h);
        }

        ctx.stroke();

        // draw key points if there are any
        var curve = l.getCurve(types[i]);
        for (var j = 0; j < curve.length; j++) {
            ctx.beginPath();
            ctx.arc(curve[j].x * w, h - curve[j].y * h, 1.5*(w / 100), 0, 2 * Math.PI);
            ctx.fill();
        }
    }
}

function createLayerParam(layerName, param) {
    var html = '<div class="parameter" layerName="' + layerName + '" paramName="' + param + '">';

    html += '<div class="paramLabel">' + param + '</div>';
    html += '<div class="paramSlider" layerName="' + layerName + '" paramName="' + param + '"></div>';
    html += '<div class="paramInput ui inverted transparent input" layerName="' + layerName + '" paramName="' + param + '"><input type="text"></div>';

    html += '</div>';

    return html;
}

function addSliders(layerName, sectionName, params) {
    var html = '';

    for (var i = 0; i < params.length; i++) {
        html += createLayerParam(layerName, params[i]);
    }

    return html;
}

function addColorSelector(layerName, sectionName) {
    var html = '<div class="parameter" layerName="' + layerName + '" paramName="colorPicker" sectionName="' + sectionName + '"">';

    html += '<div class="paramLabel">Color</div>';
    html += '<div class="paramColor" layerName="' + layerName + '" sectionName="' + sectionName + '" paramName="colorPicker"></div>';

    html += '</div>';

    return html;
}

function addGradient(layerName, sectionName) {
    var html = '<div class="gradientDisplay" layerName="' + layerName + '" sectionName="' + sectionName + '">';
    html += '<canvas></canvas>';
    html += '</div>';

    return html;
}

function addCurves(layerName, sectionName) {
    var html = '<div class="curveDisplay" layerName="' + layerName + '" sectionName="' + sectionName + '">';
    html += '<canvas></canvas>';
    html += '</div>';

    return html;
}

function startParamSection(layerName, sectionName, type) {
    var html = '<div class="ui fitted horizontal inverted divider" layerName="' + layerName + '" adjType="' + type + '">' + sectionName + '</div>';
    html += '<div class="paramSection" layerName="' + layerName + '" sectionName="' + sectionName + '">';

    return html;
}

function endParamSection(name, type) {
    var html = '<div class="ui mini right floated red button deleteAdj" layerName="' + name + '" adjType="' + type + '">Delete Adjustment</div>';
    html += '<div class="clearBoth"></div>'
    html += '</div>';

    return html;
}

function addTabbedParamSection(layerName, sectionName, tabName, tabs, params) {
    var html = '<div class="parameter" layerName="' + layerName + '" sectionName="' + sectionName + '">';
    html += '<div class="tabLabel">' + tabName + '</div>';
    html += '<div class="ui scrolling selection dropdown tabMenu" layerName="' + layerName + '" sectionName="' + sectionName + '">';
    html += '<input type="hidden" name="' + tabName + '" value="0">';
    html += '<i class="dropdown icon"></i>';
    html += '<div class="default text"></div>';
    html += '<div class="menu">';

    for (var i = 0; i < tabs.length; i++) {
        html += '<div class="item" data-value="' + i + '">' + tabs[i] + '</div>';
    }
    html += '</div>';
    html += '</div>';
    html += '</div>';

    for (i = 0; i < params.length; i++) {
        html += createLayerParam(layerName, params[i]);
    }

    if (sectionName == "Selective Color") {

    }

    return html;
}

function addToggle(layerName, section, param, displayName) {
    html = '<div class="parameter" layerName="' + layerName + '" sectionName="' + section + '">';
    html += '<div class="paramLabel">' + displayName + '</div>';
    html += '<div class="ui fitted checkbox" paramName="' + param + '" layerName="' + layerName + '" sectionName="' + section + '">';
    html += '<input type="checkbox">';
    html += '</div></div>';

    return html;
}

function genBlendModeMenu(name) {
    var menu = '<div class="ui scrolling selection dropdown blendModeMenu" layerName="' + name + '">';
    menu += '<input type="hidden" name="Blend Mode" value="' + c.getLayer(name).blendMode() + '">';
    menu += '<i class="dropdown icon"></i>';
    menu += '<div class="default text">Blend Mode</div>';
    menu += '<div class="menu">';
    menu += '<div class="item" data-value="0">Normal</div>';
    menu += '<div class="item" data-value="1">Multiply</div>';
    menu += '<div class="item" data-value="2">Screen</div>';
    menu += '<div class="item" data-value="3">Overlay</div>';
    menu += '<div class="item" data-value="4">Hard Light</div>';
    menu += '<div class="item" data-value="5">Soft Light</div>';
    menu += '<div class="item" data-value="6">Linear Dodge (Add)</div>';
    menu += '<div class="item" data-value="7">Color Dodge</div>';
    menu += '<div class="item" data-value="8">Linear Burn</div>';
    menu += '<div class="item" data-value="9">Linear Light</div>';
    menu += '<div class="item" data-value="10">Color</div>';
    menu += '<div class="item" data-value="11">Lighten</div>';
    menu += '<div class="item" data-value="12">Darken</div>';
    menu += '<div class="item" data-value="13">Pin Light</div>';
    menu += '</div>';
    menu += '</div>';

    return menu;
}

function genAddAdjustmentButton(name) {
    var b = '<div class="ui mini icon top left pointing dropdown button addAdjustment" layerName="' + name + '">';
    b += '<i class="plus icon"></i>';
    b += '<div class="menu">';
    b += '<div class="header">Add Adjustment</div>';
    b += '<div class="item" data-value="0">HSL</div>';
    b += '<div class="item" data-value="1">Levels</div>';
    // curves omitted
    b += '<div class="item" data-value="3">Exposure</div>';
    // gradient map omitted
    b += '<div class="item" data-value="5">Selective Color</div>';
    b += '<div class="item" data-value="6">Color Balance</div>';
    b += '<div class="item" data-value="7">Photo Filter</div>';
    b += '<div class="item" data-value="8">Colorize</div>';
    b += '<div class="item" data-value="9">Lighter Colorize</div>';
    b += '<div class="item" data-value="10">Overwrite Color</div>';
    b += '</div>';
    b += '</div>';

    return b;
}

// adds an adjustment to the specified layer
// the UI should probably regenerate the relevant controls
function addAdjustmentToLayer(name, adjType) {
    if (adjType === 0) {
        c.getLayer(name).addHSLAdjustment(0, 0, 0);
    }
    else if (adjType === 1) {
        c.getLayer(name).addLevelsAdjustment(0, 255);
    }
    // curves omitted
    else if (adjType === 3) {
        c.getLayer(name).addExposureAdjustment(0, 0, 1);
    }
    // gradient map omitted
    else if (adjType === 5) {
        c.getLayer(name).selectiveColor(false, {});
    }
    else if (adjType === 6) {
        c.getLayer(name).colorBalance(false, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    else if (adjType === 7) {
        c.getLayer(name).addPhotoFilter({preserveLuma: true, r: 1, g: 1, b: 1, density: 1 });
    }
    else if (adjType === 8) {
        c.getLayer(name).colorize(1, 1, 1, 1);
    }
    else if (adjType === 9) {
        c.getLayer(name).lighterColorize(1, 1, 1, 1);
    }
    else if (adjType === 10) {
        c.getLayer(name).overwriteColor(1, 1, 1, 1);
    }
}

function createShadowState(tree, order) {
    // just keep the entire loaded tree it's easier
    // it will have html but eh who cares
    docTree = tree;
    modifiers = {};

    // populate modifiers
    for (var i = 0; i < order.length; i++) {
        var layer = c.getLayer(order[i]);
        modifiers[order[i]] = { 'groupOpacity' : 100, 'groupVisible' : true, 'visible' : layer.visible(), 'opacity' : layer.opacity() };
    }
}

/*===========================================================================*/
/* File System                                                               */
/*===========================================================================*/

function importFile() {
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
            splitChar = '\\';
        }

        var folder = file.split(splitChar).slice(0, -1).join('/');

        var filename = file.split(splitChar);
        filename = filename[filename.length - 1];
        $('#fileNameText').html(filename);

        fs.readFile(file, function(err, data) {
            if (err) {
                throw err;
            }

            // load the data
            importLayers(JSON.parse(data), folder);
        }); 
    });
}

function openFile() {
    dialog.showOpenDialog({
        filters: [ { name: 'Darkroom Files', extensions: ['dark'] } ],
        title: "Open File"
    }, function(filePaths) {
        if (filePaths === undefined) {
            return;
        }
        
        var file = filePaths[0];

        // need to split out the directory path from the file path
        var splitChar = '/';

        if (file.includes('\\')) {
            splitChar = '\\';
        }

        var folder = file.split(splitChar).slice(0, -1).join('/');

        var filename = file.split(splitChar);
        filename = filename[filename.length - 1];
        $('#fileNameText').html(filename);
        currentFile = file;

        fs.readFile(file, function(err, data) {
            if (err) {
                throw err;
            }

            // load the data
            loadLayers(JSON.parse(data), folder);
        }); 
    });
}

function importLayers(doc, path) {
    // create new compositor object
    deleteAllControls();
    initCompositor();
    var order = [];
    var movebg = false;
    var data = doc.layers;
    var sets = doc.sets;
    var layer, type, adjustment, i, colors;

    // gather data about adjustments and groups and add layers as needed
    var metadata = {};
    for (var layerName in data) {
        layer = data[layerName];
        var group = layerName;
        
        // check groups
        if ("group" in layer) {
            // do not create a layer for this, layer is clipping mask
            // clipping masks typically come before their target layer, so create metadata fields
            // if they don't exist
            group = layer.group;

            if (!(group in metadata)) {
                metadata[group] ={};
                metadata[group].adjustments = [];
            }
        }
        else {
            if (!(group in metadata)) {
                metadata[layerName] ={};
                metadata[layerName].adjustments = [];
            }

            metadata[layerName].type = layer.kind;

            if ("filename" in layer) {
                // standard layer, create layer
                c.addLayer(layerName, path + "/" + layer.filename);
            }
            else {
                // adjustment layer, create layer
                c.addLayer(layerName);
            }

            c.getLayer(layerName).type(layer.kind);
        }
        
        // adjustment layer information
        // layer kind
        type = layer.kind;
        if (type === "LayerKind.HUESATURATION") {
            metadata[group].adjustments.push("HSL");
            metadata[group].HSL = layer.adjustment;
        }
        else if (type === "LayerKind.LEVELS") {
            metadata[group].adjustments.push("LEVELS");
            metadata[group].LEVELS = layer.adjustment;
        }
        else if (type === "LayerKind.CURVES") {
            metadata[group].adjustments.push("CURVES");
            metadata[group].CURVES = layer.adjustment;
        }
        else if (type === "LayerKind.EXPOSURE") {
            metadata[group].adjustments.push("EXPOSURE");
            metadata[group].EXPOSURE = layer.adjustment;
        }
        else if (type === "LayerKind.GRADIENTMAP") {
            metadata[group].adjustments.push("GRADIENTMAP");
            metadata[group].GRADIENTMAP = layer.adjustment;
        }
        else if (type === "LayerKind.SELECTIVECOLOR") {
            metadata[group].adjustments.push("SELECTIVECOLOR");
            metadata[group].SELECTIVECOLOR = layer.adjustment;
        }
        else if (type === "LayerKind.COLORBALANCE") {
            metadata[group].adjustments.push("COLORBALANCE");
            metadata[group].COLORBALANCE = layer.adjustment;
        }
        else if (type === "LayerKind.PHOTOFILTER") {
            metadata[group].adjustments.push("PHOTOFILTER");
            metadata[group].PHOTOFILTER = layer.adjustment;
        }
        else if (type === "LayerKind.SOLIDFILL") {
            if ("filename" in layer) {
                // is not an adjustment, reset the image to white so the fill works out
                c.resetImages(layerName);
            }

            // solid fill uses special controls based on the blend mode
            if (layer.blendMode === "BlendMode.COLORBLEND") {
                // unknown default color, will check if that data can be extracted
                metadata[group].adjustments.push("COLORIZE");
            }
            else if (layer.blendMode === "BlendMode.LIGHTERCOLOR") {
                metadata[group].adjustments.push("LIGHTER_COLORIZE");
            }
            else {
                metadata[group].adjustments.push("OVERWRITE_COLOR");
            }
        }

        console.log("Pre-processed layer " + layerName + " of type " + layer.kind);
    }

    // create controls and set initial values
    for (layerName in data) {
        // if no metadata exists skip
        if (!(layerName in metadata)) {
            continue;
        }

        layer = data[layerName];

        // at this point the proper layers have been added, we need to order
        // and add adjustments to them

        var adjustmentList = metadata[layerName].adjustments;

        for (i = 0; i < adjustmentList.length; i++) {
            type = adjustmentList[i];

            if (type === "HSL") {
                adjustment = metadata[layerName].HSL;
                var hslData = {"hue" : 0, "saturation" : 0, "lightness" : 0};
                
                if ("adjustment" in adjustment) {
                    hslData = adjustment.adjustment[0];
                }

                // need to extract adjustment params here
                c.getLayer(layerName).addHSLAdjustment(hslData.hue, hslData.saturation, hslData.lightness);
            }
            if (type === "LEVELS") {
                adjustment = metadata[layerName].LEVELS;

                var levelsData = {};

                if ("adjustment" in adjustment) {
                    levelsData = adjustment.adjustment[0];
                }

                var inMin = ("input" in levelsData) ? levelsData.input[0] : 0;
                var inMax = ("input" in levelsData) ? levelsData.input[1] : 255;
                var gamma = ("gamma" in levelsData) ? levelsData.gamma : 1;
                var outMin = ("output" in levelsData) ? levelsData.output[0] : 0;
                var outMax = ("output" in levelsData) ? levelsData.output[1] : 255;

                c.getLayer(layerName).addLevelsAdjustment(inMin, inMax, gamma, outMin, outMax);
            }
            else if (type === "CURVES") {
                adjustment = metadata[layerName].CURVES;

                for (var channel in adjustment) {
                    if (channel === "class") {
                        continue;
                    }

                    // normalize values, my system uses floats
                    var curve = adjustment[channel];
                    for (i = 0; i < curve.length; i++) {
                        curve[i].x = curve[i].x / 255;
                        curve[i].y = curve[i].y / 255;
                    }

                    c.getLayer(layerName).addCurve(channel, curve);
                }
            }
            else if (type === "EXPOSURE") {
                adjustment = metadata[layerName].EXPOSURE;

                c.getLayer(layerName).addExposureAdjustment(adjustment.exposure, adjustment.offset, adjustment.gammaCorrection);
            }
            else if (type === "GRADIENTMAP") {
                adjustment = metadata[layerName].GRADIENTMAP;

                colors = adjustment.gradient.colors;
                var pts = [];
                var gc = [];
                var stops = adjustment.gradient.interfaceIconFrameDimmed;

                for (i = 0; i < colors.length; i++) {
                    pts.push(colors[i].location / stops);
                    gc.push({"r" : colors[i].color.red / 255, "g" : colors[i].color.green / 255, "b" : colors[i].color.blue / 255 });
                }
                c.getLayer(layerName).addGradient(pts, gc);
            }
            else if (type === "SELECTIVECOLOR") {
                adjustment = metadata[layerName].SELECTIVECOLOR;
 
                var relative = (adjustment.method === "relative") ? true : false;
                colors = adjustment.colorCorrection;
                var sc = {};

                for (i = 0; i < colors.length; i++) {
                    var name;
                    var adjust = {};

                    for (var id in colors[i]) {
                        if (id === "colors") {
                            name = colors[i][id];
                        }
                        else if (id === "yellowColor") {
                            adjust.yellow = colors[i][id].value / 100;
                        }
                        else {
                            adjust[id] = colors[i][id].value / 100;
                        }
                    }

                    sc[name] = adjust;
                }

                c.getLayer(layerName).selectiveColor(relative, sc);
            }
            else if (type === "COLORBALANCE") {
                adjustment = metadata[layerName].COLORBALANCE;

                c.getLayer(layerName).colorBalance(adjustment.preserveLuminosity, adjustment.shadowLevels[0] / 100,
                    adjustment.shadowLevels[1] / 100, adjustment.shadowLevels[2] / 100,
                    adjustment.midtoneLevels[0] / 100, adjustment.midtoneLevels[1] / 100, adjustment.midtoneLevels[2] / 100,
                    adjustment.highlightLevels[0] / 100, adjustment.highlightLevels[1] / 100, adjustment.highlightLevels[2] / 100);
            }
            else if (type === "PHOTOFILTER") {
                adjustment = metadata[layerName].PHOTOFILTER;
                var color = adjustment.color;

                var dat = { "preserveLuma" : adjustment.preserveLuminosity, "density" : adjustment.density / 100 };

                if ("luminance" in color) {
                    dat.luminance = color.luminance;
                    dat.a = color.a;
                    dat.b = color.b;
                }
                else if ("hue" in color) {
                    dat.hue = color.hue.value;
                    dat.saturation = color.saturation;
                    dat.brightness = color.brightness;
                }

                c.getLayer(layerName).photoFilter(dat);
            }
            else if (type === "COLORIZE") {
                // currently no way to get info about this
                c.getLayer(layerName).colorize(1, 1, 1, 0);
            }
            else if (type === "LIGHTER_COLORIZE") {
                c.getLayer(layerName).lighterColorize(1, 1, 1, 0);
            }
            else if (type === "OVERWRITE_COLOR") {
                c.getLayer(layerName).overwriteColor(1, 1, 1, 0);
            }
        }

        // update properties
        var cLayer = c.getLayer(layerName);
        cLayer.blendMode(blendModes[layer.blendMode]);
        cLayer.opacity(layer.opacity);
        cLayer.visible(layer.visible);

        insertLayerElem(layerName, sets);
        //createLayerControl(layerName, false, layer["kind"]);

        console.log("Added layer " + layerName);
    }

    // render to page
    var layerData = generateControlHTML(sets, order);
    $('#layerControls').html(layerData);

    // save group visibility info and individual layer transparency modifiers here
    createShadowState(sets, order);

    // bind events
    for (i = 0; i < order.length; i++) {
        bindLayerEvents(order[i]);
    }
    bindGlobalEvents();

    // update internal structure
    c.setLayerOrder(order);
    initSearch();
    renderImage('importLayers()');
    initCanvas();
}

function loadLayers(doc, path) {
    // create new compositor object
    deleteAllControls();
    initCompositor();
    var order = [];
    var movebg = false;
    var data = doc.layers;
    var ver = doc.version;

    if (ver === undefined) {
        ver = 0;
    }

    // when loading an existing darkroom file we have some things already filled in
    // so we just load them from disk
    modifiers = doc.modifiers;

    docTree = doc.docTree;

    // we can skip metadata creation
    // and instead just load layers directly
    // note that docTree has already been populated correctly here
    for (var layerName in data) {
        var layer = data[layerName];

        // add the layer and relevant adjustments
        if (layer.isAdjustment) {
            c.addLayer(layerName);
        }
        else {
            c.addLayer(layerName, path + "/" + layer.filename);
        }

        var cl = c.getLayer(layerName);
        var o = layer.opacity;

        cl.opacity(o);
        cl.visible(layer.visible);
        cl.blendMode(layer.blendMode);

        var adjustmentsList = layer.adjustments;
        for (var type in adjustmentsList) {
            var adj = Number(type);
            for (var key in adjustmentsList[type]) {
                cl.addAdjustment(adj, key, adjustmentsList[type][key]);
            }
        }

        // gradient
        if ("gradient" in layer) {
            // add the gradient
            cl.addGradient(layer.gradient);
        }

        // curves
        if ("curves" in layer) {
            for (var channel in layer.curves) {
                cl.addCurve(channel, layer.curves[channel]);
            }
        }

        // special cases
        if (layer.type === "LayerKind.SOLIDFILL" && layer.isAdjustment === false) {
            c.resetImages(layerName);
        }

        console.log("Added layer " + layerName);
    }

    // but the shadow state may have some residual group settings from last time
    // because the save file encodes those settings in the individual layers, we should
    // reset the shadow state first.
    resetShadowState();

    // render to page
    var layerData = generateControlHTML(docTree, order);
    $('#layerControls').html(layerData);

    // check save version. If we're out of date, we need to update the controls.
    // Do a complete regen and rebind here
    if (ver < saveVersion) {
        console.log("Older save version detected. Regenerating controls...");

        for (var layerName in data) {
            regenLayerControls(layerName);
        }
    }
    else {
        // bind events
        for (var i = 0; i < order.length; i++) {
            bindLayerEvents(order[i]);
        }
    }

    // global controls
    bindGlobalEvents();

    // update internal structure
    c.setLayerOrder(order);
    initSearch();
    renderImage("loadLayers()");
    initCanvas();
}

// saves the document in an easier to load format
function save(file) {
    var out = {};
    out.version = saveVersion;

    // save the document tree
    // this will have the html attached to it but that's ok.
    // can be directly loaded
    out.docTree = docTree;
    out.modifiers = modifiers;

    // layer data
    var layers = {};
    var cl = c.getAllLayers();
    for (var layerName in cl) {
        var l = cl[layerName];
        layers[layerName] = {};

        layers[layerName].opacity = l.opacity();
        layers[layerName].visible = l.visible();
        layers[layerName].blendMode = l.blendMode();
        layers[layerName].isAdjustment = l.isAdjustmentLayer();
        layers[layerName].filename = l.image().filename();
        layers[layerName].type = l.type();
        layers[layerName].adjustments = {};

        // adjustments
        var adjTypes = l.getAdjustments();
        for (var i = 0; i < adjTypes.length; i++) {
            layers[layerName].adjustments[adjTypes[i]] = l.getAdjustment(adjTypes[i]);

            // extra data
            if (adjTypes[i] === adjType.GRADIENTMAP) {
                layers[layerName].gradient = l.getGradient();
            }

            if (adjTypes[i] === adjType.CURVES) {
                layers[layerName].curves = {};

                var channels = l.getAdjustment(adjTypes[i]);
                for (var chan in channels) {
                    layers[layerName].curves[chan] = l.getCurve(chan);
                }
            }
        }
    }

    out.layers = layers;

    fs.writeFile(file, JSON.stringify(out, null, 2), (err) => {
        if (err) {
            showStatusMsg(err.toString(), "ERROR", "Error Saving File");
        }
        else {
            showStatusMsg(file, "OK", "File Saved");
        }
    });
}

function saveAsCmd() {
    dialog.showSaveDialog({
        filters: [ { name: 'Darkroom Files', extensions: ['dark'] } ],
        title: "Save"
    }, function(filePaths) {
        if (filePaths === undefined) {
            return;
        }
        
        var file = filePaths;

        // need to split out the directory path from the file path
        var splitChar = '/';

        if (file.includes('\\')) {
            splitChar = '\\';
        }

        var folder = file.split(splitChar).slice(0, -1).join('/');

        var filename = file.split(splitChar);
        filename = filename[filename.length - 1];
        $('#fileNameText').html(filename);
        currentFile = file;

        save(file);
    });
}

function saveCmd() {
    if (currentFile === "") {
        saveAsCmd();
    }
    else {
        save(currentFile);
    }
}

function saveImgCmd() {
    dialog.showSaveDialog({
        filters: [ { name: 'PNG', extensions: ['png'] } ],
        title: "Save Render"
    }, function(filePaths) {
        if (filePaths === undefined) {
            return;
        }
        
        var file = filePaths;

        c.render().save(file);
    });
}

/*===========================================================================*/
/* UI Callbacks                                                              */
/*===========================================================================*/

function handleParamChange(layerName, ui) {
    // hopefully ui has the name of the param somewhere
    var paramName = $(ui.handle).parent().attr("paramName");

    if (paramName == "opacity") {
        // update the modifiers and compute acutual value
        modifiers[layerName].opacity = ui.value;

        c.getLayer(layerName).opacity(ui.value * (modifiers[layerName].groupOpacity / 100));

        // find associated value box and dump the value there
        $(ui.handle).parent().next().find("input").val(String(ui.value));
    }
}

function handleHSLParamChange(layerName, ui) {
    var paramName = $(ui.handle).parent().attr("paramName");

    if (paramName == "hue") {
        c.getLayer(layerName).addAdjustment(adjType.HSL, "hue", ui.value);
    }
    else if (paramName == "saturation") {
        c.getLayer(layerName).addAdjustment(adjType.HSL, "sat", ui.value);
    }
    else if (paramName == "lightness") {
        c.getLayer(layerName).addAdjustment(adjType.HSL, "light", ui.value);

    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleLevelsParamChange(layerName, ui) {
    var paramName = $(ui.handle).parent().attr("paramName");

    if (paramName == "inMin") {
        c.getLayer(layerName).addAdjustment(adjType.LEVELS, "inMin", ui.value);
    }
    else if (paramName == "inMax") {
        c.getLayer(layerName).addAdjustment(adjType.LEVELS, "inMax", ui.value);
    }
    else if (paramName == "gamma") {
        c.getLayer(layerName).addAdjustment(adjType.LEVELS, "gamma", ui.value);
    }
    else if (paramName == "outMin") {
        c.getLayer(layerName).addAdjustment(adjType.LEVELS, "outMin", ui.value);
    }
    else if (paramName == "outMax") {
        c.getLayer(layerName).addAdjustment(adjType.LEVELS, "outMax", ui.value);
    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleExposureParamChange(layerName, ui) {
    var paramName = $(ui.handle).parent().attr("paramName");

    if (paramName === "exposure") {
        c.getLayer(layerName).addAdjustment(adjType.EXPOSURE, "exposure", ui.value);
    }
    else if (paramName === "offset") {
        c.getLayer(layerName).addAdjustment(adjType.EXPOSURE, "offset", ui.value);
    }
    else if (paramName === "gamma") {
        c.getLayer(layerName).addAdjustment(adjType.EXPOSURE, "gamma", ui.value);
    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleColorBalanceParamChange(layerName, ui) {
    var paramName = $(ui.handle).parent().attr("paramName");

    if (paramName === "shadow R") {
        c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "shadowR", ui.value);
    }
    else if (paramName === "shadow G") {
        c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "shadowG", ui.value);
    }
    else if (paramName === "shadow B") {
        c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "shadowB", ui.value);
    }
    else if (paramName === "mid R") {
        c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "midR", ui.value);
    }
    else if (paramName === "mid G") {
        c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "midG", ui.value);
    }
    else if (paramName === "mid B") {
        c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "midB", ui.value);
    }
    else if (paramName === "highlight R") {
        c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "highR", ui.value);
    }
    else if (paramName === "highlight G") {
        c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "highG", ui.value);
    }
    else if (paramName === "highlight B") {
        c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "highB", ui.value);
    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handlePhotoFilterParamChange(layerName, ui) {
    var paramName = $(ui.handle).parent().attr("paramName");
    
    if (paramName === "density") {
        c.getLayer(layerName).addAdjustment(adjType.PHOTO_FILTER, "density", ui.value);
    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleColorizeParamChange(layerName, ui) {
    var paramName = $(ui.handle).parent().attr("paramName");

    if (paramName === "alpha") {
        c.getLayer(layerName).addAdjustment(adjType.COLORIZE, "a", ui.value);
    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleLighterColorizeParamChange(layerName, ui) {
     var paramName = $(ui.handle).parent().attr("paramName");

    if (paramName === "alpha") {
        c.getLayer(layerName).addAdjustment(adjType.LIGHTER_COLORIZE, "a", ui.value);
    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleOverwriteColorizeParamChange(layerName, ui) {
     var paramName = $(ui.handle).parent().attr("paramName");

    if (paramName === "alpha") {
        c.getLayer(layerName).addAdjustment(adjType.OVERWRITE_COLOR, "a", ui.value);
    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleSelectiveColorParamChange(layerName, ui) {
    var channel = $(ui.handle).parent().parent().parent().find('.text').html();
    var paramName = $(ui.handle).parent().attr("paramName");

    c.getLayer(layerName).selectiveColorChannel(channel, paramName, ui.value / 100);

    $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function updateColor(layer, adjustment, color) {
    layer.addAdjustment(adjustment, "r", color.r);
    layer.addAdjustment(adjustment, "g", color.g);
    layer.addAdjustment(adjustment, "b", color.b);
}

function deleteAllControls() {
    // may need to unlink callbacks
    $('#layerControls').html('');
}

function toggleGroupVisibility(group, doc) {
    // find the group and then set children
    if (typeof(doc) !== "object")
        return;

    if (group in doc) {
        // if the visibility key doesn't exist, the group was previous visible, init to that state
        if (!("visible" in doc[group])) {
            doc[group].visible = true;
        }

        doc[group].visible = !doc[group].visible;
        updateChildVisibility(doc[group], doc[group].visible);

        // groups are unique, once found we're done
        return doc[group].visible;
    }

    var ret;
    for (var key in doc) {
        // still looking for the group
        var val = toggleGroupVisibility(group, doc[key]);

        if (typeof(val) === 'boolean') {
            ret = val;
        }
    }
    return ret;
}

function updateChildVisibility(doc, val) {
    if (typeof(doc) !== "object")
        return null;

    for (var key in doc) {
        if (typeof(doc[key]) !== "object") {
            continue;
        }

        if (key in modifiers) {
            // layer
            // update group vis setting
            modifiers[key].groupVisible = val;

            // update the layer state
            c.getLayer(key).visible(modifiers[key].groupVisible && modifiers[key].visible);
        }
        else {
            // not a layer
            if (!("visible" in doc[key])) {
                doc[key].visible = true;
            }

            updateChildVisibility(doc[key], val && doc[key].visible);
        }
    }
}

function groupOpacityChange(group, val, doc) {
    // find the group and then set children
    if (typeof(doc) !== "object")
        return;

    if (group in doc) {
        doc[group].opacity = val;

        updateChildOpacity(doc[group], doc[group].opacity);

        // groups are unique, once found we're done
        $('.groupInput[setName="' + group + '"] input').val(String(val));
        return;
    }

    for (var key in doc) {
        // still looking for the group
        groupOpacityChange(group, val, doc[key]);
    }
}

function updateChildOpacity(doc, val) {
    for (var key in doc) {
        if (typeof(doc[key]) !== "object") {
            continue;
        }

        if (key in modifiers) {
            // layer
            // update group opacity setting
            modifiers[key].groupOpacity = val;

            // update the layer state
            c.getLayer(key).opacity((modifiers[key].groupOpacity / 100) * modifiers[key].opacity);
        }
        else {
            // not a layer
            if (!("opacity" in doc[key])) {
                doc[key].opacity = 100;
            }

            updateChildOpacity(doc[key], val * (doc[key].opacity / 100));
        }
    }
}

function showStatusMsg(msg, type, title) {
    var msgArea = $("#messageQueue");
    var html = '';

    if (type === "OK") {
        html += '<div class="ui positive small message';
    }
    else if (type === "ERROR") {
        html += '<div class="ui negative small message';
    }
    else {
        html += '<div class="ui info small message';
    }

    html += ' transition hidden" messageId="' + msgId + '">';
    html += '<div class="header">' + title + '</div>';
    html += '<p>' + msg + '<p>';
    html += '</div>';

    msgArea.prepend(html);

    var msgElem = $('div[messageId="' + msgId + '"]');

    msgElem.transition({ animation: 'fade up', onComplete: function() {
        setTimeout(function() {
            msgElem.transition({ animation: 'fade up', onComplete: function () {
                msgElem.remove();
            }});
        }, 2500);
    }});

    msgId += 1;
}

// Renders an image from the compositor module and
// then puts it in an image tag
function renderImage(callerName) {
    if (g_renderPause !== true) {
        g_renderID++;
        var myRenderID = g_renderID;
        addRenderLog(myRenderID, callerName);

        c.asyncRender(settings.renderSize, function(err, img) {
            var dat = 'data:image/png;base64,';
            dat += img.base64();
            $("#render").html('<img src="' + dat + '"" />');
            removeRenderLog(myRenderID);
        });
    }
}

// logs a render call to the render queue
// caller is responsible for tracking completion and removing the entry.
function addRenderLog(renderID, callerName, sampleID = -1) {
    var html = '<div class="item" renderID="' + renderID + '">';
    html += '<div class="content">';
    html += '<div class="header">' + renderID + ': ' + callerName + '</div>';
    html += '<div class="description">';

    if (sampleID !== -1) {
        html += 'Sample ID: ' + sampleID;
    }
    else {
        html += 'Main View Render';
    }

    html += '</div>';
    html += '</div>';
    html += '</div>';

    $('#renderQueue').append(html);
}

function removeRenderLog(renderID) {
    $('.item[renderID="' + renderID + '"]').remove();
}

function showPreview(sample) {
    var img = $(sample).find('img');
    $('#preview').html(img.clone());
    $('#preview').show();
    $('#render').hide();

    if (img.hasClass("newSample")) {
        var sampleId = parseInt(img.attr("sampleId"));
        img.removeClass("newSample").addClass("fullRenderQueued");

        g_renderID++;
        var myRenderID = g_renderID;
        addRenderLog(myRenderID, sampleId + ' full size preview', sampleId);

        // we want to render this sample now at high quality, async
        c.asyncRenderContext(sampleIndex[sampleId].context, settings.renderSize, function(err, img) {
            // replace the relevant image tags
            // because this is single threaded, if at the time of render completion, the user has
            // previewed a sample, this will also update the sample image (we copied it so the selector
            // will also apply to the preview).
            sampleIndex[sampleId].img = img;
            var src = 'data:image/png;base64,' + img.base64();
            $('img[sampleId="' + sampleId + '"]').attr('src', src).removeClass('fullRenderQueued');
            removeRenderLog(myRenderID);
        });
    }
}

function hidePreview() {
    $('#render').show();
    $('#preview').hide();
}

// replaces the current compositor context with the sample's context
function pickSample(elem) {
    // get the sample
    var sampleId = parseInt($(elem).attr('sampleId'));
    c.setContext(sampleIndex[sampleId].context);

    // the model changed, update the ui elements
    updateLayerControls();
}

function updateLayerControls() {
    // a few things have to happen in this function:
    // - Reset the shadow state (the full state is contained in the context)
    // - Get a list of all the layers in the compositor
    // - Iterate through them and grab the relevant controls, and update them
    // - Control updates should not trigger callbacks (otherwise we will be rendering forever)
    //   or this function should disable rendering until the end
    // - Render at the end
    
    resetShadowState();
    $('.groupSlider').slider('value', 100);
    $('.groupButton').html('<i class="unhide icon"></i>').removeClass("black").addClass('white');

    g_renderPause = true;

    var layers = c.getAllLayers();
    for (var layerName in layers) {
        var layer = layers[layerName];

        // general controls
        updateSliderControl(layerName, "opacity", "", layer.opacity());
        
        // visibility
        var button = $('button[layerName="' + layerName + '"]');
        if (modifiers[layerName].visible) {
            button.html('<i class="unhide icon"></i>');
            button.removeClass("black");
            button.addClass("white");
        }
        else {
            button.html('<i class="hide icon"></i>');
            button.removeClass("white");
            button.addClass("black");
        }

        // blend mode
        $('.blendModeMenu[layerName="' + layerName + '"]').dropdown('set selected', layer.blendMode());

        // adjustment controls
        var adjustments = layer.getAdjustments();
        for (var i = 0; i < adjustments.length; i++) {
            var type = adjustments[i];
            var adj = layer.getAdjustment(type);

            if (type === 0) {
                updateSliderControl(layerName, "hue", "Hue/Saturation", adj.hue);
                updateSliderControl(layerName, "saturation", "Hue/Saturation", adj.sat);
                updateSliderControl(layerName, "lightness", "Hue/Saturation", adj.light);
            }
            else if (type === 1) {
                // levels
                updateSliderControl(layerName, "inMin", "Levels", adj.inMin);
                updateSliderControl(layerName, "inMax", "Levels", adj.inMax);
                updateSliderControl(layerName, "gamma", "Levels", adj.gamma);
                updateSliderControl(layerName, "outMin", "Levels", adj.outMin);
                updateSliderControl(layerName, "outMax", "Levels", adj.outMax);
            }
            else if (type === 2) {
                // curves
                updateCurve(layerName);
            }
            else if (type === 3) {
                // exposure
                updateSliderControl(layerName, "exposure", "Exposure", adj.exposure);
                updateSliderControl(layerName, "offset", "Exposure", adj.offset);
                updateSliderControl(layerName, "gamma", "Exposure", adj.gamma);
            }
            else if (type === 4) {
                // gradient
                updateGradient(layerName);
            }
            else if (type === 5) {
                // selective color
                // need to detect which option is currently selected
                var activeChannel = $('.tabMenu[layerName="' + layerName + '"][sectionName="Selective Color"]').dropdown('get text');
                var sc = layer.selectiveColor()[activeChannel];
                updateSliderControl(layerName, "cyan", "Selective Color", sc.cyan * 100);
                updateSliderControl(layerName, "magenta", "Selective Color", sc.magenta * 100);
                updateSliderControl(layerName, "yellow", "Selective Color", sc.yellow * 100);
                updateSliderControl(layerName, "black", "Selective Color", sc.black * 100);
            }
            else if (type === 6) {
                // color balance
                updateSliderControl(layerName, "shadow R", "Color Balance", adj.shadowR);
                updateSliderControl(layerName, "shadow G", "Color Balance", adj.shadowG);
                updateSliderControl(layerName, "shadow B", "Color Balance", adj.shadowB);
                updateSliderControl(layerName, "mid R", "Color Balance", adj.midR);
                updateSliderControl(layerName, "mid G", "Color Balance", adj.midG);
                updateSliderControl(layerName, "mid B", "Color Balance", adj.midB);
                updateSliderControl(layerName, "highlight R", "Color Balance", adj.highR);
                updateSliderControl(layerName, "highlight G", "Color Balance", adj.highG);
                updateSliderControl(layerName, "highlight B", "Color Balance", adj.highB);
            }
            else if (type === 7) {
                // photo filter
                updateColorControl(layerName, "Photo Filter", adj);
            }
            else if (type === 8) {
                // colorize
                updateColorControl(layerName, "Colorize", adj);
            }
            else if (type === 9) {
                // lighter colorize
                updateColorControl(layerName, "Lighter Colorize", adj);
            }
            else if (type === 10) {
                // overwrite color
                updateColorControl(layerName, "Overwrite Color", adj);
            }
        }
    }

    g_renderPause = false;
    renderImage("updateLayerControls()");
}

function updateSliderControl(name, param, section, val) {
    if (section === "") {
        $('.paramSlider[layerName="' + name + '"][paramName="' + param + '"]').slider("value", val);
        $('.paramInput[layerName="' + name + '"][paramName="opacity"] input').val(String(val.toFixed(2)));
    }
    else {
        $('div[sectionName="' + section +'"] .paramSlider[layerName="' + name + '"][paramName="' + param +  '"]').slider("value", val);
        $('div[sectionName="' + section +'"] .paramInput[layerName="' + name + '"][paramName="' + param +  '"] input').val(String(val.toFixed(2)));
    }
}

function updateColorControl(name, section, adj) {
    var selector = '.paramColor[layerName="' + name + '"][sectionName="' + section + '"]';
    var colorStr = "rgb(" + parseInt(adj.r * 255) + ","+ parseInt(adj.g * 255) + ","+ parseInt(adj.b * 255) + ")";
    $(selector).css({"background-color" : colorStr });
}

function resetShadowState() {
    for (var name in modifiers) {
        modifiers[name] = { 'groupOpacity' : 100, 'groupVisible' : true, 'visible' : c.getLayer(name).visible(), 'opacity' : c.getLayer(name).opacity() };
    }
}

/*===========================================================================*/
/* Search                                                                    */
/*===========================================================================*/

function initSearch() {
    sampleIndex = {};
    sampleId = 0;
    $('#sampleContainer .sampleWrapper').empty();
}

function runSearch(elem) {
    if ($(elem).hasClass("disabled")) {
            // is disabled during stopping to prevent multiple stop commands
        return;
    }

    if ($(elem).hasClass("green")) {
        // search is starting
        $(elem).removeClass("green");
        $(elem).addClass("red");
        $(elem).html("Stop Search");
        initSearch();
        c.startSearch(settings.search.mode, settings.search, settings.sampleThreads, settings.sampleRenderSize);
        console.log("Search started");
    }
    else {
        // search is stopping
        $(elem).html("Stopping Search...");
        showStatusMsg("", "", "Stopping Search");
        $(elem).addClass("disabled");

        // this blocks, may want some indication that it is working, loading sign for instance
        // in fact, this function should be async with a callback to indicate completion.
        c.stopSearch(err => {
            $(elem).removeClass("red");
            $(elem).removeClass("disabled");
            $(elem).addClass("green");
            $(elem).html("Start Search");
            showStatusMsg("", "OK", "Search Stopped");
            console.log("Search stopped");
        });
    }
}

function stopSearch() {
    var elem = $("#runSearchBtn");

    // search is stopping
    $(elem).html("Stopping Search...");
    showStatusMsg("", "", "Stopping Search");
    $(elem).addClass("disabled");

    // this blocks, may want some indication that it is working, loading sign for instance
    // in fact, this function should be async with a callback to indicate completion.
    c.stopSearch(err => {
        $(elem).removeClass("red");
        $(elem).removeClass("disabled");
        $(elem).addClass("green");
        $(elem).html("Start Search");
        showStatusMsg("", "OK", "Search Stopped");
        console.log("Search stopped");
    });
}

function processNewSample(img, ctx, meta) {
    // discard sample if too many already in the results section

    // eventually we will need references to each context element in order
    // to render the images at full size
    sampleIndex[sampleId] = { "img" : img, "context" : ctx, "meta" : meta };
    $('#sampleContainer .sampleWrapper').append(createSampleContainer(img, sampleId));

    // bind the dimmer
    $('#sampleContainer .sample[sampleId="' + sampleId + '"] .image').dimmer({
        on: 'hover',
        duration: {
            show: 100,
            hide: 100
        }
    });

    // start the menu
    $('#sampleContainer .sample[sampleId="' + sampleId + '"] .dropdown').dropdown({
        action: 'hide'
    });

    sampleId += 1;

    // if we have too many samples we should stop
    if (sampleId > settings.maxResults) {
        stopSearch();
    }
}

function createSampleContainer(img, id) {
    var html = '<div class="ui card sample" sampleId="' + id + '">';
    html += '<div class="ui dimmable image">';

    // dimmer - other controls
    html += createSampleControls(id);

    // metadata display
    //html += '<div class="ui black left ribbon label"></div>'
    
    // id display
    html += '<div class="ui black small floating circular label sampleId">' + id + '</div>';

    html += '<img class="newSample" sampleId="' + id + '" src="data:image/png;base64,' + img.base64() + '">';
    html += '</div></div>';
    return html;
}

function createSampleControls(id) {
    var html = '<div class="ui dimmer">';
    html += '<div class="content">';
    html += '<div class="center">';
    html += '<div class="ui inverted header">ID: ' + id + '</div>';
    html += '<div class="ui buttons">';
    html += '<div class="ui compact primary inverted button pickSampleCmd" sampleId="' + id + '">Pick</div>';
    html += '<div class="ui compact inverted icon top left pointing dropdown button"><i class="ui options icon"></i>';

    // dropdown menu creation
    html += '<div class="menu">';
    html += '<div class="header">Sample Actions</div>';
    html += '<div class="item pickSampleCmd" sampleId="' + id + '">Pick Sample</div>';
    html += '</div></div>';
    
    html += '</div></div></div></div>';

    return html;
}

/*===========================================================================*/
/* Mask Drawing                                                              */
/*===========================================================================*/

var paths = {};
var pathIndex = 0;
var prevPt;
var canvasUpdated = true;

function initCanvas() {
    // set internal resolution to 1:1 with full res render
    var img = c.render();
    var w = img.width();
    var h = img.height();

    var canvas = $('#maskCanvas');
    canvas.attr({width:w, height:h});
    g_ctx = canvas[0].getContext("2d");
    g_ctx.clearRect(0, 0, w, h);
    g_ctx.strokeStyle = "#FFFFFF";
    g_ctx.lineWidth = 10;
    g_ctx.lineJoin = "round";

    paths = [];
    pathIndex = 0;

    g_drawReady = true;
}

function canvasMousedown(e, elem) {
    g_isPainting = true;
    pathIndex++;
    
    // paths.push({ id: pathIndex, type: settings.maskMode, tool: settings.maskTool });

    // so the canvas is placed using the fit setting and the basic calculation will not work.
    var pt = screenToCanvas(e.pageX - $(elem).offset().left, e.pageY - $(elem).offset().top, elem.width, elem.height, elem.offsetWidth, elem.offsetHeight);
    
    if (settings.maskTool === "paint") {
        paths[pathIndex] = {type : "paint", pts: []};
        paths[pathIndex].pts.push(pt);
    }
    else if (settings.maskTool == "rect") {
        paths[pathIndex] = {type : "rect", pt1: pt, finished: false};
    }

    paths[pathIndex].mode = settings.maskMode;
    
    canvasUpdated = false;
}

function canvasMousemove(e, elem) {
    if (g_isPainting) {
        var pt = screenToCanvas(e.pageX - $(elem).offset().left, e.pageY - $(elem).offset().top, elem.width, elem.height, elem.offsetWidth, elem.offsetHeight);

        if (settings.maskTool === "paint") {
            paths[pathIndex].pts.push(pt);
        }
        else if (settings.maskTool == "rect") {
            paths[pathIndex].pt2 = pt;
        }

        canvasUpdated = false;
    }
}

function canvasMouseup(e, elem) {
    if (settings.maskTool === "rect") {
        paths[pathIndex].finished = true;
    }

    g_isPainting = false;
    canvasUpdated = false;
}

function clearCanvas() {
    paths = [];
    g_ctx.clearRect(0, 0, g_ctx.canvas.width, g_ctx.canvas.height);
    canvasUpdated = false;
}

function repaint() {
    // redraws the mask at regular intervals. If no update is needed, it just returns
    // for efficiency.
    window.requestAnimationFrame(repaint);
    if (g_drawReady === false) {
        canvasUpdated = true;
        return;
    }
    if (canvasUpdated === true)
        return;

    g_ctx.clearRect(0, 0, g_ctx.canvas.width, g_ctx.canvas.height);
    g_ctx.lineWidth = 10;
    g_ctx.lineJoin = "round";

    for (var p in paths) {
        if (paths[p].mode == "mask") {
            g_ctx.strokeStyle = "#FFFFFF";
            g_ctx.fillStyle = "#FFFFFF";
            g_ctx.globalCompositeOperation = "source-over";
        }
        else if (paths[p].mode == "erase") {
            g_ctx.strokeStyle = "#000000";
            g_ctx.fillStyle = "#000000";
            g_ctx.globalCompositeOperation = "destination-out";
        }

        if (paths[p].type === "paint") {
            g_ctx.beginPath();
            g_ctx.moveTo(paths[p].pts[0].x, paths[p].pts[0].y);

            for (var i = 0; i < paths[p].pts.length; i++) {
                g_ctx.lineTo(paths[p].pts[i].x, paths[p].pts[i].y);
            }

            g_ctx.stroke();
        }
        else if (paths[p].type == "rect") {
            // compute rectangle args here, need top left and width height
            if (paths[p].pt2 !== undefined) {
                var x = (paths[p].pt1.x < paths[p].pt2.x) ? paths[p].pt1.x : paths[p].pt2.x;
                var y = (paths[p].pt1.y < paths[p].pt2.y) ? paths[p].pt1.y : paths[p].pt2.y;

                var w = Math.abs(paths[p].pt1.x - paths[p].pt2.x);
                var h = Math.abs(paths[p].pt1.y - paths[p].pt2.y);

                if (paths[p].finished) {
                    g_ctx.fillRect(x, y, w, h);
                }
                else {
                    g_ctx.beginPath();
                    g_ctx.rect(x, y, w, h);
                    g_ctx.stroke();
                }
            }
        }
    }

    canvasUpdated = true;
}

// converts screen coordinates to internal canvas coordinates
function screenToCanvas(sX, sY, w, h, sW, sH) {
    // the canvas is positioned centered and scaled to fit
    var actualW, actualH, scale;
    
    // determine actual dimensions
    scale = Math.min(sH / h, sW / w);
    actualW = w * scale;
    actualH = h * scale;

    // determine offset
    var xOffset = (sW - actualW) / 2;
    var yOffset = (sH - actualH) / 2;

    // return point
    return { x: ((sX - xOffset) / actualW) * w, y: ((sY - yOffset) / actualH) * h };
}
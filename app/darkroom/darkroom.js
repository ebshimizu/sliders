/* jshint esversion: 6, maxerr: 1000, node: true */

const comp = require('../native/build/Release/compositor');
var events = require('events');
var { dialog, app } = require('electron').remote;
var fs = require('fs-extra');
var chokidar = require('chokidar');
var child_process = require('child_process');
var drt = require('./dr');
const uiTools = require('./uiToolkit');
const saveVersion = 1.00; // big jump, renderer changed a lot
const versionString = "0.1";

function inherits(target, source) {
  for (var k in source.prototype)
    target.prototype[k] = source.prototype[k];
}

inherits(comp.Compositor, events);
comp.setLogLevel(1);

// initializes a global compositor object to operate on
var c, docTree, dr;
var g_flatGroups = {};
var g_groupsByLayer = {};
var currentFile = "";
var cp;
var msgId = 0, sampleId = 0;
var g_sampleIndex = {};
var g_sideboard = {};
var g_sideboardID = 100001;        // note: this is a huge hack to remove ID conflicts
var g_sideboardReserveStart = 100000;
var maxThreads = comp.hardware_concurrency();
var g_searchProcess;
var g_layerSelector;
var g_groupPanel;
var g_metaGroupList = {};
var g_moveModeLayer = null;
var g_moveModeInitPoint;
var settings = {
  "showSampleId": true,
  "sampleRows": 6,
  "sampleThreads": parseInt(maxThreads / 2),
  "sampleRenderSize": "thumb",
  "renderSize": "full",
  "maskMode": "mask",
  "maskTool": "paint",
  "search": {
    "useVisibleLayersOnly": 1,
    "modifyLayerBlendModes": 0,
    "mode": 1,
    "maxFailures": 25,
    "toggleRate": 0.5,
    "axisReq": 2,
    "setHueThreshold": 0.4,
    "setSatThreshold": 0.4,
    "setBrightThreshold": 0.4,
    "setStructThreshold": 0.15,
    "brightTolerance": 0.75,
    "hueTolerance": 0.75,
    "clipTolerance": 0.85,
    "ssimA": 0,
    "ssimB": 0,
    "ssimG": 1,
    "useStructOnly" : 1
  },
  "maxResults": 100,
  "unconstrainedDensity": 1000,
  "constrainedDensity": 100,
  "unconstrainedWeight": 1,
  "constrainedWeight": 4,
  "ceresConfig": "Release",
  "metadataDisplay": "score",
  "detailedLog": false,
};

// global settings vars
var g_renderPause = false;
var g_isPainting = false;
var g_ctx;  // drawing context for mask canvas
var g_drawReady = false;
var g_renderID = 0;
var g_historyID = 0;
var g_history = {};
var g_editCounter = 0;
var g_editMenuDefaults;

const blendModes = {
  "BlendMode.NORMAL": 0,
  "BlendMode.MULTIPLY": 1,
  "BlendMode.SCREEN": 2,
  "BlendMode.OVERLAY": 3,
  "BlendMode.HARDLIGHT": 4,
  "BlendMode.SOFTLIGHT": 5,
  "BlendMode.LINEARDODGE": 6,
  "BlendMode.COLORDODGE": 7,
  "BlendMode.LINEARBURN": 8,
  "BlendMode.LINEARLIGHT": 9,
  "BlendMode.COLORBLEND": 10,
  "BlendMode.LIGHTEN": 11,
  "BlendMode.DARKEN": 12,
  "BlendMode.PINLIGHT": 13,
  "BlendMode.COLORBURN": 14,
  "BlendMode.VIVIDLIGHT": 15,
  "BlendMode.PASSTHROUGH": 16
};

const adjType = {
  "HSL": 0,
  "LEVELS": 1,
  "CURVES": 2,
  "EXPOSURE": 3,
  "GRADIENTMAP": 4,
  "SELECTIVE_COLOR": 5,
  "COLOR_BALANCE": 6,
  "PHOTO_FILTER": 7,
  "COLORIZE": 8,
  "LIGHTER_COLORIZE": 9,
  "OVERWRITE_COLOR": 10,
  "INVERT": 11,
  "BRIGHTNESS": 12,
  "OPACITY": 1000
};

const adjToString = {
  0: "HSL",
  1: "LEVELS",
  2: "CURVES",
  3: "EXPOSURE",
  4: "GRADIENTMAP",
  5: "SELECTIVE_COLOR",
  6: "COLOR_BALANCE",
  7: "PHOTO_FILTER",
  8: "COLORIZE",
  9: "LIGHTER_COLORIZE",
  10: "OVERWRITE_COLOR",
  11: "INVERT",
  12: "BRIGHTNESS",
  1000: "OPACITY"
}

const num2Str = {
  1: "one",
  2: "two",
  3: "three",
  4: "four",
  5: "five",
  6: "six",
  7: "seven",
  8: "eight",
  9: "nine",
  10: "ten"
};

const searchModeStrings = {
  0: "Debug",
  1: "Random",
  2: "Directed Random",
  3: "MCMC",
  4: "Generational Ceres",
  5: "Exploratory"
}

const g_constraintModesStrings = {
  0: "Full Color",
  1: "Hue",
  2: "Fixed",
  4: "Hue and Saturation",
  5: "Group",
  6: "Brightness"
}

// we have to define the groups somewhere, we'll stick them at the top of this file because
// clearly there's not enough stuff in here already
const g_actionGroups = {
  "Vanquish": {
    "waves": {
      "contents": [
        "Wave 1",
        "Wave 2",
        "Wave 3",
        "Wave 4",
        "Wave 5",
        "Wave 6",
        "Wave 7",
        "Wave 8",
        "Wave 9",
        "Wave 10",
        "Wave 11",
        "Wave 12",
        "Wave 13",
        "Wave 14",
        "Wave 15",
        "Wave 16",
        "Wave 17",
        "Wave 18",
        "Wave 19"
      ],
      "ops": ["more", "less"]
    }
  },
  "Shapes": {
    "shapes": {
      "contents": [
        "Ellipse 1",
        "Ellipse 2",
        "Rectangle 1"
      ],
      "ops" : ["more", "less"]
    }
  }
}

const g_structureGroups = {
  "Vanquish": g_actionGroups.Vanquish.waves.contents,
  "Shapes": g_actionGroups.Shapes.shapes.contents
}

// Search settings are shared between instances of the app for now
var g_ceresSettings;

/*===========================================================================*/
/* Data Structures                                                           */
/*===========================================================================*/
class Sample {
  constructor(img, ctx, meta) {
    this.img = img;
    this.context = ctx;
    this.meta = meta;
  }
}

/*===========================================================================*/
/* Recurring Events                                                         */
/*===========================================================================*/

// kickoff the render loop.
window.requestAnimationFrame(repaint);

var watcher = chokidar.watch("./ceresOut", {
  "ignoreInitial": true,
  "awaitWriteFinish": {
    stabilityThreshold: 2000,
    pollInterval: 100
  }
});

watcher.on('change', function (filename, stats) {
  console.log("Processing file " + filename);

  processFile(filename);
});

watcher.on('add', function (filename, stats) {
  console.log("Processing file " + filename);

  processFile(filename);
});

// quick named function for loading stuff in case we want to call it from command line
function processFile(filename) {
  // this directory should have just json files in it soooo let's just load them
  // and hopefully it won't crash
  var ceresData = c.ceresToContext(filename);

  // assuming this isn't null
  if (ceresData) {
    // append filename to metadata
    ceresData.metadata["filename"] = filename;

    // append to results for inspection
    processNewSample(c.renderContext(ceresData.context, settings.sampleRenderSize), ceresData.context, ceresData.metadata, true);
  }
}

/*===========================================================================*/
/* Common                                                                    */
/*===========================================================================*/

// draws the given Compositor.Image on the given canvas element
// canvas is expected to be a jquery object
function drawImage(image, canvas) {
  var ctx = canvas[0].getContext("2d");
  var w = canvas[0].width;
  var h = canvas[0].height;

  ctx.clearRect(0, 0, w, h);

  var imDat = ctx.getImageData(0, 0, w, h);
  image.writeToImageData(imDat);

  //var imDat = new ImageData(image.data(), image.width(), image.height());
  createImageBitmap(imDat).then(function (imbit) {
    ctx.drawImage(imbit, 0, 0, w, h);
  });
}

function eraseCanvas(canvas) {
  var ctx = canvas[0].getContext("2d");
  var w = canvas[0].width;
  var h = canvas[0].height;

  ctx.clearRect(0, 0, w, h);
}

/*===========================================================================*/
/* Initialization                                                            */
/*===========================================================================*/

// Initializes html element listeners on document load
function init() {
  // autoload
  //openLayers("C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/shapes/shapes.json", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/shapes")

  initCompositor();
  initUI();
  loadSettings();

  // load ceres settings
  fs.readFile('./codegen/ceres_settings.json', function (err, data) {
    if (err) {
      throw err;
    }

    g_ceresSettings = JSON.parse(data);
    updateCeresSettings();
  });
}

// initialize and rebind events for the compositor. Should always be called
// instead of manually re-creating compositor object.
function initCompositor() {
  c = new comp.Compositor();

  c.on('sample', function (img, ctx, meta) {
    processNewSample(img, ctx, meta);
  });
}

// binds common events and sets up things in general
function initUI() {
  g_editMenuDefaults = $('#editModalDropdown .menu').html();
  $('#mapVizPopup').hide();

  // isotope
  $('#sampleWrapper').isotope({
    itemSelector: '.sample',
    getSortData: {
      id: '[sampleId] parseInt',
      score: function (itemElem) {
        var score = $(itemElem).find('.label[metadata-key="score"]').attr('metadata-value');
        return parseFloat(score);
      },
      filename: '.label[metadata-key="filename"]'
    },
    sortBy: 'id',
    layoutMode: 'fitRows',
    percentPosition: true
  });

  // menu commands
  $("#renderCmd").on("click", function () {
    renderImage("#renderCmd click callback");
  });
  $("#importCmd").click(importFile);
  $("#openCmd").click(function () { openFile(false) });
  $("#transferCmd").click(function () { openFile(true) });
  $("#exitCmd").click(function () { app.quit(); });
  $("#saveCmd").click(saveCmd);
  $("#saveAsCmd").click(saveAsCmd);
  $("#saveImgCmd").click(saveImgCmd);
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
  $("#clearCanvasCmd").click(clearCanvas);
  $('#exportAllSamplesCmd').click(exportAllSamples);
  $('#showAppInfoCmd').click(() => { $('#versionModal').modal('show'); });
  $('#generateCeresCodeCmd').click(generateCeresCode);
  $('#clearSamples').click(() => { $('#sampleWrapper').empty().isotope(); sampleId = 0; });
  $('#extractConstraints').click(extractConstraints);
  $('#ceresEval').click(ceresEval);
  $('#computeError').click(computeError);
  $('#deleteAllConstraints').click(deleteAllDebugConstraints);
  $('#exportSampleData').click(exportSampleData);
  $('#importSampleData').click(importSampleData);
  $('#mapViz canvas').mousemove(mapMouseMove);
  $('#mapViz canvas').mouseup(mapMouseUp);
  $('#saveUIConfig').click(saveCustomUI);
  $('#openUIConfig').click(openCustomUI);

  // render size options
  $('#renderSize a.item').click(function () {
    // reset icon status
    var links = $('#renderSize a.item');
    for (var i = 0; i < links.length; i++) {
      $(links[i]).html($(links[i]).attr("name"));
    }

    // add icon to selected
    $(this).prepend('<i class="checkmark icon"></i>');

    settings.renderSize = $(this).attr("internal");

    // update the canvas
    var img = c.render(settings.renderSize);
    var w = img.width();
    var h = img.height()

    var renderCanvas = $('#renderCanvas');
    renderCanvas.attr({ width: w, height: h });

    var vizCanvas = $('#diffVizCanvas');
    vizCanvas.attr({ width: w, height: h });

    renderImage("#renderSize click callback");
  });

  // render size options
  $('#ceresBuildMode a.item').click(function () {
    // reset icon status
    var links = $('#ceresBuildMode a.item');
    for (var i = 0; i < links.length; i++) {
      $(links[i]).html($(links[i]).attr("name"));
    }

    // add icon to selected
    $(this).prepend('<i class="checkmark icon"></i>');

    settings.ceresConfig = $(this).attr("name");
  });

  $('#sampleRenderSize a.item').click(function () {
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

  $('#sampleThreads a.item').click(function () {
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

  // layers
  $('#layerControlsMenu .item').tab();

  // settings
  $("#showSampleId").checkbox({
    onChecked: () => { settings.showSampleId = true; $("#sampleContainer").removeClass("noIds"); $("#sideboardWrapper").removeClass("noIds"); },
    onUnchecked: () => { settings.showSampleId = false; $("#sampleContainer").addClass("noIds"); $("#sideboardWrapper").addClass("noIds"); }
  });

  $('#sampleRows').dropdown({
    action: 'activate',
    onChange: function (value, text) {
      settings.sampleRows = parseInt(text);
      $('#sampleWrapper').removeClass("one two three four five six seven eight nine ten");
      $('#sampleWrapper').addClass(value);
      $('#sampleWrapper').isotope(); // relayout
      $('#sideboardWrapper').removeClass("one two three four five six seven eight nine ten");
      $('#sideboardWrapper').addClass(value);
    }
  });

  $('#sortMode').dropdown({
    action: 'activate',
    onChange: function (value, text) {
      settings.sampleSortMode = value;

      var sortData = {
        id: '[sampleId] parseInt',
        filename: '.label[metadata-key="filename"]'
      };

      if (value !== 'id' && value !== 'filename') {
        sortData[value] = function (itemElem) {
          var val = $(itemElem).find('.label[metadata-key="' + value + '"]').attr('metadata-value');
          return parseFloat(val);
        };
      }

      $('#sampleWrapper').isotope({
        getSortData: sortData
      });
      $('#sampleWrapper').isotope('updateSortData');      
      $('#sampleWrapper').isotope({ sortBy: value });
    }
  });

  $('#metadataDisplay').dropdown({
    action: 'activate',
    onChange: function (value, text) {
      settings.metadataDisplay = value;

      $('#sampleWrapper .ribbon.label').removeClass('visible');
      $('#sampleWrapper .ribbon.label[metadata-key="' + value + '"]').addClass('visible');
    }
  });

  $('#maxSamples input').change(function () {
    settings.maxResults = parseInt($(this).val());
    g_sideboardID += settings.maxResults;
  });

  $('#udensity input').change(function () {
    settings.unconstrainedDensity = parseInt($(this).val());
  });

  $('#cdensity input').change(function () {
    settings.constrainedDensity = parseInt($(this).val());
  });

  $('#uweight input').change(function () {
    settings.unconstrainedWeight = parseInt($(this).val());
  });

  $('#cweight input').change(function () {
    settings.constrainedWeight = parseInt($(this).val());
  });

  $('#expMaxFail input').change(function () {
    settings.search.maxFailures = parseFloat($('#expMaxFail input').val());
  });

  $('.expParam input').change(function () {
    var p = $(this).parent('.expParam').attr("param");
    settings.search[p] = parseFloat($(this).val());
  });;

  // search settings
  $('#sampleControls .top.menu .item').tab();
  $('#samplesTabs .item').tab();

  $('#useVisibleLayersOnly').checkbox({
    onChecked: function () { settings.search.useVisibleLayersOnly = 1; },
    onUnchecked: function () { settings.search.useVisibleLayersOnly = 0; }
  });

  $('#modifyLayerBlendModes').checkbox({
    onChecked: () => { settings.search.modifyLayerBlendModes = 1; },
    onUnchecked: () => { settings.search.modifyLayerBlendModes = 0; }
  });

  $('#detailedConstraintLog').checkbox({
    onChecked: () => { settings.detailedLog = true; },
    onUnchecked: () => { settings.detailedLog = false; }
  });

  $('#useStructOnly').checkbox({
    onChecked: () => { settings.search.useStructOnly = 1; },
    onUnchecked: () => { settings.search.useStructOnly = -1; }
  });

  $('#searchModeSelector').dropdown({
    action: 'activate',
    onChange: function (value, text) {
      settings.search.mode = parseInt(value);
      $('#searchModeSelector .text').html(text);
    }
  });

  $('#genLogLevel').dropdown({
    action: 'activate',
    onChange: function (value, text) {
      g_ceresSettings.evo.logLevel = parseInt(value) - 1;
      $('#genLogLevel .text').html(text);
    }
  });

  $('#returnOrderSelector').dropdown({
    action: 'activate',
    onChange: function (value, text) {
      g_ceresSettings.evo.returnOrder = parseInt(value) - 1;
      $('#returnOrderSelector .text').html(text);
    }
  });

  $('#expStructSelector').dropdown({
    action: 'activate',
    onChange: function (value, text) {
      settings.search.structMode = parseInt(value) - 1;
      $('#expStructSelector .text').html(text);
    }
  });

  $('.ceresBool').checkbox({
    onChecked: function () { g_ceresSettings["evo"][$(this).attr("name")] = true; },
    onUnchecked: function () { g_ceresSettings["evo"][$(this).attr("name")] = false; }
  });

  $('.ceresNum input').change(function () {
    g_ceresSettings["evo"][$(this).parent('.ceresNum').attr('param')] = parseFloat($(this).val());
  });

  // sample event bindings
  $('#sampleWrapper').on('mouseover', '.sample', function () {
    showPreview(this);
  });

  $('#sampleWrapper').on('mouseout', '.sample', function () {
    hidePreview();
  });

  $('#sampleWrapper').on('click', '.pickSampleCmd', function () {
    pickSample(this);
  });

  $('#sampleWrapper').on('click', '.exportSampleCmd', function () {
    exportSample(parseInt($(this).attr("sampleId")));
  });

  $('#sampleWrapper').on('click', '.stashSampleCmd', function () {
    stashSample(parseInt($(this).attr("sampleId")));
  });

  // Sideboard event bindings
  // some people might note these are basically the same with a few exceptions.
  $('#sideboardWrapper').on('mouseover', '.sample', function () {
    showPreview(this);
  });

  $('#sideboardWrapper').on('mouseout', '.sample', function () {
    hidePreview();
  });

  $('#sideboardWrapper').on('click', '.pickSampleCmd', function () {
    pickSample(this);
  });

  $('#sideboardWrapper').on('click', '.exportSampleCmd', function () {
    exportSample(parseInt($(this).attr("sampleId")));
  });

  $('#sideboardWrapper').on('click', '.deleteStashSampleCmd', function () {
    deleteStashSample(parseInt($(this).attr("sampleId")));
  });

  // debug: if any sliders exist on load, initialize them with no listeners
  $('.paramSlider').slider({
    orientation: "horizontal",
    range: "min",
    max: 100,
    min: 0
  });

  // buttons
  $("#runSearchBtn").click(function () {
    runSearch(this);
  });

  //$("#maskCanvas").mousemove(function (e) { canvasMousemove(e, this); });
  //$("#maskCanvas").mouseout(function (e) { canvasMouseup(e, this); });
  $('#exitMoveButton').hide();
  $('#exitMoveButton').click(function() {
    stopMoveMode();
  });

  // mask tools
  $('#maskOpacitySlider .slider').slider({
    orientation: "horizontal",
    range: "min",
    max: 1,
    min: 0,
    step: 0.001,
    value: 1,
    change: function (event, ui) {
      $('#imageContainer #mask canvas').css("opacity", ui.value);
    }
  });

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

  $('#createNewEdit').click(createNewEdit);

  $('#editMaskComplete').click(editMaskComplete);
  $('#editMaskComplete').hide();
  $('#modeStatus').hide();

  $('#editModalDropdown').dropdown({
    action: 'activate',
    onChange: editModalDropdownChange
  });

  $('#newEditModal').modal({
    closable: false,
    onDeny: closeEditModal,
    onApprove: setupEdit
  });

  $('#actionGroupSelector').dropdown({
    action: 'activate',
    onChange: function (value, text) {
      updateActiveGroups(text);
    }
  });

  $('#stickyMessage').hide();
  $('#stickyMessage .remove.icon').click(function () {
    $('#stickyMessage').hide();
  })

  $('#setConstraintLayerColor').click(function () {
    if ($('#colorPicker').hasClass('hidden')) {
      // move color picker to spot
      var offset = $(this).offset();

      cp.setColor(g_currentColor, 'hex');
      cp.startRender();

      $("#colorPicker").css({ 'left': '', 'right': '', 'top': '', 'bottom': '' });
      if (offset.top + $(this).height() + $('#colorPicker').height() > $('body').height()) {
        $('#colorPicker').css({ "left": offset.left, top: offset.top - $('#colorPicker').height() });
      }
      else {
        $('#colorPicker').css({ "left": offset.left, top: offset.top + $(this).height() + 18 });
      }

      // assign callbacks to update proper color
      cp.color.options.actionCallback = function (e, action) {
        console.log(action);
        if (action === "changeXYValue" || action === "changeZValue" || action === "changeInputValue") {
          g_currentColor = cp.color.colors.HEX;
          g_canvasUpdated = false;

          $('#setConstraintLayerColor').css({ "background-color": "#" + cp.color.colors.HEX });
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
  $('#setConstraintLayerColor').css({ "background-color": "#" + g_currentColor });

  $('#newConstraintLayer').click(function () {
    $('#newConstraintLayerModal').modal({
      onApprove: function () {
        newConstraintLayer($('#newConstraintLayerModal input').val(), 0);
        $('#newConstraintLayerModal input').val("");
      }
    });
    $('#newConstraintLayerModal').modal('show');
  });

  $('#deleteConstraintLayer').click(function () {
    if (g_activeConstraintLayer === null)
      return;

    $('#deleteConstraintLayerModal .layerName').html(g_activeConstraintLayer);
    $('#deleteConstraintLayerModal').modal({
      onApprove: function () {
        deleteConstraintLayer(g_activeConstraintLayer);
      }
    });
    $('#deleteConstraintLayerModal').modal('show');
  });

  $('#mask-tools').hide();

  // ceres debug
  $('#ceresAddPoint').click(() => { selectDebugConstraint(); });
  $('#exportToCeres').click(() => { sendToCeres(); });
  $('#runCeres').click(() => { runCeres(); });
  $('#importFromCeres').click(() => { importFromCeres(); });
  $('#ceresRoundtrip').click(() => { ceresAll(); });

  cp = new ColorPicker({
    noAlpha: true,
    appendTo: document.getElementById('colorPicker')
  });

  $('.cp-exit').click(() => {
    $('#colorPicker').addClass('hidden');
    $('#colorPicker').removeClass('visible');
  });

  g_groupPanel = new uiTools.GroupPanel('groups', '#groupControlPanel', '#secondaryGroupControlPanel');

  // place version numbers
  $('#appVersionLabel').html(versionString);
  $('#saveVersionLabel').html(saveVersion);
}

function initLayerSelector() {
  g_layerSelector = new uiTools.LayerSelector({
    "selectionCanvas": "#maskCanvas",
    "sidebar": "#layerEditControls",
    "selectionMode": "localPoint",
    "rankMode": "goal",
    "rankThreshold": 0,
    "optUI": "#layerSelectOptionsTab"
  });

  $("#maskCanvas").mousedown(function (e) { canvasMousedown(e, this); });
  $("#maskCanvas").mouseup(function (e) { canvasMouseup(e, this); });
}

// Loads UI settings from the settings object. Assumes all settings in the object
// are the ones that should be used (doesn't load from external sources in this function)
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
  $('#sampleThreads i').remove();
  $('#sampleThreads a.item[name="' + settings.sampleThreads + '"]').addClass("selected").prepend('<i class="checkmark icon"></i>');

  $('#ceresBuildMode a.item').removeClass("selected");
  $('#ceresBuildMode i').remove();
  $('#ceresBuildMode a.item[name="' + settings.ceresConfig + '"]').addClass("selected").prepend('<i class="checkmark icon"></i>');

  if (settings.search.useVisibleLayersOnly)
    $('#useVisibleLayersOnly').checkbox('set checked');
  else
    $('#useVisibleLayersOnly').checkbox('set unchecked');

  if (settings.search.modifyLayerBlendModes)
    $('#modifyLayerBlendModes').checkbox('set checked');
  else
    $('#modifyLayerBlendModes').checkbox('set unchecked');

  // added 0.36
  if (!('useStructOnly' in settings.search))
    settings.search.useStructOnly = 1;

  if (settings.search.useStructOnly > 0)
    $('#useStructOnly').checkbox('set checked');
  else 
    $('#useStructOnly').checkbox('set unchecked');

  $('#searchModeSelector .text').html(searchModeStrings[settings.search.mode]);
  $('#maxSamples input').val(settings.maxResults);

  $('#udensity input').val(settings.unconstrainedDensity);
  $('#cdensity input').val(settings.constrainedDensity);
  $('#uweight input').val(settings.unconstrainedWeight);
  $('#cweight input').val(settings.constrainedWeight);

  if ('metadataDisplay' in settings) {
    $('#sampleWrapper').removeClass("score filename");
    $('#sampleWrapper').addClass(settings.metadataDisplay);
    $('#sideboardWrapper').removeClass("score filename");
    $('#sideboardWrapper').addClass(settings.metadataDisplay);
    $('#metadataDisplay').dropdown('set selected', settings.metadataDisplay);
  }
  else {
    $('#metadataDisplay').dropdown('set selected', 'score');
  }

  // added: 0.26
  if ('sampleSortMode' in settings) {
    $('#sortMode').dropdown('set selected', settings.sampleSortMode);
  }
  else {
    $('#sortMode').dropdown('set selected', 'id');
  }

  // added: 0.28
  if ('detailedLog' in settings) {
    $('#detailedConstraintLog').checkbox(settings.detailedLog ? 'check' : 'uncheck');
  }
  else {
    $('#detailedConstraintLog').checkbox('uncheck');
  }

  // added: 0.29
  if ('activeGroups' in settings) {
    $('#actionGroupSelector').dropdown('set selected', settings.activeGroups);
  }

  // added 0.31, absence of key uses default
  $('#expMaxFail input').val(settings.search.maxFailures);

  // added 0.33
  if (!('toggleRate' in settings.search))
    settings.search.toggleRate = 0.5;
  $('#expToggleRate input').val(settings.search.toggleRate);

  if (!('axisReq' in settings.search))
    settings.search.axisReq = 2;
  $('#expAxisReq input').val(settings.search.axisReq);

  if (!('setHueThreshold' in settings.search))
    settings.search.setHueThreshold = 0.4;
  $('#expSetHueThreshold input').val(settings.search.setHueThreshold);

  if (!('setSatThreshold' in settings.search))
    settings.search.setSatThreshold = 0.4;
  $('#expSetSatThreshold input').val(settings.search.setSatThreshold);

  if (!('setBrightThreshold' in settings.search))
    settings.search.setBrightThreshold = 0.4;
  $('#expSetBrightThreshold input').val(settings.search.setBrightThreshold);

  if (!('setStructThreshold' in settings.search))
    settings.search.setStructThreshold = 0.15;
  $('#expSetStructThreshold input').val(settings.search.setStructThreshold);

  if (!('brightTolerance' in settings.search))
    settings.search.brightTolerance = 0.75;
  $('#expBrightTolerance input').val(settings.search.brightTolerance);

  if (!('hueTolerance' in settings.search))
    settings.search.hueTolerance = 0.75;
  $('#expHueTolerance input').val(settings.search.hueTolerance);

  if (!('clipTolerance' in settings.search))
    settings.search.clipTolerance = 0.85;
  $('#expClipTolerance input').val(settings.search.clipTolerance);

  // added in 0.34
  if (!('structMode' in settings.search))
    settings.search.structMode = 0;
  $('#expStructSelector').dropdown('set selected', settings.search.structMode + 1);

  if (!('structBinSize' in settings.search))
    settings.search.structBinSize = 16;
  $('#expStructBinSize input').val(settings.search.structBinSize);

  // added 0.35
  if (!('ssimA' in settings.search))
    settings.search.ssimA = 0;
  $('#expSSIMA input').val(settings.search.ssimA);

  if (!('ssimB' in settings.search))
    settings.search.ssimB = 0;
  $('#expSSIMB input').val(settings.search.ssimB);

  if (!('ssimG' in settings.search))
    settings.search.ssimG = 1;
  $('#expSSIMG input').val(settings.search.ssimG);
}

function updateCeresSettings() {
  // updates ui elements with current values stored in the ceres settings object
  // this object gets written to disk before running a ceres search.
  $('#genReturnSize input').val(g_ceresSettings["evo"]["returnSize"]);
  $('#genIters input').val(g_ceresSettings["evo"]["maxIters"]);
  $('#genPopSize input').val(g_ceresSettings["evo"]["popSize"]);
  $('#genPoolSize input').val(g_ceresSettings["evo"]["poolSize"]);
  $('#genArcSize input').val(g_ceresSettings["evo"]["archiveSize"]);
  $('#genK input').val(g_ceresSettings["evo"]["selectK"]);
  $('#genMutation input').val(g_ceresSettings["evo"]["mutationRate"]);
  $('#genCrossover input').val(g_ceresSettings["evo"]["crossoverRate"]);
  $('#genCrossoverChance input').val(g_ceresSettings["evo"]["crossoverChance"]);
  $('#genCeresRate input').val(g_ceresSettings["evo"]["ceresRate"]);

  $('#genIncludeStart').checkbox(g_ceresSettings["evo"]["includeStartConfig"] ? 'set checked' : 'set unchecked');
  $('#genOptimizeFitness').checkbox(g_ceresSettings["evo"]["optimizeBeforeFitness"] ? 'set checked' : 'set unchecked');
  $('#genElitist').checkbox(g_ceresSettings["evo"]["elitistReproduction"] ? 'set checked' : 'set unchecked');
  $('#genOptimizeFinal').checkbox(g_ceresSettings["evo"]["optimizeFinal"] ? 'set checked' : 'set unchecked');
  $('#genOptimizeArc').checkbox(g_ceresSettings["evo"]["optimizeArc"] ? 'set checked' : 'set unchecked');
  $('#genExportArc').checkbox(g_ceresSettings["evo"]["exportArchives"] ? 'set checked' : 'set unchecked');
  $('#genExportPopulations').checkbox(g_ceresSettings["evo"]["exportPopulations"] ? 'set checked' : 'set unchecked');

  $('#genLogLevel').dropdown('set selected', g_ceresSettings.evo.logLevel + 1);
  $('#returnOrderSelector').dropdown('set selected', g_ceresSettings.evo.returnOrder + 1);
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
    else if (type === 11) {
      html += startParamSection(name, "Invert");
      html += endParamSection(name, type);
    }
    else if (type === 12) {
      html += startParamSection(name, "Brightness and Contrast");
      html += addSliders(name, "Brightness and Contrast", ["brightness", "contrast"]);
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
  if (typeof (doc) !== "object")
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
  $('.setName').click(function () {
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
  $('.layerSet .groupButton').click(function () {
    // get group name
    var group = $(this).attr("setName");

    // toggle shadow doc visibility
    var visible = toggleGroupVisibility(group, docTree);

    // also flip the linked group in the photoshop tree
    let secondary = $('.visibleButton[layerName="' + group + '"]');
    if (visible) {
      $(this).html('<i class="unhide icon"></i>');
      $(this).removeClass("black");
      $(this).addClass("white");
      secondary.html('<i class="unhide icon"></i>');
      secondary.removeClass("black");
      secondary.addClass("white");
    }
    else {
      $(this).html('<i class="hide icon"></i>');
      $(this).removeClass("white");
      $(this).addClass("black");
      secondary.html('<i class="hide icon"></i>');
      secondary.removeClass("white");
      secondary.addClass("black");
    }

    renderImage(".layerSet visibility change callback");
  });

  // group opacity
  $('.groupSlider[paramName="opacity"]').each(function(idx, elem) {
    $(this).slider({
      orientation: "horizontal",
      range: "min",
      max: 100,
      min: 0,
      step: 0.1,
      value: c.getLayer($(this).attr('setName')).opacity() * 100,
      stop: function (event, ui) {
        // since these are basically just uh, regular layers, we can treat them as such.
        groupOpacityChange($(this).attr('setName'), ui.value);
        renderImage(".layerSet opacity slider change callback");
        $('.paramSlider[layerName="' + $(this).attr("setName") + '"][paramName="opacity"]').slider('value', ui.value);
      },
      slide: function (event, ui) { groupOpacityChange($(this).attr("setName"), ui.value); },
      change: function (event, ui) { groupOpacityChange($(this).attr("setName"), ui.value); }
    });

    $('.groupInput[setName="' + $(this).attr('setName') + '"] input').val(c.getLayer($(this).attr('setName')).opacity() * 100);
  });

  // input box events
  $('.groupInput input').blur(function () {
    var data = parseFloat($(this).val());
    var group = $(this).parent().attr("setName");
    $('.groupSlider[setName="' + group + '"]').slider("value", data);
    $('.paramSlider[layerName="' + group + '"][paramName="opacity"]').slider('value', data);
    renderImage(".layerSet opacity input change callback");
  });
  $('.groupInput input').keydown(function (event) {
    if (event.which != 13)
      return;

    var data = parseFloat($(this).val());
    var group = $(this).parent().attr("setName");
    $('.groupSlider[setName="' + group + '"]').slider("value", data);
    $('.paramSlider[layerName="' + group + '"][paramName="opacity"]').slider('value', data);
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
    else if (type === 12) {
      bindBrightnessEvents(name, "Brightness and Contrast", layer);
    }
  }
}

function bindStandardEvents(name, layer) {
  // visibility
  $('button[layerName="' + name + '"]').on('click', function () {
    // check status of button
    let visible = !$('.visibleButton[layerName="' + name + '"] i').hasClass('unhide');
    layer.visible(visible);

    var button = $('button[layerName="' + name + '"]');

    if (visible) {
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
  if (c.getLayer(name).visible()) {
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
    onChange: function (value, text) {
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

  bindLayerParamControl(name, layer, "opacity", layer.opacity() * 100, "", { "uiHandler": handleParamChange });
}

function bindSectionEvents(name, layer) {
  // simple hide function for sections
  $('.layer[layerName="' + name + '"] .divider').click(function () {
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
  bindLayerParamControl(name, layer, "hue", (layer.getAdjustment(adjType.HSL).hue - 0.5) * 360, sectionName,
    { "range": false, "max": 180, "min": -180, "step": 0.1, "uiHandler": handleHSLParamChange });
  bindLayerParamControl(name, layer, "saturation", (layer.getAdjustment(adjType.HSL).sat - 0.5) * 200, sectionName,
    { "range": false, "max": 100, "min": -100, "step": 0.1, "uiHandler": handleHSLParamChange });
  bindLayerParamControl(name, layer, "lightness", (layer.getAdjustment(adjType.HSL).light - 0.5) * 200, sectionName,
    { "range": false, "max": 100, "min": -100, "step": 0.1, "uiHandler": handleHSLParamChange });
}

function bindLevelsEvents(name, sectionName, layer) {
  bindLayerParamControl(name, layer, "inMin", (layer.getAdjustment(adjType.LEVELS).inMin * 255), sectionName,
    { "range": "min", "max": 255, "min": 0, "step": 1, "uiHandler": handleLevelsParamChange });
  bindLayerParamControl(name, layer, "inMax", (layer.getAdjustment(adjType.LEVELS).inMax * 255), sectionName,
    { "range": "max", "max": 255, "min": 0, "step": 1, "uiHandler": handleLevelsParamChange });
  bindLayerParamControl(name, layer, "gamma", (layer.getAdjustment(adjType.LEVELS).gamma * 10), sectionName,
    { "range": false, "max": 10, "min": 0, "step": 0.01, "uiHandler": handleLevelsParamChange });
  bindLayerParamControl(name, layer, "outMin", (layer.getAdjustment(adjType.LEVELS).outMin * 255), sectionName,
    { "range": "min", "max": 255, "min": 0, "step": 1, "uiHandler": handleLevelsParamChange });
  bindLayerParamControl(name, layer, "outMax", (layer.getAdjustment(adjType.LEVELS).outMax * 255), sectionName,
    { "range": "max", "max": 255, "min": 0, "step": 1, "uiHandler": handleLevelsParamChange });
}

function bindExposureEvents(name, sectionName, layer) {
  bindLayerParamControl(name, layer, "exposure", (layer.getAdjustment(adjType.EXPOSURE).exposure - 0.5) * 10, sectionName,
    { "range": false, "max": 5, "min": -5, "step": 0.1, "uiHandler": handleExposureParamChange });
  bindLayerParamControl(name, layer, "offset", layer.getAdjustment(adjType.EXPOSURE).offset - 0.5, sectionName,
    { "range": false, "max": 0.5, "min": -0.5, "step": 0.01, "uiHandler": handleExposureParamChange });
  bindLayerParamControl(name, layer, "gamma", layer.getAdjustment(adjType.EXPOSURE).gamma * 10, sectionName,
    { "range": false, "max": 10, "min": 0.01, "step": 0.01, "uiHandler": handleExposureParamChange });
}

function bindColorBalanceEvents(name, sectionName, layer) {
  bindLayerParamControl(name, layer, "shadow R", (layer.getAdjustment(adjType.COLOR_BALANCE).shadowR - 0.5) * 2, sectionName,
    { "range": false, "max": 1, "min": -1, "step": 0.01, "uiHandler": handleColorBalanceParamChange });
  bindLayerParamControl(name, layer, "shadow G", (layer.getAdjustment(adjType.COLOR_BALANCE).shadowG - 0.5) * 2, sectionName,
    { "range": false, "max": 1, "min": -1, "step": 0.01, "uiHandler": handleColorBalanceParamChange });
  bindLayerParamControl(name, layer, "shadow B", (layer.getAdjustment(adjType.COLOR_BALANCE).shadowB - 0.5) * 2, sectionName,
    { "range": false, "max": 1, "min": -1, "step": 0.01, "uiHandler": handleColorBalanceParamChange });
  bindLayerParamControl(name, layer, "mid R", (layer.getAdjustment(adjType.COLOR_BALANCE).midR - 0.5) * 2, sectionName,
    { "range": false, "max": 1, "min": -1, "step": 0.01, "uiHandler": handleColorBalanceParamChange });
  bindLayerParamControl(name, layer, "mid G", (layer.getAdjustment(adjType.COLOR_BALANCE).midG - 0.5) * 2, sectionName,
    { "range": false, "max": 1, "min": -1, "step": 0.01, "uiHandler": handleColorBalanceParamChange });
  bindLayerParamControl(name, layer, "mid B", (layer.getAdjustment(adjType.COLOR_BALANCE).midB - 0.5) * 2, sectionName,
    { "range": false, "max": 1, "min": -1, "step": 0.01, "uiHandler": handleColorBalanceParamChange });
  bindLayerParamControl(name, layer, "highlight R", (layer.getAdjustment(adjType.COLOR_BALANCE).highR - 0.5) * 2, sectionName,
    { "range": false, "max": 1, "min": -1, "step": 0.01, "uiHandler": handleColorBalanceParamChange });
  bindLayerParamControl(name, layer, "highlight G", (layer.getAdjustment(adjType.COLOR_BALANCE).highG - 0.5) * 2, sectionName,
    { "range": false, "max": 1, "min": -1, "step": 0.01, "uiHandler": handleColorBalanceParamChange });
  bindLayerParamControl(name, layer, "highlight B", (layer.getAdjustment(adjType.COLOR_BALANCE).highB - 0.5) * 2, sectionName,
    { "range": false, "max": 1, "min": -1, "step": 0.01, "uiHandler": handleColorBalanceParamChange });

  bindToggleControl(name, sectionName, layer, "preserveLuma", "COLOR_BALANCE");
}

function bindPhotoFilterEvents(name, sectionName, layer) {
  bindColorPickerControl('.paramColor[layerName="' + layer.name() + '"][sectionName="' + sectionName + '"]', "PHOTO_FILTER", layer);

  bindLayerParamControl(name, layer, "density", layer.getAdjustment(adjType.PHOTO_FILTER).density, sectionName,
    { "range": "min", "max": 1, "min": 0, "step": 0.01, "uiHandler": handlePhotoFilterParamChange });

  bindToggleControl(name, sectionName, layer, "preserveLuma", "PHOTO_FILTER");
}

function bindColorizeEvents(name, sectionName, layer) {
  bindColorPickerControl('.paramColor[layerName="' + layer.name() + '"][sectionName="' + sectionName + '"]', "COLORIZE", layer);

  bindLayerParamControl(name, layer, "alpha", layer.getAdjustment(adjType.COLORIZE).a, sectionName,
    { "range": "min", "max": 1, "min": 0, "step": 0.01, "uiHandler": handleColorizeParamChange });
}

function bindLighterColorizeEvents(name, sectionName, layer) {
  bindColorPickerControl('.paramColor[layerName="' + layer.name() + '"][sectionName="' + sectionName + '"]', "LIGHTER_COLORIZE", layer);

  bindLayerParamControl(name, layer, "alpha", layer.getAdjustment(adjType.LIGHTER_COLORIZE).a, sectionName,
    { "range": "min", "max": 1, "min": 0, "step": 0.01, "uiHandler": handleLighterColorizeParamChange });
}

function bindOverwriteColorEvents(name, sectionName, layer) {
  bindColorPickerControl('.paramColor[layerName="' + layer.name() + '"][sectionName="' + sectionName + '"]', "OVERWRITE_COLOR", layer);

  bindLayerParamControl(name, layer, "alpha", layer.getAdjustment(adjType.OVERWRITE_COLOR).a, sectionName,
    { "range": "min", "max": 1, "min": 0, "step": 0.01, "uiHandler": handleOverwriteColorizeParamChange });
}

function bindBrightnessEvents(name, sectionName, layer) {
  bindLayerParamControl(name, layer, "brightness", (layer.getAdjustment(adjType.BRIGHTNESS).brightness - 0.5) * 2, sectionName,
    { "range": false, "max": 1, "min": -1, "step": 0.01, "uiHandler": handleBrightnessParamChange });
  bindLayerParamControl(name, layer, "contrast", (layer.getAdjustment(adjType.BRIGHTNESS).contrast - 0.5) * 2, sectionName,
    { "range": false, "max": 1, "min": -1, "step": 0.01, "uiHandler": handleBrightnessParamChange });
}

function bindSelectiveColorEvents(name, sectionName, layer) {
  // parameter controls
  bindLayerParamControl(name, layer, "cyan", (layer.selectiveColorChannel("reds", "cyan") - 0.5) * 200, sectionName,
    { "range": false, "max": 100, "min": -100, "step": 0.01, "uiHandler": handleSelectiveColorParamChange });
  bindLayerParamControl(name, layer, "magenta", (layer.selectiveColorChannel("reds", "magenta") - 0.5) * 200, sectionName,
    { "range": false, "max": 100, "min": -100, "step": 0.01, "uiHandler": handleSelectiveColorParamChange });
  bindLayerParamControl(name, layer, "yellow", (layer.selectiveColorChannel("reds", "yellow") - 0.5) * 200, sectionName,
    { "range": false, "max": 100, "min": -100, "step": 0.01, "uiHandler": handleSelectiveColorParamChange });
  bindLayerParamControl(name, layer, "black", (layer.selectiveColorChannel("reds", "black") - 0.5) * 200, sectionName,
    { "range": false, "max": 100, "min": -100, "step": 0.01, "uiHandler": handleSelectiveColorParamChange });

  $('.tabMenu[layerName="' + name + '"][sectionName="' + sectionName + '"]').dropdown('set selected', 0);
  $('.tabMenu[layerName="' + name + '"][sectionName="' + sectionName + '"]').dropdown({
    action: 'activate',
    onChange: function (value, text) {
      // update sliders
      var params = ["cyan", "magenta", "yellow", "black"];
      for (var j = 0; j < params.length; j++) {
        s = 'div[sectionName="' + sectionName + '"] .paramSlider[layerName="' + name + '"][paramName="' + params[j] + '"]';
        i = 'div[sectionName="' + sectionName + '"] .paramInput[layerName="' + name + '"][paramName="' + params[j] + '"] input';

        var val = (layer.selectiveColorChannel(text, params[j]) - 0.5) * 200;
        $(s).slider({ value: val });
        $(i).val(String(val.toFixed(2)));
      }
    }
  });

  bindToggleControl(name, sectionName, layer, "relative", "SELECTIVE_COLOR");
}

function bindLayerParamControl(name, layer, paramName, initVal, sectionName, psettings = {}) {
  var s, i;

  if (sectionName !== "") {
    s = 'div[sectionName="' + sectionName + '"] .paramSlider[layerName="' + name + '"][paramName="' + paramName + '"]';
    i = 'div[sectionName="' + sectionName + '"] .paramInput[layerName="' + name + '"][paramName="' + paramName + '"] input';
  }
  else {
    s = '.paramSlider[layerName="' + name + '"][paramName="' + paramName + '"]';
    i = '.paramInput[layerName="' + name + '"][paramName="' + paramName + '"] input';
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
    stop: function (event, ui) {
      psettings.uiHandler(name, ui);
      renderImage('layer ' + name + ' parameter ' + paramName + ' change');
    },
    slide: function (event, ui) { psettings.uiHandler(name, ui); },
    change: function (event, ui) { psettings.uiHandler(name, ui); },
  });

  $(i).val(String(initVal.toFixed(2)));

  // input box events
  $(i).blur(function () {
    var data = parseFloat($(this).val());
    $(s).slider("value", data);
    renderImage('layer ' + name + ' parameter ' + paramName + ' change');
  });
  $(i).keydown(function (event) {
    if (event.which != 13)
      return;

    var data = parseFloat($(this).val());
    $(s).slider("value", data);
    renderImage('layer ' + name + ' parameter ' + paramName + ' change');
  });
}

function bindColorPickerControl(selector, adjustmentType, layer) {
  $(selector).click(function () {
    if ($('#colorPicker').hasClass('hidden')) {
      // move color picker to spot
      var thisElem = $(selector);
      var offset = thisElem.offset();

      var adj = layer.getAdjustment(adjType[adjustmentType]);
      cp.setColor({ "r": adj.r * 255, "g": adj.g * 255, "b": adj.b * 255 }, 'rgb');
      cp.startRender();

      $("#colorPicker").css({ 'left': '', 'right': '', 'top': '', 'bottom': '' });
      if (offset.top + thisElem.height() + $('#colorPicker').height() > $('body').height()) {
        $('#colorPicker').css({ "right": "10px", top: offset.top - $('#colorPicker').height() });
      }
      else {
        $('#colorPicker').css({ "right": "10px", top: offset.top + thisElem.height() });
      }

      // assign callbacks to update proper color
      cp.color.options.actionCallback = function (e, action) {
        console.log(action);
        if (action === "changeXYValue" || action === "changeZValue" || action === "changeInputValue") {
          var color = cp.color.colors.rgb;
          updateColor(layer, adjType[adjustmentType], color);
          $(thisElem).css({ "background-color": "#" + cp.color.colors.HEX });

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
  var colorStr = "rgb(" + parseInt(adj.r * 255) + "," + parseInt(adj.g * 255) + "," + parseInt(adj.b * 255) + ")";
  $(selector).css({ "background-color": colorStr });
}

function bindToggleControl(name, sectionName, layer, param, adjustment) {
  var elem = '.checkbox[paramName="' + param + '"][layerName="' + name + '"][sectionName="' + sectionName + '"]';
  $(elem).checkbox({
    onChecked: function () {
      layer.addAdjustment(adjType[adjustment], param, 1);
      if (layer.visible()) { renderImage('layer ' + name + ' parameter ' + param + ' toggle'); }
    },
    onUnchecked: function () {
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
  canvas.attr({ width: canvas.width(), height: canvas.height() });
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
  canvas.attr({ width: w, height: h });
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
      ctx.arc(curve[j].x * w, h - curve[j].y * h, 1.5 * (w / 100), 0, 2 * Math.PI);
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
  html += '<div class="clearBoth"></div>';
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
  menu += '<div class="item" data-value="14">Color Burn</div>';
  menu += '<div class="item" data-value="15">Vivid Light</div>';
  menu += '</div>';
  menu += '</div>';

  return menu;
}

function genAddAdjustmentButton(name) {
  var b = '<div class="ui mini icon top left pointing scrolling dropdown button addAdjustment" layerName="' + name + '">';
  b += '<i class="plus icon"></i>';
  b += '<div class="menu">';
  b += '<div class="header">Add Adjustment</div>';
  b += '<div class="item" data-value="0">HSL</div>';
  b += '<div class="item" data-value="1">Levels</div>';
  // curves omitted
  b += '<div class="item" data-value="3">Exposure</div>';
  // gradient map omitted
  //b += '<div class="item" data-value="5">Selective Color</div>';
  b += '<div class="item" data-value="6">Color Balance</div>';
  b += '<div class="item" data-value="7">Photo Filter</div>';
  b += '<div class="item" data-value="8">Colorize</div>';
  b += '<div class="item" data-value="9">Lighter Colorize</div>';
  b += '<div class="item" data-value="10">Overwrite Color</div>';
  b += '<div class="item" data-value="11">Invert</div>';
  b += '<div class="item" data-value="12">Brightness and Contrast</div>';
  b += '</div>';
  b += '</div>';

  return b;
}

// adds an adjustment to the specified layer
// the UI should probably regenerate the relevant controls
function addAdjustmentToLayer(name, adjType) {
  if (adjType === 0) {
    c.getLayer(name).addHSLAdjustment(0.5, 0.5, 0.5);
  }
  else if (adjType === 1) {
    c.getLayer(name).addLevelsAdjustment(0, 1);
  }
  // curves omitted
  else if (adjType === 3) {
    c.getLayer(name).addExposureAdjustment(0.5, 0.5, 1 / 10);
  }
  // gradient map omitted
  else if (adjType === 5) {
    c.getLayer(name).selectiveColor(false, {});
  }
  else if (adjType === 6) {
    c.getLayer(name).colorBalance(false, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5);
  }
  else if (adjType === 7) {
    c.getLayer(name).photoFilter({ preserveLuma: true, r: 1, g: 1, b: 1, density: 1 });
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
  else if (adjType === 11) {
    c.getLayer(name).addInvertAdjustment();
  }
  else if (adjType === 12) {
    c.getLayer(name).brightnessContrast(0.5, 0.5);
  }
}

function createShadowState(tree, order) {
  // just keep the entire loaded tree it's easier
  // it will have html but eh who cares
  docTree = tree;
}

/*===========================================================================*/
/* File System / Export / Import                                            */
/*===========================================================================*/

function importFile() {
  dialog.showOpenDialog({
    filters: [{ name: 'JSON Files', extensions: ['json'] }],
    title: "Load Layers"
  }, function (filePaths) {
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

    fs.readFile(file, function (err, data) {
      if (err) {
        throw err;
      }

      g_historyID = 0;
      $('#historyItems').empty();

      // load the data
      importLayers(JSON.parse(data), folder);
    });
  });
}

function openFile(transfer) {
  dialog.showOpenDialog({
    filters: [{ name: 'Darkroom Files', extensions: ['dark'] }],
    title: "Open File"
  }, function (filePaths) {
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

    if (!transfer) {
      $('#fileNameText').html(filename);
    }

    currentFile = file;

    fs.readFile(file, function (err, data) {
      if (err) {
        throw err;
      }

      // reset some ids
      g_historyID = 0;
      $('#historyItems').empty();

      // load the data
      loadLayers(JSON.parse(data), folder, transfer);
    });
  });
}

// uh yeah this is here because this file is 4000 lines long and nothing about organization makes sense anymore
function constructLayerHierarchy(sets, currentNode, data, layerData) {
  // rendering order is now based only on the top level of the tree,
  // and each group maintains its own render order
  // ordering is important here and i hope the object doesn't sort keys...
  let currentSet = [];
  for (let k in sets) {
    // this is a clipping mask and should not be in the layer order
    if (k in layerData && 'group' in layerData[k])
      continue;

    currentSet.unshift(k);
    constructLayerHierarchy(sets[k], k, data, layerData);
  }

  if (currentNode === '') {
    data.topLevel = currentSet;
  }
  else {
    data.layers[currentNode] = currentSet;
  }
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

  // under new rendering rules, we'll need to construct the hierarchy here and then
  // fill in the blanks later.
  // this should clean up the import code in the end.
  let compOrder = { topLevel: [], layers: {}};
  constructLayerHierarchy(sets, "", compOrder, data);

  // now we simply allocate layers
  for (let layerName in compOrder.layers) {
    // couple cases here
    // we'll need to check to see if there's a group field since that indicates
    // a clipping mask and we should adjust accordingly
    if (layerName in data) {
      let layer = data[layerName];

      // we'll come back and add adjustments later
      if ('group' in layer) {
        console.log('Adjustment Layer ' + layerName + ' delyaying load...')
        continue;
      }

      if ("filename" in data[layerName]) {
        c.addLayer(layerName, path + '/' + layer.filename)
      }
      else {
        c.addLayer(layerName);
      }

      if ("mask" in layer) {
        c.addMask(layerName, path + "/" + layer.mask);
      }
      
      c.getLayer(layerName).type(layer.kind);
      console.log('Loaded image data for layer ' + layerName);
    }
    else {
      // render group
      c.addLayer(layerName);
      c.getLayer(layerName).blendMode(blendModes['BlendMode.PASSTHROUGH']);
      c.getLayer(layerName).setPrecompOrder(compOrder.layers[layerName]);
      console.log('Added render group ' + layerName);
    }
  }

  // ok layers exist, time to figure out adjustment layers
  for (let layerName in data) {
    let layer = data[layerName];
    let targetLayer = layerName;
    let targetFound = false;

    // find the actual target layer by walking up the group tree
    while (!targetFound) {
      if ('group' in data[targetLayer]) {
        targetLayer = data[targetLayer].group;
      }
      else {
        targetFound = true;
      }
    }

    let type = layer.kind;
    let adjustment = layer.adjustment
    if (type === "LayerKind.HUESATURATION") {
      var hslData = { "hue": 0, "saturation": 0, "lightness": 0 };

      if ("adjustment" in adjustment) {
        hslData = adjustment.adjustment[0];
      }

      // need to extract adjustment params here
      c.getLayer(targetLayer).addHSLAdjustment((hslData.hue / 360) + 0.5, (hslData.saturation / 200) + 0.5, (hslData.lightness / 200) + 0.5);
    }
    else if (type === "LayerKind.LEVELS") {
      var levelsData = {};

      if ("adjustment" in adjustment) {
        levelsData = adjustment.adjustment[0];
      }

      var inMin = ("input" in levelsData) ? levelsData.input[0] / 255 : 0;
      var inMax = ("input" in levelsData) ? levelsData.input[1] / 255 : 1;
      var gamma = ("gamma" in levelsData) ? levelsData.gamma / 10 : 0.1;
      var outMin = ("output" in levelsData) ? levelsData.output[0] / 255 : 0;
      var outMax = ("output" in levelsData) ? levelsData.output[1] / 255 : 1;

      c.getLayer(targetLayer).addLevelsAdjustment(inMin, inMax, gamma, outMin, outMax);
    }
    else if (type === "LayerKind.CURVES") {
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

        c.getLayer(targetLayer).addCurve(channel, curve);
      }
    }
    else if (type === "LayerKind.EXPOSURE") {
      // import format should have exposure [-20, 20]
      c.getLayer(targetLayer).addExposureAdjustment((adjustment.exposure / 40), adjustment.offset + 0.5, adjustment.gammaCorrection / 10);
    }
    else if (type === "LayerKind.GRADIENTMAP") {
      colors = adjustment.gradient.colors;
      var pts = [];
      var gc = [];
      var stops = adjustment.gradient.interfaceIconFrameDimmed;

      for (i = 0; i < colors.length; i++) {
        pts.push(colors[i].location / stops);
        gc.push({ "r": colors[i].color.red / 255, "g": colors[i].color.green / 255, "b": colors[i].color.blue / 255 });
      }
      c.getLayer(targetLayer).addGradient(pts, gc);
    }
    else if (type === "LayerKind.SELECTIVECOLOR") {
      var relative = (adjustment.method === "absolute") ? false : true;
      colors = adjustment.colorCorrection;
      var sc = {};

      if (colors !== undefined) {
        for (i = 0; i < colors.length; i++) {
          var name;
          var adjust = {};

          for (var id in colors[i]) {
            if (id === "colors") {
              name = colors[i][id];
            }
            else if (id === "yellowColor") {
              adjust.yellow = (colors[i][id].value / 200) + 0.5;
            }
            else {
              adjust[id] = (colors[i][id].value / 200) + 0.5;
            }
          }

          sc[name] = adjust;
        }
      }

      c.getLayer(targetLayer).selectiveColor(relative, sc);
    }
    else if (type === "LayerKind.COLORBALANCE") {
      c.getLayer(targetLayer).colorBalance(adjustment.preserveLuminosity, (adjustment.shadowLevels[0] / 200) + 0.5,
        (adjustment.shadowLevels[1] / 200) + 0.5, (adjustment.shadowLevels[2] / 200) + 0.5,
        (adjustment.midtoneLevels[0] / 200) + 0.5, (adjustment.midtoneLevels[1] / 200) + 0.5, (adjustment.midtoneLevels[2] / 200) + 0.5,
        (adjustment.highlightLevels[0] / 200) + 0.5, (adjustment.highlightLevels[1] / 200) + 0.5, (adjustment.highlightLevels[2] / 200) + 0.5);
    }
    else if (type === "LayerKind.PHOTOFILTER") {
      var color = adjustment.color;

      var dat = { "preserveLuma": adjustment.preserveLuminosity, "density": adjustment.density / 100 };

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

      c.getLayer(targetLayer).photoFilter(dat);
    }
    else if (type === "LayerKind.SOLIDFILL") {
      if ("filename" in layer) {
        // is not an adjustment, reset the image to white so the fill works out
        c.resetImages(layerName);
      }

      // solid fill uses special controls based on the blend mode
      if (layer.blendMode === "BlendMode.COLORBLEND") {
        // unknown default color, will check if that data can be extracted
        c.getLayer(targetLayer).colorize(1, 1, 1, 0);
      }
      else if (layer.blendMode === "BlendMode.LIGHTERCOLOR") {
        c.getLayer(targetLayer).lighterColorize(1, 1, 1, 0);
      }
      else {
        c.getLayer(targetLayer).overwriteColor(1, 1, 1, 0);
      }
    }
    else if (type === "LayerKind.INVERSION") {
      c.getLayer(targetLayer).addInvertAdjustment();
    }
    else if (type === "LayerKind.BRIGHTNESSCONTRAST") {
      // range: [-100, 100] -> [0, 1]
      c.getLayer(targetLayer).brightnessContrast((adjustment.brightness + 100) / 200, (adjustment.center + 100) / 200);
    }

    // update properties
    // if this wasn't a clipping layer we should do this now
    // we know it's not clipping if the target and current names are the same
    if (targetLayer === layerName) {
      var cLayer = c.getLayer(layerName);
      cLayer.blendMode(blendModes[layer.blendMode]);
      cLayer.opacity(layer.opacity / 100);
      cLayer.visible(layer.visible);

      // check for conditional blending
      if ("blendRange" in layer) {
        var cb = layer.blendRange[0];
        var channel = cb.channel;
        cb.destWhiteMax = cb.desaturate;

        delete cb.desaturate;
        delete cb.channel;

        for (var i in cb) {
          cb[i] = cb[i] / 255;
        }

        cLayer.conditionalBlend(channel, cb);
      }

      insertLayerElem(layerName, sets);
      //createLayerControl(layerName, false, layer["kind"]);

      console.log("Added layer " + layerName);
    }
    else {
      console.log("Added adjustment layer " + layerName + ' to ' + targetLayer + ' of type ' + type);
    }
  }

  // render to page
  var layerData = generateControlHTML(sets, order);
  $('#layerControls').html(layerData);

  createShadowState(sets, order);
  // save group visibility info and individual layer transparency modifiers here
  setupFlatGroups(docTree, []);

  // bind events
  for (i = 0; i < order.length; i++) {
    bindLayerEvents(order[i]);
  }
  bindGlobalEvents();

  // and set the top level order
  c.setLayerOrder(compOrder.topLevel);

  // update internal structure
  //c.setLayerOrder(order);
  initSearch();
  renderImage('importLayers()');
  initCanvas();
  g_groupPanel.updatePreviewCanvas();
  initLayerSelector();
}

function loadLayers(doc, path, transfer) {
  // create new compositor object
  deleteAllControls();

  if (!transfer) {
    initCompositor();
  }

  var order = [];
  var movebg = false;
  var data = doc.layers;
  var ver = doc.version;

  if (ver < 1) {
    // force re-import for version 1
    showStatusMsg("Files older than version 1.00 must be re-imported.", "ERROR", "Save Version " + ver);
    return;
  }

  if (ver === undefined) {
    ver = 0;
  }

  // when loading an existing darkroom file we have some things already filled in
  // so we just load them from disk
  docTree = doc.docTree;

  // we can skip metadata creation
  // and instead just load layers directly
  // note that docTree has already been populated correctly here
  for (var layerName in data) {
    var layer = data[layerName];

    // add the layer and relevant adjustments if this isn't a transfer
    if (!transfer) {
      if (layer.isAdjustment || layer.precompOrder.length > 0) {
        c.addLayer(layerName);
      }
      else {
        c.addLayer(layerName, path + "/" + layer.filename);
      }

      if (layer.mask) {
        c.addMask(layerName, path + "/" + layer.mask);
      }
      c.getLayer(layerName).setPrecompOrder(layer.precompOrder);
    }

    var cl = c.getLayer(layerName);

    if (cl) {
      var o = layer.opacity;

      cl.opacity(o);
      cl.visible(layer.visible);
      cl.blendMode(layer.blendMode);

      if (layer.offset)
        cl.offset(layer.offset.x, layer.offset.y);

      // as of 0.36
      if ("conditionalBlend" in layer) {
        cl.conditionalBlend(layer.conditionalBlend.channel, layer.conditionalBlend.params);
      }

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

      // selective color
      if ("selectiveColor" in layer) {
        var rel = layer.selectiveColor.relative === 1;
        delete layer.selectiveColor.relative;

        cl.selectiveColor(rel, layer.selectiveColor);
      }

      // special cases
      if (layer.type === "LayerKind.SOLIDFILL" && layer.isAdjustment === false) {
        c.resetImages(layerName);
      }

      console.log("Added layer " + layerName);
    }
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

    for (var name in data) {
      regenLayerControls(name);
    }

    showStatusMsg("Check parameter values and re-import if needed.", "INFO", "Loaded Older Save Version " + ver);
  }
  else {
    // bind events
    for (var i = 0; i < order.length; i++) {
      bindLayerEvents(order[i]);
    }
  }

  // flatten
  if (ver < 0.4) {
    setupFlatGroups();
  }
  else {
    // load the groups, the layers should already exist
    for (let g in doc.groups) {
      let group = doc.groups[g];
      c.addGroupFromExistingLayer(group.name, group.affectedLayers, group.order, group.readOnly);

      if (group.effect) {
        c.addGroupEffect(group.name, group.effect);
      }
    }

    updateGroupUI();
  }

  // global controls
  bindGlobalEvents();

  // update internal structure
  c.setLayerOrder(doc.layerOrder);
  initSearch();
  renderImage("loadLayers()");
  initCanvas();
  initLayerSelector();
  g_groupPanel.updatePreviewCanvas();

  if (ver > 0.23) {
    settings = doc.settings;
    loadSettings();
  }

  // load mask layers
  if (doc.mask !== undefined) {
    g_activeConstraintLayer = doc.mask.activeConstraintLayer;
    g_paths = doc.mask.paths;
    g_pathIndex = doc.mask.pathIndex;
    g_constraintLayers = doc.mask.constraintLayers;
    g_canvasUpdated = false;

    if ("editCount" in doc.mask) {
      g_editCounter = doc.mask.editCount;
    }

    // update controls
    $('#editItems').html('');
    for (var l in g_constraintLayers) {
      // create the controls
      addEditToList(l, g_constraintLayers[l].mode, g_constraintLayers[l].color);
    }

    // prune paths, some may be null for some reason
    for (var p in g_paths) {
      if (g_paths[p] === null)
        delete g_paths[p];
    }
  }

  // load constraints
  // added: 0.27
  if (doc.constraints !== undefined) {
    console.log("Loading constraints")

    // a bit redundant but calling the functions to add constraints as if they
    // were just created from the native layer also handles UI creation
    for (cid in doc.constraints) {
      var constraint = doc.constraints[cid]
      addDebugConstraint(constraint.x, constraint.y, constraint.color, constraint.weight);
    }
  }
}

// saves the document in an easier to load format
function save(file) {
  var out = {};
  out.version = saveVersion;

  // save the document tree
  // this will have the html attached to it but that's ok.
  // can be directly loaded
  out.docTree = docTree;

  // layer data
  var layers = {};
  var cl = c.getAllLayers();
  for (var layerName in cl) {
    var l = c.getLayer(layerName);
    layers[layerName] = {};

    layers[layerName].opacity = l.opacity();
    layers[layerName].visible = l.visible();
    layers[layerName].blendMode = l.blendMode();
    layers[layerName].isAdjustment = l.isAdjustmentLayer();
    layers[layerName].filename = l.image().filename();
    layers[layerName].type = l.type();
    layers[layerName].conditionalBlend = l.conditionalBlend();
    layers[layerName].adjustments = {};
    layers[layerName].precompOrder = l.getPrecompOrder();
    layers[layerName].offset = l.offset();

    var mask = l.getMask();
    if (mask) {
      layers[layerName].mask = mask.filename();
    }

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

      if (adjTypes[i] === adjTypes.SELECTIVECOLOR) {
        layers[layerName].selectiveColor = l.selectiveColor();
      }
    }
  }

  // group save
  let groupOrder = c.getGroupOrder();
  let groups = {};
  for (let g in groupOrder) {
    let group = c.getGroup(groupOrder[g].group);
    let groupData = {};
    groupData.name = groupOrder[g].group;
    groupData.order = groupOrder[g].val;
    groupData.affectedLayers = group.affectedLayers;
    groupData.readOnly = group.readOnly;
    groupData.effect = group.effect;
    groups[groupData.name] = groupData;
  }

  out.groups = groups;
  out.layers = layers;
  out.layerOrder = c.getTopLayerOrder();

  // mask data
  out.mask = {};
  out.mask.paths = g_paths;
  out.mask.pathIndex = g_pathIndex;
  out.mask.constraintLayers = g_constraintLayers;
  out.mask.activeConstraintLayer = g_activeConstraintLayer;
  out.mask.editCount = g_editCounter;

  // constraints
  out.constraints = g_ceresDebugConstraints;

  //settings
  out.settings = settings;

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
    filters: [{ name: 'Darkroom Files', extensions: ['dark'] }],
    title: "Save"
  }, function (filePaths) {
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
    filters: [{ name: 'PNG', extensions: ['png'] }],
    title: "Save Render"
  }, function (filePaths) {
    if (filePaths === undefined) {
      return;
    }

    var file = filePaths;

    c.render().save(file);
  });
}

// exports a sample with ability to select filename
function exportSample(id) {
  var sample = g_sampleIndex[id];

  // open dialog
  dialog.showSaveDialog({
    filters: [{ name: 'PNG', extensions: ['png'] }],
    title: "Export Sample"
  }, function (filePaths) {
    if (filePaths === undefined) {
      return;
    }

    exportSingleSample(sample, filePaths);
    showStatusMsg("Saved sample " + id + " to " + file, "OK", "Export Complete");
  });
}

// exports everything in the sampleIndex map
function exportAllSamples() {
  // location to save things
  dialog.showOpenDialog({
    properties: ['openDirectory'],
    title: "Select Export Directory"
  }, function (filePaths) {
    if (filePaths === undefined)
      return;

    var folder = filePaths;
    showStatusMsg("Saving " + Object.keys(g_sampleIndex).length + " samples to " + folder, '', "Export Started");

    for (var id in g_sampleIndex) {
      exportSingleSample(g_sampleIndex[id], folder + "/" + id + ".png");
    }

    showStatusMsg("Saved " + Object.keys(g_sampleIndex).length + " samples to " + folder, "OK", "Export Complete");
  });
}

// saves a sample to the specified path. Will also save .context and .meta
// json files associated with the sample
function exportSingleSample(sample, file) {
  sample.img.save(file);

  // metadata export
  // need to split out the directory path from the file path
  var splitChar = '/';

  if (file.includes('\\')) {
    splitChar = '\\';
  }

  var folder = file.split(splitChar).slice(0, -1).join('/');
  var filename = file.split(splitChar);
  filename = filename[filename.length - 1].split(".")[0];

  // metadata
  fs.writeFile(folder + "/" + filename + ".meta.json", JSON.stringify(sample.meta, null, 2), (err) => {
    if (err) {
      showStatusMsg(err.toString(), "ERROR", "Error Saving Metadata File");
    }
  });

  // context
  fs.writeFile(folder + "/" + filename + ".context.json", JSON.stringify(contextToJSON(sample.context), null, 2), (err) => {
    if (err) {
      showStatusMsg(err.toString(), "ERROR", "Error Saving Context File");
    }
  });
}

// saves the sample vector and related data to a file. Can be used to 
function exportSampleData() {
  // open dialog
  dialog.showSaveDialog({
    filters: [{ name: 'Sample Data', extensions: ['samples'] }],
    title: "Export Sample"
  },
  function (filePaths) {
    if (filePaths === undefined)
      return;

    var serialData = {};

    for (var id in g_sampleIndex) {
      var sample = g_sampleIndex[id];
      serialData[id] = { 'ctx': sample.context.layerVector(c), 'meta': sample.meta };
    }

    fs.writeFile(filePaths, JSON.stringify(serialData, null, 2), (err) => {
      if (err) {
        showStatusMsg(err.toString(), "ERROR", "Error exporting sample data");
      }
    });

    showStatusMsg("Exported current samples to " + filePaths, "OK", "Export Complete");
  });
}

function importSampleData() {
  dialog.showOpenDialog({
    filters: [{ name: 'Sample Files', extensions: ['samples'] }],
    title: "Open File"
  },
  function (filePaths) {
    if (filePaths === undefined) {
      return;
    }

    // delete current samples
    initSearch();

    fs.readFile(filePaths[0], function (err, data) {
      if (err) {
        throw err;
      }

      var samples = JSON.parse(data);

      for (var id in samples) {
        var ctx = c.contextFromVector(samples[id].ctx);
        var img = c.renderContext(ctx);
        processNewSample(img, ctx, samples[id].meta, true);
      }

      showStatusMsg("Samples loaded from " + filePaths, "OK", "Load Complete");
    });
  });
}

function contextToJSON(ctx) {
  var layers = {};
  var cl = ctx.keys();
  for (var i in cl) {
    var layerName = cl[i];
    var l = ctx.getLayer(layerName);
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
    for (var j = 0; j < adjTypes.length; j++) {
      layers[layerName].adjustments[adjTypes[j]] = l.getAdjustment(adjTypes[j]);

      // extra data
      if (adjTypes[j] === adjType.GRADIENTMAP) {
        layers[layerName].gradient = l.getGradient();
      }

      if (adjTypes[j] === adjType.CURVES) {
        layers[layerName].curves = {};

        var channels = l.getAdjustment(adjTypes[j]);
        for (var chan in channels) {
          layers[layerName].curves[chan] = l.getCurve(chan);
        }
      }
    }
  }

  return layers;
}

/*===========================================================================*/
/* UI Callbacks                                                              */
/*===========================================================================*/

function handleParamChange(layerName, ui) {
  // hopefully ui has the name of the param somewhere
  var paramName = $(ui.handle).parent().attr("paramName");

  if (paramName == "opacity") {
    // groups are applied in reverse order
    let val = ui.value / 100;
    c.getLayer(layerName).opacity(val);

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
  }
}

function handleHSLParamChange(layerName, ui) {
  var paramName = $(ui.handle).parent().attr("paramName");

  if (paramName == "hue") {
    c.getLayer(layerName).addAdjustment(adjType.HSL, "hue", (ui.value / 360) + 0.5);
  }
  else if (paramName == "saturation") {
    c.getLayer(layerName).addAdjustment(adjType.HSL, "sat", (ui.value / 200) + 0.5);
  }
  else if (paramName == "lightness") {
    c.getLayer(layerName).addAdjustment(adjType.HSL, "light", (ui.value / 200) + 0.5);
  }

  // find associated value box and dump the value there
  $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleLevelsParamChange(layerName, ui) {
  var paramName = $(ui.handle).parent().attr("paramName");

  if (paramName == "inMin") {
    c.getLayer(layerName).addAdjustment(adjType.LEVELS, "inMin", ui.value / 255);
  }
  else if (paramName == "inMax") {
    c.getLayer(layerName).addAdjustment(adjType.LEVELS, "inMax", ui.value / 255);
  }
  else if (paramName == "gamma") {
    c.getLayer(layerName).addAdjustment(adjType.LEVELS, "gamma", ui.value / 10);
  }
  else if (paramName == "outMin") {
    c.getLayer(layerName).addAdjustment(adjType.LEVELS, "outMin", ui.value / 255);
  }
  else if (paramName == "outMax") {
    c.getLayer(layerName).addAdjustment(adjType.LEVELS, "outMax", ui.value / 255);
  }

  // find associated value box and dump the value there
  $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleExposureParamChange(layerName, ui) {
  var paramName = $(ui.handle).parent().attr("paramName");

  if (paramName === "exposure") {
    c.getLayer(layerName).addAdjustment(adjType.EXPOSURE, "exposure", (ui.value / 10) + 0.5);
  }
  else if (paramName === "offset") {
    c.getLayer(layerName).addAdjustment(adjType.EXPOSURE, "offset", ui.value + 0.5);
  }
  else if (paramName === "gamma") {
    c.getLayer(layerName).addAdjustment(adjType.EXPOSURE, "gamma", ui.value / 10);
  }

  // find associated value box and dump the value there
  $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleColorBalanceParamChange(layerName, ui) {
  var paramName = $(ui.handle).parent().attr("paramName");

  if (paramName === "shadow R") {
    c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "shadowR", (ui.value / 2) + 0.5);
  }
  else if (paramName === "shadow G") {
    c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "shadowG", (ui.value / 2) + 0.5);
  }
  else if (paramName === "shadow B") {
    c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "shadowB", (ui.value / 2) + 0.5);
  }
  else if (paramName === "mid R") {
    c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "midR", (ui.value / 2) + 0.5);
  }
  else if (paramName === "mid G") {
    c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "midG", (ui.value / 2) + 0.5);
  }
  else if (paramName === "mid B") {
    c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "midB", (ui.value / 2) + 0.5);
  }
  else if (paramName === "highlight R") {
    c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "highR", (ui.value / 2) + 0.5);
  }
  else if (paramName === "highlight G") {
    c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "highG", (ui.value / 2) + 0.5);
  }
  else if (paramName === "highlight B") {
    c.getLayer(layerName).addAdjustment(adjType.COLOR_BALANCE, "highB", (ui.value / 2) + 0.5);
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

function handleBrightnessParamChange(layerName, ui) {
  var paramName = $(ui.handle).parent().attr("paramName");

  if (paramName === "brightness") {
    c.getLayer(layerName).addAdjustment(adjType.BRIGHTNESS, "brightness", (ui.value / 2) + 0.5);
  }
  else if (paramName === "contrast") {
    c.getLayer(layerName).addAdjustment(adjType.BRIGHTNESS, "contrast", (ui.value / 2) + 0.5);
  }

  // find associated value box and dump the value there
  $(ui.handle).parent().next().find("input").val(String(ui.value));
}

function handleSelectiveColorParamChange(layerName, ui) {
  var channel = $(ui.handle).parent().parent().parent().find('.text').html();
  var paramName = $(ui.handle).parent().attr("paramName");

  c.getLayer(layerName).selectiveColorChannel(channel, paramName, (ui.value / 200) + 0.5);

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
  deleteAllUIComponents();
}

function toggleGroupVisibility(group, doc) {
  c.getLayer(group).visible(!c.getLayer(group).visible());

  return c.getLayer(group).visible();
}

function groupOpacityChange(group, val, doc) {
  c.getLayer(group).opacity(val / 100);

  $('.groupInput[setName="' + group + '"] input').val(String(val));
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
    type = "INFO";
    html += '<div class="ui info small message';
  }

  html += ' transition hidden" messageId="' + msgId + '">';
  html += '<div class="header">' + title + '</div>';
  html += '<p>' + msg + '<p>';
  html += '</div>';

  msgArea.append(html);
  console.log("[" + type + "]" + " " + title + ": " + msg);

  var msgElem = $('div[messageId="' + msgId + '"]');

  msgElem.transition({
    animation: 'fade up', onComplete: function () {
      setTimeout(function () {
        msgElem.transition({
          animation: 'fade up', onComplete: function () {
            msgElem.remove();
          }
        });
      }, 2500);
    }
  });

  msgId += 1;
}

// Renders an image from the compositor module and
// then puts it in an image tag
function renderImage(callerName) {
  if (g_renderPause !== true) {
    g_renderID++;
    var myRenderID = g_renderID;
    addRenderLog(myRenderID, callerName);

    g_historyID++;
    var myHistoryID = g_historyID;
    // history time
    g_history[myHistoryID] = { context: c.getContext() };

    c.asyncRender(settings.renderSize, function (err, img) {
      //var dat = 'data:image/png;base64,';
      //dat += img.base64();
      //$("#render").html('<img src="' + dat + '"" />');
      drawImage(img, $('#renderCanvas'));

      removeRenderLog(myRenderID);

      // more history time
      g_history[myHistoryID].img = img;
      addHistory(myHistoryID);
    });

    g_groupPanel.invalidateCardImages();
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

function addHistory(id) {
  var html = '<div class="ui item">';
  html += '<div class="ui medium image"><canvas id="history_' + id + '" width="' + g_history[id].img.width() + '" height="' + g_history[id].img.height() + '"></canvas></div>';
  html += '<div class="ui middle aligned content"><div class="header">History ID: ' + id + '</div>';
  html += '<div class="ui divider"></div><div class="description"><div class="ui inverted mini button" historyID="' + id + '">Restore</div></div>';
  html += '</div>';

  $("#historyItems").prepend(html);

  // draw to canvas
  var canvas = $('#history_' + id);
  var ctx = canvas[0].getContext("2d");
  var w = canvas[0].width;
  var h = canvas[0].height;

  var imDat = new ImageData(g_history[id].img.data(), g_history[id].img.width(), g_history[id].img.height());
  createImageBitmap(imDat).then(function (imbit) {
    ctx.drawImage(imbit, 0, 0, w, h);
  });

  $('.button[historyID="' + id + '"]').click(function () {
    c.setContext(g_history[id].context);
    updateLayerControls();
  });
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

    if (sampleId > g_sideboardReserveStart) {
      // we want to render this sample now at high quality, async
      c.asyncRenderContext(g_sideboard[sampleId].context, settings.renderSize, function (err, img) {
        // replace the relevant image tags
        // because this is single threaded, if at the time of render completion, the user has
        // previewed a sample, this will also update the sample image (we copied it so the selector
        // will also apply to the preview).
        g_sideboard[sampleId].img = img;
        var src = 'data:image/png;base64,' + img.base64();
        $('img[sampleId="' + sampleId + '"]').attr('src', src).removeClass('fullRenderQueued');
        removeRenderLog(myRenderID);
      });
    }
    else {
      // we want to render this sample now at high quality, async
      c.asyncRenderContext(g_sampleIndex[sampleId].context, settings.renderSize, function (err, img) {
        // replace the relevant image tags
        // because this is single threaded, if at the time of render completion, the user has
        // previewed a sample, this will also update the sample image (we copied it so the selector
        // will also apply to the preview).
        g_sampleIndex[sampleId].img = img;
        var src = 'data:image/png;base64,' + img.base64();
        $('img[sampleId="' + sampleId + '"]').attr('src', src).removeClass('fullRenderQueued');
        removeRenderLog(myRenderID);
      });
    }
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
  c.setContext(g_sampleIndex[sampleId].context);

  // the model changed, update the ui elements
  updateLayerControls();
}

// stashes the selected sample to the sideboard
function stashSample(id) {
  // find the object
  var sample = g_sampleIndex[id];

  // change the id
  var newID = g_sideboardID;
  g_sideboardID++;

  // create a new element
  $('#sideboardWrapper').append(createSampleContainer(sample.img, newID));

  // Replace the stash menu option with delete
  $('#sideboardWrapper .card[sampleId="' + newID + '"] .item.stashSampleCmd').removeClass('stashSampleCmd').addClass('deleteStashSampleCmd');
  $('#sideboardWrapper .card[sampleId="' + newID + '"] .item.deleteStashSampleCmd').html('Delete');

  // add to sideboard list
  g_sideboard[newID] = sample;

  // bind the dimmer
  $('#sideboardWrapper .sample[sampleId="' + newID + '"] .image').dimmer({
    on: 'hover',
    duration: {
      show: 100,
      hide: 100
    }
  });

  // start the menu
  $('#sideboardWrapper .sample[sampleId="' + newID + '"] .dropdown').dropdown({
    action: 'hide'
  });
}

function deleteStashSample(id) {
  // delete the object
  delete g_sideboard[id];

  // delete the elem
  $('#sideboardWrapper .sample[sampleId="' + id + '"]').remove();

  hidePreview();
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
    updateSliderControl(layerName, "opacity", "", layer.opacity() * 100);

    // visibility
    var button = $('button[layerName="' + layerName + '"]');
    if (c.getLayer(layerName).visible()) {
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
        // hue wraps. let javascript handle the fmod op
        if (adj.hue < 0) {
          adj.hue = 1 + (adj.hue % 1);
        }

        updateSliderControl(layerName, "hue", "Hue/Saturation", ((adj.hue % 1) - 0.5) * 360);
        updateSliderControl(layerName, "saturation", "Hue/Saturation", (adj.sat - 0.5) * 200);
        updateSliderControl(layerName, "lightness", "Hue/Saturation", (adj.light - 0.5) * 200);
      }
      else if (type === 1) {
        // levels
        updateSliderControl(layerName, "inMin", "Levels", adj.inMin * 255);
        updateSliderControl(layerName, "inMax", "Levels", adj.inMax * 255);
        updateSliderControl(layerName, "gamma", "Levels", adj.gamma * 10);
        updateSliderControl(layerName, "outMin", "Levels", adj.outMin * 255);
        updateSliderControl(layerName, "outMax", "Levels", adj.outMax * 255);
      }
      else if (type === 2) {
        // curves
        updateCurve(layerName);
      }
      else if (type === 3) {
        // exposure
        updateSliderControl(layerName, "exposure", "Exposure", (adj.exposure - 0.5) * 10);
        updateSliderControl(layerName, "offset", "Exposure", adj.offset - 0.5);
        updateSliderControl(layerName, "gamma", "Exposure", adj.gamma * 10);
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
        updateSliderControl(layerName, "cyan", "Selective Color", (sc.cyan - 0.5) * 200);
        updateSliderControl(layerName, "magenta", "Selective Color", (sc.magenta - 0.5) * 200);
        updateSliderControl(layerName, "yellow", "Selective Color", (sc.yellow - 0.5) * 200);
        updateSliderControl(layerName, "black", "Selective Color", (sc.black - 0.5) * 200);
      }
      else if (type === 6) {
        // color balance
        updateSliderControl(layerName, "shadow R", "Color Balance", adj.shadowR * 2 - 1);
        updateSliderControl(layerName, "shadow G", "Color Balance", adj.shadowG * 2 - 1);
        updateSliderControl(layerName, "shadow B", "Color Balance", adj.shadowB * 2 - 1);
        updateSliderControl(layerName, "mid R", "Color Balance", adj.midR * 2 - 1);
        updateSliderControl(layerName, "mid G", "Color Balance", adj.midG * 2 - 1);
        updateSliderControl(layerName, "mid B", "Color Balance", adj.midB * 2 - 1);
        updateSliderControl(layerName, "highlight R", "Color Balance", adj.highR * 2 - 1);
        updateSliderControl(layerName, "highlight G", "Color Balance", adj.highG * 2 - 1);
        updateSliderControl(layerName, "highlight B", "Color Balance", adj.highB * 2 - 1);
      }
      else if (type === 7) {
        // photo filter
        updateColorControl(layerName, "Photo Filter", adj);
        updateSliderControl(layerName, "density", "Photo Filter", adj.density);
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
    $('div[sectionName="' + section + '"] .paramSlider[layerName="' + name + '"][paramName="' + param + '"]').slider("value", val);
    $('div[sectionName="' + section + '"] .paramInput[layerName="' + name + '"][paramName="' + param + '"] input').val(String(val.toFixed(2)));
  }
}

function updateColorControl(name, section, adj) {
  var selector = '.paramColor[layerName="' + name + '"][sectionName="' + section + '"]';
  var colorStr = "rgb(" + parseInt(adj.r * 255) + "," + parseInt(adj.g * 255) + "," + parseInt(adj.b * 255) + ")";
  $(selector).css({ "background-color": colorStr });
}

function resetShadowState() {
}

/*===========================================================================*/
/* Search                                                                    */
/*===========================================================================*/

function initSearch() {
  g_sampleIndex = {};
  sampleId = 0;
  $('#sampleWrapper').empty();
  //$('#sideboardWrapper').empty();
  resetMetadataFilters();
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

    if (settings.search.mode === 4) {
      ceresAll();
    }
    else {
      // couple things to add to the search settings
      var searchSettings = settings.search;
      searchSettings.mutationRate = parseFloat($('#genMutation input').val());
      searchSettings.crossoverChance = parseFloat($('#genCrossoverChance input').val());
      searchSettings.crossoverRate = parseFloat($('#genCrossover input').val());

      // search groups
      c.clearSearchGroups();
      c.addSearchGroup({ type: 0, layers: g_structureGroups[settings.activeGroups] });

      c.startSearch(settings.search.mode, settings.search, settings.sampleThreads, settings.sampleRenderSize);
    }
    console.log("Search started");
  }
  else {
    // search is stopping
    $(elem).html("Stopping Search...");
    showStatusMsg("", "", "Stopping Search");
    $(elem).addClass("disabled");

    if (settings.search.mode === 4) {
      g_searchProcess.kill();
      $(elem).removeClass("red");
      $(elem).removeClass("disabled");
      $(elem).addClass("green");
      $(elem).html("Start Search");
      showStatusMsg("", "OK", "Search Stopped");
    }
    else {
      c.stopSearch(err => {
        $(elem).removeClass("red");
        $(elem).removeClass("disabled");
        $(elem).addClass("green");
        $(elem).html("Start Search");
        showStatusMsg("", "OK", "Search Stopped");
      });
    }
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

function processNewSample(img, ctx, meta, force) {
  // discard sample if too many already in the results section
  if (force !== true) {
    if (sampleId > settings.maxResults)
      return;
  }

  // eventually we will need references to each context element in order
  // to render the images at full size
  g_sampleIndex[sampleId] = new Sample(img, ctx, meta);
  var elem = $(createSampleContainer(img, sampleId, meta));
  $('#sampleWrapper').append(elem).isotope('appended', elem).isotope();

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

  // update the metadata menu
  updateMetadataFilters(meta);

  sampleId += 1;

  // if we have too many samples we should stop
  if (force !== true && sampleId > settings.maxResults) {
    stopSearch();
  }
}

// updates the metadata display menu and sort options
function updateMetadataFilters(meta) {
  // additive only. don't want to remove options while search is happening
  for (var key in meta) {
    if (!($('#metadataDisplay .item[data-value="' + key + '"]').length)) {
      $('#metadataDisplay .menu').append('<div class="item" data-value="' + key + '">' + key + '</div>');
      $('#sortMode .menu').append('<div class="item" data-value="' + key + '">' + key + '</div>');

      $('#metadataDisplay').dropdown('refresh');
      $('#sortMode').dropdown('refresh');
    }
  }
}

function resetMetadataFilters() {
  $('#metadataDisplay .menu').html('<div class="item" data-value="none">None</div>');
  $('#sortMode .menu').html('<div class="item" data-value="id">ID</div>');

  $('#metadataDisplay').dropdown('refresh');
  $('#metadataDisplay').dropdown('set selected', 'none');

  $('#sortMode').dropdown('refresh');
  $('#sortMode').dropdown('set selected', 'id');
}

function createSampleContainer(img, id, meta) {
  var html = '<div class="ui card sample" sampleId="' + id + '">';
  html += '<div class="ui dimmable image">';

  // dimmer - other controls
  html += createSampleControls(id);

  // metadata display
  for (var key in meta) {
    var metaVal;

    if (typeof (meta[key]) === 'string') {
      metaVal = meta[key];
    }
    else {
      metaVal = meta[key].toFixed(5);
    }

    if (key === 'reason') {
      html += '<div class="ui black left ribbon label" metadata-key="' + key + '">' + key + ": " + metaVal + "</div>";
    }
    else {
      html += '<div class="ui black left ribbon label" metadata-key="' + key + '" metadata-value="' + metaVal + '">' + key + ": " + metaVal + "</div>";
    }
  }

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
  html += '<div class="ui compact inverted icon top dropdown button"><i class="ui options icon"></i>';

  // dropdown menu creation
  html += '<div class="menu">';
  html += '<div class="header">Sample Actions</div>';
  html += '<div class="item pickSampleCmd" sampleId="' + id + '">Pick</div>';
  html += '<div class="item stashSampleCmd" sampleId="' + id + '">Stash</div>';
  html += '<div class="item exportSampleCmd" sampleId="' + id + '">Export</div>';
  html += '</div></div>';

  html += '</div></div></div></div>';

  return html;
}

/*===========================================================================*/
/* Mask/Constraint Drawing                                                   */
/*===========================================================================*/

var g_paths = {};
var g_pathIndex = 0;
var g_canvasUpdated = true;
var g_constraintLayers = {};
var g_activeConstraintLayer = null;
var g_currentColor = "FFFFFF";
var g_ceresDebugPickPoint = false;

function newConstraintLayer(name, mode) {
  // constraint layers are initialized to white full color constraint mode
  g_constraintLayers[name] = { "name": name, "mode": mode, "active": true, color: "FFFFFF" };
}

function setActiveConstraintLayer(name) {
  if (name in g_constraintLayers) {
    g_activeConstraintLayer = name;
    g_constraintLayers[name].active = true;
  }
  else {
    g_activeConstraintLayer = null;
  }
}

function hideAllLayers() {
  for (var layer in g_constraintLayers) {
    g_constraintLayers[layer].active = false;
  }
}

function setConstraintLayerVisible(name) {
  g_constraintLayers[name].active = true;
}

function setConstraintLayerMode(name, mode) {
  g_constraintLayers[name].mode = mode;
}

function deleteConstraintLayer(name) {
  delete g_constraintLayers[name];

  for (var p in g_paths) {
    if (g_paths[p].layer === name) {
      delete g_paths[p];
    }
  }

  if (g_activeConstraintLayer === name) {
    g_activeConstraintLayer = null;
  }

  g_canvasUpdated = false;
}

function initCanvas() {
  // set internal resolution to 1:1 with full res render
  var img = c.render();
  var w = img.width();
  var h = img.height();

  var canvas = $('#maskCanvas');
  canvas.attr({ width: w, height: h });

  var renderCanvas = $('#renderCanvas');
  renderCanvas.attr({ width: w, height: h });

  var previewCanvas = $('#previewCanvas');
  previewCanvas.attr({ width: w, height: h });

  var vizCanvas = $('#diffVizCanvas');
  vizCanvas.attr({ width: w, height: h });

  g_ctx = canvas[0].getContext("2d");
  g_ctx.clearRect(0, 0, w, h);
  g_ctx.strokeStyle = "#FFFFFF";
  g_ctx.lineWidth = 10;
  g_ctx.lineJoin = "round";

  g_paths = [];
  g_pathIndex = 0;

  g_drawReady = true;
}

function startMoveMode(layerName) {
  g_moveModeLayer = layerName;
  $('#exitMoveButton').show();
}

function stopMoveMode() {
  g_moveModeLayer = null;
  $('.layerMoveButton').removeClass('green');
  $('#exitMoveButton').hide();
}

function canvasMousedown(e, elem) {
  if (g_moveModeLayer !== null) {
    g_moveModeInitPoint = screenToCanvas(e.pageX - $(elem).offset().left, e.pageY - $(elem).offset().top,
      elem.width, elem.height, elem.offsetWidth, elem.offsetHeight);
  }
  else {
    // permanent short circuit?
    return;

    if (g_ceresDebugPickPoint === true) {
      g_ceresDebugPickPoint = false;
      // callback to add ceres point
      var pt = screenToCanvas(e.pageX - $(elem).offset().left, e.pageY - $(elem).offset().top, elem.width, elem.height, elem.offsetWidth, elem.offsetHeight);
      addDebugConstraint(pt.x, pt.y);
      return;
    }

    if (g_activeConstraintLayer === null) {
      showStatusMsg("Create a constraint layer first.", "ERROR", "Cannot Draw Constraint")
      return;
    }

    g_isPainting = true;
    g_pathIndex++;

    // g_paths.push({ id: g_pathIndex, type: settings.maskMode, tool: settings.maskTool });

    // so the canvas is placed using the fit setting and the basic calculation will not work.
    var pt = screenToCanvas(e.pageX - $(elem).offset().left, e.pageY - $(elem).offset().top, elem.width, elem.height, elem.offsetWidth, elem.offsetHeight);

    if (settings.maskTool === "paint") {
      g_paths[g_pathIndex] = { type: "paint", pts: [], layer: g_activeConstraintLayer, color: g_currentColor };
      g_paths[g_pathIndex].pts.push(pt);
    }
    else if (settings.maskTool == "rect") {
      g_paths[g_pathIndex] = { type: "rect", pt1: pt, finished: false, layer: g_activeConstraintLayer, color: g_currentColor };
    }

    g_paths[g_pathIndex].mode = settings.maskMode;

    g_canvasUpdated = false;
  }
}

function canvasMousemove(e, elem) {
  if (g_isPainting) {
    var pt = screeToCanvas(e.pageX - $(elem).offset().left, e.pageY - $(elem).offset().top, elem.width, elem.height, elem.offsetWidth, elem.offsetHeight);

    if (settings.maskTool === "paint") {
      g_paths[g_pathIndex].pts.push(pt);
    }
    else if (settings.maskTool == "rect") {
      g_paths[g_pathIndex].pt2 = pt;
    }

    g_canvasUpdated = false;
  }
}

function canvasMouseup(e, elem) {
  if (g_moveModeLayer !== null) {
    let pt = screenToCanvas(e.pageX - $(elem).offset().left, e.pageY - $(elem).offset().top,
      elem.width, elem.height, elem.offsetWidth, elem.offsetHeight);
    let offsetX = (g_moveModeInitPoint.x - pt.x) / elem.width;
    let offsetY = (g_moveModeInitPoint.y - pt.y) / elem.height;

    c.offsetLayer(g_moveModeLayer, offsetX, offsetY);

    $('.offset').each(function(idx, elem) {
      let name = $(elem).attr('layerName');
      let offset = c.getLayer(name).offset();
      $(elem).find('.offsetText').text('Offset: ' + offset.x.toFixed(2) + ', ' + offset.y.toFixed(2));
    });

    renderImage('Layer Moved');
  }
  else {
    if (g_isPainting) {
      if (settings.maskTool === "rect") {
        g_paths[g_pathIndex].finished = true;
      }
    }

    g_isPainting = false;
    g_canvasUpdated = false;
  }
}

function clearCanvas() {
  g_paths = [];
  g_ctx.clearRect(0, 0, g_ctx.canvas.width, g_ctx.canvas.height);
  g_canvasUpdated = false;
}

function repaint() {
  // redraws the mask at regular intervals. If no update is needed, it just returns
  // for efficiency.
  window.requestAnimationFrame(repaint);
  if (g_drawReady === false) {
    g_canvasUpdated = true;
    return;
  }
  if (g_canvasUpdated === true)
    return;

  g_ctx.clearRect(0, 0, g_ctx.canvas.width, g_ctx.canvas.height);
  g_ctx.lineWidth = 10;
  g_ctx.lineJoin = "round";

  for (var p in g_paths) {
    if (g_paths[p] !== null) {
      var layer = g_constraintLayers[g_paths[p].layer];

      if (!layer.active)
        continue;

      if (g_paths[p].mode == "mask") {
        g_ctx.strokeStyle = '#' + layer.color; //g_paths[p].color;
        g_ctx.fillStyle = '#' + layer.color; //g_paths[p].color;
        g_ctx.globalCompositeOperation = "source-over";
      }
      else if (g_paths[p].mode == "erase") {
        g_ctx.strokeStyle = "#000000";
        g_ctx.fillStyle = "#000000";
        g_ctx.globalCompositeOperation = "destination-out";
      }

      if (g_paths[p].type === "paint") {
        g_ctx.beginPath();
        g_ctx.moveTo(g_paths[p].pts[0].x, g_paths[p].pts[0].y);

        for (var i = 0; i < g_paths[p].pts.length; i++) {
          g_ctx.lineTo(g_paths[p].pts[i].x, g_paths[p].pts[i].y);
        }

        g_ctx.stroke();
      }
      else if (g_paths[p].type == "rect") {
        // compute rectangle args here, need top left and width height
        if (g_paths[p].pt2 !== undefined) {
          var x = (g_paths[p].pt1.x < g_paths[p].pt2.x) ? g_paths[p].pt1.x : g_paths[p].pt2.x;
          var y = (g_paths[p].pt1.y < g_paths[p].pt2.y) ? g_paths[p].pt1.y : g_paths[p].pt2.y;

          var w = Math.abs(g_paths[p].pt1.x - g_paths[p].pt2.x);
          var h = Math.abs(g_paths[p].pt1.y - g_paths[p].pt2.y);

          if (g_paths[p].finished) {
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
  }

  g_canvasUpdated = true;
}

// Dumps the mask layers to the back end compositor
function syncMaskState() {
  c.clearMask();

  for (var l in g_constraintLayers) {
    hideAllLayers();
    g_constraintLayers[l].active = true;

    var layer = g_constraintLayers[l];
    layer.active = true;
    g_canvasUpdated = false;
    repaint();

    var canvas = $('#maskCanvas');
    // dump the canvas data to a string
    var data = canvas[0].toDataURL('image/png').substring(22);
    var name = l;
    var type = layer.mode;

    c.setMaskLayer(name, type, parseInt(canvas.attr("width")), parseInt(canvas.attr("height")), data);
  }

  // restore visibility
  for (var l in g_constraintLayers)
    g_constraintLayers[l].active = true;

  g_canvasUpdated = false;
  repaint();
  setActiveConstraintLayer(g_activeConstraintLayer);
}

function extractConstraints() {
  showStatusMsg("Computing constraints from current mask", "", "Constraint Extraction Started");

  deleteAllDebugConstraints();
  syncMaskState();

  // for now debugging is on
  var constraints = c.getPixelConstraints({
    "detailedLog": settings.detailedLog,
    "unconstrainedDensity": settings.unconstrainedDensity,
    "constrainedDensity": settings.constrainedDensity,
    "unconstrainedWeight": settings.unconstrainedWeight,
    "constrainedWeight": settings.constrainedWeight
  });

  console.log("Returned " + constraints.length + " samples.");

  for (var i in constraints) {
    var constraint = constraints[i];
    addDebugConstraint(constraint.pt.x, constraint.pt.y, constraint.color, constraint.weight);
  }

  showStatusMsg("Constraints shown in the Ceres Testing panel", "OK", "Constraint Extraction Complete");
  hideAllLayers();
  g_canvasUpdated = false;
  repaint();
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

function createNewEdit() {
  // bring up a dialog box containing the available options
  // presented in a more user-friendly way
  $('#editModalDropdown').dropdown('clear');
  $('#newEditModal .secondary').hide();
  $('#editColorPicker').hide();
  $('#newEditModal').modal('show');
}

function editModalDropdownChange(value, text) {
  if (value === "color" || value === "hue" || value == "brightness") {
    // show color picker
    $('#groupMagnitudePicker').hide();
    $('#editColorPicker').show();
    $('#newEditModal .secondary').show();
  }
  else if (value === "fixed") {
    $('#editColorPicker').hide();
    $('#groupMagnitudePicker').hide();
    $('#newEditModal .secondary').hide();
  }
  else {
    // check if it's a group
    var matches = value.match(/([\w\d]+)-(.*)/);

    if (matches !== null) {
      // we have a group
      var op = matches[1];
      var group = matches[2];

      if (op === "more" || op === "less") {
        $('#groupMagnitudeMenu').dropdown('set selected', 'some');
        $('#groupMagnitudePicker').show();
        $('#editColorPicker').hide();
        $('#newEditModal .secondary').show();
      }
    }
  }
}

function closeEditModal() {
  // upkeep things
  $('#colorPicker').addClass('hidden');
  $('#colorPicker').removeClass('visible');
}

function setupEdit() {
  $('#colorPicker').addClass('hidden');
  $('#colorPicker').removeClass('visible');

  // if using color, hue, or fixed the user now needs to select the part of the
  // image to edit. We should popup a message somewhere telling them to draw
  // and pop up a "done" button on the status bar
  // might also want to change the color of the status bar to indicate a different mode
  var type = $('#editModalDropdown').dropdown('get value');

  // make a new masking layer for this edit, switch to drawing mode
  var layerName = type + String(g_editCounter);
  g_editCounter++;

  var mode = 0;

  if (type === "color") {
    mode = 4;
  }
  else if (type === "hue") {
    mode = 1;
  }
  else if (type === "fixed") {
    mode = 2;
  }
  else if (type === "brightness") {
    mode = 6;
  }
  else {
    // groups are fairly varied so if there's not a specific mode, assume groups
    mode = 5;
  }

  newConstraintLayer(layerName, mode);

  if (mode === 2 || mode === 5) {
    g_constraintLayers[layerName].color = 'AAAAAA';
  }
  else {
    g_constraintLayers[layerName].color = cp.color.colors.HEX;
  }

  if (mode === 5) {
    // groups have no selection capability at the moment
    addGroupEditToList(layerName, type);

    // TODO: For now switching to look at exploratory search. The hope is that this 
  }
  else {
    setActiveConstraintLayer(layerName);

    // setup and show UI elements
    $('#modeStatus').html('Active Edit: ' + layerName);
    $('#modeStatus').show();

    $('#createNewEdit').hide();
    $('#editMaskComplete').show();

    $('#stickyMessage .segment .text').html('On the image, draw where you would like to apply your edit to. When done, click the "Done Editing Mask" button');
    $('#stickyMessage').show();

    // add this edit to the edit list window
    addEditToList(layerName, mode);
  }

  showStatusMsg("Edit " + layerName + " added with type " + g_constraintModesStrings[mode], "OK", "New Edit Created");
}

function addEditToList(layerName, mode, color) {
  var typeStr = g_constraintModesStrings[mode];

  var html = '<div class="item" layerName="' + layerName + '">';
  html += '<div class="right floated content">';

  if (mode === 0 || mode === 1 || mode === 4 || mode === 6) {
    html += '<div class="ui small icon button color" layerName="' + layerName + '" data-inverted="" data-tooltip="Change Color" data-position="left center"><i class="paint brush icon"></i></div>';
  }

  html += '<div class="ui small icon button redraw" layerName="' + layerName + '" data-inverted="" data-tooltip="Redraw" data-position="left center"><i class="edit icon"></i></div>';
  html += '<div class="ui small icon button view" layerName="' + layerName + '" data-inverted="" data-tooltip="View" data-position="left center"><i class="unhide icon"></i></div>';
  html += '<div class="ui small red icon button delete" layerName="' + layerName + '" data-inverted="" data-tooltip="Delete" data-position="left center"><i class="remove icon"></i></div>';
  html += '</div>';
  html += '<div class="content">';
  html += '<div class="header">' + layerName + '</div>';
  html += '<div class="description">Type: ' + typeStr + '</div>';
  html += '</div></div>';

  $('#editItems').append(html);

  // event bindings
  $('#editPanel .redraw[layerName="' + layerName + '"]').click(function () { redrawLayer(layerName); });
  $('#editPanel .view[layerName="' + layerName + '"]').click(function () { viewLayer(layerName); });
  $('#editPanel .delete[layerName="' + layerName + '"]').click(function () { deleteLayer(layerName); });

  if (mode === 0 || mode === 1 || mode === 4 || mode === 6) {
    if (color) {
      $('#editPanel .color[layerName="' + layerName + '"]').css({ "background-color": "#" + color });
    }
    else {
      $('#editPanel .color[layerName="' + layerName + '"]').css({ "background-color": "#" + cp.color.colors.HEX });
    }
    $('#editPanel .color[layerName="' + layerName + '"]').click(function () { showLayerColorPicker(layerName); });
  }
}

function addGroupEditToList(layerName, type) {
  // type is organized as "[op]-name", parse here
  var parse = type.match(/([\w\d]+)-(.*)/);
  var op = parse[1];
  var group = parse[2];

  var html = '<div class="item" layerName="' + layerName + '">';
  html += '<div class="right floated content">';

  html += '<div class="ui small red icon button delete" layerName="' + layerName + '" data-inverted="" data-tooltip="Delete" data-position="left center"><i class="remove icon"></i></div>';
  html += '</div>';
  html += '<div class="content">';
  html += '<div class="header">' + layerName + '</div>';
  html += '<div class="description">Group: ' + group + ', Operation: ' + op + '</div>';
  html += '</div></div>';

  $('#editItems').append(html);

  // event bindings
  $('#editPanel .delete[layerName="' + layerName + '"]').click(function () { deleteLayer(layerName); });

  g_constraintLayers[layerName].op = op;
  g_constraintLayers[layerName].group = group;

  // get the selected magnitude
  var mag = $('#groupMagnitudeMenu').dropdown('get value');

  // since values can change between creation and search (manual edits), calculate the actual value later
  g_constraintLayers[layerName].target = mag;

}

function redrawLayer(layerName) {
  // clear the layer
  // meaning delete all relevant paths
  hideAllLayers();
  setActiveConstraintLayer(layerName);

  for (var p in g_paths) {
    if (g_paths[p].layer === layerName) {
      delete g_paths[p];
    }
  }

  g_canvasUpdated = false;
  repaint();

  $('#modeStatus').html('Active Edit: ' + layerName);
  $('#modeStatus').show();

  $('#createNewEdit').hide();
  $('#editMaskComplete').show();

  $('#stickyMessage .segment .text').html('When you are done redrawing the affected areas, click the "Done Editing Mask" button');
  $('#stickyMessage').show();
}

function viewLayer(layerName) {
  hideAllLayers();
  setConstraintLayerVisible(layerName);
  g_canvasUpdated = false;
  repaint();

  $('#modeStatus').html('Viewing Edit: ' + layerName);
  $('#modeStatus').show();

  $('#editMaskComplete').html("Done Viewing Edit");
  $('#editMaskComplete').show();

  showStatusMsg("", "INFO", "Viewing Edit " + layerName);
}

function deleteLayer(layerName) {
  // clean up UI if deleted while editing
  if (g_activeConstraintLayer === layerName) {
    editMaskComplete();
  }

  deleteConstraintLayer(layerName);
  g_canvasUpdated = false;

  $('#editItems .item[layerName="' + layerName + '"]').remove();

  showStatusMsg("", "OK", "Deleted Edit " + layerName);
}

function showLayerColorPicker(layerName) {
  if ($('#colorPicker').hasClass('hidden')) {
    var target = $('#editPanel .color[layerName="' + layerName + '"]');

    // move color picker to spot
    var offset = target.offset();

    cp.setColor(target.css('background-color'), 'rgb');
    cp.startRender();

    $("#colorPicker").css({ 'left': '', 'right': '', 'top': '', 'bottom': '' });
    if (offset.top + target.height() + $('#colorPicker').height() > $('body').height()) {
      $('#colorPicker').css({ "left": offset.left - $('#colorPicker').width(), "top": offset.top - $('#colorPicker').height() });
    }
    else {
      $('#colorPicker').css({ "left": offset.left - $('#colorPicker').width() + target.outerWidth(), "top": offset.top + target.outerHeight() });
    }

    // assign callbacks to update proper color
    cp.color.options.actionCallback = function (e, action) {
      console.log(action);
      if (action === "changeXYValue" || action === "changeZValue" || action === "changeInputValue") {
        g_constraintLayers[layerName].color = cp.color.colors.HEX;
        g_canvasUpdated = false;

        target.css({ "background-color": "#" + cp.color.colors.HEX });
      }
    };

    $('#colorPicker').addClass('visible');
    $('#colorPicker').removeClass('hidden');
  }
  else {
    $('#colorPicker').addClass('hidden');
    $('#colorPicker').removeClass('visible');
  }
}

function editMaskComplete() {
  // when user is done editing the mask, this is called

  // remove ui elements
  $('#editMaskComplete').hide();
  $('#createNewEdit').show();
  $('#stickyMessage').hide();
  $('#modeStatus').hide();

  // reset, view changes the html but it's a special case at the moment
  $('#editMaskComplete').html("Done Editing");

  // hide the layer, and all layers really
  setActiveConstraintLayer(null);
  hideAllLayers();

  g_canvasUpdated = false;
  repaint();
}

/*===========================================================================*/
/* Ceres Optimizer                                                           */
/*===========================================================================*/

g_ceresDebugConstraints = {};
g_ceresDebugPtIndex = 0;

function ceresAll() {
  sendToCeres();

  runCeres();
}

function generateCeresCode() {
  c.computeExpContext(c.getContext(), 0, 0, "ceresFunc");
  fs.createReadStream('ceresFunc.h').pipe(fs.createWriteStream('../native/src/ceresFunc.h'));
  fs.unlink('ceresFunc.h', (exc) => { if (exc) console.log(exc); });
  showStatusMsg("Beginning Ceres compilation...", "OK", "Ceres Code Generation Complete")
  $('#runCeres').addClass("loading");
  $('#ceresRoundtrip').addClass("loading");

  child_process.exec('"./codegen/ceresCompile.bat" ' + settings.ceresConfig, (error, stdout, stderr) => {
    console.log(stderr);
    console.log(stdout);

    if (error) {
      showStatusMsg("Check console log for details", "ERROR", "Ceres Compilation Failure");
      console.log(error);
      $('#runCeres').removeClass("loading");
      $('#runCeres').addClass("disabled");
      $('#ceresRoundtrip').removeClass("loading");
      $('#ceresRoundtrip').addClass("disabled");
    }
    else {
      showStatusMsg("See console log for details.", "OK", "Ceres Compilation Complete");
      $('#runCeres').removeClass("loading");
      $('#runCeres').removeClass("disabled");
      $('#ceresRoundtrip').removeClass("loading");
      $('#ceresRoundtrip').removeClass("disabled");
    }
  });
}

function sendToCeres() {
  // arrange params
  var pts = [];
  var targets = [];
  var weights = [];

  for (var id in g_ceresDebugConstraints) {
    var data = g_ceresDebugConstraints[id];

    pts.push({ 'x': data.x, 'y': data.y });
    targets.push(data.color);
    weights.push(data.weight);        // all weights are 1 right now
  }

  c.paramsToCeres(c.getContext(), pts, targets, weights, "./codegen/ceres.json");

  // there's some extra data that'd be nice to put in there, like, all the group constraints
  //var data = JSON.parse(fs.readFileSync("./codegen/ceres.json"));



  showStatusMsg("Exported to ./codegen/ceres.json", "OK", "Ceres Data File Exported");
}

function runCeres(callback) {
  // clears the output dir first. The UI expects all results to be new files, due to potential
  // access problems and crashes if an existing file is updated
  fs.emptyDirSync("./ceresOut");

  // write the settings file
  showStatusMsg("", "INFO", "Writing Ceres Settings File");
  fs.writeFileSync("./codegen/ceres_settings.json", JSON.stringify(g_ceresSettings, null, 2));

  // invokes the ceres command line application
  var cmd = '../../ceres_harness/x64/' + settings.ceresConfig + '/ceresHarness.exe';

  if (callback === undefined) {
    callback = (code) => {
      //showStatusMsg("Ceres process finished running. Check console for errors.", "OK", "Ceres Execution Complete");
      $('#status .progress').progress('update progress', g_ceresSettings.evo.maxIters);

      var elem = $('#runSearchBtn');
      elem.removeClass("red");
      elem.removeClass("disabled");
      elem.addClass("green");
      elem.html("Start Search");

      setTimeout(() => { $('#status').transition('fade'); }, 5000);
    };
  }

  showStatusMsg("Executing command '" + cmd + "'", "", "Running Ceres");

  $('#status .progress').progress({
    total: g_ceresSettings.evo.maxIters,
    text: {
      success: 'Search Complete'
    }
  });
  $('#status .progress').progress('update progress', 0);

  $('#status').transition('fade');

  g_searchProcess = child_process.spawn(cmd, ['config', './codegen/ceres_settings.json']);

  g_searchProcess.stdout.on('data', function (data) {
    var logLine = `${data}`;
    var match = logLine.match(/Generation (\d+)/);
    if (match) {
      $('#status .progress').progress('update progress', parseInt(match[1]));
      $('#status .progress').progress('set label', 'Iteration ' + (parseInt(match[1]) + 1) + ' of ' + g_ceresSettings.evo.maxIters);
    }
    console.log(logLine);
  });

  g_searchProcess.stderr.on('data', (data) => { console.log(`stderr: ${data}`); });
  g_searchProcess.on('close', callback);
}

function ceresEval() {
  sendToCeres();
  fs.emptyDirSync("./ceresOut");

  var cmd = '"../../ceres_harness/x64/' + settings.ceresConfig + '/ceresHarness.exe" eval ./codegen/ceres.json ./ceresOut/';

  callback = (error, stdout, stderr) => {
    if (error) {
      showStatusMsg("Check console log for details.", "ERROR", "Ceres Execution Failure");
      console.log(error);
    }
  };

  showStatusMsg("Executing command '" + cmd + "'", "", "Running Ceres");

  child_process.exec(cmd, { "maxBuffer": 1000 * 1024 }, callback);
}

function computeError() {
  // send the current context to the back end to generate this
  var ctx = c.getContext();
  c.computeErrorMap(ctx, "error.png");

  showStatusMsg("Output to ./error.png", "OK", "Error Map for Current Composition Exported");
}

function importFromCeres() {
  // get context
  var ceresData = c.ceresToContext("./codegen/ceres_result.json");

  // append to results for inspection
  processNewSample(c.renderContext(ceresData.context, settings.sampleRenderSize), ceresData.context, ceresData.metadata);
}

function selectDebugConstraint() {
  // selects a point on the canvas and creates a constraint in the list
  g_ceresDebugPickPoint = true;
  $('#ceresAddPoint').addClass('disabled');
}

function deleteAllDebugConstraints() {
  $('#debugCeresConstraints').html('');

  g_ceresDebugConstraints = {};
}

function addDebugConstraint(x, y, color, weight) {
  // i guess this works to truncate to int?
  x = ~~x;
  y = ~~y;

  var data = { 'id': g_ceresDebugPtIndex, 'x': x, 'y': y, 'color': { 'r': 1, 'g': 1, 'b': 1 } };
  data.weight = 1;

  if (color !== undefined) {
    data.color = color;
  }

  if (weight !== undefined) {
    data.weight = weight;
  }

  var html = '<div class="item" pt-id="' + g_ceresDebugPtIndex + '">';
  html += '<div class="right floated content">';
  html += '<div class="ui mini input"><input type="text" /></div>';
  html += '<div class="ui mini button target">Target</div>';
  html += '<div class="ui mini red icon right floated button delete"><i class="erase icon"></i></div>';
  html += '<div class="ui mini right floated icon button useCurrent" data-content="Use Current Pixel Color"><i class="eyedropper icon"></i></div>';
  html += '</div>';
  html += '<div class="content">';
  html += '<div class="header">x: ' + x + ', y: ' + y + '</div></div></div>';

  var id = data.id;
  g_ceresDebugConstraints[g_ceresDebugPtIndex] = data;
  g_ceresDebugPtIndex++;

  $('#debugCeresConstraints').append(html);

  // event bindings
  $('#debugCeresConstraints .item[pt-id="' + data.id + '"] input').change(() => {
    g_ceresDebugConstraints[data.id].weight = parseFloat($('#debugCeresConstraints .item[pt-id="' + data.id + '"] input').val());
  });
  $('#debugCeresConstraints .item[pt-id="' + data.id + '"] input').val(data.weight);

  $('#debugCeresConstraints .item[pt-id="' + data.id + '"] .delete').click(() => {
    delete g_ceresDebugConstraints[id];
    $('#debugCeresConstraints .item[pt-id="' + id + '"]').remove();
  });

  $('#debugCeresConstraints .item[pt-id="' + data.id + '"] .useCurrent').popup();
  $('#debugCeresConstraints .item[pt-id="' + data.id + '"] .useCurrent').click(() => {
    var constraint = g_ceresDebugConstraints[id];
    var currentColor = c.renderPixel(c.getContext(), constraint.x, constraint.y);

    g_ceresDebugConstraints[id].color.r = currentColor.r;
    g_ceresDebugConstraints[id].color.g = currentColor.g;
    g_ceresDebugConstraints[id].color.b = currentColor.b;

    $('#debugCeresConstraints .item[pt-id="' + data.id + '"] .target').css({ "background-color": "rgb(" + ~~(currentColor.r * 255) + "," + ~~(currentColor.g * 255) + "," + ~~(currentColor.b * 255) + ")" });
  });

  $('#debugCeresConstraints .item[pt-id="' + data.id + '"] .target').click(() => {
    if ($('#colorPicker').hasClass('hidden')) {
      // move color picker to spot
      var offset = $('#debugCeresConstraints .item[pt-id="' + data.id + '"] .target').offset();
      var width = $('#debugCeresConstraints .item[pt-id="' + data.id + '"] .target').width();
      var height = $('#debugCeresConstraints .item[pt-id="' + data.id + '"] .target').height();

      var c = g_ceresDebugConstraints[id].color;
      cp.setColor({ "r": c.r * 255, "g": c.g * 255, "b": c.b * 255 }, 'rgb');
      cp.startRender();

      $("#colorPicker").css({ 'left': '', 'right': '', 'top': '', 'bottom': '' });
      if (offset.top + height + $('#colorPicker').height() > $('body').height()) {
        $('#colorPicker').css({ "left": offset.left - $('#colorPicker').width() + width * 2, top: offset.top - $('#colorPicker').height() });
      }
      else {
        $('#colorPicker').css({ "left": offset.left - $('#colorPicker').width() + width * 2, top: offset.top + height + 18 });
      }

      // assign callbacks to update proper color
      cp.color.options.actionCallback = function (e, action) {
        console.log(action);
        if (action === "changeXYValue" || action === "changeZValue" || action === "changeInputValue") {
          var color = cp.color.colors.rgb;
          g_ceresDebugConstraints[id].color.r = color.r;
          g_ceresDebugConstraints[id].color.g = color.g;
          g_ceresDebugConstraints[id].color.b = color.b;

          $('#debugCeresConstraints .item[pt-id="' + data.id + '"] .target').css({ "background-color": "#" + cp.color.colors.HEX });
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

  $('#ceresAddPoint').removeClass('disabled');
  $('#debugCeresConstraints .item[pt-id="' + data.id + '"] .target').css({ "background-color": "rgb(" + ~~(data.color.r * 255) + "," + ~~(data.color.g * 255) + "," + ~~(data.color.b * 255) + ")" });
}

function updateActiveGroups(text) {
  $('#groupsItems').empty();
  settings.activeGroups = text;

  // add a list element for each thing in the thing if the thing exists
  if (text in g_actionGroups) {
    resetEditMenu();

    for (var groupName in g_actionGroups[text]) {
      var itemList = "";
      for (var index in g_actionGroups[text][groupName].contents) {
        if (index !== "0")
          itemList += ", ";

        itemList += g_actionGroups[text][groupName].contents[index];
      }

      var html = '<div class="item"><div class="content"><div class="header">' + groupName + '</div><div class="description">Contents: ' + itemList + '</div></div></div>';
      $('#groupsItems').append(html);

      // menu entry
      for (var index in g_actionGroups[text][groupName].ops) {
        var op = g_actionGroups[text][groupName].ops[index];

        if (op === "more") {
          $('#editModalDropdown .menu').append('<div class="item" data-value="more-' + groupName + '">add ' + groupName + '</div>');
        }
        else if (op === "less") {
          $('#editModalDropdown .menu').append('<div class="item" data-value="less-' + groupName + '">remove ' + groupName + '</div>');
        }
      }
    }

  }
}

function resetEditMenu() {
  $('#editModalDropdown .menu').html(g_editMenuDefaults);
  $('#editModalDropdow').dropdown('clear');
}

/*===========================================================================*/
/* Dimensionality Reduction and Visualization                                */
/*===========================================================================*/

function runPCA(samples) {
  dr = new drt.PCA(c, $('#mapViz canvas'));

  if (samples) {
    dr.setSamples(samples);
  }
  else {
    dr.setSamples(g_sampleIndex);
  }
  dr.embed("PCA");
}

function runPCAOnFolder(dir) {
  fs.readdir(dir, function (err, files) {
    if (err) {
      throw err;
    }

    var samples = [];

    for (var f in files) {
      var fname = dir + files[f];
      console.log("Reading " + fname);
      var ctx = c.contextFromDarkroom(fname);
      var img = c.renderContext(ctx);
      var meta = { file: fname };
      samples.push(new Sample(img, ctx, meta));
    }

    runPCA(samples);
  });
}

function mapMouseMove(event) {
  if (dr) {
    dr.mouseMove(event);
  }
}

// moves the current projected point and updates the render
function mapMouseUp(event) {
  if (dr) {
    dr.mouseUp(event);
  }
}

// temporary stuff (at least that's the intent)
var tempModel;
var modelSamples = [];

// only run this on watercolor (this is why it's supposed to be temp)
function createModel() {
  tempModel = new comp.Model(c);
  tempModel.analyze({
    "watercolor": [
      "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/watercolor/wc_weak_1.dark",
      "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/watercolor/wc_weak_2.dark",
      "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/watercolor/wc_moderate_1.dark",
      "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/watercolor/wc_moderate_2.dark",
      "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/watercolor/wc_strong_1.dark",
      "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/watercolor/wc_strong_2.dark",
    ],
    "scribbles": [
      "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/scribbles/scribbles_weak_1.dark",
      "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/scribbles/scribbles_weak_2.dark",
      "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/scribbles/scribbles_moderate_1.dark",
      "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/scribbles/scribbles_moderate_2.dark",
      "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/scribbles/scribbles_strong_1.dark",
      "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/scribbles/scribbles_strong_2.dark",
    ]
  });
  //tempModel.addSlider("FG Watercolor", [
  //  { "layerName": "PC_1", "param": "opacity", "adjustmentType": 1000, "type": "dynamicSine", "f": 10, "A0": 0, "A1": 0.2, "D0": 0, "D1": 0.8, "phase": 0 },
  //  { "layerName": "Reveal Normal Photo (Brush Mask)", "param": "opacity", "adjustmentType": 1000, "type": "lerp", "xs": [0, 0.3], "ys": [1, 0] },
  //  { "layerName": "PC_2", "param": "opacity", "adjustmentType": 1000, "type": "dynamicSine", "f": 10, "A0": 0, "A1": 0.2, "D0": 0, "D1": 0.8, "phase": 2.5 },
  //  { "layerName": " Splatter Texture", "param": "opactiy", "adjustmentType": 1000, "type": "dynamicSine", "f": 10, "A0": 0, "A1": 0.2, "D0": 0, "D1": 0.8, "phase": 5 },
  //  { "layerName": "Main Photo Visibility", "param": "opacity", "adjustmentType": 1000, "type": "dynamicSine", "f": 7, "A0": 0, "A1": 0.4, "D0": 0, "D1": 0.8, "phase": 7.5 }
    //{ "layerName": "PC_3", "param": "opacity", "adjustmentType": 1000, "type": "dynamicSine", "f": 20, "A0": 0.5, "A1": 0.5, "D0": 0.5, "D1": 0.5, "phase": 0 },
    //{ "layerName": "PC_4", "param": "opacity", "adjustmentType": 1000, "type": "dynamicSine", "f": 20, "A0": 0.5, "A1": 0.5, "D0": 0.5, "D1": 0.5, "phase": 5 },
    //{ "layerName": "PC_5", "param": "opacity", "adjustmentType": 1000, "type": "dynamicSine", "f": 20, "A0": 0.5, "A1": 0.5, "D0": 0.5, "D1": 0.5, "phase": 10 },
    //{ "layerName": "PC_6", "param": "opacity", "adjustmentType": 1000, "type": "dynamicSine", "f": 20, "A0": 0.5, "A1": 0.5, "D0": 0.5, "D1": 0.5, "phase": 15 },
  //]);
  //tempModel.addSlider("BG Watercolor", [
  //  { "layerName": "BG_Texture_1", "param": "opacity", "adjustmentType": 1000, "type": "dynamicSine", "f": 10, "A0": 0.1, "A1": 0.2, "D0": 0, "D1": 0.8, "phase": 0 },
  //  { "layerName": "BG_Texture_2", "param": "opacity", "adjustmentType": 1000, "type": "dynamicSine", "f": 10, "A0": 0.1, "A1": 0.2, "D0": 0, "D1": 0.8, "phase": 1 },
  //  { "layerName": "Watercolor Texture_4", "param": "opacity", "adjustmentType": 1000, "type": "dynamicSine", "f": 10, "A0": 0.1, "A1": 0.2, "D0": 0, "D1": 0.8, "phase": 2 },
  //  { "layerName": " Watercolor Texture_3", "param": "opacity", "adjustmentType": 1000, "type": "dynamicSine", "f": 10, "A0": 0.1, "A1": 0.2, "D0": 0, "D1": 0.8, "phase": 3 },
  //  { "layerName": "Watercolor Texture_2", "param": "opacity", "adjustmentType": 1000, "type": "dynamicSine", "f": 10, "A0": 0.1, "A1": 0.2, "D0": 0, "D1": 0.8, "phase": 4 },
  //  { "layerName": "Watercolor Texture_1", "param": "opacity", "adjustmentType": 1000, "type": "dynamicSine", "f": 10, "A0": 0.1, "A1": 0.2, "D0": 0, "D1": 0.8, "phase": 5 }
  //]);
  //tempModel.addSchema("C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/sf_schema.json");
  tempModel.addSliderFromExamples("FG Watercolor", [
    "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/fg_watercolor/fgwc_weak_2.dark",
    "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/fg_watercolor/fgwc_weak_1.dark",
    "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/fg_watercolor/fgwc_weak_3.dark",
    "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/fg_watercolor/fgwc_moderate_2.dark",
    "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/fg_watercolor/fgwc_moderate_1.dark",
    "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/fg_watercolor/fgwc_strong_2.dark",
    "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/fg_watercolor/fgwc_strong_1.dark"
  ]);
}

function report() {
  fs.writeFileSync("./report.json", JSON.stringify(tempModel.report(), null, 2));
}

function sample() {
  var s = tempModel.sample();
  processNewSample(c.renderContext(s), s, {}, true);
}

function sampleN(n) {
  modelSamples = [];

  for (var i = 0; i < n; i++) {
    modelSamples.push(tempModel.sample());
  }

  for (var i = 0; i < n; i++) {
    let idx = i;
    c.asyncRenderContext(modelSamples[i], settings.sampleRenderSize, function (err, img) {
      processNewSample(img, modelSamples[idx], {}, true);
    });
  }
}

function nonParametricLocalSample(ctx, n, alpha, k) {
  for (var i = 0; i < n; i++) {
    let sample = tempModel.nonParametricLocalSample(ctx, alpha, k);
    c.asyncRenderContext(sample, settings.sampleRenderSize, function (err, img) {
      processNewSample(img, sample, {}, true);
    });
  }
}

function schemaSample(ctx, n, constraints) {
  for (var i = 0; i < n; i++) {
    let sample = tempModel.schemaSample(ctx, constraints);
    c.asyncRenderContext(sample.context, settings.sampleRenderSize, function (err, img) {
      processNewSample(img, sample.context, sample.metadata, true);
    });
  }
}

function sliderSample(ctx, id, n, min, max) {
  // takes n samples between min and max (inclusive)
  var interval = (max - min) / n;

  for (var i = 0; i <= n; i++) {
    let sample = tempModel.sliderSample(ctx, id, interval * i + min);
    c.asyncRenderContext(sample.context, settings.sampleRenderSize, function (err, img) {
      processNewSample(img, sample.context, sample.metadata, true);
    });
  }
}

function createSliderGUI(name) {
  var html = '<div class="ui item">';
  html += '<div class="content">';
  html += '<div class="header">' + name + '</div>';
  html += '<div class="parameter" sliderName="' + name + '">';
  html += '<div class="paramLabel">' + name + '</div>';
  html += '<div class="paramSlider" sliderName="' + name + '"></div>';
  html += '<div class="paramInput ui inverted transparent input" sliderName="' + name + '"><input type="text"></div>';
  html += '</div></div></div>';

  $('#sliderItems').append(html);

  var s = '.paramSlider[sliderName="' + name + '"]';
  var i = '.paramInput[sliderName="' + name + '"] input';

  // event bindings
  $(s).slider({
    orientation: "horizontal",
    range: "min",
    max: 1,
    min: 0,
    step: 0.001,
    value: 0,
    //stop: function (event, ui) { highLevelSliderChange(name, ui); },
    //slide: function (event, ui) { highLevelSliderChange(name, ui); },
    change: function (event, ui) { highLevelSliderChange(name, ui); }
  });

  $(i).val("0");

  // input box events
  $(i).blur(function () {
    var data = parseFloat($(this).val());
    $(s).slider("value", data);
  });
  $(i).keydown(function (event) {
    if (event.which != 13)
      return;

    var data = parseFloat($(this).val());
    $(s).slider("value", data);
  });
}

function highLevelSliderChange(sliderName, ui) {
  var sample = tempModel.sliderSample(c.getContext(), sliderName, ui.value);

  // update slider controls n stuff
  c.setContext(sample.context);
  updateLayerControls();

  var i = '.paramInput[sliderName="' + sliderName + '"] input';
  $(i).val(String(ui.value.toFixed(3)));
}

/*===========================================================================*/
/* Custom UI Components                                                      */
/*===========================================================================*/

var g_uiComponents = {};

function addSlider(layer, param, type) {
  var slider = new uiTools.Slider({ 'name': layer, 'param': param, 'type': type });
  slider.setVal({ 'context': c.getContext() });
  slider.createUI($('#sliderItems'));

  g_uiComponents[slider.displayName] = slider;
}

function addMetaSlider(name, params) {
  var ms = new uiTools.MetaSlider({ 'name': name });

  for (var p in params) {
    var param = params[p];

    // defaults to [0,1], will leave like this unless otherwise needed
    ms.addSlider(param.layer, param.param, param.type, [0, 1], [0, 1]);
  }

  ms.createUI($('#sliderItems'));
  g_uiComponents[ms.displayName] = ms;
}

// instead of a sampler, creates a metaslider that adds in the layers in random order
// instead of sampling it
function addOrderedSlider(name, params, link) {
  var ds = new uiTools.OrderedSlider({ 'name': name });

  // determine interval
  var interval = 1 / params.length;

  for (var i = 0; i < params.length; i++) {
    var x = [0, interval * i, interval * (i + 1), 1];
    var y = [0, 0, 1, 1];

    ds.addSlider(params[i].layer, params[i].param, params[i].type, x, y);
  }

  if (link) {
    ds.linkedMetaSlider = g_uiComponents[link];
  }

  ds.createUI($('#sliderItems'));
  g_uiComponents[ds.displayName] = ds;
}

function autoDeterministicSampler(name, ctx) {
  // takes a context, sorts the layers, adds a deterministic sampler
  var keys = ctx.keys();
  var dat = [];

  for (var i in keys) {
    var id = keys[i];

    var layer = ctx.getLayer(id);
    if (!layer.isAdjustmentLayer()) {
      var stats = layer.image().stats();
      stats.id = id;
      dat.push(stats);
    }
  }

  dat.sort(function (a, b) {
    return -(a.totalAlpha - b.totalAlpha);
  });

  var params = [];
  for (var i in dat) {
    var d = dat[i];

    if (d.id !== "brush") {
      params.push({ layer: d.id, param: "opacity", type: adjType["OPACITY"] });
    }
  }

  addOrderedSlider(name, params, null);
}

function importanceSlider(name, size) {
  var li = c.localImportance(c.getContext(), size); 

  addOrderedSlider(name, [], null);
  g_uiComponents[name].importanceData = li;
}

function addSampler(name, params, link) {
  var sampler = new uiTools.Sampler({ 'name': name });

  for (var p in params) {
    var param = params[p];

    sampler.addParam(param.layer, param.param, param.type);
  }

  if (link) {
    sampler.linkedMetaSlider = g_uiComponents[link];
  }

  sampler.createUI($('#sliderItems'));
  g_uiComponents[sampler.displayName] = sampler;
}

function addColorPicker(layer, type) {
  var color = new uiTools.ColorPicker(layer, type);
  color.createUI($('#sliderItems'));
  g_uiComponents[color.displayName] = color;
}

function deleteSlider(name) {
  g_uiComponents[name].deleteUI();
  delete g_uiComponents[name];
}

function deleteAllUIComponents() {
  $('#sliderItems').empty();
  g_uiComponents = {};
}

// add individual sliders for each param in the image
function addAllParamSliders() {
  // go through the current context and extract all the params
  var ctx = c.getContext();
  var layerNames = ctx.keys();

  for (var name in layerNames) {
    var layer = c.getLayer(layerNames[name]);

    addSlider(layerNames[name], "opacity", adjType["OPACITY"]);

    // adjustments
    var adjustments = layer.getAdjustments();

    for (var i = 0; i < adjustments.length; i++) {
      var data = layer.getAdjustment(adjustments[i]);

      for (var paramName in data) {
        addSlider(layerNames[name], paramName, adjustments[i]);
      }
    }
  }
}

function saveCustomUI() {
  dialog.showSaveDialog({
    filters: [{ name: 'Darkroom UI File', extensions: ['drui'] }],
    title: "Save UI Config"
  }, function (filePaths) {
    if (filePaths === undefined) {
      return;
    }

    var file = filePaths;

    exportCustomUI(file);
  })
}

// exports the current custom UI to a file
function exportCustomUI(file) {
  fs.writeFile(file, JSON.stringify(g_uiComponents, null, 2), (err) => {
    if (err) {
      showStatusMsg(err.toString(), "ERROR", "Error Saving UI File");
    }
    else {
      showStatusMsg(file, "OK", "UI File Saved");
    }
  });
}

function openCustomUI() {
  dialog.showOpenDialog({
    filters: [{ name: 'Darkroom UI File', extensions: ['drui'] }],
    title: "Load UI Config"
  }, function (filePaths) {
    if (filePaths === undefined) {
      return;
    }

    var file = filePaths[0];
    loadCustomUI(file);
  });
}

function loadCustomUI(file) {
  fs.readFile(file, function (err, data) {
    if (err) {
      throw err;
    }

    var data = JSON.parse(data);

    // convert into actual objects
    for (var id in data) {
      var elem = data[id];

      if (elem.UIType === "Slider") {
        var slider = new uiTools.Slider({ json: elem });
        g_uiComponents[slider.displayName] = slider;
      }
      else if (elem.UIType === "MetaSlider") {
        var slider = new uiTools.MetaSlider({ json: elem });
        g_uiComponents[slider.displayName] = slider;
      }
      else if (elem.UIType === "Sampler") {
        var sampler = new uiTools.Sampler({ json: elem });
        g_uiComponents[sampler.displayName] = sampler;
      }
      else if (elem.UIType === "ColorPicker") {
        var color = new uiTools.ColorPicker(elem.layer, elem.type);
        g_uiComponents[color.displayName] = color;
      }
      else if (elem.UIType === "OrderedSlider") {
        var sampler = new uiTools.OrderedSlider({ json: elem, importanceData: elem.importanceData });
        g_uiComponents[sampler.displayName] = sampler;
      }
    }

    // link samplers to metasliders (if applicable)
    for (var id in g_uiComponents) {
      if (g_uiComponents[id].UIType === "Sampler" || g_uiComponents[id].UIType === "DeterministicSampler") {
        if (data[id].linkedMetaSlider) {
          g_uiComponents[id].linkedMetaSlider = g_uiComponents[data[id].linkedMetaSlider];
        }
      }
    }

    // generate ui
    for (var id in g_uiComponents) {
      g_uiComponents[id].createUI($('#sliderItems'));
    }
  });
}

function addSliderSelector(name, size) {
  var li = c.localImportance(c.getContext(), size);

  var slider = new uiTools.SliderSelector(name, li);
  slider.setOrder("depth", true);
  slider.createUI($('#sliderItems'));

  g_uiComponents[name] = slider;
}

/* Flat groups */

// takes a document in a tree like the modifiers variable, flattens it, and stores results
// in g_flatGroups and g_groupsByLayer
function flattenDoc(doc, path) {
  for (let key in doc) {
    // drop opacity value in tree nodes
    if (key === "opacity")
      continue;

    let node = doc[key];
    // leaf nodes have no object values
    let isLeaf = true;
    for (let k in node) {
      if (typeof node[k] === 'object') {
        isLeaf = false;
        break;
      }
    }

    if (isLeaf) {
      // add layer to flat groups based on the current path
      for (let g in path) {
        let group = path[g]
        if (!(group in g_flatGroups))
          g_flatGroups[group] = { maxDepth: -(path.length), layers: [] };

        // if the doc tree has layers that were converted into adjustments, skip
        g_flatGroups[group].layers.push(key);
        g_flatGroups[group].maxDepth = Math.max(g_flatGroups[group].maxDepth, -(path.length));
      }

      g_groupsByLayer[key] = path.slice();
    }
    else {
      var newPath = path.slice();
      newPath.push(key);
      flattenDoc(node, newPath);
    }
  }
}

// takes the old import doc tree / old files and converts to new
// flat grouping mode
function setupFlatGroups() {
  g_flatGroups = {};
  g_groupsByLayer = {};

  flattenDoc(docTree, []);

  // setup UI and back end structures
  for (let g in g_flatGroups) {
    c.addGroup(g, g_flatGroups[g].layers, g_flatGroups[g].maxDepth, true);
    addGroupUI(g);
  }
}

function addGroupUI(name) {
  g_groupPanel.addGroupToList(name);
}

function updateGroupUI() {
  // kind of a load, adds the groups to the ui
  let order = c.getGroupOrder();
  for (let o in order) {
    addGroupUI(order[o].group);
  }
}

function removeMetaGroup(name) {
  // tell the compositor to remove the group, update the ui
  c.deleteGroup(name);
  g_groupPanel.updateGroupDropdown();

  //g_metaGroupList[name].deleteUI();
  delete g_metaGroupList[name];
}

// auto grouping functions
// returns a list of layers with score 
function getSimilarColorTo(layerName) {
  // render thumbnail views of all current layers
  let vals = [];
  let allLayers = c.getFlatLayerOrder();
  let current = c.getContext();
  let cmpImg = c.renderOnlyLayer(current, layerName, 'micro');

  for (let layer of allLayers) {
    let y = c.renderOnlyLayer(current, layer, 'micro');

    let dist = cmpImg.chamferDistance(y);
    vals.push({ layer: layer, dist: dist });
  }

  // sort
  vals.sort(function(a, b) {
    return a.dist - b.dist;
  });

  return vals;
}
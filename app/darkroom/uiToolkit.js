// note: not sure if comp is actually required here since it should be in scope from
// earlier included scripts
// speaking of "scope" in javascript, blindly calling functions that affect the global
// ui state and the global compositor object (c) is totally ok here
const comp = require('../native/build/Release/compositor');

class Slider {
  constructor(data) {
    if ('slider' in data) {
      this.slider = data.slider;
    }
    else if ('json' in data) {
      this.slider = new comp.Slider(data.json);
    }
    else {
      this.slider = new comp.Slider(data.name, data.param, data.type);
    }
  }

  get displayName() {
    return this.slider.displayName();
  }

  set displayName(name) {
    this.slider.displayName(name);
  }

  get layer() {
    return this.slider.layer();
  }

  get param() {
    return this.slider.param();
  }

  get type() {
    return this.slider.type();
  }

  get value() {
    return this.slider.getVal();
  }

  get UIType() {
    return "Slider";
  }

  get order() {
    return this._order;
  }

  set order(value) {
    this._order = value;
  }

  setVal(dat) {
    if ("val" in dat) {
      var newContext = this.slider.setVal(dat.val, dat.context);
      c.setContext(newContext);
      updateLayerControls();
    }
    else {
      this.slider.setVal(dat.context);
    }
  }

  toJSON() {
    return this.slider.toJSON();
  }

  refreshUI() {
    // updates the ui elements without triggering a re-render
    var i = '.paramInput[sliderName="' + this.displayName + '"] input';
    var s = '.paramSlider[sliderName="' + this.displayName + '"]'; 
    $(i).val(String(this.value.toFixed(3)));
    $(s).slider("value", this.value);
  }

  // constructs the ui element, inserts it into the container, and binds the proper events
  createUI(container) {
    var html = '<div class="ui item" sliderName="' + this.displayName + '">';
    html += '<div class="content">';
    //html += '<div class="header">' + this.displayName + '</div>';
    html += '<div class="parameter" sliderName="' + this.displayName + '">';
    html += '<div class="paramLabel">' + this.displayName+ '</div>';
    html += '<div class="paramSlider" sliderName="' + this.displayName + '"></div>';
    html += '<div class="paramInput ui inverted transparent input" sliderName="' + this.displayName + '"><input type="text"></div>';

    if (this.order !== undefined) {
      html += '<div class="sliderOrderLabel">' + this.order.toPrecision(4) + '</div>';
    }

    html += '</div></div></div>';

    container.append(html);

    // event bindings
    var s = '.paramSlider[sliderName="' + this.displayName + '"]';
    var i = '.paramInput[sliderName="' + this.displayName + '"] input';

    let self = this;

    // event bindings
    $(s).slider({
      orientation: "horizontal",
      range: "min",
      max: 1,
      min: 0,
      step: 0.001,
      value: this.value,
      stop: function (event, ui) { self.sliderCallback(ui); },
      slide: function (event, ui) { $(i).val(String(ui.value.toFixed(3))); },
      //change: function (event, ui) { self.sliderCallback(ui); }
    });

    $(i).val(String(this.value.toFixed(3)));

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

  sliderCallback(ui) {
    var i = '.paramInput[sliderName="' + this.displayName + '"] input';
    $(i).val(String(ui.value.toFixed(3)));

    this.setVal({ val: ui.value, context: c.getContext() });
  }

  deleteUI() {
    $('.item[sliderName="' + this.displayName + '"]').remove();
  }
}

class MetaSlider {
  constructor(obj) {
    if ('json' in obj) {
      this.mainSlider = new comp.MetaSlider(obj.json)
      this.subSliders = {};

      for (var id in this.keys) {
        this.subSliders[this.keys[id]] = new Slider({ slider: this.mainSlider.getSlider(this.keys[id]) });
      }
    }
    else {
      this.mainSlider = new comp.MetaSlider(obj.name);
      this.subSliders = {};
    }
  }

  get displayName() {
    return this.mainSlider.displayName();
  }

  get size() {
    return this.mainSlider.size();
  }

  get keys() {
    return this.mainSlider.names();
  }

  get value() {
    return this.mainSlider.getVal();
  }

  get UIType() {
    return "MetaSlider";
  }

  setContext(val, context) {
    var newCtx = this.mainSlider.setContext(val, context);

    for (var id in this.subSliders) {
      this.subSliders[id].refreshUI();
    }

    c.setContext(newCtx);
    updateLayerControls();
  }

  addSlider(name, paramName, type, xs, ys) {
    var id = this.mainSlider.addSlider(name, paramName, type, xs, ys);

    // ui handling nonsense now
    var slider = this.mainSlider.getSlider(id);
    this.subSliders[id] = new Slider({ 'slider': slider });

    return id;
  }

  deleteSlider(id) {
    this.mainSlider.deleteSlider(id);

    delete this.subSliders[id];
  }

  deleteAllSliders() {
    var keys = this.keys;
    for (var id in keys) {
      this.deleteSlider(keys[id]);
    }

    $('.item[sliderName="' + this.displayName + '"] .list').empty();
  }

  toJSON() {
    return this.mainSlider.toJSON();
  }

  // constructs the ui element, inserts it into the container, and binds the proper events
  // also creates the subslider elements (eventually)
  createUI(container) {
    var html = '<div class="ui item" sliderName="' + this.displayName + '">';
    html += '<div class="content">';
    html += '<div class="header">' + this.displayName + '</div>';
    html += '<div class="parameter" sliderName="' + this.displayName + '">';
    html += '<div class="paramLabel">' + this.displayName + '</div>';
    html += '<div class="paramSlider" sliderName="' + this.displayName + '"></div>';
    html += '<div class="paramInput ui inverted transparent input" sliderName="' + this.displayName + '"><input type="text"></div>';
    html += '</div>';

    // we need a sub list here
    html += '<div class="ui horizontal fitted inverted divider">Component Sliders</div><div class="ui list"></div>';

    html += '</div></div>';

    container.append(html);

    var sectionID = '.item[sliderName="' + this.displayName + '"] .ui.list';
    $(sectionID).transition('hide');

    $('.item[sliderName="' + this.displayName + '"] .divider').click(function () {
      $(sectionID).transition('fade down');
    });

    // event bindings
    var s = '.paramSlider[sliderName="' + this.displayName + '"]';
    var i = '.paramInput[sliderName="' + this.displayName + '"] input';

    let self = this;

    // event bindings
    $(s).slider({
      orientation: "horizontal",
      range: "min",
      max: 1,
      min: 0,
      step: 0.001,
      value: this.value,
      //stop: function (event, ui) { highLevelSliderChange(name, ui); },
      slide: function (event, ui) {
        $(i).val(String(ui.value.toFixed(3)));
        for (var id in this.subSliders) {
          this.subSliders[id].refreshUI();
        }
      },
      change: function (event, ui) { self.sliderCallback(ui); }
    });

    $(i).val(String(this.value.toFixed(3)));

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

    // add the subslider elements
    for (var id in this.subSliders) {
      this.subSliders[id].createUI($('.item[sliderName="' + this.displayName + '"] .list'));
    }
  }

  sliderCallback(ui) {
    var i = '.paramInput[sliderName="' + this.displayName + '"] input';
    $(i).val(String(ui.value.toFixed(3)))

    this.setContext(ui.value, c.getContext());
  }

  deleteUI() {
    $('.item[sliderName="' + this.displayName + '"]').remove();
  }

  updateMax(ctx) {
    this.mainSlider.reassignMax(ctx);

    // updating the max may have chagned the context, so do that here too
    var newCtx = this.mainSlider.setContext(this.value, ctx);

    for (var id in this.subSliders) {
      this.subSliders[id].refreshUI();
    }

    c.setContext(newCtx);
  }
}

class Sampler {
  constructor(obj) {
    if ('name' in obj) {
      this.sampler = new comp.Sampler(obj.name);
      this.linkedMetaSlider = null;
    }
    else if ('json' in obj) {
      this.sampler = new comp.Sampler(obj.json);
    }
  }

  get keys() {
    return this.sampler.params();
  }

  get displayName() {
    return this.sampler.displayName();
  }

  set linkedMetaSlider(uiElem) {
    this._linkedMetaSlider = uiElem;
  }

  get linkedMetaSlider() {
    return this._linkedMetaSlider;
  }

  get UIType() {
    return "Sampler";
  }

  sample(x) {
    var newCtx = this.sampler.sample(x, c.getContext());
    c.setContext(newCtx);

    // this can change the value of the context, so recompute score after this step
    if (this.linkedMetaSlider) {
      this.linkedMetaSlider.updateMax(newCtx);
    }

    $('.item[controlName="' + this.displayName + '"] .score').text(this.eval().toFixed(3));

    updateLayerControls();
  }

  eval() {
    return this.sampler.eval(c.getContext());
  }

  addParam(layer, param, type) {
    this.sampler.addParam(layer, param, type);
  }

  toJSON() {
    var core = this.sampler.toJSON();

    if (this.linkedMetaSlider) {
      core.linkedMetaSlider = this.linkedMetaSlider.displayName;
    }
    else {
      core.linkedMetaSlider = null;
    }

    return core;
  }

  createUI(container) {
    var html = '<div class="ui item" controlName="' + this.displayName + '">';
    html += '<div class="content">';
    html += '<div class="header">' + this.displayName + '</div>';
    html += '<div class="score">' + this.eval().toFixed(3) + '</div>';
    html += '<div class="four small ui buttons">';
    html += '<button class="ui button" buttonVal="low">A Little</button>';
    html += '<button class="ui button" buttonVal="medium">Some</button>';
    html += '<button class="ui button" buttonVal="high">A Lot</button>';
    html += '<button class="ui active button" buttonVal="all">All</button>';
    html += '</div></div></div>';

    container.append(html);
    var cname = this.displayName;
    var self = this;

    // event bindings
    $('.item[controlName="' + this.displayName + '"] .button').click(function () {
      var type = $(this).attr('buttonVal');

      // eventually these shortcuts will probably be replaced by actual params
      if (type === 'low') {
        self.sample(0.25);
      }
      else if (type === 'medium') {
        self.sample(0.5);
      }
      else if (type === 'high') {
        self.sample(0.75);
      }
      else if (type === 'all') {
        self.sample(1);
      }

      $('.item[controlName="' + cname + '"] .button').removeClass('active');
      $(this).addClass('active');
    });
  }

  deleteUI() {
    $('.item[controlName="' + this.displayName + '"]').remove();
  }
}

class OrderedSlider extends MetaSlider {
  constructor(obj) {
    super(obj);

    this._importanceData = obj.importanceData;
    this._currentSortMode = "";
  }

  toJSON() {
    var obj = this.mainSlider.toJSON();
    obj.UIType = "OrderedSlider";

    obj.importanceData = this._importanceData;

    return obj;
  }

  setContext(val, scale, context) {
    var newCtx = this.mainSlider.setContext(val, scale, context);

    for (var id in this.subSliders) {
      this.subSliders[id].refreshUI();
    }

    c.setContext(newCtx);
    updateLayerControls();
  }

  get UIType() {
    return "DeterministicSampler";
  }

  set importanceData(data) {
    this._importanceData = data;
  }

  // constructs the ui element, inserts it into the container, and binds the proper events
  // also creates the subslider elements (eventually)
  createUI(container) {
    var html = '<div class="ui item" sliderName="' + this.displayName + '">';
    html += '<div class="content">';
    html += '<div class="header">' + this.displayName + '</div>';

    // dropdown menu location
    html += '<div class="ui selection dropdown sortModes"></div>';

    html += '<div class="parameter" sliderName="' + this.displayName + '">';
    html += '<div class="paramLabel">' + this.displayName + ' Amount</div>';
    html += '<div class="paramSlider" sliderName="' + this.displayName + '"></div>';
    html += '<div class="paramInput ui inverted transparent input" sliderName="' + this.displayName + '"><input type="text"></div>';
    html += '</div>';

    html += '<div class="parameter" sliderName="' + this.displayName + '-str">';
    html += '<div class="paramLabel">' + this.displayName + ' Strength</div>';
    html += '<div class="paramSlider" sliderName="' + this.displayName + '-str"></div>';
    html += '<div class="paramInput ui inverted transparent input" sliderName="' + this.displayName + '-str"><input type="text"></div>';
    html += '</div>';

    // we need a sub list here
    html += '<div class="ui horizontal fitted inverted divider">Component Sliders</div><div class="ui list"></div>';

    html += '</div></div>';

    container.append(html);

    this.populateDropdown();

    var sectionID = '.item[sliderName="' + this.displayName + '"] .ui.list';
    $(sectionID).transition('hide');

    $('.item[sliderName="' + this.displayName + '"] .divider').click(function () {
      $(sectionID).transition('fade down');
    });

    // event bindings
    var s = '.paramSlider[sliderName="' + this.displayName + '"]';
    var i = '.paramInput[sliderName="' + this.displayName + '"] input';

    let self = this;

    // event bindings
    $(s).slider({
      orientation: "horizontal",
      range: "min",
      max: 1,
      min: 0,
      step: 0.001,
      value: this.value,
      //stop: function (event, ui) { highLevelSliderChange(name, ui); },
      slide: function (event, ui) {
        $(i).val(String(ui.value.toFixed(3)));
        for (var id in this.subSliders) {
          this.subSliders[id].refreshUI();
        }
      },
      change: function (event, ui) { self.sliderCallback(ui); }
    });

    $(i).val(String(this.value.toFixed(3)));

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

    var s2 = '.paramSlider[sliderName="' + this.displayName + '-str"]';
    var i2 = '.paramInput[sliderName="' + this.displayName + '-str"] input';

    // event bindings
    $(s2).slider({
      orientation: "horizontal",
      range: "min",
      max: 1,
      min: 0,
      step: 0.001,
      value: this.value,
      //stop: function (event, ui) { highLevelSliderChange(name, ui); },
      slide: function (event, ui) {
        $(i2).val(String(ui.value.toFixed(3)));
        for (var id in this.subSliders) {
          this.subSliders[id].refreshUI();
        }
      },
      change: function (event, ui) { self.sliderCallback(ui); }
    });

    $(i2).val(String(1));
    $(s2).slider("value", 1);

    // input box events
    $(i2).blur(function () {
      var data = parseFloat($(this).val());
      $(s2).slider("value", data);
    });

    $(i2).keydown(function (event) {
      if (event.which != 13)
        return;

      var data = parseFloat($(this).val());
      $(s2).slider("value", data);
    });

    // add the subslider elements
    this.createSubSliderUI();
  }

  createSubSliderUI() {
    for (var id in this.subSliders) {
      this.subSliders[id].createUI($('.item[sliderName="' + this.displayName + '"] .list'));
    }
  }

  populateDropdown() {
    var menu = $('.item[sliderName="' + this.displayName + '"] .sortModes');

    // remove existing options
    menu.empty();
    var html = '<i class="dropdown icon"></i>';
    html += '<div class="default text">Ordering</div>';
    html += '<div class="menu">';

    // explicitly specify sort modes, the order data has a lot of extra stuff
    html += '<div class="item" data-value="depth">Depth</div>';
    html += '<div class="item" data-value="deltaMag">Gradient Magnitude</div>';
    html += '<div class="item" data-value="mssim">MSSIM</div>';
    html += '<div class="item" data-value="totalAlpha">Average Alpha</div>';

    html += '</div>';
    menu.append(html);

    var self = this;

    // dropdown events
    menu.dropdown({
      action: 'activate',
      onChange: function (value, text, selectedItem) {
        self.sort(value);
      }
    });
    menu.dropdown('set selected', this._currentSortMode);
  }

  sort(key) {
    this.deleteAllSliders();

    // add sliders back in the proper order
    this._currentSortMode = key;

    // default order is decending but depth is like ascending so
    if (key === "depth" || key === "mssim") {
      this._importanceData.sort(function (a, b) {
        return (a[key] - b[key]);
      });
    }
    else {
      this._importanceData.sort(function (a, b) {
        return -(a[key] - b[key]);
      });
    }

    // re-assign sliders
    var interval = 1 / this._importanceData.length;

    for (var i = 0; i < this._importanceData.length; i++) {
      var x = [0, interval * i, interval * (i + 1), 1];
      var y = [0, 0, 1, 1];

      var param = this._importanceData[i];
      var id = this.addSlider(param.layerName, param.param, param.adjType, x, y);
      this.subSliders[id].order = param[key];
    }

    // re-create sliders
    this.createSubSliderUI();

    // turns out we don't actually need a ui element to trigger the callback
    this.sliderCallback(null);
  }

  sliderCallback(ui) {
    var i = '.paramInput[sliderName="' + this.displayName + '"] input';
    var i2 = '.paramInput[sliderName="' + this.displayName + '-str"] input';
    var s = '.paramSlider[sliderName="' + this.displayName + '"]';
    var s2 = '.paramSlider[sliderName="' + this.displayName + '-str"]';

    $(i).val(String($(s).slider("value").toFixed(3)));
    $(i2).val(String($(s2).slider("value").toFixed(3)));

    this.setContext($(s).slider("value"), $(s2).slider("value"), c.getContext());

    if (this.linkedMetaSlider) {
      this.linkedMetaSlider.updateMax(c.getContext());
    }
  }
}

class ColorPicker {
  constructor(layer, type) {
    this._layer = layer;
    this._type = type;
  }

  get displayName() {
    return this.layer + this.type;
  }

  get layer() {
    return this._layer;
  }

  get type() {
    return this._type;
  }

  toJSON() {
    return { type: this._type, layer: this._layer, UIType: "ColorPicker" };
  }

  createUI(container) {
    var html = '<div class="ui item" controlName="' + this.displayName + '">';
    html += '<div class="header">' + this.displayName + '</div>';
    html += '<div class="paramColor" paramName="colorPicker"></div>';
    html += '</div>';

    container.append(html);

    // callbacks and init
    var selector = '.item[controlName="' + this.displayName + '"] .paramColor';
    var self = this;

    $(selector).click(function () {
      if ($('#colorPicker').hasClass('hidden')) {
        // move color picker to spot
        var thisElem = $(selector);
        var offset = thisElem.offset();

        var adj = c.getLayer(self.layer).getAdjustment(self.type);
        cp.setColor({ "r": adj.r * 255, "g": adj.g * 255, "b": adj.b * 255 }, 'rgb');
        cp.startRender();

        $("#colorPicker").css({ 'right': '', 'top': '' });
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
            updateColor(c.getLayer(self.layer), self.type, color);
            $(thisElem).css({ "background-color": "#" + cp.color.colors.HEX });

            updateLayerControls();
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

    var adj = c.getLayer(this.layer).getAdjustment(this.type);
    var colorStr = "rgb(" + parseInt(adj.r * 255) + "," + parseInt(adj.g * 255) + "," + parseInt(adj.b * 255) + ")";
    $(selector).css({ "background-color": colorStr });
  }
}

class SliderSelector {
  constructor(name, orderData) {
    this._orderData = orderData;
    this._name = name;
    this._selectedIndex = 0;
    this._vizMode = "soloLayer";
  }

  get orderData() {
    return this._orderData;
  }

  set orderData(data) {
    this._orderData = orderData;
  }

  get name() {
    return this._name;
  }

  set name(val) {
    this._name = val;

    // update UI here as well
    this._uiElem.find('.header').text(this._name);
    this._uiElem.attr('componentName', this._name);
  }

  get vizMode() {
    return this._vizMode;
  }

  set vizMode(val) {
    this._vizMode = val;
  }

  // sorts and then updates the slider's order
  setOrder(key, ascending) {
    // add sliders back in the proper order
    this._currentSortMode = key;

    // ascending or descending order
    if (ascending) {
      this._orderData.sort(function (a, b) {
        return (a[key] - b[key]);
      });
    }
    else {
      this._orderData.sort(function (a, b) {
        return -(a[key] - b[key]);
      });
    }
  }

  createUI(container) {
    var html = '<div class="ui item layerSelector" componentName="' + this.name + '">';
    html += '<div class="content">';
    html += '<div class="header">' + this.name + '</div>';
    html += '<div class="selectedLabel">Selected Layer: <span></span></div>';
    html += '<div class="layerSelectorSlider"></div>';
    html += '</div></div>';

    container.append(html);

    // cache the element in the object
    this._uiElem = $('.layerSelector[componentName="' + this.name + '"]');

    var sliderElem = this._uiElem.find('.layerSelectorSlider');
    let self = this;

    // event bindings
    sliderElem.slider({
      orientation: "horizontal",
      max: self._orderData.length - 1,
      min: 0,
      step: 1,
      value: self._selectedIndex,
      start: function (event, ui) { self.startViz(); },
      stop: function (event, ui) { self.stopViz(); },
      slide: function (event, ui) {
        self.setSelectedLayer(ui.value);
      }
    });
  }

  deleteUI() {
    this._uiElem.remove();
    this._uiElem = null;
  }

  setSelectedLayer(index) {
    this._selectedIndex = Math.trunc(index);

    var layerData = this._orderData[this._selectedIndex];
    this._uiElem.find('.selectedLabel span').text(layerData.layerName);

    // update visualization
    this.updateViz();
  }

  get selectedLayer() {
    return this._orderData[this._selectedIndex];
  }

  stopViz() {
    $('#mainView').removeClass('half').addClass('full');
    $('#sideView').removeClass('half').addClass('hidden');

    // update the selected layer controls, right now just handles
    // single layer selection
    if (this._layerControls) {
      this._layerControls.deleteUI();
    }
    this._layerControls = new LayerControls(this.selectedLayer.layerName);
    this._layerControls.createUI($('#layerEditControls'));
  }

  startViz() {
    // might be some mode-dependent stuff in here at some point
    $('#mainView').removeClass('full').addClass('half');
    $('#sideView').removeClass('hidden').addClass('half');
  }

  updateViz() {
    var currentLayer = this._orderData[this._selectedIndex];
    var name = currentLayer.layerName;

    if (this._vizMode === "soloLayer") {
      // get layer image
      var layer = c.getLayer(name);

      if (!layer.isAdjustmentLayer()) {
        drawImage(layer.image(), $('#diffVizCanvas'));
      }
      else {
        clearCanvas($('#diffVizCanvas'));
      }
    }
  }
}

// this class creates a layer control widget, including sliders for every individual
// control. This is a partial re-write of the spaghetti in the main darkroom.js 
// and does currently duplicate a lot of code
class LayerControls {
  constructor(layerName) {
    this._name = layerName;
    this._layer = c.getLayer(this._name);
  }

  get layer() {
    return this._layer;
  }

  get name() {
    return this._name;
  }

  deleteUI() {
    if (this._uiElem) {
      this._uiElem.remove();
    }
  }

  // yeah so this basically copies over a lot of the stuff in "insertLayerElem" from
  // the main darkroom.js file. Eventually it'd be nice if that code could be class-based instead
  // but that's not really a priority at the moment.
  createUI(container) {
    var html = '<div class="layer" layerName="' + this._name + '">';
    html += '<h3 class="ui grey inverted header">' + this._name + '</h3>';

    if (this._layer.visible()) {
      html += '<button class="ui icon button mini white visibleButton" layerName="' + this._name + '">';
      html += '<i class="unhide icon"></i>';
    }
    else {
      html += '<button class="ui icon button mini black visibleButton" layerName="' + this._name + '">';
      html += '<i class="hide icon"></i>';
    }

    html += '</button>';

    // i love javascript, where the definition order doesn't matter, and the scope is made up
    html += genBlendModeMenu(this._name);
    html += genAddAdjustmentButton(this._name);
    html += createLayerParam(this._name, "opacity");

    // separate handlers for each adjustment type
    var adjustments = this._layer.getAdjustments();

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

    container.append(html);
    this._uiElem = container.find('.layer[layerName="' + this._name + '"]');

    // bindings
    this.bindEvents();
  }

  // assumes this._uiElem exists and has been inserted into the DOM
  bindEvents() {
    var self = this;
    var visibleButton = this._uiElem.find('button.visibleButton');
    visibleButton.on('click', function () {
      // check status of button
      var visible = self._layer.visible();

      // i think modifiers is in the global scope so it should still be ok here
      // it still sucks there should be an actual object managing this
      self._layer.visible(!visible && modifiers[name].groupVisible);
      modifiers[self._name].visible = !visible;

      var button = $(this);
      if (modifiers[self._name].visible) {
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
      renderImage('layer ' + self._name + ' visibility change');
    });

    // set starting visibility
    if (modifiers[this._name].visible) {
      visibleButton.html('<i class="unhide icon"></i>');
      visibleButton.removeClass("black");
      visibleButton.addClass("white");
    }
    else {
      visibleButton.html('<i class="hide icon"></i>');
      visibleButton.removeClass("white");
      visibleButton.addClass("black");
    }

    // blend mode
    var blendModeMenu = this._uiElem.find('.blendModeMenu');
    blendModeMenu.dropdown({
      action: 'activate',
      onChange: function (value, text) {
        self._layer.blendMode(parseInt(value));
        renderImage('layer ' + self._name + ' blend mode change');
      },
      'set selected': self._layer.blendMode()
    });

    blendModeMenu.dropdown('set selected', this._layer.blendMode());

    // add adjustment menu, currently disabled in this view
    //this._uiElem.find('.addAdjustment').dropdown({
    //  action: 'hide',
    //  onChange: function (value, text) {
        // add the adjustment or something
    //    addAdjustmentToLayer(name, parseInt(value));
    //    regenLayerControls(name);
    //    renderImage('layer ' + name + ' adjustment added');
    //  }
    //});

    this.bindParam("opacity", this._layer.opacity() * 100, "", { });

    // time for the parameter bindings
  }

  bindParam(paramName, initVal, section, config) {
    var s, i;
    var self = this;

    if (section !== "") {
      s = this._uiElem.find('div[sectionName="' + section + '"] .paramSlider[paramName="' + paramName + '"]');
      i = this._uiElem.find('div[sectionName="' + section + '"] .paramInput[paramName="' + paramName + '"] input');
    }
    else {
      s = this._uiElem.find('.paramSlider[paramName="' + paramName + '"]');
      i = this._uiElem.find('.paramInput[paramName="' + paramName + '"] input');
    }

    // defaults
    if (!("range" in config)) {
      config.range = "min";
    }
    if (!("max" in config)) {
      config.max = 100;
    }
    if (!("min" in config)) {
      config.min = 0;
    }
    if (!("step" in config)) {
      config.step = 0.1;
    }

    $(s).slider({
      orientation: "horizontal",
      range: config.range,
      max: config.max,
      min: config.min,
      step: config.step,
      value: initVal,
      stop: function (event, ui) {
        self.paramHandler(event, ui, paramName);
        renderImage('layer ' + self._name + ' parameter ' + paramName + ' change');
      },
      slide: function (event, ui) { self.paramHandler(event, ui, paramName) },
      change: function (event, ui) { self.paramHandler(event, ui, paramName) }
    });

    $(i).val(String(initVal.toFixed(2)));

    // input box events
    $(i).blur(function () {
      var data = parseFloat($(this).val());
      $(s).slider("value", data);
      renderImage('layer ' + self._name + ' parameter ' + self._paramName + ' change');
    });
    $(i).keydown(function (event) {
      if (event.which != 13)
        return;

      var data = parseFloat($(this).val());
      $(s).slider("value", data);
      renderImage('layer ' + self._name + ' parameter ' + self._paramName + ' change');
    });
  }

  paramHandler(event, ui, paramName) {
    if (paramName === "opacity") {
      // update the modifiers and compute acutual value
      modifiers[this._name].opacity = ui.value / 100;
      this._layer.opacity((ui.value / 100) * (modifiers[this._name].groupOpacity / 100));
    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
  }
}

exports.Slider = Slider;
exports.MetaSlider = MetaSlider;
exports.Sampler = Sampler;
exports.OrderedSlider = OrderedSlider;
exports.ColorPicker = ColorPicker;
exports.SliderSelector = SliderSelector;
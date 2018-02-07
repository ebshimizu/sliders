// note: not sure if comp is actually required here since it should be in scope from
// earlier included scripts
// speaking of "scope" in javascript, blindly calling functions that affect the global
// ui state and the global compositor object (c) is totally ok here
const comp = require('../native/build/Release/compositor');

const rankModes = {
  "goal" : -1,
  "alpha": 0,
  "visibilityDelta": 1,
  "specVisibilityDelta": 2
}

const mouseMode = {
  "normal": 0,
  "updateOnHover": 1,
  "contextClick" : 2
}

const clickMapVizMode = {
  "clusters": 0,
  "density": 1,
  "uniqueClusters": 2
}

const PreviewMode = {
  "rawLayer": 1,
  "animatedParams": 2,
  "isolatedComp": 3,
  "diffComp": 4,
  "staticSolidColor" : 5,
  "animatedSolidColor" : 6,
  "invertedColor" : 7,
  "animatedRawLayer" : 8
}

const AnimationMode = {
  "bounce": 1,
  "snap" : 2
}

const GoalType = {
  "none" : 1,
  "targetColor": 2,
  "relativeBrightness": 3,
  "colorize": 5,
  "saturate": 6
}

// nicer formatting for display
const GoalString = {
  "none" : "No Active Goal",
  "targetColor": "Target Color",
  "relativeBrightness": "Relative Brightness",
  "relativeChroma": "Relative Chroma",
  "colorize": "Colorize",
  "saturate": "Saturate"
}

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

    // dropdown menu
    html += this.genVizModeMenu();

    html += '<div class="selectedLabel">Selected Layer: <span></span></div>';
    html += '<div class="layerSelectorSlider"></div>';
    html += '</div></div>';

    // cache the element
    this._uiElem = $(html);
    container.append(this._uiElem);

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

    var menu = this._uiElem.find('.vizModeMenu');
    menu.dropdown({
      action: 'activate',
      onChange: function (value, text, selectedItem) {
        self.vizMode = value;
      }
    });
    menu.dropdown('set selected', this._vizMode);
  }

  genVizModeMenu() {
    var html = '<div class="ui dropdown selection vizModeMenu">';
    html += '<i class="dropdown icon"></i>';
    html += '<div class="default text">Visualization Mode</div>';
    html += '<div class="menu">';

    // explicitly specify sort modes, the order data has a lot of extra stuff
    html += '<div class="item" data-value="soloLayer">Display Original Layer</div>';

    html += '</div></div>';

    return html;
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
        eraseCanvas($('#diffVizCanvas'));
      }
    }
    if (this._vizMode === "diff") {

    }
  }
}

// due to potential layer selection methods using multiple parts of the interface,
// everything should be managed within in this object.
// a layer selector may or may not use a slider selector
// as part of its selection process, and is likely to use a number of 
// unique components
class LayerSelector {
  constructor(opts) {
    // tag computation
    c.analyzeAndTag();

    // expected to be a jquery selector
    this._selectionCanvas = $(opts.selectionCanvas);

    // also a jquery selector
    this._sidebar = $(opts.sidebar);

    // and also a jquery selector
    this._optUI = $(opts.optUI);

    // string or int, not sure yet
    this._selectionMode = opts.selectionMode;

    this._rankMode = opts.rankMode;
    this._rankThreshold = opts.rankThreshold;

    // interaction mode
    this._mouseMode = mouseMode.normal;

    // initial clickmap settings
    // TODO: visible controls for this
    this._clickMapDepth = 4;
    this._clickMapThreshold = 0.05;
    this._clickMapNorm = false;
    this._useClickMap = false;
    this._useTags = false;
    this._useGoals = true;
    this._maxLevel = 3;

    // ui
    // filter menu initialized with dummy vars for now
    this._filterMenu = new FilterMenu(c.uniqueTags());

    // goal menu initialization
    this._goalMenu = new GoalMenu();
    this.initCanvas();
    this.initUI();

    // more ui!
    this._paramSelectPanel = new ParameterSelectPanel("paramPanel", $('#layerSelectPanel'));

    // goal stuff
    this._goal = {};
  }

  get goal() {
    return this._goalMenu.goal;
  }

  initUI() {
    this.deleteUI();

    var self = this;
    this._sidebar.html();

    // append some stuff to the thing
    this._controlArea = $('<div class="ui inverted relaxed divided list"></div>');
    this._sidebar.append(this._controlArea);

    // create the options
    this._optionUIList = $('<div class="ui relaxed inverted list"></div>');
    this._optUI.append(this._optionUIList);

    // importance map visualization options
    // remove existing
    $('#mapDisplayMenu').remove();
    this._mapDisplayMenu = $(`
      <div class="ui floating mini selection dropdown button" id="mapDisplayMenu">
        <span class="text">None</span>
        <div class="menu">
          <div class="header">Importance Map Display</div>
          <div class="item" data-value="none">None</div>
        </div>
      </div>
    `);
    $('#imgStatusBar').append(this._mapDisplayMenu);

    // binding
    $('#mapDisplayMenu').dropdown({
      action: 'activate',
      onChange: function (value, text, $selectedItem) {
        if (value === "none") {
          self.hideImportanceMap();
        }
        else {
          self.showImportanceMap(value);
        }
      }
    });
    $('#mapDisplayMenu').dropdown('set selected', "none");

    this._clickMapDisplayMenu = $(`
      <div class="ui floating mini selection dropdown button" id="clickMapDisplayMenu">
        <span class="text">None</span>
        <div class="menu">
          <div class="header">Click Map Display</div>
          <div class="item" data-value="none">None</div>
          <div class="item" data-value="uniqueClusters">Unique Clusters</div>
          <div class="item" data-value="density">Layer Density</div>
          <div class="item" data-value="clusters">Clusters</div>
        </div>
      </div>
    `);
    $('#imgStatusBar').append(this._clickMapDisplayMenu);
    $('#clickMapDisplayMenu').dropdown({
      action: 'activate',
      onChange: function (value, text, $selectedItem) {
        if (value === "none") {
          // this just erases the canvas so we can reuse it
          self.hideImportanceMap();
        }
        else {
          self.showClickMapViz(value);
        }
      }
    });
    $('#clickMapDisplayMenu').dropdown('set selected', "none");

    // hide one of these
    if (this._useClickMap) {
      $('#mapDisplayMenu').hide();
    }
    else {
      $('#clickMapDisplayMenu').hide();
    }

    // create the things
    // ranking modes
    var modeMenu = $(`
    <div class="item">
      <div class="ui right floated content">
        <div class="ui right pointing dropdown inverted button" id="selectRankMenu">
          <span class="text">[Rank Mode]</span>
          <div class="menu">
            <div class="item" data-value="alpha">Average Alpha</div>
            <div class="item" data-value="visibilityDelta">Visibility Delta</div>
            <div class="item" data-value="specVisibilityDelta">Speculative Visibility Delta</div>
            <div class="item" data-value="goal">Goal-based Selection</div>
          </div>
        </div>
      </div>
      <div class="content">
        <div class="header">Selected Layer Ranking</div>
      </div>
    </div>`);
    this._optionUIList.append(modeMenu);

    $('#selectRankMenu').dropdown({
      action: 'activate',
      onChange: function (value, text) {
        self._rankMode = value;
        $('#selectRankMenu .text').html(text);
      }
    });
    $('#selectRankMenu').dropdown('set selected', this._rankMode);

    // selection Modes, temporarily disabled
    var selectionMode = $(`
    <div class="item">
      <div class="ui right floated content">
        <div class="ui right pointing dropdown inverted button" id="selectModeMenu">
          <span class="text">[Select Mode]</span>
          <div class="menu">
            <div class="item" data-value="localPoint">Point</div>
            <div class="item" data-value="localBox">Box</div>
          </div>
        </div>
      </div>
      <div class="content">
        <div class="header">Selection Mode</div>
      </div>
    </div>`);
    this._optionUIList.append(selectionMode);

    $('#selectModeMenu').dropdown({
      action: 'activate',
      onChange: function (value, text) {
        self._selectionMode = value;
        $('#selectModeMenu .text').html(text);
      }
    });
    $('#selectModeMenu').dropdown('set selected', this._selectionMode);

    // mouse modes
    var mouseModeMenu = $(`
    <div class="item">
      <div class="ui right floated content">
        <div class="ui right pointing dropdown inverted button" id="mouseModeMenu">
          <span class="text">[Mouse Mode]</span>
          <div class="menu">
            <div class="item" data-value="0">Normal</div>
            <div class="item" data-value="1">Update On Hover</div>
            <div class="item" data-value="2">Context Click</div>
          </div>
        </div>
      </div>
      <div class="content">
        <div class="header">Mouse Selection Mode</div>
      </div>
    </div>`);
    this._optionUIList.append(mouseModeMenu);

    $('#mouseModeMenu').dropdown({
      action: 'activate',
      onChange: function (value, text) {
        self._mouseMode = parseInt(value);
        $('#mouseModeMenu .text').html(text);
      }
    });
    $('#mouseModeMenu').dropdown('set selected', this._mouseMode.toString());

    // threshold
    var thresholdSetting = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui input" id="selectThreshold">
                  <input type="number" />
              </div>
          </div>
          <div class="content">
              <div class="header">Display Threshold</div>
              <div class="description">Layers with a rank score above the threshold will be displayed</div>
          </div>
      </div>
    `);
    this._optionUIList.append(thresholdSetting);
    thresholdSetting.find('input').val(this._rankThreshold);

    $('#selectThreshold input').change(function () {
      self._rankThreshold = parseFloat($(this).val());
    });

    // importance maps
    var importanceMap = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui buttons">
                <div class="ui button" id="generateMaps">Generate</div>
                <div class="ui button" id="dumpMaps">Dump</div>
              </div>
          </div>
          <div class="content">
              <div class="header">Importance Maps</div>
              <div class="description">Generates Importance Maps for all layers using current settings</div>
          </div>
      </div>
    `);
    this._optionUIList.append(importanceMap);
    $('#generateMaps').click(function () {
      c.computeAllImportanceMaps(rankModes[self._rankMode], c.getContext());
      showStatusMsg("Generation Complete", "OK", "Importance Maps")
      self.updateMapMenu();
    });
    $('#dumpMaps').click(function () {
      dialog.showOpenDialog({
        properties: ['openDirectory']
      }, function (files) {
        c.dumpImportanceMaps(files[0]);
        showStatusMsg("Export Complete", "OK", "Importance Maps")
      });
    });

    // click maps
    var clickMapButtons = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui buttons">
                <div class="ui button" id="createClickMap">Create</div>
                <div class="ui button" id="computeClickMap">Compute</div>
              </div>
          </div>
          <div class="content">
              <div class="header">Click Map</div>
              <div class="description">Initialization settings for click maps.</div>
          </div>
      </div>
    `);
    this._optionUIList.append(clickMapButtons);
    $('#createClickMap').click(function () {
      self.createClickMap();
    });
    $('#computeClickMap').click(function () {
      self.computeClickMap();
    });

    // use click map
    var cmuse = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui toggle checkbox" id="cmUse">
                  <input type="checkbox" />
              </div>
          </div>
          <div class="content">
              <div class="header">Use Click Map</div>
              <div class="description">Use the click map instead of importance maps for selection.</div>
          </div>
      </div>
    `);
    this._optionUIList.append(cmuse);
    $('#cmUse').checkbox({
      onChecked: function () {
        self._useClickMap = true;
        $('#clickMapDisplayMenu').show();
        $('#mapDisplayMenu').hide();
      },
      onUnchecked: function () {
        self._useClickMap = false;
        $('#clickMapDisplayMenu').hide();
        $('#mapDisplayMenu').show();
      }
    });

    $('#cmUse').checkbox(((this._useClickMap) ? 'check' : 'uncheck'));
    
    // click map depth
    var cmdepth = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui input" id="cmDepth">
                  <input type="number" />
              </div>
          </div>
          <div class="content">
              <div class="header">Click Map Depth</div>
              <div class="description">Target depth for the computed map.</div>
          </div>
      </div>
    `);
    this._optionUIList.append(cmdepth);
    cmdepth.find('input').val(this._clickMapDepth);

    $('#cmDepth input').change(function () {
      self._clickMapDepth = parseInt($(this).val());
    });

    // click map threshold
    var cmthreshold = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui input" id="cmThreshold">
                  <input type="number" />
              </div>
          </div>
          <div class="content">
              <div class="header">Click Map JND Threshold</div>
              <div class="description">When initialized, layers with importance maps above this value will be includede.</div>
          </div>
      </div>
    `);
    this._optionUIList.append(cmthreshold);
    cmthreshold.find('input').val(this._clickMapThreshold);

    $('#cmThreshold input').change(function () {
      self._clickMapThreshold = parseFloat($(this).val());
    });

    // max level 
    var maxlevel = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui input" id="maxLevel">
                  <input type="number" />
              </div>
          </div>
          <div class="content">
              <div class="header">Max Depth</div>
              <div class="description">Maximum depth to use for selection with poisson disk maps</div>
          </div>
      </div>
    `);
    this._optionUIList.append(maxlevel);
    maxlevel.find('input').val(this._maxLevel);

    $('#maxLevel input').change(function () {
      self._maxLevel = parseInt($(this).val());
    });

    // pdisk init
    var pdisks = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui buttons">
                <div class="ui button" id="initPdisks">Init Sample Patterns</div>
              </div>
          </div>
          <div class="content">
              <div class="header">Poisson Disks</div>
              <div class="description">Initialization controls for Poisson Disk sample patterns.</div>
          </div>
      </div>
    `);
    this._optionUIList.append(pdisks);
    $('#initPdisks').click(function () {
      c.initPoissonDisks();
    });

    // use tags
    var taguse = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui toggle checkbox" id="tagUse">
                  <input type="checkbox" />
              </div>
          </div>
          <div class="content">
              <div class="header">Use Tags</div>
              <div class="description">Use tags to determine intent.</div>
          </div>
      </div>
    `);
    this._optionUIList.append(taguse);
    $('#tagUse').checkbox({
      onChecked: function () {
        self._useTags = true;
      },
      onUnchecked: function () {
        self._useTags = false;
      }
    });

    $('#tagUse').checkbox(((this._useTags) ? 'check' : 'uncheck'));
  }

  deleteUI() {
    this._sidebar.html();
    this._optUI.html();
  }

  rankLayers() {
    // check that currentpt is defined
    if (!this._currentPt) {
      this.deleteAllLayers();
      return [];
    }

    var rank;

    if (this._useClickMap) {
      if (this._selectionMode === "localPoint") {
        // just get the thing
        if (this._clickMap) {
          var layers = this._clickMap.active(this._currentPt.x, this._currentPt.y);

          for (var i = 0; i < layers.length; i++) {
            // we don't actually compute score right now so set it to 0 and short circuit the
            // culling based on threshold for now
            layers[i] = { name: layers[i], score: 0 };
          }

          return layers;
        }
        else {
          console.log('ERROR: Click Map not inialized');
          return [];
        }
      }
    }
    else {
      if (this._selectionMode === "localBox") {
        // find the segments with the most "importance" then display them along with thumbnails
        // in the sidebar
        var x = Math.min(this._currentRect.pt1.x, this._currentRect.pt2.x);
        var y = Math.min(this._currentRect.pt1.y, this._currentRect.pt2.y);
        var w = Math.abs(this._currentRect.pt1.x - this._currentRect.pt2.x);
        var h = Math.abs(this._currentRect.pt1.y - this._currentRect.pt2.y);

        // returns the layer names and the importance values
        rank = c.regionalImportance(this._rankMode, { 'x': x, 'y': y, 'w': w, 'h': h });
      }
      else if (this._selectionMode === "localPoint") {
        rank = c.pointImportance(this._rankMode, { 'x': this._currentPt.x, 'y': this._currentPt.y }, c.getContext());
      }

      rank.sort(function (a, b) {
        return -(a.score - b.score);
      });
    }

    // cull and display
    var displayLayers = [];
    for (var i = 0; i < rank.length; i++) {
      if (rank[i].score > this._rankThreshold) {
        displayLayers.push(rank[i]);
      }
    }

    return displayLayers;
  }

  tagFilter(layers) {
    var filtered = [];
    var tags = this._filterMenu.selectedTags;

    // no tags = nop
    if (!tags || tags.length === 0)
      return layers;

    for (var i = 0; i < layers.length; i++) {
      var include = true;

      for (var t = 0; t < tags.length; t++) {
        if (!c.hasTag(layers[i].name, tags[t])) {
          include = false;
          break;
        }
      }

      if (include) {
        filtered.push(layers[i]);
      }
    }

    return filtered;
  }

  goalSelect() {
    if (this._selectionMode === "localBox") {
      var x = Math.min(this._currentRect.pt1.x, this._currentRect.pt2.x);
      var y = Math.min(this._currentRect.pt1.y, this._currentRect.pt2.y);
      var w = Math.abs(this._currentRect.pt1.x - this._currentRect.pt2.x);
      var h = Math.abs(this._currentRect.pt1.y - this._currentRect.pt2.y); 

      return c.goalSelect(this.goal, c.getContext(), x, y, w, h, this._maxLevel);
    }
    else {
      return c.goalSelect(this.goal, c.getContext(), this._currentPt.x, this._currentPt.y, this._maxLevel);
    }
  }

  selectLayers() {
    if (this._rankMode === "goal") {
      // goal-based selection is a bit different
      var layers = this.goalSelect();

      let layerNames = [];
      for (let l in layers) {
        layerNames.push(l);
      }
      g_groupPanel.displaySelectedLayers(layerNames);
      //this._paramSelectPanel.layers = layers;
    }
    else {
      var displayLayers = this.rankLayers();

      if (this._useTags) {
        displayLayers = this.tagFilter(displayLayers);
      }

      let layers = {};
      let layerNames = [];
      for (let l in displayLayers) {
        //layers[displayLayers[l].name] = {};
        //layers[displayLayers[l].name][adjType.OPACITY] = [{ param: 'opacity', val: 1}];
        layerNames.push(displayLayers[l].name);
      }

      g_groupPanel.displaySelectedLayers(layerNames);
      //this._paramSelectPanel.layers = layers;
      //this.showLayers(displayLayers);
    }
  }

  createClickMap() {
    this._clickMap = c.createClickMap(rankModes[this._rankMode], c.getContext());

    showStatusMsg("", "OK", "Click Map Created");
  }

  computeClickMap() {
    this._clickMap.init(this._clickMapThreshold, this._clickMapNorm);
    this._clickMap.compute(this._clickMapDepth);

    showStatusMsg("", "OK", "Click Map Generated");
  }

  showLayers(layers) {
    this.deleteAllLayers();

    for (var i = 0; i < layers.length; i++) {
      var control = new LayerControls(layers[i].name);
      control.createUI(this._controlArea);
      control.displayThumb = true;
      control.scoreLabel = layers[i].score;
      this._layerControls.push(control);
    }
  }

  showLayerSelectPopup(screenPtX, screenPtY) {
    if (this._layerSelectPopup) {
      this._layerSelectPopup.deleteUI();
    }

    var layers = this.rankLayers();
    this._layerSelectPopup = new LayerSelectPopup(layers, this, screenPtX, screenPtY);
  }

  deleteAllLayers() {
    if (this._layerControls) {
      for (var i = 0; i < this._layerControls.length; i++) {
        this._layerControls[i].deleteUI();
      }
    }

    this._layerControls = [];
  }

  // initializes the current selection canvas by doing the following:
  // - unbinding any current handlers
  // - updating the canvas size (1:1 with max res image pixels)
  // - clearing the canvas
  // - binding new events
  initCanvas() {
    // unbind
    this._selectionCanvas.off();

    // resize
    var dims = c.imageDims("full");
    this._selectionCanvas.attr({ width: dims.w, height: dims.h });

    // clear
    var ctx = this._selectionCanvas[0].getContext("2d");
    ctx.clearRect(0, 0, dims.w, dims.h);

    // rebind
    var self = this;
    this._selectionCanvas.mousedown(function (e) { self.canvasMouseDown(e) });
    this._selectionCanvas.mouseup(function (e) { self.canvasMouseUp(e) });
    this._selectionCanvas.mousemove(function (e) { self.canvasMouseMove(e) });
    this._selectionCanvas.mouseout(function (e) { self.canvasMouseOut(e) });

    // internal state
    this._drawing = false;
    this._currentRect = {};
  }

  // bindings
  canvasMouseDown(event) {
    // mode check
    if (this._mouseMode === mouseMode.normal) {
      if (this._selectionMode === "localBox") {
        if (!this._drawing) {
          this._currentRect.pt1 = this.screenToLocal(event.pageX, event.pageY);
          this._currentRect.pt2 = this._currentRect.pt1;
          this._drawing = true;
          this.updateCanvas();
        }
      }
    }
  }

  canvasMouseUp(event) {
    // button check
    if (event.which === 1) {
      if (this._mouseMode === mouseMode.normal) {
        if (this._selectionMode === "localBox") {
          if (this._drawing) {
            this._drawing = false;
            this._currentRect.pt2 = this.screenToLocal(event.pageX, event.pageY);
            this.updateCanvas();
            this.selectLayers();
          }
        }
        else if (this._selectionMode === "localPoint") {
          this._currentPt = this.screenToLocal(event.pageX, event.pageY);
          this.selectLayers();
        }
      }
      else if (this._mouseMode === mouseMode.updateOnHover) {
        // when the mouse is clicked for update on hover we keep track of that
        // point and show the list of layers at that point on mouse out
        this._lockedPt = this.screenToLocal(event.pageX, event.pageY);
        this._currentPt = this._lockedPt;
        this.selectLayers();
      }
      else if (this._mouseMode === mouseMode.contextClick) {
        // this mode pops up a window (or like an arrangement of thumbnails)
        // with layer thumbnails and names. clicking on one of the popups opens those
        // layer controls on the right (for now, maybe inline?)
        this._currentPt = this.screenToLocal(event.pageX, event.pageY);
        this.showLayerSelectPopup(event.pageX, event.pageY);
      }
    }
    else if (event.which === 3) {
      //this._filterMenu.showAt(event.pageX, event.pageY);
      this._goalMenu.showAt(event.pageX, event.pageY);
    }
  }

  canvasMouseMove(event) {
    if (this._mouseMode === mouseMode.normal) {
      if (this._selectionMode === "localBox") {
        if (this._drawing) {
          this._currentRect.pt2 = this.screenToLocal(event.pageX, event.pageY);
          this.updateCanvas();
        }
      }
    }
    else if (this._mouseMode === mouseMode.updateOnHover) {
      this._currentPt = this.screenToLocal(event.pageX, event.pageY);
      this.selectLayers();
    }
  }

  canvasMouseOut(event) {
    if (this._mouseMode === mouseMode.normal) {
      if (this._selectionMode === "localBox") {
        if (this._drawing) {
          this._currentRect.pt2 = this.screenToLocal(event.pageX, event.pageY);
          this.updateCanvas();
        }
      }
    }
    else if (this._mouseMode === mouseMode.updateOnHover) {
      this._currentPt = this._lockedPt;
      this.selectLayers();
    }
  }

  // canvas callbacks
  updateCanvas() {
    if (this._selectionMode === "localBox") {
      var x = Math.min(this._currentRect.pt1.x, this._currentRect.pt2.x);
      var y = Math.min(this._currentRect.pt1.y, this._currentRect.pt2.y);
      var w = Math.abs(this._currentRect.pt1.x - this._currentRect.pt2.x);
      var h = Math.abs(this._currentRect.pt1.y - this._currentRect.pt2.y);

      var ctx = this._selectionCanvas[0].getContext("2d");
      ctx.clearRect(0, 0, this._selectionCanvas[0].width, this._selectionCanvas[0].height);

      ctx.strokeStyle = "#FF0000";
      ctx.lineWidth = "2";
      ctx.beginPath();
      ctx.rect(x, y, w, h);
      ctx.stroke();
    }
  }

  // other UI callbacks
  updateMapMenu() {
    // clear existing elements
    $('#mapDisplayMenu .item').remove();
    var layers = c.getLayerNames();
    $('#mapDisplayMenu .menu').append('<div class="item" data-value="none">None</div>');

    for (var i in layers) {
      $('#mapDisplayMenu .menu').append('<div class="item" data-value="' + layers[i] + '">' + layers[i] + '</div>');
    }

    $('#mapDisplayMenu').dropdown('refresh');
    $('#mapDisplayMenu').dropdown('set selected', 'none');
  }

  hideImportanceMap() {
    eraseCanvas($('#previewCanvas'));
  }

  showImportanceMap(layer) {
    // get the map
    var imap = c.getImportanceMap(layer, rankModes[this._rankMode]);

    // this should throw an exception instead of crashing if the importance map doesn't exist
    // if it does, we'll catch and show an error message
    try {
      drawImage(imap.image, $('#previewCanvas'));
    }
    catch (err) {
      showStatusMsg(err.message, "ERROR", "Error drawing importance map");
    }
  }

  showClickMapViz(type) {
    try {
      var img = this._clickMap.visualize(clickMapVizMode[type]);
      drawImage(img, $('#previewCanvas'));
    }
    catch (err) {
      showStatusMsg(err.message, "ERROR", "Error drawing click map visualization.")
    }
  }

  // coordinate conversion functions
  // relative coords are [0, 1], local coords are within the bounds defined by min and max
  // screen coords are between the canvas bounds
  relativeToLocal(x, y) {
    var lx = x * this._selectionCanvas[0].width;
    var ly = y * this._selectionCanvas[0].height;

    return {x: lx, y: ly};
  }

  localToRelative(x, y) {
    var rx = x / this._selectionCanvas[0].width;
    var ry = y / this._selectionCanvas[0].height;

    return { x: rx, y: ry };
  }

  screenToRelative(x, y) {
    // determine scale
    var w = this._selectionCanvas[0].width;
    var h = this._selectionCanvas[0].height;
    var scale = Math.min(this._selectionCanvas.width() / w, this._selectionCanvas.height() / h);

    // position correction
    var offset = this._selectionCanvas.offset();
    x = x - offset.left;
    y = y - offset.top;

    // determine the actual offset
    w = w * scale;
    h = h * scale;
    var xOffset = (this._selectionCanvas.width() - w) / 2;
    var yOffset = (this._selectionCanvas.height() - h) / 2;

    // remove offset
    var lx = ((x - xOffset) / w);
    lx = Math.min(Math.max(lx, 0), 1);

    var ly = ((y - yOffset) / h);
    ly = Math.min(Math.max(ly, 0), 1);

    return { x: lx, y: ly };
  }

  screenToLocal(x, y) {
    var rel = this.screenToRelative(x, y);
    return this.relativeToLocal(rel.x, rel.y);
  }
}

// clicking on one of these layers pops up a thing on the right
class LayerSelectPopup {
  constructor(layers, parent, x, y) {
    this._layers = layers;
    this._screenX = x;
    this._screenY = y;

    // so we still need to call functions in the calling object because it's nicer
    // and because javascript don't care about inheritance or definition order
    // we can just go nuts
    this._parent = parent;

    this.createUI();
  }

  createUI() {
    // first delete any duplicate ids
    $('#layerSelectPopup').remove();

    // create new element and append to the body (it's an absolute positioned element)
    this._uiElem = $(`
      <div class="ui inverted segment" id="layerSelectPopup">
        <div class="ui top attached inverted label">Layers</div>
        <div class="ui mini icon button closeButton"><i class="window close outline icon"></i></div>
        <div class="ui two column grid">

        </div>
      </div>
    `);
    $('body').append(this._uiElem);

    // populate with layer cards
    for (var i in this._layers) {
      var name = this._layers[i].name;
      var dims = c.imageDims("full");

      var layerElem = '<div class="column">'
      layerElem += '<div class="ui card" layerName="' + name + '">';
      layerElem += '<canvas width="' + dims.w + '" height="' + dims.h + '"></canvas>';
      layerElem += '<div class="extra content">' + name + '</div>';
      layerElem += '</div></div>';

      this._uiElem.find('.grid').append(layerElem);

      // canvas size and drawing
      var l = c.getLayer(name);
      var canvas = $('#layerSelectPopup div[layerName="' + name + '"] canvas');
      if (!l.isAdjustmentLayer()) {
        drawImage(c.getLayer(name).image(), canvas);
      }
    }

    // bindings
    var self = this;
    $('#layerSelectPopup .card').click(function () {
      var name = $(this).attr('layerName');
      self._parent.showLayers([{ 'name': name, 'score': 0 }]);
    });

    $('#layerSelectPopup .closeButton').click(function () {
      self.deleteUI();
    });

    // positioning
    // bounds
    var maxTop = $('body').height() - this._uiElem.height() - 50;

    var top = Math.min(maxTop, this._screenY);

    this._uiElem.css({ left: this._screenX, top: top });
  }

  deleteUI() {
    this._uiElem.remove();
  }
}

// this is a bit more detailed than the layer select popup
// in that it's able to group arbitrary combinations of parameters and
// layers together, along with a number of different visualization modes.
// this is also a panel that can be placed in a number of places depending on where it's initialized.
class ParameterSelectPanel {
  // layers is expected to be an object with the format layer_name: { adjustment_type: [parameters] }
  // this is subject to change
  constructor(name, parent) {
    this._layers = {};

    this._previewMode = PreviewMode.animatedParams;
    this._parentContainer = parent;

    // nuke everything in the container
    this._parentContainer.html('');

    this._name = name;
    this._renderSize = "small";
    this._activeControls = [];

    this._animationData = {};
    this._animationCache = {};
    this._loopSize = 30;
    this._fps = 30;
    this._animationMode = AnimationMode.bounce;
    this._frameHold = 15;
    this.displayOnMain = true;

    this.initUI();
  }

  initUI() {
    // set up all the things
    // delete duplicates
    $('.parameterSelectPanel[name="' + this._name + '"]').remove();

    this._uiElem = '<div class="parameterSelectPanel" name="' + this._name + '">';
    this._uiElem += '<div class="ui mini icon button optionsButton"><i class="setting icon"></i></div>';
    this._uiElem += '<div class="ui mini icon button addGroupButton" data-content="Create Group"><i class="plus icon"></i></div>';
    this._uiElem += this.createGroupModMenus();
    this._uiElem += '<div class="ui two column grid"></div></div>';
    this._uiElem = $(this._uiElem);

    var layerControlContainer = '<div class="parameterSelectLayerPanel standardLayerFormat" name="' + this._name + '">';
    layerControlContainer += '<div class="ui top attached inverted label">Layer Controls</div>';
    layerControlContainer += '<div class="ui mini icon button backButton"><i class="arrow left icon"></i></div>';
    layerControlContainer += '</div>';

    this._layerControlUIElem = $(layerControlContainer);
    this._parentContainer.append(this._uiElem);
    this._parentContainer.append(this._layerControlUIElem);
    this._layerControlUIElem.hide();

    // layer control panel will need a close button (sits on top of panels)
    var self = this;
    $('.parameterSelectLayerPanel[name="' + this._name + '"] .backButton').click(function () {
      self.hideLayerControl();
    });

    $('.parameterSelectPanel[name="' + this._name + '"] .optionsButton').click(function () {
      $('.parameterSelectOptions[name="' + self._name + '"]').toggle();
    });

    $('.parameterSelectPanel[name="' + this._name + '"] .addGroupButton').popup();
    $('.parameterSelectPanel[name="' + this._name + '"] .addGroupButton').click(function () {
      self.addNewLayerGroup();
    });

    // buttons n stuff
    $('.parameterSelectPanel[name="' + this._name + '"] .addToGroupButton').dropdown({
      onChange: self.addSelectedLayersToGroup
    });

    $('.parameterSelectPanel[name="' + this._name + '"] .removeFromGroupButton').dropdown({
      onChange: self.removeSelectedLayersFromGroup
    });

    this.initSettingsUI();
    this.updateGroupModMenus();
  }

  // little helper to create those dropdown menu buttons
  // returns the html needed
  createGroupModMenus() {
    let addMenu = '<div class="ui mini icon top right pointing dropdown button addToGroupButton" data-content="Add To Group">';
    addMenu += '<i class="add circle icon"></i>';
    addMenu += '<div class="menu"><div class="header">Add Layers to Group</div></div></div>';

    let removeMenu = '<div class="ui mini icon top right pointing dropdown button removeFromGroupButton" data-content="Remove From Group">';
    removeMenu += '<i class="minus circle icon"></i>';
    removeMenu += '<div class="menu"><div class="header">Remove Layers from Group</div></div></div>';

    return addMenu + removeMenu;
  }

  updateGroupModMenus() {
    // clear items
    $('.parameterSelectPanel[name="' + this._name + '"] .addToGroupButton .menu .item').remove();
    $('.parameterSelectPanel[name="' + this._name + '"] .removeFromGroupButton .menu .item').remove();

    // get groups
    let order = c.getGroupOrder();
    for (let o in order) {
      let g = order[o];
      if (!c.getGroup(g.group).readOnly) {
        $('.parameterSelectPanel[name="' + this._name + '"] .addToGroupButton .menu').append('<div class="item">' + g.group + "</div>");
        $('.parameterSelectPanel[name="' + this._name + '"] .removeFromGroupButton .menu').append('<div class="item">' + g.group + "</div>");
      }
    }

    $('.parameterSelectPanel[name="' + this._name + '"] .addToGroupButton .menu').dropdown('refresh');
    $('.parameterSelectPanel[name="' + this._name + '"] .removeFromGroupButton .menu').dropdown('refresh');
  }

  addSelectedLayersToGroup(value, text, elem) {
    // gather the layers
    let names = [];
    let selected = $('.groupSelectCheckbox.checked');
    selected.each(function (i, elem) {
      names.push($(elem).attr('name'));
    });

    g_metaGroupList[value].addLayers(names);
    renderImage('Group Contents Change');
  }

  removeSelectedLayersFromGroup(value, text, elem) {
    let names = [];
    let selected = $('.groupSelectCheckbox.checked');
    selected.each(function (i, elem) {
      names.push($(elem).attr('name'));
    });

    g_metaGroupList[value].removeLayers(names);
    renderImage('Group Contents Change');
  }

  // in a separate function cause it's just long and annoying
  initSettingsUI() {
    var container = '<div class="parameterSelectOptions" name="' + this._name + '">'
    container += '<div class="ui top attached inverted label">Layer Selection Panel Settings</div>';
    container += '<div class="ui mini icon button backButton"><i class="remove icon"></i></div>';
    container += '<div class="ui relaxed inverted list"></div></div>';
    this._parentContainer.append(container);

    this._settingsList = $('.parameterSelectOptions[name="' + this._name + '"] .list');

    // append stuff 
    var previewMode = $(`
    <div class="item">
      <div class="ui right floated content">
        <div class="ui right pointing dropdown inverted button" param="_previewMode">
          <span class="text">[Preview Mode]</span>
          <div class="menu">
            <div class="item" data-value="1" param="_previewMode">Layer Pixels (unfiltered)</div>
            <div class="item" data-value="2" param="_previewMode">Animated Parameters</div>
          </div>
        </div>
      </div>
      <div class="content">
        <div class="header">Layer Preview Mode</div>
      </div>
    </div>`);
    this._settingsList.append(previewMode);

    // animation mode
    var animationMode = $(`
    <div class="item">
      <div class="ui right floated content">
        <div class="ui right pointing dropdown inverted button" param="_animationMode">
          <span class="text">[Animation Mode]</span>
          <div class="menu">
            <div class="item" data-value="1" param="_animationMode">Bounce</div>
            <div class="item" data-value="2" param="_animationMode">Snap</div>
          </div>
        </div>
      </div>
      <div class="content">
        <div class="header">Animation Mode</div>
      </div>
    </div>`);
    this._settingsList.append(animationMode);

    // Loop Size
    var loopSize = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui input" id="selectThreshold" param="_loopSize">
                  <input type="number" param="_loopSize"/>
              </div>
          </div>
          <div class="content">
              <div class="header">Loop Size</div>
              <div class="description">Number of frames in an animation preview loop.</div>
          </div>
      </div>
    `);
    this._settingsList.append(loopSize);

    // fps
    var fps = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui input" param="_fps">
                  <input type="number" param="_fps"/>
              </div>
          </div>
          <div class="content">
              <div class="header">FPS</div>
              <div class="description">Animation Frame Rate</div>
          </div>
      </div>
    `);
    this._settingsList.append(fps);

    // frame hold
    var frameHold = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui input" param="_frameHold">
                  <input type="number" param="_frameHold" />
              </div>
          </div>
          <div class="content">
              <div class="header">Frame Hold</div>
              <div class="description">Number of Frames to hold at the end of an animation cycle.</div>
          </div>
      </div>
    `);
    this._settingsList.append(frameHold);

    // display on main
    var displayOnMain = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui toggle checkbox" param="displayOnMain">
                  <input type="checkbox" param="displayOnMain" />
              </div>
          </div>
          <div class="content">
              <div class="header">Display Layer Preview On Main Canvas</div>
              <div class="description">Shows the preview animation/hover on the main canvas instead of in the thumbnail.</div>
          </div>
      </div>
    `);
    this._settingsList.append(displayOnMain);

    // bindings, try to do them all at once. dropdowns are a bit more difficult
    // input boxes
    var self = this;
    $('.parameterSelectOptions[name="' + this._name + '"] .list input').change(function () {
      var param = $(this).attr("param");
      self[param] = parseInt($(this).val());
    });

    // checkbox
    $('.parameterSelectOptions[name="' + this._name + '"] .list .checkbox').checkbox({
      onChecked: function () {
        var param = $(this).attr("param");
        self[param] = true;
      },
      onUnchecked: function () {
        var param = $(this).attr("param");
        self[param] = false;
      }
    });

    // dropdowns
    $('.parameterSelectOptions[name="' + this._name + '"] .list .dropdown').dropdown({
      onChange: function (value, text, $selectedItem) {
        // need to debug this a bit
        var param = $selectedItem.attr("param");
        self[param] = parseInt(value);
        console.log($selectedItem);
      }
    })

    // set initial values
    $('.parameterSelectOptions[name="' + this._name + '"] input[param="_loopSize"]').val(this._loopSize);
    $('.parameterSelectOptions[name="' + this._name + '"] input[param="_fps"]').val(this._fps);
    $('.parameterSelectOptions[name="' + this._name + '"] input[param="_frameHold"').val(this._frameHold);
    $('.parameterSelectOptions[name="' + this._name + '"] .checkbox[param="displayOnMain"]').checkbox(this.displayOnMain ? 'set checked' : 'set unchecked');
    $('.parameterSelectOptions[name="' + this._name + '"] .dropdown[param="_previewMode"]').dropdown('set selected', this._previewMode);
    $('.parameterSelectOptions[name="' + this._name + '"] .dropdown[param="_animationMode"]').dropdown('set selected', this._animationMode);

    // exit button
    $('.parameterSelectOptions[name="' + this._name + '"] .backButton').click(function () {
      $('.parameterSelectOptions[name="' + self._name + '"]').hide();
    });
    $('.parameterSelectOptions[name="' + self._name + '"]').hide();
  }

  addNewLayerGroup() {
    // this will add a new meta-group to the current project
    var self = this;
    $('#newMetaGroupModal').modal({
      closable: false,
      onDeny: function () { },
      onApprove: function () {
        let name = $('#newMetaGroupModal input').val();

        if (c.isGroup(name))
          return false;

        // gather the layers
        let names = [];
        let selected = $('.groupSelectCheckbox.checked');
        selected.each(function (i, elem) {
          names.push($(elem).attr('name'));
        });

        // TODO: UPDATE GROUP CREATION PROCESS
        c.addGroup(name, names, 0, false);
        addGroupUI(name);
        self.updateGroupModMenus();
      }
    }).modal('show');
  }

  set displayOnMain(val) {
    this._showOnMain = val;

    // prepare the preview canvas
    if (val === true) {
      var dims = c.imageDims(this._renderSize);
      $('#previewCanvas').attr({ width: dims.w, height: dims.h });
    }

    $('#previewCanvas').hide();
  }

  get displayOnMain() {
    return this._showOnMain;
  }

  deleteUI() {
    this._uiElem.remove();
    this._layerControlUIElem.remove();
  }

  deleteLayerCards() {
    this._uiElem.find('.grid').html('');

    // changing the layers deletes the cache.
    // we can check for object equality to preserve things that are the same if needed.
    this._animationCache = {};
  }

  createLayerCards() {
    // populate with layer + adjustment cards
    var dims = c.imageDims(this._renderSize);
    for (var i in this._layers) {
      var name = i;

      // get the adjustments
      var adjustments = this._layers[i];
      for (var a in adjustments) {
        var adj = a;

        // get the parameters
        // eventually this'll be parameter groups probably
        var layerElem = '<div class="column">';
        var displayText = name + " - " + adjToString[adj]
        var id = name + adj;

        layerElem += '<div class="ui card" layerName="' + name + '" adj="' + adj + '" cardID="' + id + '">';
        layerElem += '<canvas width="' + dims.w + '" height="' + dims.h + '"></canvas>';
        layerElem += '<div class="extra content">' + displayText + '</div>';
        //layerElem += '<div class="ui checkbox groupSelectCheckbox" name="' + name + '"><input type="checkbox"></div></div>';
        layerElem += '</div></div>';

        this._uiElem.find('.grid').append(layerElem);

        // bindings are based on the drawing mode
        this.bindLayerCard(name, adj, adjustments[a], id);
      }
    }
  }

  bindLayerCard(name, adj, params, id) {
    // canvas and layer objects
    var l = c.getLayer(name);
    var canvas = $('.parameterSelectPanel[name="' + this._name + '"] div[cardID="' + id + '"] canvas');
    var elem = $('.parameterSelectPanel[name="' + this._name + '"] div[cardID="' + id + '"]');
    var cbox = elem.find('.groupSelectCheckbox');
    var self = this;

    if (this._previewMode === PreviewMode.rawLayer) {
      if (!l.isAdjustmentLayer()) {
        drawImage(c.getCachedImage(name, this._renderSize), canvas);
      }
    }
    else if (this._previewMode === PreviewMode.animatedParams) {
      // this preview mode will animate the parameters to the point where the layer
      // was determined to have satisfied the goal conditions.
      // this will bind a mouseover event to start the animation loop for the hovered layer
      // panel. the images will be rendered on demand until the cache is full, at which point it'll
      // update at full speed 
      canvas.mouseover(function () { self.animateStart(name, adj, params, id); });
      canvas.mouseout(function () { self.animateStop(name, adj, params, id); });

      // just draw the composition as normal for the first frame
      drawImage(c.renderContext(c.getContext(), this._renderSize), canvas);
    }

    // onclick bindings are the same regardless
    canvas.click(function () {
      self.showLayerControl(name, adj);
    });

    cbox.checkbox();
  }

  // displays the layer control panel and the controls associated with the given layer
  showLayerControl(name, adj) {
    // assumed to be clear
    // generate the layer controller
    // TODO: layer control needs to highlight relevant parameters
    var layerControl = new LayerControls(name);
    layerControl.createUI(this._layerControlUIElem);
    layerControl.displayThumb = true;

    // i am leaving the possibility for multiple layers open but really it's just like 
    // always the one layer for now
    this._activeControls.push(layerControl);
    this._layerControlUIElem.show();
  }

  hideLayerControl() {
    // clear and hide
    for (var i = 0; i < this._activeControls.length; i++) {
      this._activeControls[i].deleteUI();
    }
    this._activeControls = [];

    this._layerControlUIElem.hide();
  }

  animateStart(name, adj, params, id) {
    // start the animation loop and initialize data structs
    var self = this;

    this._animationData = {};
    this._animationData.layerName = name;
    this._animationData.adjustment = parseInt(adj);
    this._animationData.params = params;  // contains param and val (which is now the target val)
    this._animationData.cardID = id;
    this._animationData.forward = true;
    this._animationData.canvas = $('.parameterSelectPanel[name="' + this._name + '"] div[cardID="' + id + '"] canvas'); 
    this._animationData.currentFrame = 0;
    this._animationData.held = 0;

    // for now opacity is always a full cycle parameter instead of whatever value the
    // goal selector spits out (this is basically the case for when a specific objective isn't 
    // selected but i'm not sure how we want to expose that to the user at this time)
    // so i guess TODO: something
    var ctx = c.getContext();
    if (this._animationData.adjustment === adjType.OPACITY) {
      this._animationData.fullCycle = true;
      this._animationData.startVal = { "opacity": 0 };
      this._animationData.params[0].val = 1;
      //this._animationData.startVal = ctx.getLayer(this._animationData.layerName).opacity();
    }
    else {
      this._animationData.startVal = {};
      for (var p in this._animationData.params) {
        var param = this._animationData.params[p];
        this._animationData.startVal[param.param] = ctx.getLayer(this._animationData.layerName).getAdjustment(this._animationData.adjustment)[param.param];
      }
    }

    if (!(id in this._animationCache))
      this._animationCache[id] = {};

    // show preview canvas if applicable
    if (this.displayOnMain) {
      $('#previewCanvas').show();
      $('#renderCanvas').hide();
    }

    this._intervalID = setInterval(function () {
      // state is tracked internally by the selector object
      self.drawNextFrame();
    }, 1000 / this._fps);
  }

  animateStop(name, adj, param, id) {
    clearInterval(this._intervalID);

    // redraw base look?
    // i dunno what the base look should be
    $('#previewCanvas').hide();
    $('#renderCanvas').show();
  }

  drawNextFrame() {
    // We'll want to do a few things in each loop:
    // - check to see if the image we want is already in the cache
    // - if is in cache, draw to thumbnail
    // - if not in cache:
    // -- interpolate the parameters to proper value
    // -- render
    // -- put in cache
    // -- draw as normal

    // check for existence in cache
    if (!(this._animationData.currentFrame in this._animationCache[this._animationData.cardID])) {
      // if not, render
      // render context
      var ctx = c.getContext();

      // simple lerp of the values for now
      var t = this._animationData.currentFrame / this._loopSize;
      for (var p in this._animationData.params) {
        var param = this._animationData.params[p].param;
        var target = this._animationData.params[p].val;

        var val = this._animationData.startVal[param] * (1 - t) + target * t;

        // update context
        if (this._animationData.adjustment === adjType.OPACITY) {
          ctx.getLayer(this._animationData.layerName).opacity(val);
        }
        else {
          ctx.getLayer(this._animationData.layerName).addAdjustment(this._animationData.adjustment, param, val);
        }
      }

      // ensure visibility
      ctx.getLayer(this._animationData.layerName).visible(true);

      // render
      var img = c.renderContext(ctx, this._renderSize);

      // stash in cache
      this._animationCache[this._animationData.cardID][this._animationData.currentFrame] = img;
    }

    // render
    drawImage(this._animationCache[this._animationData.cardID][this._animationData.currentFrame], this._animationData.canvas);

    // render to preview canvas if option is checked
    if (this.displayOnMain) {
      drawImage(this._animationCache[this._animationData.cardID][this._animationData.currentFrame], $('#previewCanvas'));
    }

    // check frame hold
    if (this._animationData.currentFrame === this._loopSize - 1 && this._animationData.held < this._frameHold) {
      this._animationData.held += 1;
    }
    else {
      // increment current frame
      if (this._animationData.forward)
        this._animationData.currentFrame += 1;
      else
        this._animationData.currentFrame -= 1;

      // reset if out of bounds.
      if (this._animationMode === AnimationMode.bounce) {
        //bounces between forward and backward.
        if (this._animationData.currentFrame >= this._loopSize) {
          this._animationData.forward = false;
          this._animationData.held = 0;
        }
        else if (this._animationData.currentFrame <= 0) {
          this._animationData.forward = true;
        }
      }
      else if (this._animationMode === AnimationMode.snap) {
        if (this._animationData.currentFrame >= this._loopSize) {
          this._animationData.currentFrame = 0;
          this._animationData.held = 0;
        }
      }
    }
  }

  set layers(data) {
    this._layers = data;

    // trigger ui update
    this.deleteLayerCards();
    this.createLayerCards();
  }

  get layers() {
    return this._layers;
  }
}

// the group panel is a rewrite of the param select panel
// the group panel is a global object that can be accessed by other parts of the interface
// (the selection system for instance) 
// two containing elements are required for this object to work: a main component where the
// group contents are listed, and another component where the current selected layers
// are shown
class GroupPanel {
  // primary and secondary are jquery objects for the primary container
  // and secondary container used by the object
  constructor(name, primary, secondary) {
    this._name = name;
    this._primary = primary;
    this._secondary = secondary;
    this._activeControls = [];
    this._groupControl = null;
    this._currentGroup = "";

    this._previewMode = PreviewMode.animatedParams;
    this._layerSelectPreviewMode = PreviewMode.staticSolidColor;
    this._renderSize = "small";
    this._animationData = {};
    this._animationCache = {};
    this._loopSize = 30;
    this._fps = 30;
    this._animationMode = AnimationMode.bounce;
    this._frameHold = 15;
    this._displayOnMain = true;
    this._selectedLayers = [];
    this._intervalID = null;
    this._hoveredLayer = null;

    // initialize preview canvas
    var dims = c.imageDims(this._renderSize);
    $('#previewCanvas').attr({ width: dims.w, height: dims.h });

    this.initUI();
  }

  get primarySelector() {
    return '.groupPanel[name="' + this._name + '"]';
  }

  get secondarySelector() {
    return '.secondaryGroupPanel[name="' + this._name + '"]';
  }

  deleteUI() {
    this._primary.find(this.primarySelector).remove();
    this._secondary.find(this.secondarySelector).remove();
  }

  initUI() {
    // delete stuff
    this.deleteUI();

    // basically the same form as parameter select panel but with a few more options n stuff
    // primary first
    let pelem = '<div class="groupPanel" name="' + this._name + '">';
    pelem += '<div class="ui mini icon button optionsButton"><i class="setting icon"></i></div>';
    pelem += '<div class="groupSelectDropdown">';
    pelem += '<div class="ui selection dropdown"><i class="dropdown icon"></i>';
    pelem += '<div class="default text">No Group Selected</div>';
    pelem += '<div class="menu"></div></div>';
    pelem += '<div class="ui right floated labeled icon button newGroupButton"><i class="plus icon"></i>New Group</div></div>';
    pelem += '<div class="groupControls standardLayerFormat"></div>';
    pelem += '<div class="ui two column grid groupContents"></div></div>';

    // layer control overlay
    let layerControlContainer = '<div class="groupPanelLayerControls standardLayerFormat" name="' + this._name + '">';
    layerControlContainer += '<div class="ui top attached inverted label">Layer Controls</div>';
    layerControlContainer += '<div class="ui mini icon button backButton"><i class="arrow left icon"></i></div>';
    layerControlContainer += '</div>';

    this._primary.append(pelem);
    this._primary.find(this.primarySelector).append(layerControlContainer);

    this.initSettingsUI();

    // secondary
    let selem = '<div class="secondaryGroupPanel" name="' + this._name + '">';
    selem += '<div class="ui compact groupButtons buttons">';
    selem += '<button class="ui button" name="addAllToGroup">Add All</button>';
    selem += '<button class="ui button" name="removeAllFromGroup">Remove All</button>';
    selem += '</div>';
    selem += '<div class="ui inverted header">Selected Layers</div>';
    selem += '<table class="ui inverted selectable table"><tbody></tbody</table>';
    selem += '</div>';

    this._secondary.append(selem);

    // bindings
    let self = this;
    $(this.primarySelector + ' .backButton').click(function() {
      self.hideLayerControl();
    });

    $(this.primarySelector + ' .optionsButton').click(function() {
      $(self.primarySelector + ' .groupSelectOptions').toggle();
    });

    $(this.primarySelector + ' .groupSelectDropdown div.dropdown').dropdown({
      onChange: function(value, text, $selectedItem) { self.updateGroup(value); }
    });

    $(this.primarySelector + ' .groupSelectDropdown .newGroupButton').click(function() {
      self.addNewGroup();
    });

    $(this.secondarySelector).find('.button[name="addAllToGroup"]').click(function() {
      if (!c.isGroup(self._currentGroup) || c.getGroup(self._currentGroup).readOnly)
        return;

      for (let l in self._selectedLayers) {
        if (!c.isGroup(self._selectedLayers[l])) {
          c.addLayerToGroup(self._selectedLayers[l], self._currentGroup);
        }
      }
      $(self.secondarySelector).find('tr .checkbox').checkbox('set checked');
      self.updateLayerCards();
      renderImage('Group Membership Change');
    });

    $(this.secondarySelector).find('.button[name="removeAllFromGroup"]').click(function() {
      if (!c.isGroup(self._currentGroup) || c.getGroup(self._currentGroup).readOnly)
        return;

      for (let l in self._selectedLayers) {
        if (!c.isGroup(self._selectedLayers[l])) {
          c.removeLayerFromGroup(self._selectedLayers[l], self._currentGroup);
        }
      }
      $(self.secondarySelector).find('tr .checkbox').checkbox('set unchecked');
      self.updateLayerCards();
      renderImage('Group Membership Change');
    });

    this.hideLayerControl();

    $(document).keydown(function(event) {
      let oldMode = self._layerSelectPreviewMode;
      // shift
      if (event.keyCode === 16) {
        self._layerSelectPreviewMode = PreviewMode.rawLayer;
      }
      else if (event.keyCode === 17) {
        self._layerSelectPreviewMode = PreviewMode.animatedParams;
      }

      // if the mode was changed reboot
      if (oldMode !== self._layerSelectPreviewMode) {
        self.stopVis()
        self.startVis(self._hoveredLayer, { mode: self._layerSelectPreviewMode });
      }
    });
    $(document).keyup(function(event) {
      if (event.keyCode === 16 || event.keyCode === 17) {
        self._layerSelectPreviewMode = PreviewMode.staticSolidColor;
        self.stopVis();
        self.startVis(self._hoveredLayer, { mode: self._layerSelectPreviewMode });
      }
    });
  }

  initSettingsUI() {
    var container = '<div class="groupSelectOptions" name="' + this._name + '">'
    container += '<div class="ui top attached inverted label">Groups Panel Settings</div>';
    container += '<div class="ui mini icon button backButton"><i class="remove icon"></i></div>';
    container += '<div class="ui relaxed inverted list"></div></div>';
    this._primary.find(this.primarySelector).append(container);

    this._settingsList = $(this.primarySelector + ' .groupSelectOptions .list');

    // append stuff 
    var previewMode = $(`
    <div class="item">
      <div class="ui right floated content">
        <div class="ui right pointing dropdown inverted button" param="_previewMode">
          <span class="text">[Preview Mode]</span>
          <div class="menu">
            <div class="item" data-value="1" param="_previewMode">Layer Pixels (unfiltered)</div>
            <div class="item" data-value="2" param="_previewMode">Animated Parameters</div>
            <div class="item" data-value="5" param="_previewMode">Solid Color</div>
            <div class="item" data-value="8" param="_previewMode">Animated Layer Pixels</div>
          </div>
        </div>
      </div>
      <div class="content">
        <div class="header">Layer Preview Mode</div>
      </div>
    </div>`);
    this._settingsList.append(previewMode);

    var lsPreviewMode = $(`
    <div class="item">
      <div class="ui right floated content">
        <div class="ui right pointing dropdown inverted button" param="_layerSelectPreviewMode">
          <span class="text">[Preview Mode]</span>
          <div class="menu">
            <div class="item" data-value="1" param="_layerSelectPreviewMode">Layer Pixels (unfiltered)</div>
            <div class="item" data-value="2" param="_layerSelectPreviewMode">Animated Parameters</div>
            <div class="item" data-value="5" param="_layerSelectPreviewMode">Solid Color</div>
            <div class="item" data-value="8" param="_layerSelectPreviewMode">Animated Layer Pixels</div>
          </div>
        </div>
      </div>
      <div class="content">
        <div class="header">Layer Selection Preview Mode</div>
      </div>
    </div>`);
    this._settingsList.append(lsPreviewMode);

    // animation mode
    var animationMode = $(`
    <div class="item">
      <div class="ui right floated content">
        <div class="ui right pointing dropdown inverted button" param="_animationMode">
          <span class="text">[Animation Mode]</span>
          <div class="menu">
            <div class="item" data-value="1" param="_animationMode">Bounce</div>
            <div class="item" data-value="2" param="_animationMode">Snap</div>
          </div>
        </div>
      </div>
      <div class="content">
        <div class="header">Animation Mode</div>
      </div>
    </div>`);
    this._settingsList.append(animationMode);

    // Loop Size
    var loopSize = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui input" id="selectThreshold" param="_loopSize">
                  <input type="number" param="_loopSize"/>
              </div>
          </div>
          <div class="content">
              <div class="header">Loop Size</div>
              <div class="description">Number of frames in an animation preview loop.</div>
          </div>
      </div>
    `);
    this._settingsList.append(loopSize);

    // fps
    var fps = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui input" param="_fps">
                  <input type="number" param="_fps"/>
              </div>
          </div>
          <div class="content">
              <div class="header">FPS</div>
              <div class="description">Animation Frame Rate</div>
          </div>
      </div>
    `);
    this._settingsList.append(fps);

    // frame hold
    var frameHold = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui input" param="_frameHold">
                  <input type="number" param="_frameHold" />
              </div>
          </div>
          <div class="content">
              <div class="header">Frame Hold</div>
              <div class="description">Number of Frames to hold at the end of an animation cycle.</div>
          </div>
      </div>
    `);
    this._settingsList.append(frameHold);

    // display on main
    var displayOnMain = $(`
      <div class="item">
          <div class="ui right floated content">
              <div class="ui toggle checkbox" param="_displayOnMain">
                  <input type="checkbox" param="_displayOnMain" />
              </div>
          </div>
          <div class="content">
              <div class="header">Display Layer Preview On Main Canvas</div>
              <div class="description">Shows the preview animation/hover on the main canvas instead of in the thumbnail.</div>
          </div>
      </div>
    `);
    this._settingsList.append(displayOnMain);

    // bindings, try to do them all at once. dropdowns are a bit more difficult
    // input boxes
    var self = this;
    $(this.primarySelector + ' .groupSelectOptions .list input').change(function () {
      var param = $(this).attr("param");
      self[param] = parseInt($(this).val());
    });

    // checkbox
    $(this.primarySelector + ' .groupSelectOptions .list .checkbox').checkbox({
      onChecked: function () {
        var param = $(this).attr("param");
        self[param] = true;
      },
      onUnchecked: function () {
        var param = $(this).attr("param");
        self[param] = false;
      }
    });

    // dropdowns
    $(this.primarySelector + ' .groupSelectOptions .list .dropdown').dropdown({
      onChange: function (value, text, $selectedItem) {
        // need to debug this a bit
        var param = $selectedItem.attr("param");
        self[param] = parseInt(value);
        console.log($selectedItem);
      }
    })

    // set initial values
    $(this.primarySelector + ' input[param="_loopSize"]').val(this._loopSize);
    $(this.primarySelector + ' input[param="_fps"]').val(this._fps);
    $(this.primarySelector + ' input[param="_frameHold"').val(this._frameHold);
    let cb = this._displayOnMain;
    $(this.primarySelector + ' .checkbox[param="_displayOnMain"]').checkbox((cb ? 'set checked' : 'set unchecked'));
    this._displayOnMain = cb;
    $(this.primarySelector + ' .dropdown[param="_previewMode"]').dropdown('set selected', this._previewMode);
    $(this.primarySelector + ' .dropdown[param="_animationMode"]').dropdown('set selected', this._animationMode);
    $(this.primarySelector + ' .dropdown[param="_layerSelectPreviewMode"]').dropdown('set selected', this._layerSelectPreviewMode);

    // exit button
    var self = this;
    $(this.primarySelector + ' .groupSelectOptions .backButton').click(function () {
      $(self.primarySelector + ' .groupSelectOptions').hide();
    });
    $(self.primarySelector + ' .groupSelectOptions').hide();
  }

  updateGroupDropdown() {
    let elem = $(this.primarySelector + ' .groupSelectDropdown div.dropdown');

    // store current selection
    let selected = elem.dropdown('get value');
    elem.find('.menu').html('');

    // get group list
    let order = c.getGroupOrder();
    let useSelected = false;
    for (let o in order) {
      let name = order[o].group;
      if (name === selected)
        useSelected = true;
      
      elem.find('.menu').append('<div class="item" data-value="' + name + '">' + name + '</div>');
    }

    elem.dropdown('refresh');
    if (useSelected) {
      elem.dropdown('set value', selected);
      elem.dropdown('set text', selected);
    }
    else {
      elem.dropdown('set value', "");
      elem.dropdown('set text', "No Group Selected");
      $(this.primarySelector + ' .groupContents').html('');
    }
  }


  updateGroup(value) {
    // uh, delete the thing
    if (this._groupControl) {
      this._groupControl.deleteUI();
      delete this._groupControl;
    }

    // nothing selected
    if (value === "")
      return;

    // add the group control thing
    this._groupControl = new GroupControls(value);
    this._groupControl.createUI($(this.primarySelector + ' .groupControls'));
    this._currentGroup = value;

    this.displaySelectedLayers(this._selectedLayers);
    this.updateLayerCards();
  }

  addNewGroup() {
    var self = this;
    $('#newMetaGroupModal').modal({
      closable: false,
      onDeny: function () { },
      onApprove: function () {
        let name = $('#newMetaGroupModal input').val();
        
        if (c.isGroup(name))
          return false;
        
        c.addGroup(name, [], 0, false);
        self.updateGroupDropdown();
        $(self.primarySelector + ' .groupSelectDropdown div.dropdown').dropdown('set exactly', name);
      }
    }).modal('show');
  }

  hideLayerControl() {
    // clear and hide
    for (var i = 0; i < this._activeControls.length; i++) {
      this._activeControls[i].deleteUI();
    }
    this._activeControls = [];

    this._primary.find(this.primarySelector + ' .groupPanelLayerControls').hide();
  }

  // displays the layer control panel and the controls associated with the given layer
  showLayerControl(name) {
    // assumed to be clear
    // generate the layer controller
    // TODO: layer control needs to highlight relevant parameters
    var layerControl = new LayerControls(name);
    layerControl.createUI($(this.primarySelector + ' .groupPanelLayerControls'));
    layerControl.displayThumb = true;

    // i am leaving the possibility for multiple layers open but really it's just like 
    // always the one layer for now
    this._activeControls.push(layerControl);
    $(this.primarySelector + ' .groupPanelLayerControls').show();
  }

  // simplified version of the general parameter selection window
  // can go back later if needed, not really needed now
  updateLayerCards() {
    let group = c.getGroup(this._currentGroup);
    //$(this.primarySelector + ' .groupContents').html('');
    this._animationCache = {};

    let dims = c.imageDims(this._renderSize);
    for (let l in group.affectedLayers) {
      let layerName = group.affectedLayers[l];

      if ($(this.primarySelector).find('.card[layerName="' + layerName + '"]').length === 0) {
        let elem = '<div class="column">';
        elem += '<div class="ui card" layerName="' + layerName + '">';
        elem += '<canvas width="' + dims.w + '" height="' + dims.h + '"></canvas>';
        elem += '<div class="extra content">' + layerName + '</div>';
        elem += '</div></div>';

        $(this.primarySelector + ' .groupContents').append(elem);
        this.bindLayerCard(layerName);
      }
    }

    // if was removed, delete
    $(this.primarySelector).find('.card').each(function(i, elem) {
      if(group.affectedLayers.indexOf($(this).attr('layerName')) < 0) {
        $(this).parent().remove();
      }
    });
  }

  bindLayerCard(name) {
    let l = c.getLayer(name);
    let canvas = $(this.primarySelector + ' .groupContents div[layerName="' + name + '"] canvas');
    let elem = $(this.primarySelector + ' .groupContents div[layerName="' + name + '"]');
    var self = this;

    canvas.mouseover(function () {
      self.startVis(name, { mode: self._previewMode, canvas: $(self.primarySelector + ' .groupContents div[layerName="' + name + '"] canvas') });
    });
    canvas.mouseout(function () {
      self.stopVis(name);
    });

    // just draw the composition as normal for the first frame
    c.asyncRenderContext(c.getContext(), this._renderSize, function(err, img) {
      drawImage(img, canvas);
    });

    canvas.click(function () {
      self.showLayerControl(name);
    });
  }

  displaySelectedLayers(layers) {
    // delete existing
    $(this.secondarySelector).find('tbody').html('');
    this._selectedLayers = layers;

    // sticks a table of the selected layers in the secondary view of this object
    for (let l in layers) {
      if (c.isGroup(layers[l]))
        continue;

      // create the element
      let elem = '<tr layer-name="' + layers[l] + '">';
      elem += '<td class="collapsing"><div class="ui toggle checkbox"><input type="checkbox"></div></td>';
      elem += '<td class="activeArea" layer-name="' + layers[l] + '">' + layers[l] + '</td>';
      elem += '</tr>';

      // append
      $(this.secondarySelector).find('tbody').append(elem);

      // bindings
      let layerName = layers[l];
      let self = this;
      $(this.secondarySelector).find('tr[layer-name="' + layers[l] + '"] .checkbox').checkbox({
        onChecked: function() {
          c.addLayerToGroup(layerName, self._currentGroup);
          renderImage('Group Membership Change');
        },
        onUnchecked: function() {
          c.removeLayerFromGroup(layerName, self._currentGroup);
          renderImage('Group Membership Change');
        },
        beforeChecked: function() {
          return !c.getGroup(self._currentGroup).readOnly;
        },
        beforeUnchecked: function() {
          return !c.getGroup(self._currentGroup).readOnly;
        },
        onChange: function() { self.updateLayerCards(); }
      });

      if (c.layerInGroup(layers[l], this._currentGroup)) {
        $(this.secondarySelector).find('tr[layer-name="' + layers[l] + '"] .checkbox').checkbox('set checked');
      }
    }

    let self = this;
    $(this.secondarySelector).find('tr').mouseover(function() {
      self._hoveredLayer = $(this).attr('layer-name');
      self.startVis($(this).attr('layer-name'), { mode: self._layerSelectPreviewMode });
    });
    $(this.secondarySelector).find('tr').mouseout(function() {
      self._hoveredLayer = null;
      self.stopVis($(this).attr('layer-name'));
    });
    $(this.secondarySelector).find('td.activeArea').click(function() {
      self.hideLayerControl();
      self.showLayerControl($(this).attr('layer-name'));
    });
  }

  startVis(name, opts = {}) {
    // opts should have the desired mode, and an optional extra target canvas to draw on
    // more options may be added later of course
    if (name === null)
      return;

    if (this._displayOnMain) {
      $('#previewCanvas').show();
      $('#renderCanvas').hide();
    }

    if (opts.mode === PreviewMode.rawLayer) {
      if (!c.getLayer(name).isAdjustmentLayer()) {
        if (this._displayOnMain) {
          drawImage(c.getCachedImage(name, this._renderSize), $('#previewCanvas'));
        }
        if (opts.canvas) {
          drawImage(c.getCachedImage(name, this._renderSize), opts.canvas);
        }
      }      
    }
    else if (opts.mode === PreviewMode.animatedParams || opts.mode === PreviewMode.animatedRawLayer) {
      if (this._intervalID !== null) {
        this.animateStop(name);
      }

      this.animateStart(name, opts);
    }
    else if (opts.mode === PreviewMode.staticSolidColor) {
      if (!c.getLayer(name).isAdjustmentLayer() && !c.isGroup(name)) {
        if (this._displayOnMain) {
          drawImage(c.getCachedImage(name, this._renderSize).fill(1, 0, 0), $('#previewCanvas'));
          $('#renderCanvas').show();
        }
        if (opts.canvas) {
          drawImage(c.getCachedImage(name, this._renderSize).fill(1, 0, 0), opts.canvas);
        }
      }
    }
  }

  stopVis(name, opts = {}) {
    $('#previewCanvas').hide();
    $('#renderCanvas').show();

    this.animateStop(name);
  }

  // also a simplified version of the param select animation method
  animateStart(name, opts) {
    // start the animation loop and initialize data structs
    var self = this;

    this._animationData = {};
    this._animationData.layerName = name;
    this._animationData.adjustment = adjType.OPACITY;

    // basically two keyframes, can change later when/if needed
    // format: array of objects with { adj, param, val }
    this._animationData.start = [{adj: adjType.OPACITY, param: "opacity", val: 0}];
    this._animationData.end = [{adj: adjType.OPACITY, param: "opacity", val: 1}];
    
    this._animationData.forward = true;
    this._animationData.canvas = opts.canvas; // $(this.primarySelector + ' .groupContents div[layerName="' + name + '"] canvas'); 
    this._animationData.currentFrame = 0;
    this._animationData.held = 0;
    this._animationData.mode = opts.mode;

    var ctx = c.getContext();

    if (!(name in this._animationCache))
      this._animationCache[name] = {};

    // show preview canvas if applicable
    if (this._displayOnMain) {
      $('#previewCanvas').show();
      $('#renderCanvas').hide();
    }

    this._intervalID = setInterval(function () {
      // state is tracked internally by the selector object
      self.drawNextFrame();
    }, 1000 / this._fps);
  }
  
  animateStop(name) {
    clearInterval(this._intervalID);
    this._intervalID = null;

    $('#previewCanvas').hide();
    $('#renderCanvas').show();
  }

  drawNextFrame() {
    // We'll want to do a few things in each loop:
    // - check to see if the image we want is already in the cache
    // - if is in cache, draw to thumbnail
    // - if not in cache:
    // -- interpolate the parameters to proper value
    // -- render
    // -- put in cache
    // -- draw as normal

    // check for existence in cache
    if (!(this._animationData.currentFrame in this._animationCache[this._animationData.layerName])) {
      // if not, render
      // render context
      var ctx = c.getContext();

      // simple lerp of the values for now
      var t = this._animationData.currentFrame / this._loopSize;

      for (let p = 0; p < this._animationData.start.length; p++) {
        let startFrame = this._animationData.start[p];
        let endFrame = this._animationData.end[p];

        let val = startFrame.val * (1 - t) + endFrame.val * t;

        if (startFrame.adj === adjType.OPACITY) {
          ctx.getLayer(this._animationData.layerName).opacity(val);
        }
        else {
          ctx.getLayer(this._animationData.layerName).addAdjustment(startFrame.adj, startFrame.param, val);
        }
      }

      // ensure visibility
      ctx.getLayer(this._animationData.layerName).visible(true);
      
      // other layer blending settings
      if (this._animationData.mode === PreviewMode.animatedRawLayer) {
        // for animated raw layer we interp between current blend settings (t = 0) and opacity 0 (t = 1) for
        // all other layers 
        let order = c.getLayerNames();
        for (let l in order) {
          if (order[l] === this._animationData.layerName) {
            ctx.getLayer(order[l]).opacity(1);
          }
          else {
            let lval = ctx.getLayer(order[l]).opacity() * (1 - t);
            ctx.getLayer(order[l]).opacity(lval);
          }
        }
      }

      // render
      var img = c.renderContext(ctx, this._renderSize);

      // stash in cache
      this._animationCache[this._animationData.layerName][this._animationData.currentFrame] = img;
    }

    // render
    if (this._animationData.canvas) {
      drawImage(this._animationCache[this._animationData.layerName][this._animationData.currentFrame], this._animationData.canvas);
    }

    // render to preview canvas if option is checked
    if (this._displayOnMain) {
      drawImage(this._animationCache[this._animationData.layerName][this._animationData.currentFrame], $('#previewCanvas'));
    }

    // check frame hold
    if (this._animationData.currentFrame === this._loopSize - 1 && this._animationData.held < this._frameHold) {
      this._animationData.held += 1;
    }
    else {
      // increment current frame
      if (this._animationData.forward)
        this._animationData.currentFrame += 1;
      else
        this._animationData.currentFrame -= 1;

      // reset if out of bounds.
      if (this._animationMode === AnimationMode.bounce) {
        //bounces between forward and backward.
        if (this._animationData.currentFrame > this._loopSize) {
          this._animationData.forward = false;
          this._animationData.held = 0;
        }
        else if (this._animationData.currentFrame < 0) {
          this._animationData.forward = true;
        }
      }
      else if (this._animationMode === AnimationMode.snap) {
        if (this._animationData.currentFrame > this._loopSize) {
          this._animationData.currentFrame = 0;
          this._animationData.held = 0;
        }
      }
    }
  }
}

class FilterMenu {
  constructor(tags) {
    if (tags) {
      this._tags = tags;
    }
    else {
      this._tags = [];
    }

    this.createUI();
  }

  get selectedTags() {
    return this._selectedTags;
  }

  createUI() {
    // make a new id element if none exists
    $('#filterMenu').remove();

    this._uiElem = $(`
      <div class="ui inverted segment" id="filterMenu">
        <div class="ui top attached inverted label">Selection Intent</div>
        <div class="ui mini icon button closeButton"><i class="window close outline icon"></i></div>
        <div class="ui multiple selection search fluid dropdown">
          <input name="tags" type="hidden">
          <i class="dropdown icon"></i>
          <div class="default text">None</div>
          <div class="menu">

          </div>  
        </div>
      </div>
    `);

    $('body').append(this._uiElem);

    var self = this;
    // initialize dropdown
    $('#filterMenu .ui.dropdown').dropdown({
      action: 'activate',
      onChange: function (value, text, $selectedItem) {
        // ugh need to make this an array even if it's just one selection
        if (typeof (value) === "string") {
          self._selectedTags = [value];
        }
        else {
          self._selectedTags = value;
        }
      }
    });

    // button
    $('#filterMenu .closeButton').click(function () {
      self.hide();
    });

    // add the tags
    this.updateTags();

    $('#filterMenu').hide();
  }

  updateTags() {
    for (var i = 0; i < this._tags.length; i++) {
      $('#filterMenu .menu').append('<div class="item" data-value="' + this._tags[i] + '">' + this._tags[i] + '</div>');
    }

    $('#filterMenu .ui.dropdown').dropdown('refresh');
  }

  showAt(x, y) {
    // displays the thing at screen position x, y
    var maxTop = $('body').height() - this._uiElem.height() - 50;
    var top = Math.min(maxTop, y);

    this._uiElem.css({ left: x, top: top });
    $('#filterMenu').show();
  }

  hide() {
    $('#filterMenu').hide();
  }
}

class GoalMenu {
  constructor() {
    // idk what goes here yet.
    // wait there's at least this
    this.initUI();

    this._goalColor = { r: 1, g: 1, b: 1 };
    this._goalDirection = 0;
  }

  initUI() {
    // very similar to the tag menu huh
    $('#goalMenu').remove();

    var uiElem = `
      <div class="ui inverted segment" id="goalMenu">
        <div class="ui top attached inverted label">Goal Definition</div>
        <div class="ui mini icon button closeButton"><i class="window close outline icon"></i></div>
        <div class="ui selection fluid dropdown" id="primaryGoalSelect">
          <input name="goals" type="hidden">
          <i class="dropdown icon"></i>
          <div class="default text">None</div>
          <div class="menu">`;

    // add the elements
    for (var type in GoalType) {
      uiElem += '<div class="item" data-value="' + GoalType[type] + '">' + GoalString[type] + "</div>";
    }

    uiElem += '</div></div><div class="ui divider"></div>'
    uiElem += '<div id="goalOptionsSection"></div>'
    uiElem += '</div>';

    this._uiElem = $(uiElem);

    $('body').append(this._uiElem);

    // goal status text
    $('#goalStatus').remove();

    this._goalStatusElem = $('<div id="goalStatus"></div>');
    $('#imgStatusBar').append(this._goalStatusElem);

    // bindings
    var self = this;
    // initialize dropdown
    $('#primaryGoalSelect').dropdown({
      action: 'activate',
      onChange: function (value, text, $selectedItem) {
        // this is gonna be fun... we either have to append additional elements here or pre-create
        // and then show them. Going to do appending first, and will change if it becomes a problem.
        self.updateActiveSections(parseInt(value));
      }
    });

    // button
    $('#goalMenu .closeButton').click(function () {
      self.hide();
    });

    // set active
    $('#primaryGoalSelect').dropdown('set selected', GoalType.none);

    this.hide();
  }

  updateActiveSections(goalType) {
    this._activeGoalType = goalType;

    // clear
    $('#goalOptionsSection').html('');

    if (this._activeGoalType === GoalType.none) {
      // nothin
    }
    else if (this._activeGoalType === GoalType.targetColor) {
      var colorStr = 'rgb(' + parseInt(this._goalColor.r * 255) + ',' + parseInt(this._goalColor.g * 255) + ',' + parseInt(this._goalColor.b * 255) + ')';

      var elem = '<div class="ui small inverted header">Target Color Settings</div>';
      elem += '<div class="goalColorPicker fluid ui button" style="background-color:' + colorStr + ';"> Select Color</div > ';

      $('#goalOptionsSection').append($(elem));

      // bindings
      var self = this;
      $('#goalOptionsSection .goalColorPicker').click(function () {
        self.toggleColorPicker($('#goalOptionsSection .goalColorPicker'), "_goalColor");
      });
    }
    else if (this._activeGoalType === GoalType.relativeBrightness || this._activeGoalType === GoalType.saturate) {
      var elem = '<div class="ui selection fluid dropdown relativeBrightnessMenu">';
      elem += '<input name="direction" type="hidden" />';
      elem += '<i class="dropdown icon"></i>';
      elem += '<div class="default text">Direction</div>';
      elem += '<div class="menu">';
      elem += '<div class="item" data-value="-1">Less</div>';
      elem += '<div class="item" data-value="1">More</div>';
      elem += '</div></div>';

      $('#goalOptionsSection').append(elem);

      var self = this;
      $('#goalOptionsSection .relativeBrightnessMenu').dropdown({
        onChange: function (value, text, $selectedItem) {
          self._goalDirection = parseInt(value);
          self.updateStatus();
        }
      });
    }
    else if (this._activeGoalType === GoalType.colorize) {
      var colorStr = 'rgb(' + parseInt(this._goalColor.r * 255) + ',' + parseInt(this._goalColor.g * 255) + ',' + parseInt(this._goalColor.b * 255) + ')';

      var elem = '<div class="ui small inverted header">Colorize Settings</div>';
      elem += '<div class="goalColorPicker fluid ui button" style="background-color:' + colorStr + ';">Select Color (Hue)</div > ';
  
      $('#goalOptionsSection').append($(elem));

      // bindings
      var self = this;
      $('#goalOptionsSection .goalColorPicker').click(function () {
        self.toggleColorPicker($('#goalOptionsSection .goalColorPicker'), "_goalColor");
      });
    }

    this.updateStatus();
  }

  toggleColorPicker(elem, storeIn) {
    if ($('#colorPicker').hasClass('hidden')) {
      // move color picker to spot
      var offset = elem.offset();

      var cc = this[storeIn];
      cp.setColor({ "r": cc.r * 255, "g": cc.g * 255, "b": cc.b * 255 }, 'rgb');
      cp.startRender();

      $("#colorPicker").css({ 'left': '', 'right': '', 'top': '', 'bottom': '' });
      if (offset.top + elem.outerHeight() + $('#colorPicker').height() > $('body').height()) {
        $('#colorPicker').css({ "left": offset.left, top: offset.top - $('#colorPicker').height() });
      }
      else {
        $('#colorPicker').css({ "left": offset.left, top: offset.top + elem.outerHeight() });
      }

      // assign callbacks to update proper color
      var self = this;
      cp.color.options.actionCallback = function (e, action) {
        console.log(action);
        if (action === "changeXYValue" || action === "changeZValue" || action === "changeInputValue") {
          self[storeIn] = cp.color.colors.rgb;
          self.updateStatus();
          $(elem).css({ "background-color": "#" + cp.color.colors.HEX });
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

  // displays the goal menu at the specified position
  showAt(x, y) {
    var maxTop = $('body').height() - this._uiElem.height() - 50;
    var top = Math.min(maxTop, y);

    this._uiElem.css({ left: x, top: top });
    this._uiElem.show();
  }

  hide() {
    this._uiElem.hide();

    // also hide the color picker
    $('#colorPicker').addClass('hidden');
    $('#colorPicker').removeClass('visible');
  }

  get goal() {
    // this is going to be a mess for a while because the spec for the goal object
    // will likely keep changing constantly
    // so you'll need to cross reference this with the Selection.h file to figure out what
    // these numbers mean
    if (this._activeGoalType === GoalType.none) {
      // none is technically a "select any" operation
      // type 0 is "select any", target 0 is "no target
      // color doesn't matter
      return { 'type': 0, 'target': 0, 'color': { r: 0, g: 0, b: 0 } };
    }
    else if (this._activeGoalType === GoalType.targetColor) {
      // type 2 is "Select Target Color", target 3 is "Exact", color is, uh, the target color
      return { 'type': 2, 'target': 3, 'color': this._goalColor };
    }
    else if (this._activeGoalType === GoalType.relativeBrightness) {
      return { 'type': 3, 'target': (this._goalDirection > 0) ? 1 : 2, 'color': { r: 0, g: 0, b: 0 } };
    }
    else if (this._activeGoalType === GoalType.saturate) {
      return { 'type': 6, 'target': (this._goalDirection > 0) ? 1 : 2, 'color': { r: 0, g: 0, b: 0 } };
    }
    else if (this._activeGoalType === GoalType.colorize) {
      return { 'type': 5, 'target': 1, 'color': this._goalColor };
    }
  }

  updateStatus() {
    var elem = "Current Goal: " + $('#primaryGoalSelect').dropdown('get text');

    if (this._activeGoalType === GoalType.targetColor || this._activeGoalType === GoalType.colorize) {
      var colorStr = 'rgb(' + parseInt(this._goalColor.r * 255) + ',' + parseInt(this._goalColor.g * 255) + ',' + parseInt(this._goalColor.b * 255) + ')';

      elem += '<div class="goalColor" style="background-color:' + colorStr + ';"></div>';
    }
    else if (this._activeGoalType === GoalType.relativeBrightness) {
      elem = "Current Goal: " + (this._goalDirection > 0 ? "more" : "less") + " brightness";
    }
    else if (this._activeGoalType === GoalType.saturate) {
      elem = "Current Goal: " + ((this._goalDirection > 0) ? "" : "De-") + "Saturate";
    }

    this._goalStatusElem.html(elem);
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

  set displayThumb(val) {
    this._displayThumb = val;

    if (this._displayThumb === true) {
      this._uiElem.find('.thumb').addClass('show');
    }
    else {
      this._uiElem.find('.thumb').removeClass('show');
    }
  }

  get displayThumb() {
    return this._displayThumb;
  }

  get scoreLabel() {
    return this._scoreLabel;
  }

  set scoreLabel(val) {
    this._scoreLabel = val;
    this._uiElem.find(".scoreLabel").show();
    this._uiElem.find(".scoreLabel .detail").text(val);
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
    this._container = container;
    
    this._uiElem = $(this.buildUI());
    container.append(this._uiElem);

    // bindings
    this.bindEvents();
    this.drawThumb();
  }

  rebuildUI() {
    this._uiElem.remove();
    this._uiElem = $(this.buildUI());
    this._container.append(this._uiElem);
    this.bindEvents();
    this.displayThumb = this.displayThumb;
    this.drawThumb();
  }

  // this may need to be updated to handle a number of different modes
  drawThumb() {
    // draw the layer thumbnail
    if (!this.layer.isAdjustmentLayer()) {
      drawImage(this.layer.image(), this._uiElem.find('canvas'));
    }
  }

  buildUI() {
    var html = '<div class="layer" layerName="' + this._name + '">';
    html += '<h3 class="ui grey inverted header">' + this._name + '</h3>';
    html += '<div class="ui mini label scoreLabel">Score<div class="detail"></div></div>';
    html += '<div class="thumb">';
    html += '<canvas></canvas>';

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
        html += startParamSection(this._name, "Hue/Saturation", type);
        html += addSliders(this._name, "Hue/Saturation", ["hue", "saturation", "lightness"]);
        html += endParamSection(this._name, type);
      }
      else if (type === 1) {
        // levels
        // TODO: Turn some of these into range sliders
        html += startParamSection(this._name, "Levels");
        html += addSliders(this._name, "Levels", ["inMin", "inMax", "gamma", "outMin", "outMax"]);
        html += endParamSection(this._name, type);
      }
      else if (type === 2) {
        // curves
        html += startParamSection(this._name, "Curves");
        html += addCurves(this._name, "Curves");
        html += endParamSection(this._name, type);
      }
      else if (type === 3) {
        // exposure
        html += startParamSection(this._name, "Exposure");
        html += addSliders(this._name, "Exposure", ["exposure", "offset", "gamma"]);
        html += endParamSection(this._name, type);
      }
      else if (type === 4) {
        // gradient
        html += startParamSection(this._name, "Gradient Map");
        html += addGradient(this._name);
        html += endParamSection(this._name, type);
      }
      else if (type === 5) {
        // selective color
        html += startParamSection(this._name, "Selective Color");
        html += addTabbedParamSection(this._name, "Selective Color", "Channel", ["reds", "yellows", "greens", "cyans", "blues", "magentas", "whites", "neutrals", "blacks"], ["cyan", "magenta", "yellow", "black"]);
        html += addToggle(this._name, "Selective Color", "relative", "Relative");
        html += endParamSection(this._name, type);
      }
      else if (type === 6) {
        // color balance
        html += startParamSection(this._name, "Color Balance");
        html += addSliders(this._name, "Color Balance", ["shadow R", "shadow G", "shadow B", "mid R", "mid G", "mid B", "highlight R", "highlight G", "highlight B"]);
        html += addToggle(this._name, "Color Balance", "preserveLuma", "Preserve Luma");
        html += endParamSection(this._name, type);
      }
      else if (type === 7) {
        // photo filter
        html += startParamSection(this._name, "Photo Filter");
        html += addColorSelector(this._name, "Photo Filter");
        html += addSliders(this._name, "Photo Filter", ["density"]);
        html += addToggle(this._name, "Photo Filter", "preserveLuma", "Preserve Luma");
        html += endParamSection(this._name, type);
      }
      else if (type === 8) {
        // colorize
        html += startParamSection(this._name, "Colorize");
        html += addColorSelector(this._name, "Colorize");
        html += addSliders(this._name, "Colorize", ["alpha"]);
        html += endParamSection(this._name, type);
      }
      else if (type === 9) {
        // lighter colorize
        // note name conflicts with previous params
        html += startParamSection(this._name, "Lighter Colorize");
        html += addColorSelector(this._name, "Lighter Colorize");
        html += addSliders(this._name, "Lighter Colorize", ["alpha"]);
        html += endParamSection(this._name, type);
      }
      else if (type === 10) {
        html += startParamSection(this._name, "Overwrite Color");
        html += addColorSelector(this._name, "Overwrite Color");
        html += addSliders(this._name, "Overwrite Color", ["alpha"]);
        html += endParamSection(this._name, type);
      }
      else if (type === 11) {
        html += startParamSection(this._name, "Invert");
        html += endParamSection(this._name, type);
      }
      else if (type === 12) {
        html += startParamSection(this._name, "Brightness and Contrast");
        html += addSliders(this._name, "Brightness and Contrast", ["brightness", "contrast"]);
        html += endParamSection(this._name, type);
      }
    }

    html += '</div></div>';

    return html;
  }

  // assumes this._uiElem exists and has been inserted into the DOM
  bindEvents() {
    var self = this;
    var visibleButton = this._uiElem.find('button.visibleButton');

    // canvas setup
    var dims = c.imageDims("full");
    this._uiElem.find('canvas').attr({ width: dims.w, height: dims.h });

    visibleButton.on('click', function () {
      // check status of button
      let visible = !visibleButton.find('i').hasClass('unhide');
      self.layer.visible(visible);

      if (visible) {
        visibleButton.html('<i class="unhide icon"></i>');
        visibleButton.removeClass("black");
        visibleButton.addClass("white");
      }
      else {
        visibleButton.html('<i class="hide icon"></i>');
        visibleButton.removeClass("white");
        visibleButton.addClass("black");
      }

      // trigger render after adjusting settings
      renderImage('layer ' + self.name + ' visibility change');
    });

    // set starting visibility
    if (this.layer.visible()) {
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

    // add adjustment menu
    this._uiElem.find('.addAdjustment').dropdown({
      action: 'hide',
      onChange: function (value, text) {
        // add the adjustment or something
        addAdjustmentToLayer(self.name, parseInt(value));
        self.rebuildUI();
        renderImage('layer ' + self.name + ' adjustment added');
      }
    });

    // section events
    this._uiElem.find('.divider').click(function () {
      $(this).siblings('.paramSection[sectionName="' + $(this).html() + '"]').transition('fade down');
    });

    this._uiElem.find('.deleteAdj').click(function () {
      var adjType = parseInt($(this).attr("adjType"));
      c.getLayer(self.name).deleteAdjustment(adjType);
      self.rebuildUI();
      renderImage('layer ' + self.name + ' adjustment deleted');
    });

    // parameters
    this.bindParam("opacity", this._layer.opacity() * 100, "", 1000, { });

    var adjustments = this.layer.getAdjustments();

    // param events
    for (var i = 0; i < adjustments.length; i++) {
      var type = adjustments[i];

      if (type === 0) {
        // hue sat
        var sectionName = "Hue/Saturation";
        this.bindParam("hue", (this.layer.getAdjustment(adjType.HSL).hue - 0.5) * 360, sectionName, type,
          { "range": false, "max": 180, "min": -180, "step": 0.1 });
        this.bindParam("saturation", (this.layer.getAdjustment(adjType.HSL).sat - 0.5) * 200, sectionName, type,
          { "range": false, "max": 100, "min": -100, "step": 0.1});
        this.bindParam("lightness", (this.layer.getAdjustment(adjType.HSL).light - 0.5) * 200, sectionName, type,
          { "range": false, "max": 100, "min": -100, "step": 0.1 });
      }
      else if (type === 1) {
        // levels
        // TODO: Turn some of these into range sliders
        var sectionName = "Levels";
        this.bindParam("inMin", (this.layer.getAdjustment(adjType.LEVELS).inMin * 255), sectionName, type,
          { "range": "min", "max": 255, "min": 0, "step": 1 });
        this.bindParam("inMax", (this.layer.getAdjustment(adjType.LEVELS).inMax * 255), sectionName, type,
          { "range": "max", "max": 255, "min": 0, "step": 1 });
        this.bindParam("gamma", (this.layer.getAdjustment(adjType.LEVELS).gamma * 10), sectionName, type,
          { "range": false, "max": 10, "min": 0, "step": 0.01 });
        this.bindParam("outMin", (this.layer.getAdjustment(adjType.LEVELS).outMin * 255), sectionName, type,
          { "range": "min", "max": 255, "min": 0, "step": 1 });
        this.bindParam("outMax", (this.layer.getAdjustment(adjType.LEVELS).outMax * 255), sectionName, type,
          { "range": "max", "max": 255, "min": 0, "step": 1 });
      }
      else if (type === 2) {
        // curves
        this.updateCurve();
      }
      else if (type === 3) {
        // exposure
        var sectionName = "Exposure";
        this.bindParam("exposure", (this.layer.getAdjustment(adjType.EXPOSURE).exposure - 0.5) * 10, sectionName, type,
          { "range": false, "max": 5, "min": -5, "step": 0.1 });
        this.bindParam("offset", this.layer.getAdjustment(adjType.EXPOSURE).offset - 0.5, sectionName, type,
          { "range": false, "max": 0.5, "min": -0.5, "step": 0.01 });
        this.bindParam("gamma", this.layer.getAdjustment(adjType.EXPOSURE).gamma * 10, sectionName, type,
          { "range": false, "max": 10, "min": 0.01, "step": 0.01 });
      }
      else if (type === 4) {
        // gradient
        this.updateGradient();
      }
      else if (type === 5) {
        // selective color
        var sectionName = "Selective Color";
        this.bindParam("cyan", (this.layer.selectiveColorChannel("reds", "cyan") - 0.5) * 200, sectionName, type,
          { "range": false, "max": 100, "min": -100, "step": 0.01 });
        this.bindParam("magenta", (this.layer.selectiveColorChannel("reds", "magenta") - 0.5) * 200, sectionName, type,
          { "range": false, "max": 100, "min": -100, "step": 0.01 });
        this.bindParam("yellow", (this.layer.selectiveColorChannel("reds", "yellow") - 0.5) * 200, sectionName, type,
          { "range": false, "max": 100, "min": -100, "step": 0.01 });
        this.bindParam("black", (this.layer.selectiveColorChannel("reds", "black") - 0.5) * 200, sectionName, type,
          { "range": false, "max": 100, "min": -100, "step": 0.01 });

        this._uiElem.find('.tabMenu[sectionName="' + sectionName + '"]').dropdown('set selected', 0);
        this._uiElem.find('.tabMenu[sectionName="' + sectionName + '"]').dropdown({
          action: 'activate',
          onChange: function (value, text) {
            // update sliders
            var params = ["cyan", "magenta", "yellow", "black"];
            for (var j = 0; j < params.length; j++) {
              s = 'div[sectionName="' + sectionName + '"] .paramSlider[layerName="' + self.name + '"][paramName="' + params[j] + '"]';
              i = 'div[sectionName="' + sectionName + '"] .paramInput[layerName="' + self.name + '"][paramName="' + params[j] + '"] input';

              var val = (self.layer.selectiveColorChannel(text, params[j]) - 0.5) * 200;
              $(s).slider({ value: val });
              $(i).val(String(val.toFixed(2)));
            }
          }
        });
      }
      else if (type === 6) {
        // color balance
        var sectionName = "Color Balance";
        this.bindParam("shadow R", (this.layer.getAdjustment(adjType.COLOR_BALANCE).shadowR - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
        this.bindParam("shadow G", (this.layer.getAdjustment(adjType.COLOR_BALANCE).shadowG - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
        this.bindParam("shadow B", (this.layer.getAdjustment(adjType.COLOR_BALANCE).shadowB - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
        this.bindParam("mid R", (this.layer.getAdjustment(adjType.COLOR_BALANCE).midR - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
        this.bindParam("mid G", (this.layer.getAdjustment(adjType.COLOR_BALANCE).midG - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
        this.bindParam("mid B", (this.layer.getAdjustment(adjType.COLOR_BALANCE).midB - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
        this.bindParam("highlight R", (this.layer.getAdjustment(adjType.COLOR_BALANCE).highR - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
        this.bindParam("highlight G", (this.layer.getAdjustment(adjType.COLOR_BALANCE).highG - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
        this.bindParam("highlight B", (this.layer.getAdjustment(adjType.COLOR_BALANCE).highB - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });

        this.bindToggle(sectionName, "preserveLuma", type);
      }
      else if (type === 7) {
        // photo filter
        var sectionName = "Photo Filter";

        this.bindColor(sectionName, type);

        this.bindParam("density", this.layer.getAdjustment(adjType.PHOTO_FILTER).density, sectionName, type,
          { "range": "min", "max": 1, "min": 0, "step": 0.01 });

        this.bindToggle(sectionName, "preserveLuma", type);
      }
      else if (type === 8) {
        // colorize
        var sectionName = "Colorize";
        this.bindColor(sectionName, type);

        this.bindParam("alpha", this.layer.getAdjustment(adjType.COLORIZE).a, sectionName, type,
          { "range": "min", "max": 1, "min": 0, "step": 0.01 });
      }
      else if (type === 9) {
        // lighter colorize
        // not name conflicts with previous params
        var sectionName = "Lighter Colorize";
        this.bindColor(sectionName, type);
        this.bindParam("alpha", this.layer.getAdjustment(adjType.LIGHTER_COLORIZE).a, sectionName, type,
          { "range": "min", "max": 1, "min": 0, "step": 0.01 });
      }
      else if (type === 10) {
        var sectionName = "Overwrite Color";
        this.bindColor(sectionName, type);
        this.bindParam("alpha", this.layer.getAdjustment(adjType.OVERWRITE_COLOR).a, sectionName, type,
          { "range": "min", "max": 1, "min": 0, "step": 0.01 });
      }
      else if (type === 12) {
        var sectionName = "Brightness and Contrast";
        this.bindParam("brightness", (this.layer.getAdjustment(adjType.BRIGHTNESS).brightness - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
        this.bindParam("contrast", (this.layer.getAdjustment(adjType.BRIGHTNESS).contrast - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
      }
    }
  }

  bindParam(paramName, initVal, section, type, config) {
    var s, i;
    var self = this;
    this._uiElem.find(".scoreLabel").hide();

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
        self.paramHandler(event, ui, paramName, type);
        renderImage('layer ' + self._name + ' parameter ' + paramName + ' change');
      },
      slide: function (event, ui) { self.paramHandler(event, ui, paramName, type) },
      change: function (event, ui) { self.paramHandler(event, ui, paramName, type) }
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

  bindToggle(sectionName, param, type) {
    var elem = this._uiElem.find('.checkbox[paramName="' + param + '"][sectionName="' + sectionName + '"]');
    var self = this;
    elem.checkbox({
      onChecked: function () {
        self.layer.addAdjustment(type, param, 1);
        if (self.layer.visible()) { renderImage('layer ' + self.name + ' parameter ' + param + ' toggle'); }
      },
      onUnchecked: function () {
        self.layer.addAdjustment(type, param, 0);
        if (self.layer.visible()) { renderImage('layer ' + self.name + ' parameter ' + param + ' toggle'); }
      }
    });

    // set initial state
    var paramVal = this.layer.getAdjustment(type, param);
    if (paramVal === 0) {
      elem.checkbox('set unchecked');
    }
    else {
      elem.checkbox('set checked');
    }
  }

  bindColor(section, type) {
    var elem = this._uiElem.find('.paramColor[sectionName="' + section + '"]');
    var self = this;

    elem.click(function () {
      if ($('#colorPicker').hasClass('hidden')) {
        // move color picker to spot
        var offset = elem.offset();

        var adj = self.layer.getAdjustment(type);
        cp.setColor({ "r": adj.r * 255, "g": adj.g * 255, "b": adj.b * 255 }, 'rgb');
        cp.startRender();

        $("#colorPicker").css({ 'left': '', 'right': '', 'top': '', 'bottom': '' });
        if (offset.top + elem.height() + $('#colorPicker').height() > $('body').height()) {
          $('#colorPicker').css({ "right": "10px", top: offset.top - $('#colorPicker').height() });
        }
        else {
          $('#colorPicker').css({ "right": "10px", top: offset.top + elem.height() });
        }

        // assign callbacks to update proper color
        cp.color.options.actionCallback = function (e, action) {
          console.log(action);
          if (action === "changeXYValue" || action === "changeZValue" || action === "changeInputValue") {
            var color = cp.color.colors.rgb;
            self.updateColor(type, color);
            $(elem).css({ "background-color": "#" + cp.color.colors.HEX });

            if (self.layer.visible()) {
              // no point rendering an invisible layer
              renderImage('layer ' + self.name + ' color change');
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

    var adj = this.layer.getAdjustment(type);
    var colorStr = "rgb(" + parseInt(adj.r * 255) + "," + parseInt(adj.g * 255) + "," + parseInt(adj.b * 255) + ")";
    elem.css({ "background-color": colorStr });
  }

  paramHandler(event, ui, paramName, type) {
    if (type === adjType["OPACITY"]) {
      let val = ui.value / 100;
      //for (let i = g_groupsByLayer[this.name].length - 1; i >= 0; i--) {
      //  val *= g_groupMods[g_groupsByLayer[this.name][i]].groupOpacity / 100;
      //}
  
      this._layer.opacity(val);
    }
    else if (type === adjType["HSL"]) {
      if (paramName === "hue") {
        this._layer.addAdjustment(adjType.HSL, "hue", (ui.value / 360) + 0.5);
      }
      else if (paramName === "saturation") {
        this._layer.addAdjustment(adjType.HSL, "sat", (ui.value / 200) + 0.5);
      }
      else if (paramName === "lightness") {
        this._layer.addAdjustment(adjType.HSL, "light", (ui.value / 200) + 0.5);
      }
    }
    else if (type === adjType["LEVELS"]) {
      if (paramName !== "gamma") {
        this._layer.addAdjustment(adjType.LEVELS, paramName, ui.value / 255);
      }
      else if (paramName === "gamma") {
        this._layer.addAdjustment(adjType.LEVELS, "gamma", ui.value / 10);
      }
    }
    else if (type === adjType["EXPOSURE"]) {
      if (paramName === "exposure") {
        this._layer.addAdjustment(adjType.EXPOSURE, "exposure", (ui.value / 10) + 0.5);
      }
      else if (paramName === "offset") {
        this._layer.addAdjustment(adjType.EXPOSURE, "offset", ui.value + 0.5);
      }
      else if (paramName === "gamma") {
        this._layer.addAdjustment(adjType.EXPOSURE, "gamma", ui.value / 10);
      }
    }
    else if (type === adjType["SELECTIVE_COLOR"]) {
      var channel = $(ui.handle).parent().parent().parent().find('.text').html();

      this._layer.selectiveColorChannel(channel, paramName, (ui.value / 200) + 0.5);
    }
    else if (type === adjType["COLOR_BALANCE"]) {
      if (paramName === "shadow R") {
        this._layer.addAdjustment(adjType.COLOR_BALANCE, "shadowR", (ui.value / 2) + 0.5);
      }
      else if (paramName === "shadow G") {
        this._layer.addAdjustment(adjType.COLOR_BALANCE, "shadowG", (ui.value / 2) + 0.5);
      }
      else if (paramName === "shadow B") {
        this._layer.addAdjustment(adjType.COLOR_BALANCE, "shadowB", (ui.value / 2) + 0.5);
      }
      else if (paramName === "mid R") {
        this._layer.addAdjustment(adjType.COLOR_BALANCE, "midR", (ui.value / 2) + 0.5);
      }
      else if (paramName === "mid G") {
        this._layer.addAdjustment(adjType.COLOR_BALANCE, "midG", (ui.value / 2) + 0.5);
      }
      else if (paramName === "mid B") {
        this._layer.addAdjustment(adjType.COLOR_BALANCE, "midB", (ui.value / 2) + 0.5);
      }
      else if (paramName === "highlight R") {
        this._layer.addAdjustment(adjType.COLOR_BALANCE, "highR", (ui.value / 2) + 0.5);
      }
      else if (paramName === "highlight G") {
        this._layer.addAdjustment(adjType.COLOR_BALANCE, "highG", (ui.value / 2) + 0.5);
      }
      else if (paramName === "highlight B") {
        this._layer.addAdjustment(adjType.COLOR_BALANCE, "highB", (ui.value / 2) + 0.5);
      }
    }
    else if (type === adjType["PHOTO_FILTER"]) {
      if (paramName === "density") {
        this._layer.addAdjustment(adjType.PHOTO_FILTER, "density", ui.value);
      }
    }
    else if (type === adjType["COLORIZE"] || type === adjType["LIGHTER_COLORIZE"] || type == adjType["OVERWRITE_COLOR"]) {
      if (paramName === "alpha") {
        this._layer.addAdjustment(type, "a", ui.value);
      }
    }
    else if (type === adjType["BRIGHTNESS"]) {
      if (paramName === "brightness") {
        this._layer.addAdjustment(adjType.BRIGHTNESS, "brightness", (ui.value / 2) + 0.5);
      }
      else if (paramName === "contrast") {
        this._layer.addAdjustment(adjType.BRIGHTNESS, "contrast", (ui.value / 2) + 0.5);
      }
    }

    // find associated value box and dump the value there
    $(ui.handle).parent().next().find("input").val(String(ui.value));
  }

  updateColor(type, color) {
    this._layer.addAdjustment(type, "r", color.r);
    this._layer.addAdjustment(type, "g", color.g);
    this._layer.addAdjustment(type, "b", color.b);
  }

  updateCurve() {
    var w = 400;
    var h = 400;

    var canvas = this._uiElem.find('.curveDisplay canvas');
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

    var l = this._layer;
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

  updateGradient() {
    var canvas = this._uiElem.find('.gradientDisplay canvas');
    canvas.attr({ width: canvas.width(), height: canvas.height() });
    var ctx = canvas[0].getContext("2d");
    var grad = ctx.createLinearGradient(0, 0, canvas.width(), canvas.height());

    // get gradient data
    var g = this._layer.getGradient();
    for (var i = 0; i < g.length; i++) {
      var color = 'rgb(' + g[i].color.r * 255 + "," + g[i].color.g * 255 + "," + g[i].color.b * 255 + ")";

      grad.addColorStop(g[i].x, color);
    }

    ctx.fillStyle = grad;
    ctx.fillRect(0, 0, canvas.width(), canvas.height());
  }
}

class GroupControls extends LayerControls {
  constructor(groupName) {
    super(groupName);
    this._groupName = groupName;
    this._group = c.getGroup(this._groupName);
  }

  deleteUI() {
    $('div.layer[layerName="' + this._groupName + '"]').remove();
  }

  createUI(container) {
    super.createUI(container);

    // add some extra buttons
    //let viewButton = '<button class="ui mini icon button showGroupButton" groupName="' + this._groupName + '" data-content="Show Group Layers">';
    //viewButton += '<i class="folder outline icon"></i></button>';
    //viewButton = $(viewButton);
    //$('div.layer[layerName="' + this._groupName + '"]').append(viewButton);

    // bindings
    var self = this;
    //viewButton.click(function() {
      // tell the layer select panel to update its stuff
    //  let layers = {};
    //  for (let l in self._group.affectedLayers) {
    //    layers[self._group.affectedLayers[l]] = {};
    //    layers[self._group.affectedLayers[l]][adjType.OPACITY] = [{ 'param' : 'opacity', 'val': 1 }];
    //  }

    //  g_layerSelector._paramSelectPanel.layers = layers;
    //});
    //viewButton.popup();

    if (!this.readOnly) {
      let deleteButton = '<button class="ui mini red icon button deleteGroupButton" groupName="' + this._groupName + '" data-content="Delete Group">';
      deleteButton += '<i class="remove icon"></i></button>';
      deleteButton = $(deleteButton);
      container.find('div.layer[layerName="' + this._groupName + '"]').append(deleteButton);

      deleteButton.click(function() {
        removeMetaGroup(self._groupName);
        g_layerSelector._paramSelectPanel.updateGroupModMenus();
      });
      deleteButton.popup();
    }

    container.find('h3.header').click(function() {
      g_groupPanel.displaySelectedLayers(c.getGroup(self._groupName).affectedLayers);
    });
  }

  get readOnly() {
    return this._group.readOnly;
  }

  get affectedLayers() {
    return this._group.affectedLayers;
  }

  set affectedLayers(layers) {
    if (!this.readOnly) {
      c.setGroupLayers(this._groupName, layers);
      this._group = c.getGroup(this._groupName);
    }
  }

  addLayers(layers) {
    if (!this.readOnly) {
      for (let l in layers) {
        c.addLayerToGroup(layers[l], this._groupName);
      }
      this._group = c.getGroup(this._groupName);
    }
  }

  removeLayers(layers) {
    if (!this.readOnly) {
      for (let l in layers) {
        c.removeLayerFromGroup(layers[l], this._groupName);
      }
      this._group = c.getGroup(this._groupName);
    }
  }
}

exports.Slider = Slider;
exports.MetaSlider = MetaSlider;
exports.Sampler = Sampler;
exports.OrderedSlider = OrderedSlider;
exports.ColorPicker = ColorPicker;
exports.SliderSelector = SliderSelector;
exports.LayerSelector = LayerSelector;
exports.FilterMenu = FilterMenu;
exports.GroupControls = GroupControls;
exports.GroupPanel = GroupPanel;
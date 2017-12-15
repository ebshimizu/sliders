// note: not sure if comp is actually required here since it should be in scope from
// earlier included scripts
// speaking of "scope" in javascript, blindly calling functions that affect the global
// ui state and the global compositor object (c) is totally ok here
const comp = require('../native/build/Release/compositor');

const rankModes = {
  "alpha": 0,
  "visibilityDelta": 1,
  "specVisibilityDelta" : 2
}

const mouseMode = {
  "normal": 0,
  "updateOnHover": 1,
  "contextClick" : 2
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

    this.initCanvas();
    this.initUI();
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
    // placed in the imgStatusBar div
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
    //this._optionUIList.append(selectionMode);

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

    // cull and display
    var displayLayers = [];
    for (var i = 0; i < rank.length; i++) {
      if (rank[i].score > this._rankThreshold) {
        displayLayers.push(rank[i]);
      }
    }
    return displayLayers;
  }

  selectLayers() {
    var displayLayers = this.rankLayers();

    this.showLayers(displayLayers);
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
    container.append(this._uiElem);
    this.bindEvents();
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
      var visible = self._layer.visible();

      // i think modifiers is in the global scope so it should still be ok here
      // it still sucks there should be an actual object managing this
      self._layer.visible(!visible && modifiers[self.name].groupVisible);
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
        this.bindParam("shadow B", (this.laye.getAdjustment(adjType.COLOR_BALANCE).shadowB - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
        this.bindParam("mid R", (this.laye.getAdjustment(adjType.COLOR_BALANCE).midR - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
        this.bindParam("mid G", (this.laye.getAdjustment(adjType.COLOR_BALANCE).midG - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
        this.bindParam("mid B", (this.laye.getAdjustment(adjType.COLOR_BALANCE).midB - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
        this.bindParam("highlight R", (this.laye.getAdjustment(adjType.COLOR_BALANCE).highR - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
        this.bindParam("highlight G", (this.laye.getAdjustment(adjType.COLOR_BALANCE).highG - 0.5) * 2, sectionName, type,
          { "range": false, "max": 1, "min": -1, "step": 0.01 });
        this.bindParam("highlight B", (this.laye.getAdjustment(adjType.COLOR_BALANCE).highB - 0.5) * 2, sectionName, type,
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
      // update the modifiers and compute acutual value
      modifiers[this._name].opacity = ui.value / 100;
      this._layer.opacity((ui.value / 100) * (modifiers[this._name].groupOpacity / 100));
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

exports.Slider = Slider;
exports.MetaSlider = MetaSlider;
exports.Sampler = Sampler;
exports.OrderedSlider = OrderedSlider;
exports.ColorPicker = ColorPicker;
exports.SliderSelector = SliderSelector;
exports.LayerSelector = LayerSelector;
const comp = require('../native/build/Release/compositor');

class Slider {
  constructor(name, param, type) {
    this.slider = new comp.Slider(name, param, type);
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

  setVal(dat) {
    if ("val" in dat) {
      var newContext = this.slider.setVal(dat.val, dat.context);
      c.setContext(newContext);
      updateLayercontrols();
    }
    else {
      this.slider.setVal(dat.context);
    }
  }

  // constructs the ui element, inserts it into the container, and binds the proper events
  createUI(container) {
    var html = '<div class="ui item">';
    html += '<div class="content">';
    html += '<div class="header">' + this.displayName + '</div>';
    html += '<div class="parameter" sliderName="' + this.displayName + '">';
    html += '<div class="paramLabel">' + this.displayName+ '</div>';
    html += '<div class="paramSlider" sliderName="' + this.displayName + '"></div>';
    html += '<div class="paramInput ui inverted transparent input" sliderName="' + this.displayName + '"><input type="text"></div>';
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
      //stop: function (event, ui) { highLevelSliderChange(name, ui); },
      //slide: function (event, ui) { highLevelSliderChange(name, ui); },
      change: function (event, ui) { self.sliderCallback(ui); }
    });

    $(i).val(String(this.value));

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
    $(i).val(String(ui.value.toFixed(3)))

    this.setVal({ val: ui.value, context: c.getContext() });
  }
}

exports.Slider = Slider;
var fs = require('fs');
var process = require('process');

function inherits(target, source) {
  for (var k in source.prototype)
    target.prototype[k] = source.prototype[k];
}

var test = require('../debug_build/Release/compositor'), events = require('events')
inherits(test.Compositor, events);

console.log('loading composition...');
var c = new test.Compositor("dance_no_opt.dark", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/canonical/architekt2/");
//c.addLayer("background", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/bird3/test/Background.png");
//c.addLayer("base", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/bird3/test/Base Color.png");
//c.addLayer("main", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/bird3/test/Main Photo Visibility.png");
console.log('Load complete');

console.log('Test Render');
c.render(c.getContext()).save('test.png');
console.log('Test Render complete');

const NS_PER_SEC = 1e9;
var times = [];
var total = 0;

for (let i = 0; i < 10; i++) {
  let time = process.hrtime();
  c.render(c.getContext());
  let diff = process.hrtime(time);
  times.push(diff[0] * NS_PER_SEC + diff[1]);
  total += times[i];
  console.log('Run ' + i + ' time ' + times[i] / 1e9 + 's');
}

console.log('Average runtime: ' + (total / 10) / 1e9 + 's');

//var s = new test.MetaSlider("hello");

//var m = new test.Model(c);

//m.addSliderFromExamples("test", [
//  "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/watercolor/wc_weak_1.dark",
//  "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/watercolor/wc_weak_2.dark",
//  "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/watercolor/wc_moderate_1.dark",
//  "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/watercolor/wc_moderate_2.dark",
//  "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/watercolor/wc_strong_1.dark",
//  "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/axis_defs/watercolor/wc_strong_2.dark"
//]);

//m.addSchema("C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/misc_actions/sf/sf_schema.json");

//var sample1 = m.schemaSample(c.getContext(), [{ "mode": 0, "axis": "Watercolor Strength", "min": 0.5, "max": 1 }, { "mode": 0, "axis": "Sketchy Lines", "min": 0.5, "max": 1 } ]);
//c.render(sample1).save("sample1.png");

//var img2 = c.render();

// save images
//img2.save("img2.png");

//for (var i = 0; i < 100; i++) {
//  var error = img1.MSSIM(img2, 16, 0, 0, 1);
//  console.log(error);
//}

//var maskColors = new test.Image("../debug/colors.png");
//var maskFixed = new test.Image("../debug/fixed.png");

//var settings = {
//    "maxFailures": 25,
//    "crossoverChance": 0.2,
//    "crossoverRate": 0.2,
//    "mutationRate" : 0.35
//}

//c.startSearch(5, settings, 1, "thumb");

//while (1) {
//}

//c.setMaskLayer("colors", 1, maskColors.width(), maskColors.height(), maskColors.base64());
//c.setMaskLayer("fixed", 2, maskFixed.width(), maskFixed.height(), maskFixed.base64());

//c.getPixelConstraints({ "detailedLog": true });
//c.ceresToContext("C:/Users/eshimizu/Documents/sliders_project/sliders/app/darkroom/codegen/ceres_result.json")

//var testImg = c.setMaskLayer("2", 250, 250, c.getLayer("2").image().base64());

//c.computeExpContext(c.getContext(), 164, 139, "ceresFunc");
//fs.createReadStream('ceresFunc.h').pipe(fs.createWriteStream('../src/ceresFunc.h'));
//c.paramsToCeres(c.getContext(), [{ "x": 184, "y": 184 }], [{ "r": 1, "g": 0, "b": 0 }], [1], "ceres.json");
//var ctx = c.ceresToContext("../../../ceres_harness/ceres_results.json");
//c.renderContext(ctx).save("test.png");

//c.render().save("test.png");
//c.renderPixel(c.getContext(), 1, 1);
//test.runAllTest(c, "results.png");
//test.runTest(c, 150, 150);
var fs = require('fs');

function inherits(target, source) {
  for (var k in source.prototype)
    target.prototype[k] = source.prototype[k];
}

var test = require('../debug_build/Debug/compositor'), events = require('events')
inherits(test.Compositor, events);

var c = new test.Compositor()
c.addLayer("1", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/shapes/Ellipse 1.png")
c.addLayer("2", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/shapes/Rectangle 1.png")
c.addLayer("3", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/test_images/shapes/Ellipse 2.png")

//c.getLayer("2").opacity(0);

var img1 = c.render();

//c.getLayer("3").opacity(0);
//c.getLayer("2").opacity(0);
c.getLayer("1").opacity(0);

var img2 = c.render();

// save images
img1.save("img1.png");
img2.save("img2.png");

for (var i = 0; i < 100; i++) {
  var error = img1.MSSIM(img2, 16, 0, 0, 1);
  console.log(error);
}

//var maskColors = new test.Image("../debug/colors.png");
//var maskFixed = new test.Image("../debug/fixed.png");

var settings = {
    "maxFailures": 25,
    "crossoverChance": 0.2,
    "crossoverRate": 0.2,
    "mutationRate" : 0.35
}

//c.startSearch(5, settings, 1, "thumb");

while (1) {
}

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
function inherits(target, source) {
  for (var k in source.prototype)
    target.prototype[k] = source.prototype[k];
}

var test = require('../debug_build/Debug/compositor'), events = require('events')
inherits(test.Compositor, events);

var c = new test.Compositor()
c.addLayer("1", "C:/Users/eshimizu/Dropbox/Documents/research/sliders_project/test_images/shapes/Ellipse 1.png")
c.addLayer("2", "C:/Users/eshimizu/Dropbox/Documents/research/sliders_project/test_images/shapes/Rectangle 1.png")
c.addLayer("3", "C:/Users/eshimizu/Dropbox/Documents/research/sliders_project/test_images/shapes/Ellipse 2.png")

c.startSearch(0, { "useVisibleLayersOnly": 1 }, 1, "");
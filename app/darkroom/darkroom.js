const comp = require('../native/build/Release/compositor')

// initializes a global compositor object to operate on
var c = new comp.Compositor()

// for testing we load up three solid color test images
c.addLayer("red", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/sliders/app/native/debug_images/solid_red.png")
c.addLayer("green", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/sliders/app/native/debug_images/solid_green.png")
c.addLayer("blue", "C:/Users/falindrith/Dropbox/Documents/research/sliders_project/sliders/app/native/debug_images/solid_blue.png")

// Initializes html element listeners on document load
function init() {
	$("#render").on("click", function() {
		renderImage()
	})


	$('.paramSlider').slider({
		orientation: "horizontal",
		range: "min",
		max: 100,
		min: 0
	});

}

// Test function that loads an image from the compositor module and
// then puts it in an image tag
function renderImage() {
	var dat = 'data:image/png;base64,' + c.render().base64()
	$("#test").html('<img src="' + dat + '"" />')
}
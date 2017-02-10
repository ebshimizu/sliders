const comp = require('../native/build/Release/compositor')
var img = new comp.Image('C:/Users/falindrith/Dropbox/Documents/research/sliders_project/sliders/app/native/test.png')

// Initializes html element listeners on document load
function addListeners() {
	$("#load").on("click", function() {
		loadImage()
	})
}

// Test function that loads an image from the compositor module and
// then puts it in an image tag
function loadImage() {
	var dat = 'data:image/png;base64,' + img.getBase64()
	$("#test").html('<img src="' + dat + '"" />')
}
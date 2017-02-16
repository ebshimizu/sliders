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

	initUI()
}

// Test function that loads an image from the compositor module and
// then puts it in an image tag
function renderImage() {
	var dat = 'data:image/png;base64,' + c.render().base64()
	$("#test").html('<img src="' + dat + '"" />')
}

function initUI() {
	// for now we assume the compositor is already initialized for testing purposes
	var layers = c.getLayerNames()

	for (var layer in layers) {
		createLayerControl(layers[layer])
	}
}

function createLayerControl(name) {
	var controls = $('#layerControls')
	var layer = c.getLayer(name)

	// create the html
	var html = '<div class="layer" layerName="' + name + '">'
	html += '<h3 class="ui grey inverted header">' + name + '</h3>'

	if (layer.visible()) {
		html += '<button class="ui icon button mini white visibleButton" layerName="' + name + '">'
		html += '<i class="unhide icon"></i>'
	}
	else {
		html += '<button class="ui icon button mini black visibleButton" layerName="' + name + '">'
		html += '<i class="hide icon"></i>'
	}

	html += '</button>'

	// generate parameters
	if (layer.blendMode() == 0) {
		// normal blending
		html += createLayerParam(name, "opacity")
	}

	html += '</div>'

	controls.append(html)

	// connect events
	// update input
	$('.paramInput[layerName="' + name + '"][paramName="opacity"] input').val(String(layer.opacity()))

	// input box events
	$('.paramInput[layerName="' + name + '"][paramName="opacity"] input').blur(function() {
		var data = parseFloat($(this).val());
		$('.paramSlider[layerName="' + name + '"][paramName="opacity"]').slider("value", data);
		renderImage();
	});
	$('.paramInput[layerName="' + name + '"][paramName="opacity"] input').keydown(function(event) {
		if (event.which != 13)
			return;

		var data = parseFloat($(this).val());
		$('.paramSlider[layerName="' + name + '"][paramName="opacity"]').slider("value", data);
		renderImage();
	});


	// visibility
	$('button[layerName="' + name + '"]').on('click', function() {
		// check status of button
		var visible = layer.visible()

		layer.visible(!visible)

		var button = $('button[layerName="' + name + '"]')
		
		if (layer.visible()) {
			button.html('<i class="unhide icon"></i>')
			button.removeClass("black")
			button.addClass("white")
		}
		else {
			button.html('<i class="hide icon"></i>')
			button.removeClass("white")
			button.addClass("black")
		}

		// trigger render after adjusting settings
		renderImage()
	})

	// param events
	$('.paramSlider[layerName="' + name + '"][paramName="opacity"]').slider({
		orientation: "horizontal",
		range: "min",
		max: 100,
		min: 0,
		step: 0.1,
		value: layer.opacity(),
		stop: function(event, ui) {
			handleParamChange(name, ui)
			renderImage()
		},
		slide: function(event, ui) { handleParamChange(name, ui) },
		change: function(event, ui) { handleParamChange(name, ui) },
	});
}

function createLayerParam(layerName, param) {
	var html = '<div class="parameter" layerName="' + layerName + '" paramName="' + param + '">'

	html += '<div class="paramLabel">' + param + '</div>'
	html += '<div class="paramSlider" layerName="' + layerName + '" paramName="' + param + '"></div>'
	html += '<div class="paramInput ui inverted transparent input" layerName="' + layerName + '" paramName="' + param + '"><input type="text"></div>'

	html += '</div>'

	return html
}

function handleParamChange(layerName, ui) {
	// hopefully ui has the name of the param somewhere
	var paramName = $(ui.handle).parent().attr("paramName")

	if (paramName == "opacity") {
		c.getLayer(layerName).opacity(ui.value);

		// find associated value box and dump the value there
		$(ui.handle).parent().next().find("input").val(String(ui.value));
	}
}
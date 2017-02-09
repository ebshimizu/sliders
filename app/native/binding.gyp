{
	"targets" : [
		{
			"target_name" : "compositor",
			"sources" : [
				"src/CompositorNode.h",
				"src/CompositorNode.cpp", 
				"src/Compositor.cpp",
				"src/Compositor.h",
				"src/Image.h",
				"src/Image.cpp",
				"src/Logger.h",
				"src/Logger.cpp",
				"src/third_party/lodepng/lodepng.cpp",
				"src/third_party/cpp-base64/base64.cpp",
				"src/addon.cpp"
			],
			"include_dirs" : [
			    "<!(node -e \"require('nan')\")"
			]
		}
	]
}
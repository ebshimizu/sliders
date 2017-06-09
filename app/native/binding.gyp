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
				"src/third_party/stb_image_resize.h",
				"src/third_party/json/src/json.hpp",
		        "src/addon.cpp",
		        "src/Layer.h",
		        "src/Layer.cpp",
		        "src/util.h",
		        "src/util.cpp",
				"src/testHarness.h",
				"src/testHarness.cpp",
				"src/ceresFunctions.h",
				"../../expression_tree/expressionContext.h"
			],
			"include_dirs" : [
			    "<!(node -e \"require('nan')\")"
			],
			"defines" : [ "NOMINMAX" ]
		}
	]
}

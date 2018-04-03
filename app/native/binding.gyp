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
				"src/constraintData.h",
				"src/constraintData.cpp",
				"src/SearchData.cpp",
				"src/searchData.h",
				"src/Histogram.cpp",
				"src/Histogram.h",
				"src/Model.h",
				"src/Model.cpp",
				"src/third_party/libsvm/svm.h",
				"src/third_party/libsvm/svm.cpp",
				"src/gibbs_with_gaussian_mixture.cpp",
				"src/gibbs_with_gaussian_mixture.h",
				"src/DimRed.cpp",
				"src/DimRed.h",
				"../../expression_tree/expressionContext.h",
				"../../expression_tree/expressionStep.h",
				"src/ClickMap.h",
				"src/ClickMap.cpp",
                "src/Selection.h",
                "src/Selection.cpp",
                "src/third_party/flann/src/cpp/flann/flann.hpp"
			],
			"include_dirs" : [
			    "<!(node -e \"require('nan')\")",
			    "src/third_party/flann/src/cpp"
			],
			"defines" : [ "NOMINMAX" ]
		}
	]
}

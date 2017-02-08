#include "Image.h"

namespace Comp {
  Image::Image(unsigned int w, unsigned int h) : _w(w), _h(h), _filename("")
  {
  }

  Image::Image(string filename)
  {
    loadFromFile(filename);
  }

  void Image::loadFromFile(string filename)
  {
    unsigned int error = lodepng::decode(_data, _w, _h, filename.c_str());

    if (error) {
      getLogger()->log("Error loading file " + filename + " as png. Error: " + lodepng_error_text(error), LogLevel::ERR);
    }
    else {
      getLogger()->log("Loaded " + filename);
    }
  }
}

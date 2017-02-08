#include "Image.h"

namespace Comp {
  Image::Image(unsigned int w, unsigned int h) : _w(w), _h(h), _filename("")
  {
  }

  Image::Image(string filename)
  {
    loadFromFile(filename);
  }

  Image::~Image()
  {
  }

  vector<unsigned char>& Image::getData()
  {
    return _data;
  }

  string Image::getBase64()
  {
    // convert raw pixels to png
    vector<unsigned char> png;

    unsigned int error = lodepng::encode(png, _data, _w, _h);

    // convert to base64 string
    return base64_encode(png.data(), png.size());
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

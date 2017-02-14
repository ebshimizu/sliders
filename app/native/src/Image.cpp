#include "Image.h"

namespace Comp {
  Image::Image(unsigned int w, unsigned int h) : _w(w), _h(h), _filename("")
  {
    _data = vector<unsigned char>(w * h * 4, 0);
  }

  Image::Image(string filename)
  {
    loadFromFile(filename);
  }

  Image::Image(const Image & other)
  {
    _w = other._w;
    _h = other._h;
    _data = other._data;
    _filename = other._filename;
  }

  Image & Image::operator=(const Image & other)
  {
    // self assignment check
    if (&other == this)
      return *this;

    _w = other._w;
    _h = other._h;
    _data = other._data;
    _filename = other._filename;
    return *this;
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
    return base64_encode(png.data(), (unsigned int)png.size());
  }

  unsigned int Image::numPx()
  {
    return _w * _h;
  }

  void Image::save(string path)
  {
    unsigned int error = lodepng::encode(path, _data, _w, _h);

    if (error) {
      getLogger()->log("Error writing image to file " + path + ". Error: " + lodepng_error_text(error), LogLevel::ERR);
    }
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

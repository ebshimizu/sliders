#include "Image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "third_party/stb_image_resize.h"

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

  void Image::save()
  {
    save("./" + _filename);
  }

  string Image::getFilename()
  {
    return _filename;
  }

  shared_ptr<Image> Image::resize(unsigned int w, unsigned int h)
  {
    shared_ptr<Image> scaled = shared_ptr<Image>(new Image(w, h));
    stbir_resize_uint8(_data.data(), _w, _h, 0, scaled->_data.data(), w, h, 0, 4);

    return scaled;
  }

  shared_ptr<Image> Image::resize(float scale)
  {
    return resize((unsigned int)(_w * scale), (unsigned int)(_h * scale));
  }

  void Image::reset(float r, float g, float b)
  {
    for (int i = 0; i < _data.size() / 4; i++) {
      _data[i * 4] = (unsigned char)(r * 255);
      _data[i * 4 + 1] = (unsigned char)(g * 255);
      _data[i * 4 + 2] = (unsigned char)(b * 255);
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

      // save the last part as the filename
      size_t pos = filename.rfind('/');
      if (pos == string::npos) {
        pos = filename.rfind('\\');
        if (pos == string::npos) {
          _filename = filename;
        }
      }

      _filename = filename.substr(pos + 1);
    }
  }
}

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

  Image::Image(unsigned int w, unsigned int h, string & data)
  {
    _w = w;
    _h = h;

    string decoded = base64_decode(data);
    vector<unsigned char> pngData = vector<unsigned char>(decoded.begin(), decoded.end());

    unsigned int error = lodepng::decode(_data, _w, _h, pngData);

    if (error) {
      getLogger()->log("Error interpreting base64 data. Potentitally fatal.", Comp::ERR);
    }
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

  RGBAColor Image::getPixel(int index)
  {
    // return black instead of dying for out of bounds
    if (index < 0 || index > _data.size() / 4) {
      // but also log
      getLogger()->log("Attempt to access out of bound pixel", Comp::WARN);
      return RGBAColor();
    }

    RGBAColor c;
    c._r = _data[index * 4] / 255.0f;
    c._g = _data[index * 4 + 1] / 255.0f;
    c._b = _data[index * 4 + 2] / 255.0f;
    c._a = _data[index * 4 + 3] / 255.0f;

    return c;
  }

  RGBAColor Image::getPixel(int x, int y)
  {
    return getPixel(x + y * _w);
  }

  bool Image::pxEq(int x1, int y1, int x2, int y2)
  {
    int index1 = (x1 + y1 * _w) * 4;
    int index2 = (x2 + y2 * _w) * 4;

    return (_data[index1] == _data[index2] &&
      _data[index1 + 1] == _data[index2 + 1] &&
      _data[index1 + 2] == _data[index2 + 2] &&
      _data[index1 + 3] == _data[index2 + 3]);
  }

  void Image::setPixel(int x, int y, float r, float g, float b, float a)
  {
    int index = (x + y * _w )* 4;
    _data[index] = (unsigned char)(r * 255);
    _data[index + 1] = (unsigned char)(g * 255);
    _data[index + 2] = (unsigned char)(b * 255);
    _data[index + 3] = (unsigned char)(a * 255);
  }

  void Image::setPixel(int x, int y, RGBAColor color)
  {
    setPixel(x, y, color._r, color._g, color._b, color._a);
  }

  int Image::initExp(ExpContext& context, string name, int index, int pxPos)
  {
    RGBAColor c = getPixel(pxPos);
    _vars._r = context.registerParam(ParamType::LAYER_PIXEL, name + "_r", c._r);
    _vars._g = context.registerParam(ParamType::LAYER_PIXEL, name + "_g", c._g);
    _vars._b = context.registerParam(ParamType::LAYER_PIXEL, name + "_b", c._b);
    _vars._a = context.registerParam(ParamType::LAYER_PIXEL, name + "_a", c._a);

    return index + 4;
  }

  Utils<ExpStep>::RGBAColorT & Image::getPixel()
  {
    return _vars;
  }

  double Image::structDiff(Image * y)
  {
    // dimension mismatch means images can't be compared
    if (y->getWidth() != getWidth() || y->getHeight() != getHeight())
      return DBL_MAX;

    // the idea here is that image y is similar to image A (this) if there exists
    // a linear transformation x s.t. Ax = y. this is basically a contrast/brightness
    // operation. Values in A are derived from the pixel values of this image.

    // set up the proper matrices
    auto& yData = y->getData();

    Eigen::VectorXd b;
    b.resize(yData.size() / 4);

    for (int i = 0; i < yData.size() / 4; i++) {
      // premult color n stuff
      double r = (yData[i * 4] / 255.0) * (yData[i * 4 + 3] / 255.0);
      double g = (yData[i * 4 + 1] / 255.0) * (yData[i * 4 + 3] / 255.0);
      double bl = (yData[i * 4 + 2] / 255.0) * (yData[i * 4 + 3] / 255.0);

      // want L channel of Lab
      Utils<double>::LabColorT Lab = Utils<double>::RGBToLab(r, g, bl);

      // construct b
      b[i] = Lab._L;
    }

    Eigen::MatrixX2d A;
    A.resize(yData.size() / 4, Eigen::NoChange);
    double avg = 0;

    // A is a bit annoying since we need the average L before assigning values
    for (int i = 0; i < _data.size() / 4; i++) {
      double r = (_data[i * 4] / 255.0) * (_data[i * 4 + 3] / 255.0);
      double g = (_data[i * 4 + 1] / 255.0) * (_data[i * 4 + 3] / 255.0);
      double bl = (_data[i * 4 + 2] / 255.0) * (_data[i * 4 + 3] / 255.0);

      // want L channel of Lab
      Utils<double>::LabColorT Lab = Utils<double>::RGBToLab(r, g, bl);
      A(i, 0) = Lab._L;
      A(i, 1) = 1;
      avg += Lab._L;
    }

    avg /= (_data.size() / 4);

    // subtract the average from column 0
    for (int i = 0; i < _data.size() / 4; i++) {
      A(i, 0) = A(i, 0) - avg;
    }

    // solve
    Eigen::Vector2d x = A.colPivHouseholderQr().solve(b);

    double error = (A*x - b).norm() / b.norm();
    return error;
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

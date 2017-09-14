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
    if (index < 0 || index >= _data.size() / 4) {
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

  vector<Eigen::VectorXd> Image::patches(int patchSize)
  {
    vector<Eigen::VectorXd> patch;

    // starts in top left, proceeds until dimensions run out.
    for (int y = 0; y < getHeight(); y += patchSize) {
      for (int x = 0; x < getWidth(); x += patchSize) {
        vector<double> luma;
        int count = 0;

        // grab the patch.
        for (int j = 0; j < patchSize; j++) {
          int yloc = y + j;

          for (int i = 0; i < patchSize; i++) {
            int xloc = x + i;

            if (xloc >= getWidth() || yloc >= getHeight())
              continue;

            // premult color n stuff
            RGBAColor c = getPixel(xloc, yloc);

            // want L channel of Lab
            Utils<double>::LabColorT Lab = Utils<double>::RGBToLab(c._r * c._a, c._g * c._a, c._b * c._a);

            // construct b
            luma.push_back(Lab._L);
            count++;
          }
        }

        Eigen::VectorXd p;
        p.resize(count);
        for (int i = 0; i < count; i++)
          p[i] = luma[i];

        //stringstream ss;
        //ss << "Patch at origin (" << x << "," << y << "): " << p;
        //getLogger()->log(ss.str());

        patch.push_back(p);
      }
    }

    return patch;
  }

  double Image::structBinDiff(Image * y, int patchSize, double threshold)
  {
    vector<double> diffs = structIndBinDiff(y, patchSize);
    int count = 0;
    for (auto& d : diffs) {
      if (d > threshold)
        count++;
    }

    return count;
  }

  double Image::structTotalBinDiff(Image * y, int patchSize)
  {
    vector<double> diffs = structIndBinDiff(y, patchSize);
    double sum = 0;
    for (auto& d : diffs) {
      sum += d;
    }

    return sum;
  }

  double Image::structAvgBinDiff(Image * y, int patchSize)
  {
    vector<double> diffs = structIndBinDiff(y, patchSize);
    double avg = 0;
    for (auto& d : diffs) {
      avg += d;
    }

    return avg / diffs.size();
  }

  vector<double> Image::structIndBinDiff(Image * y, int patchSize)
  {
    vector<Eigen::VectorXd> xBins = patches(patchSize);
    vector<Eigen::VectorXd> yBins = y->patches(patchSize);

    vector<double> means;
    vector<double> results;

    // xBins needs the average
    for (auto& b : xBins) {
      double avg = 0;
      for (int i = 0; i < b.size(); i++) {
        avg += b[i];
      }
      means.push_back(avg / b.size());
    }

    vector<double> diffs;
    for (int i = 0; i < xBins.size(); i++) {
      Eigen::MatrixX2d A;
      A.resize(xBins[i].size(), Eigen::NoChange);
      for (int j = 0; j < xBins[i].size(); j++) {
        A(j, 0) = xBins[i][j] - means[i];
        A(j, 1) = 1;
      }

      Eigen::Vector2d x = A.colPivHouseholderQr().solve(yBins[i]);

      double res = (A*x - yBins[i]).norm();
      if (isnan(res))
        res = 0;
      results.push_back(res);

      //if (yBins[i].norm() == 0)
        // should this be 0 instead?
      //  results.push_back((A*x - yBins[i]).norm());
      //else
      //  results.push_back((A*x - yBins[i]).norm() / yBins[i].norm());
    }

    return results;
  }

  double Image::MSSIM(Image * y, int patchSize, double a, double b, double g)
  {
    double mssim = 0;

    auto xp = patches(patchSize);
    auto yp = y->patches(patchSize);

    for (int i = 0; i < xp.size(); i++) {
      double ssim = SSIMBinDiff(xp[i], yp[i], a, b, g);
      getLogger()->log("ssim bin " + to_string(i) + ": " + to_string(ssim));
      mssim += ssim;
    }

    return mssim / xp.size();
  }

  double Image::SSIMBinDiff(Eigen::VectorXd x, Eigen::VectorXd y, double a, double b, double g)
  {
    // unequal vectors are different
    if (x.size() != y.size())
      return 0;

    // compute means
    double ux = 0;
    double uy = 0;
    
    for (int i = 0; i < x.size(); i++) {
      ux += x[i];
      uy += y[i];
    }

    ux /= x.size();
    uy /= y.size();


    double sx = 0;
    double sy = 0;
    double sxy = 0;

    // variance/covariance
    for (int i = 0; i < x.size(); i++) {
      sx += (x[i] - ux) * (x[i] - ux);
      sy += (y[i] - uy) * (y[i] - uy);
      sxy += (x[i] - ux) * (y[i] - uy);
    }

    sx = sqrt(sx / (x.size() - 1));
    sy = sqrt(sy / (y.size() - 1));
    sxy /= (x.size() - 1);

    // stability constants
    double c1 = 1e-3;
    double c2 = 3e-3;
    double c3 = c2 / 2;

    // luma
    double l = (2 * ux * uy + c1) / (ux * ux + uy * uy + c1);

    // contrast
    double c = (2 * sx * sy + c2) / (sx * sx + sy * sy + c2);

    // structure
    double s = (sxy + c3) / (sx * sy + c3);

    double ssim = pow(l, a) * pow(c, b) * pow(s, g);
    return ssim;
  }

  void Image::eliminateBins(vector<Eigen::VectorXd>& bins, int patchSize, double threshold)
  {
    // bins may have some stuff missing so we don't just call structIndBinDiff
    // as usual
    vector<Eigen::VectorXd> xBins = patches(patchSize);

    for (int i = 0; i < bins.size(); i++) {
      // skip eliminated bins
      if (bins[i].size() == 0)
        continue;

      double mean = 0;

      for (int j = 0; j < xBins[i].size(); j++) {
        mean += xBins[i][j];
      }
      mean /= xBins[i].size();

      Eigen::MatrixX2d A;
      A.resize(xBins[i].size(), Eigen::NoChange);

      for (int j = 0; j < xBins[i].size(); j++) {
        A(j, 0) = xBins[i][j] -mean;
        A(j, 1) = 1;
      }

      Eigen::Vector2d x = A.fullPivLu().solve(bins[i]);

      double res = (A * x - bins[i]).norm() / bins[i].norm();

      //stringstream ss;
      //getLogger()->log("Bin " + to_string(i) + " diff " + to_string(res));
      //ss << "b:\n" << bins[i] << "\nReconstruct:\n" << A * x;
      //getLogger()->log("ssim for bin " + to_string(i) + ": " + to_string(res));

      if (isnan(res))
        res = 0;

      // ssim has 1 = similar, not 0
      if (res > threshold) {
        bins[i] = Eigen::VectorXd();
      }
    }
  }

  void Image::eliminateBinsSSIM(vector<Eigen::VectorXd>& bins, int patchSize, double threshold,
    double a, double b, double g)
  {
    // bins may have some stuff missing so we don't just call structIndBinDiff
    // as usual
    vector<Eigen::VectorXd> xBins = patches(patchSize);

    for (int i = 0; i < bins.size(); i++) {
      // skip eliminated bins
      if (bins[i].size() == 0)
        continue;

      // lets try ssim

      //double mean = 0;

      //for (int j = 0; j < xBins[i].size(); j++) {
      //  mean += xBins[i][j];
      //}
      //mean /= xBins[i].size();

      //Eigen::MatrixX2d A;
      //A.resize(xBins[i].size(), Eigen::NoChange);

      //for (int j = 0; j < xBins[i].size(); j++) {
      //  A(j, 0) = xBins[i][j];// -mean;
      //  A(j, 1) = 1;
      //}

      //Eigen::Vector2d x = A.fullPivLu().solve(bins[i]);

      //double res = (A * x - bins[i]).norm() / bins[i].norm();
      double res = SSIMBinDiff(bins[i], xBins[i], a, b, g);


      //stringstream ss;
      //getLogger()->log("Bin " + to_string(i) + " diff " + to_string(res));
      //ss << "b:\n" << bins[i] << "\nReconstruct:\n" << A * x;
      //getLogger()->log("ssim for bin " + to_string(i) + ": " + to_string(res));

      if (isnan(res))
        res = 0;

      // ssim has 1 = similar, not 0
      if (res > threshold) {
        bins[i] = Eigen::VectorXd();
      }
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

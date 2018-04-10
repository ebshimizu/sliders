#include "Image.h"
#include "Histogram.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "third_party/stb_image_resize.h"

namespace Comp {
  Image::Image(unsigned int w, unsigned int h) : _w(w), _h(h), _filename("")
  {
    _data = vector<unsigned char>(w * h * 4, 0);
    _renderLayerMap = vector<string>(w * h);
  }

  Image::Image(string filename)
  {
    loadFromFile(filename);
    _renderLayerMap = vector<string>(_w * _h);
    analyze();
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

    _renderLayerMap = vector<string>(_w * _h);

    analyze();
  }

  Image::Image(const Image & other)
  {
    _w = other._w;
    _h = other._h;
    _data = other._data;
    _filename = other._filename;
    _totalAlpha = other._totalAlpha;
    _totalLuma = other._totalLuma;
    _avgAlpha = other._avgAlpha;
    _avgLuma = other._avgLuma;
    _renderLayerMap = other._renderLayerMap;
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
    _totalAlpha = other._totalAlpha;
    _totalLuma = other._totalLuma;
    _avgAlpha = other._avgAlpha;
    _avgLuma = other._avgLuma;
    _renderLayerMap = other._renderLayerMap;
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
    if (x < 0 || x >= _w || y < 0 || y >= _h)
      return;

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
      //getLogger()->log("ssim bin " + to_string(i) + ": " + to_string(ssim));
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

  double Image::histogramIntersection(Image * y, float binSize)
  {
    // assert image sizes are the same for this function
    if (y->_w != _w || y->_h != _h)
      return -1;

    // compute three separate histograms (1 per channel) for each image
    // then comput average intersection.
    // right now: this is premultiplied color, which may have to change later
    vector<shared_ptr<Histogram>> hx;
    vector<shared_ptr<Histogram>> hy;

    for (int i = 0; i < 3; i++) {
      hx.push_back(shared_ptr<Histogram>(new Histogram(binSize)));
      hy.push_back(shared_ptr<Histogram>(new Histogram(binSize)));
    }

    // create histograms
    unsigned char* xPx = _data.data();
    unsigned char* yPx = y->_data.data();

    for (int i = 0; i < _data.size() / 4; i++) {
      int idx = i * 4;
      float xa = xPx[idx + 3] / 255.0f;
      float ya = yPx[idx + 3] / 255.0f;

      hx[0]->add8BitPixel((unsigned char)(xPx[idx] * xa));
      hy[0]->add8BitPixel((unsigned char)(yPx[idx] * ya));
      hx[1]->add8BitPixel((unsigned char)(xPx[idx + 1] * xa));
      hy[1]->add8BitPixel((unsigned char)(yPx[idx + 1] * ya));
      hx[2]->add8BitPixel((unsigned char)(xPx[idx + 2] * xa));
      hy[2]->add8BitPixel((unsigned char)(yPx[idx + 2] * ya));
    }

    // diff
    double sum = 0;
    for (int i = 0; i < 3; i++) {
      sum += hx[i]->intersection(*hy[i]);
    }

    return sum / 3;
  }

  double Image::proportionalHistogramIntersection(Image * y, float binSize)
  {
    // compute three separate histograms (1 per channel) for each image
    // then comput average intersection.
    // right now: this is premultiplied color, which may have to change later
    vector<shared_ptr<Histogram>> hx;
    vector<shared_ptr<Histogram>> hy;

    for (int i = 0; i < 3; i++) {
      hx.push_back(shared_ptr<Histogram>(new Histogram(binSize)));
      hy.push_back(shared_ptr<Histogram>(new Histogram(binSize)));
    }

    // create histograms
    unsigned char* xPx = _data.data();
    unsigned char* yPx = y->_data.data();

    for (int i = 0; i < _data.size() / 4; i++) {
      int idx = i * 4;
      float xa = xPx[idx + 3] / 255.0f;

      hx[0]->add8BitPixel((unsigned char)(xPx[idx] * xa));
      hx[1]->add8BitPixel((unsigned char)(xPx[idx + 1] * xa));
      hx[2]->add8BitPixel((unsigned char)(xPx[idx + 2] * xa));
    }

    for (int i = 0; i < y->_data.size() / 4; i++) {
      int idx = i * 4;
      float ya = yPx[idx + 3] / 255.0f;

      hy[0]->add8BitPixel((unsigned char)(yPx[idx] * ya));
      hy[1]->add8BitPixel((unsigned char)(yPx[idx + 1] * ya));
      hy[2]->add8BitPixel((unsigned char)(yPx[idx + 2] * ya));
    }

    // diff
    double sum = 0;
    for (int i = 0; i < 3; i++) {
      sum += hx[i]->proportionalIntersection(*hy[i]);
    }

    return sum / 3;
  }

  void Image::analyze()
  {
    // in an attempt to quantify the "importance" of a layer, calculate some stats here
    // alpha / luma
    _totalAlpha = 0;
    _totalLuma = 0;

    for (int i = 0; i < _data.size() / 4; i++) {
      int idx = i * 4;
      float a = _data[i + 3] / 255.0f;

      auto Lab = Utils<float>::RGBToLab(a * (_data[i] / 255.0f), a* (_data[i + 1] / 255.0f), a* (_data[i + 2] / 255.0f));

      _totalAlpha += a;
      _totalLuma += Lab._L;
    }

    _avgAlpha = _totalAlpha / (_data.size() / 4);
    _avgLuma = _totalLuma / (_data.size() / 4);
    
    stringstream ss;
    ss << "Analysis complete\nTotal Alpha: " << _totalAlpha << "\nAverage Alpha: " << _avgAlpha << "\nTotal Luma: " << _totalLuma << "\nAverage Luma: " << _avgLuma;
    getLogger()->log(ss.str());
  }

  double Image::avgAlpha(int x, int y, int w, int h)
  {
    double avg = 0;

    // assumption: bounds are ok
    for (int j = y; j <= y + h; j++) {
      for (int i = x; i <= x + w; i++) {
        auto px = getPixel(i, j);
        avg += px._a;
      }
    }

    return avg / (w * h);
  }

  Image * Image::diff(Image * other)
  {
    // normalized basic diff visualization for an image
    // 3-channel (idk maybe it'll work)
    // input image is assumed to be the prior state
    Image* ret = new Image(_w, _h);
    vector<unsigned char>& diffs = ret->getData();
    vector<unsigned char> od = other->getData();

    for (int i = 0; i < _data.size() / 4; i++) {
      int idx = i * 4;

      // alpha
      diffs[idx + 3] = 255;

      diffs[idx] = (((int)_data[idx] - (int)od[idx]) / 2) + 127;
      diffs[idx + 1] = (((int)_data[idx + 1] - (int)od[idx + 1]) / 2) + 127;
      diffs[idx + 2] = (((int)_data[idx + 2] - (int)od[idx + 2]) / 2) + 127;
    }
    
    return ret;
  }

  Image* Image::fill(float r, float g, float b)
  {
    Image* ret = new Image(_w, _h);
    vector<unsigned char>& retData = ret->getData();

    for (int i = 0; i < _data.size() / 4; i++) {
      int idx = i * 4;

      retData[idx] = (unsigned char)(r * 255);
      retData[idx + 1] = (unsigned char)(g * 255);
      retData[idx + 2] = (unsigned char)(b * 255);
      retData[idx + 3] = _data[idx + 3];
    }

    return ret;
  }

  void Image::stroke(Image* inclusionMap, int size, RGBColor color)
  {
    // first assert same size
    if (inclusionMap->_w != _w || inclusionMap->_h != _h) {
      // caller needs to handle
      return;
    }
    // DEBUG
    inclusionMap->save("test.png");

    // copy image to return, since we are doing a direct modification
    //Image* ret = new Image(*this);
    RGBAColor fill;
    fill._a = 1;
    fill._r = color._r;
    fill._g = color._g;
    fill._b = color._b;

    // need to duplicate for original lookup
    Image* original = new Image(*this);

    // basically any pixel that borders an outside pixel gets the effect
    for (int y = 0; y < _h; y++) {
      for (int x = 0; x < _w; x++) {
        // conditions for stroke: current image pixel is non-zero alpha bordering zero-alpha pixel
        if (original->bordersZeroAlpha(x, y)) {
          // if true, then add stroke if the pixel also borders the outside of the current group
          if (original->bordersOutside(inclusionMap, x, y)) {
            int i = y * _w + x;
            int px = i * 4;

            // this isn't a complicated effect and is really just to demo this capability,
            // so we do a stroke in a n x n square aroud the specified pixel.
            for (int sy = -size; sy <= size; sy++) {
              for (int sx = -size; sx <= size; sx++) {
                if (sy == 0 && sx == 0)
                  continue;

                int dy = sy + y;
                int dx = sx + x;

                // bounds check
                if (dx < 0 || dx >= _w || dy < 0 || dy >= _h)
                  continue;

                // existing pixel check
                if (getPixel(dx, dy)._a > 0)
                  continue;

                // inclusion check
                if (inclusionMap->getPixel(dx, dy)._a < 1) {
                  setPixel(dx, dy, fill);
                }
              }
            }
          }
        }
      }
    }

    delete original;
  }

  float Image::chamferDistance(Image * y)
  {
    // assert same size
    if (_w != y->_w || _h != y->_h) {
      return FLT_MAX;
    }

    // this should probably subsample at some point but for now we'll do the slow thing
    // create vectors of lab colors for each of the pixels
    flann::Matrix<float> xp(new float[(_data.size() / 4) * 3], _data.size() / 4, 3);
    flann::Matrix<float> yp(new float[(_data.size() / 4) * 3], _data.size() / 4, 3);

    for (int i = 0; i < _data.size() / 4; i++) {
      RGBAColor xpixel = getPixel(i);
      RGBAColor ypixel = y->getPixel(i);

      LabColor xLab = Utils<float>::RGBAToLab(xpixel);
      LabColor yLab = Utils<float>::RGBAToLab(ypixel);

      xp[i][0] = xLab._L;
      xp[i][1] = xLab._a;
      xp[i][2] = xLab._b;

      yp[i][0] = yLab._L;
      yp[i][1] = yLab._a;
      yp[i][2] = yLab._b;
    }

    // construct flann index
    flann::Matrix<int> indices(new int[xp.rows], xp.rows, 1);
    flann::Matrix<float> dists(new float[xp.rows], xp.rows, 1);

    flann::Index<flann::L2<float>> index(yp, flann::KDTreeIndexParams(4));
    index.buildIndex();
    index.knnSearch(xp, indices, dists, 1, flann::SearchParams());

    // sum of closest point diffs
    float sum = 0;
    for (int i = 0; i < dists.rows; i++) {
      sum += dists[i][0];
    }

    return sum;
  }

  vector<string>& Image::getRenderMap()
  {
    return _renderLayerMap;
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

  bool Image::bordersOutside(Image * inclusionMap, int x, int y)
  {
    vector<int> xd = { -1, 0, 1 };
    vector<int> yd = { -1, 0, 1 };

    for (int i = 0; i < xd.size(); i++) {
      for (int j = 0; j < yd.size(); j++) {
        if (xd[i] == 0 && yd[i] == 0)
          continue;

        int xp = x + xd[i];
        int yp = y + yd[j];

        // bounds check
        if (xp < 0 || xp >= inclusionMap->_w)
          continue;
        if (yp < 0 || yp >= inclusionMap->_h)
          continue;

        // directional check
        auto pixel = inclusionMap->getPixel(xp, yp);
        if (pixel._a < 1)
          return true;
      }
    }

    return false;
  }

  bool Image::bordersZeroAlpha(int x, int y)
  {
    // if the pixel itself is zero alpha, return false
    if (getPixel(x, y)._a == 0)
      return false;

    vector<int> xd = { -1, 0, 1 };
    vector<int> yd = { -1, 0, 1 };

    for (int i = 0; i < xd.size(); i++) {
      for (int j = 0; j < yd.size(); j++) {
        if (xd[i] == 0 && yd[i] == 0)
          continue;

        int xp = x + xd[i];
        int yp = y + yd[j];

        // bounds check
        if (xp < 0 || xp >= _w || yp < 0 || yp >= _h)
          continue;

        // directional check
        if (getPixel(xp, yp)._a == 0)
          return true;
      }
    }

    return false;
  }

  float Image::closestLabDist(LabColor & x, vector<LabColor>& y)
  {
    // the really dumb implementation first
    float min = FLT_MAX;
    int minIdx = -1;

    for (int i = 0; i < y.size(); i++) {
      float dist = Utils<float>::LabL2Diff(x, y[i]);

      if (dist < min) {
        minIdx = i;
        min = dist;
      }
    }

    // maybe do something with minIdx, dunno
    return min;
  }

  ImportanceMap::ImportanceMap(int w, int h) : _w(w), _h(h)
  {
    _display = shared_ptr<Image>(new Image(_w, _h));
    _data.resize(_w * _h);
  }

  ImportanceMap::ImportanceMap(const ImportanceMap & other)
  {
    _data = other._data;
    _w = other._w;
    _h = other._h;

    // image is left alone
  }

  ImportanceMap::~ImportanceMap()
  {
  }

  void ImportanceMap::setVal(float val, int x, int y)
  {
    if (x + y * _w > _data.size()) {
      // please don't
      return;
    }

    _data[x + y * _w] = val;
  }

  double ImportanceMap::getVal(int x, int y)
  {
    if (x + y * _w > _data.size()) {
      // please don't
      return 0;
    }

    return _data[x + y * _w];
  }

  shared_ptr<Image> ImportanceMap::getDisplayableImage()
  {
    double max = getMax();
    double min = nonZeroMin();
    vector<unsigned char>& imgData = _display->getData();

    for (int i = 0; i < _data.size(); i++) {
      unsigned char scaled = (unsigned char)(((_data[i] - min) / (max - min)) * 255);
      if (_data[i] == 0) {
        scaled = 0;
      }

      imgData[i * 4] = scaled;
      imgData[i * 4 + 1] = scaled;
      imgData[i * 4 + 2] = scaled;
      imgData[i * 4 + 3] = 255;
    }

    return _display;
  }

  double ImportanceMap::getMin()
  {
    double min = DBL_MAX;

    for (int i = 0; i < _data.size(); i++) {
      if (_data[i] < min) {
        min = _data[i];
      }
    }

    return min;
  }

  double ImportanceMap::nonZeroMin()
  {
    double min = DBL_MAX;

    for (int i = 0; i < _data.size(); i++) {
      if (_data[i] == 0)
        continue;

      if (_data[i] < min) {
        min = _data[i];
      }
    }

    return min;
  }

  double ImportanceMap::getMax()
  {
    double max = DBL_MIN;

    for (int i = 0; i < _data.size(); i++) {
      if (_data[i] > max) {
        max = _data[i];
      }
    }

    return max;
  }

  void ImportanceMap::dump(string path, string base)
  {
    string imgPath = path + base + ".png";
    string dataPath = path + base + ".json";

    getDisplayableImage()->save(imgPath);

    // data
    nlohmann::json data;
    nlohmann::json rawData = nlohmann::json::array();

    for (int i = 0; i < _data.size(); i++) {
      rawData.push_back(_data[i]);
    }

    data["data"] = rawData;
    data["min"] = getMin();
    data["max"] = getMax();

    ofstream file(dataPath);
    file << data.dump(4);
  }

  void ImportanceMap::normalize()
  {
    float max = getMax();
    float min = getMin();

    for (int i = 0; i < _data.size(); i++) {
      _data[i] = (_data[i] - min) / (max - min);
    }
  }

}

#include "searchData.h"

namespace Comp {

ExpSearchSample::ExpSearchSample(shared_ptr<Image> img, Context ctx, vector<double> ctxvec) :
  _render(img), _ctx(ctx), _ctxVec(ctxvec), _brightness(2), _hue(5), _sat(0.05)
{
  preProcess();
}

ExpSearchSample::ExpSearchSample(const ExpSearchSample & other) : _render(other._render), _ctx(other._ctx),
  _brightness(other._brightness), _hue(other._hue), _sat(other._sat), _ctxVec(other._ctxVec)
{
}

ExpSearchSample::~ExpSearchSample()
{
}

double ExpSearchSample::brightnessDist(shared_ptr<ExpSearchSample> other)
{
  // using the chi-squared histogram distance for now, just due to simplicity and speed
  return _brightness.chiSq(other->_brightness);
}

double ExpSearchSample::hueDist(shared_ptr<ExpSearchSample> other)
{
  return _hue.chiSq(other->_hue);
}

double ExpSearchSample::satDist(shared_ptr<ExpSearchSample> other)
{
  return _sat.chiSq(other->_sat);
}

double ExpSearchSample::structDiff(shared_ptr<ExpSearchSample> other)
{
  // structural distance as proposed by matt, to be implemented
  return 0.0;
}

vector<double> ExpSearchSample::getContextVector()
{
  return _ctxVec;
}

shared_ptr<Image> ExpSearchSample::getImg()
{
  return _render;
}

Context ExpSearchSample::getContext()
{
  return _ctx;
}

void ExpSearchSample::preProcess()
{
  vector<unsigned char> imgData = _render->getData();
  for (int i = 0; i < imgData.size() / 4; i++) {
    int index = i * 4;
    float alpha = imgData[index + 3] / 255.0f;
    float r = (imgData[index] * alpha) / 255.0f;
    float g = (imgData[index + 1] * alpha) / 255.0f;
    float b = (imgData[index + 2] * alpha) / 255.0f;

    Utils<float>::LabColorT lab = Utils<float>::RGBToLab(r, g, b);
    Utils<float>::HSLColorT hsl = Utils<float>::RGBToHSL(r, g, b);

    _brightness.add(lab._L);
    _hue.add(hsl._h);
    _sat.add(hsl._s);
  }
}

ExpSearchSet::ExpSearchSet()
{
  // default settings for now
  // literally just random guesses
  _axisReq = 2;
  _hueThreshold = 0.4;
  _satThreshold = 0.4;
  _brightThreshold = 0.4;

  _idCounter = 0;
}

ExpSearchSet::~ExpSearchSet()
{
  _samples.clear();
}

void ExpSearchSet::setInitial(shared_ptr<ExpSearchSample> init)
{
  _init = init;
}

bool ExpSearchSet::add(shared_ptr<ExpSearchSample> x)
{
  getLogger()->log("Evaluating sample...");

  // a sample should be added to the current search set if it is different enough
  // along at least n different axes.
  // this may change in the future depending on if we want to focus on structural differences
  // record min distances
  double minBright = x->brightnessDist(_init);
  double minHue = x->hueDist(_init);
  double minSat = x->satDist(_init);

  for (auto& sample : _samples) {
    double brightDist = x->brightnessDist(sample.second);
    double hueDist = x->hueDist(sample.second);
    double satDist = x->satDist(sample.second);

    if (brightDist < minBright)
      minBright = brightDist;

    if (hueDist < minHue)
      minHue = hueDist;

    if (satDist < minSat)
      minSat = satDist;
  }

  // check if minimums are above thresholds
  int axes = 0;
  stringstream why;

  if (minBright > _brightThreshold) {
    axes++;
    why << "Brightness acceptable (" << minBright << ")\n";
  }
  if (minHue > _hueThreshold) {
    axes++;
    why << "Hue acceptable (" << minHue << ")\n";
  }
  if (minSat > _satThreshold) {
    axes++;
    why << "Saturation acceptable (" << minSat << ")\n";
  }

  if (axes >= _axisReq) {
    _samples[_idCounter] = x;
    _reasoning[_idCounter] = why.str();

    getLogger()->log("Added sample " + to_string(_idCounter) + " to set (total: " + to_string(size()) + "): " + why.str());
    
    // also dump the histograms
    getLogger()->log("Brightness Histogram\n" + x->_brightness.toString());
    getLogger()->log("Hue Histogram\n" + x->_hue.toString());
    getLogger()->log("Sat Histogram\n" + x->_sat.toString());

    _idCounter++;
    return true;
  }
  else {
    getLogger()->log("Rejected sample from set. Axis threshold not met: " + to_string(axes));
    return false;
  }
}

shared_ptr<ExpSearchSample> ExpSearchSet::get(unsigned int id)
{
  return _samples[id];
}

string ExpSearchSet::getReason(unsigned int id)
{
  return _reasoning[id];
}

int ExpSearchSet::size()
{
  return _samples.size();
}

bool ExpSearchSet::isGood(shared_ptr<ExpSearchSample> x)
{
  // couple sanity checks here
  // is the brightness all in the same bin? (moreso than the original?)
  double pctBright = x->_brightness.largestBinPercent();
  double origPctBright = _init->_brightness.largestBinPercent();

  // threshold: 70% is uniform brightness in a single bin
  if (pctBright >= 0.7) {
    // if the original image actually had this, then, well, it's fine
    if (abs(origPctBright - pctBright) > 0.05) {
      // if not, it's probably bad though
      getLogger()->log("Sample rejected. Brightness too uniform: " + to_string(pctBright));
      return false;
    }
  }

  // is the hue all in the same bin? (and is that not what the original image did?)
  double pctHue = x->_hue.largestBinPercent();
  double origPctHue = _init->_hue.largestBinPercent();

  // same idea as brightness
  if (pctHue >= 0.7) {
    // if the original image actually had this, then, well, it's fine
    if (abs(origPctHue - pctHue) > 0.05) {
      // if not, it's probably bad though
      getLogger()->log("Sample rejected. Hue too uniform: " + to_string(pctHue));
      return false;
    }
  }

  // are things overwhelmingly dark / bright?
  double pctDark = x->_brightness.getPercent(0.0);
  double pctBright2 = x->_brightness.getPercent(100.0);
  double origPctDark = _init->_brightness.getPercent(0.0);
  double origPctBright2 = _init->_brightness.getPercent(100.0);

  // same process, it's likely bad unless the original config did this
  if (pctDark >= 0.85) {
    if (abs(origPctDark - pctDark) > 0.02) {
      getLogger()->log("Sample rejected. Too dark: " + to_string(pctDark));
      return false;
    }
  }

  if (pctBright2 >= 0.85) {
    if (abs(origPctBright2 - pctBright2) > 0.02) {
      getLogger()->log("Sample rejected. Too bright: " + to_string(pctBright2));
      return false;
    }
  }
}

}
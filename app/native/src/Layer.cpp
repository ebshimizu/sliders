#include "Layer.h"

namespace Comp {
  Layer::Layer() : _name(""), _adjustment(false)
  {
  }

  Layer::Layer(string name, shared_ptr<Image> img) : _name(name), _adjustment(false)
  {
    init(img);
  }

  Layer::Layer(string name) : _name(name), _adjustment(true)
  {
    init(nullptr);
  }

  Layer::Layer(const Layer & other) :
    _name(other._name),
    _mode(other._mode),
    _opacity(other._opacity),
    _visible(other._visible),
    _image(other._image),
    _adjustment(other._adjustment),
    _adjustments(other._adjustments),
    _curves(other._curves),
    _grad(other._grad),
    _selectiveColor(other._selectiveColor)
  {
  }

  Layer & Layer::operator=(const Layer & other)
  {
    _name = other._name;
    _mode = other._mode;
    _opacity = other._opacity;
    _visible = other._visible;
    _image = other._image;
    _adjustment = other._adjustment;
    _adjustments = other._adjustments;
    _curves = other._curves;
    _grad = other._grad;
    _selectiveColor = other._selectiveColor;

    return *this;
  }

  Layer::~Layer()
  {
  }

  void Layer::setImage(shared_ptr<Image> img)
  {
    if (_adjustment) {
      getLogger()->log("Can't set image property of an adjustment layer", LogLevel::ERR);
      return;
    }

    // does not change layer settings
    _image = img;
  }

  unsigned int Layer::getWidth()
  {
    // adjustment layers have no dimensions
    if (_adjustment)
      return 0;

    return _image->getWidth();
  }

  unsigned int Layer::getHeight()
  {
    if (_adjustment)
      return 0;

    return _image->getHeight();
  }

  shared_ptr<Image> Layer::getImage() {
    if (_adjustment)
      return nullptr;

    return _image;
  }

  float Layer::getOpacity()
  {
    return _opacity;
  }

  void Layer::setOpacity(float val)
  {
    _opacity = (val > 100) ? 100 : ((val < 0) ? 0 : val);
  }

  string Layer::getName()
  {
    return _name;
  }

  void Layer::setName(string name)
  {
    _name = name;
  }

  void Layer::reset()
  {
    _mode = BlendMode::NORMAL;
    _opacity = 100;
    _visible = true;
  }

  bool Layer::isAdjustmentLayer()
  {
    return _adjustment;
  }

  map<string, float> Layer::getAdjustment(AdjustmentType type)
  {
    if (_adjustments.count(type) > 0) {
      return _adjustments[type];
    }
    
    return map<string, float>();
  }

  void Layer::deleteAdjustment(AdjustmentType type)
  {
    _adjustments.erase(type);

    if (type == AdjustmentType::CURVES) {
      _curves.clear();
    }
    if (type == AdjustmentType::GRADIENT) {
      _grad._colors.clear();
      _grad._x.clear();
    }
    if (type == AdjustmentType::SELECTIVE_COLOR) {
      _selectiveColor.clear();
    }
  }

  void Layer::deleteAllAdjustments()
  {
    _adjustments.clear();
    _curves.clear();
    _grad._colors.clear();
    _grad._x.clear();
    _selectiveColor.clear();
  }

  vector<AdjustmentType> Layer::getAdjustments()
  {
    vector<AdjustmentType> types;
    for (auto a : _adjustments) {
      types.push_back(a.first);
    }

    return types;
  }

  void Layer::addAdjustment(AdjustmentType type, string param, float val)
  {
    _adjustments[type][param] = val;
  }

  void Layer::addHSLAdjustment(float hue, float sat, float light)
  {
    _adjustments[AdjustmentType::HSL]["hue"] = hue;
    _adjustments[AdjustmentType::HSL]["sat"] = sat;
    _adjustments[AdjustmentType::HSL]["light"] = light;
  }

  void Layer::addLevelsAdjustment(float inMin, float inMax, float gamma, float outMin, float outMax)
  {
    _adjustments[AdjustmentType::LEVELS]["inMin"] = inMin;
    _adjustments[AdjustmentType::LEVELS]["inMax"] = inMax;
    _adjustments[AdjustmentType::LEVELS]["gamma"] = gamma;
    _adjustments[AdjustmentType::LEVELS]["outMin"] = outMin;
    _adjustments[AdjustmentType::LEVELS]["outMax"] = outMax;
  }

  void Layer::addCurvesChannel(string channel, Curve curve)
  {
    _adjustments[AdjustmentType::CURVES][channel] = 1;
    _curves[channel] = curve;
  }

  void Layer::deleteCurvesChannel(string channel)
  {
    _adjustments[AdjustmentType::CURVES].erase(channel);
    _curves.erase(channel);
  }

  Curve Layer::getCurveChannel(string channel)
  {
    if (_curves.count(channel) > 0) {
      return _curves[channel];
    }

    return Curve();
  }

  void Layer::addExposureAdjustment(float exp, float offset, float gamma)
  {
    _adjustments[AdjustmentType::EXPOSURE]["exposure"] = exp;
    _adjustments[AdjustmentType::EXPOSURE]["offset"] = offset;
    _adjustments[AdjustmentType::EXPOSURE]["gamma"] = gamma;
  }

  void Layer::addGradientAdjustment(Gradient grad)
  {
    _adjustments[AdjustmentType::GRADIENT]["on"] = 1;
    _grad = grad;
  }

  void Layer::addSelectiveColorAdjustment(bool relative, map<string, map<string, float>> data)
  {
    _adjustments[AdjustmentType::SELECTIVE_COLOR]["relative"] = relative ? 1.0f : 0;
    _selectiveColor = data;
  }

  float Layer::getSelectiveColorChannel(string channel, string param)
  {
    // fairly sure it just creates defaults if the keys don't exist so should be fine
    return _selectiveColor[channel][param];
  }

  void Layer::setSelectiveColorChannel(string channel, string param, float val)
  {
    _selectiveColor[channel][param] = val;
  }

  void Layer::addColorBalanceAdjustment(bool preserveLuma, float shadowR, float shadowG, float shadowB,
    float midR, float midG, float midB,
    float highR, float highG, float highB)
  {
    _adjustments[AdjustmentType::COLOR_BALANCE]["preserveLuma"] = preserveLuma ? 1.0f : 0;
    _adjustments[AdjustmentType::COLOR_BALANCE]["shadowR"] = shadowR;
    _adjustments[AdjustmentType::COLOR_BALANCE]["shadowG"] = shadowG;
    _adjustments[AdjustmentType::COLOR_BALANCE]["shadowB"] = shadowB;
    _adjustments[AdjustmentType::COLOR_BALANCE]["midR"] = midR;
    _adjustments[AdjustmentType::COLOR_BALANCE]["midG"] = midG;
    _adjustments[AdjustmentType::COLOR_BALANCE]["midB"] = midB;
    _adjustments[AdjustmentType::COLOR_BALANCE]["highR"] = highR;
    _adjustments[AdjustmentType::COLOR_BALANCE]["highG"] = highG;
    _adjustments[AdjustmentType::COLOR_BALANCE]["highB"] = highB;
  }

  void Layer::addPhotoFilterAdjustment(bool preserveLuma, float r, float g, float b, float d)
  {
    _adjustments[AdjustmentType::PHOTO_FILTER]["r"] = r;
    _adjustments[AdjustmentType::PHOTO_FILTER]["g"] = g;
    _adjustments[AdjustmentType::PHOTO_FILTER]["b"] = b;
    _adjustments[AdjustmentType::PHOTO_FILTER]["density"] = d;
    _adjustments[AdjustmentType::PHOTO_FILTER]["preserveLuma"] = preserveLuma ? 1.0f : 0;
  }

  void Layer::addColorAdjustment(float r, float g, float b, float a)
  {
    _adjustments[AdjustmentType::COLORIZE]["r"] = r;
    _adjustments[AdjustmentType::COLORIZE]["g"] = g;
    _adjustments[AdjustmentType::COLORIZE]["b"] = b;
    _adjustments[AdjustmentType::COLORIZE]["a"] = a;
  }

  void Layer::addLighterColorAdjustment(float r, float g, float b, float a)
  {
    _adjustments[AdjustmentType::LIGHTER_COLORIZE]["r"] = r;
    _adjustments[AdjustmentType::LIGHTER_COLORIZE]["g"] = g;
    _adjustments[AdjustmentType::LIGHTER_COLORIZE]["b"] = b;
    _adjustments[AdjustmentType::LIGHTER_COLORIZE]["a"] = a;
  }

  void Layer::addOverwriteColorAdjustment(float r, float g, float b, float a)
  {
    _adjustments[AdjustmentType::OVERWRITE_COLOR]["r"] = r;
    _adjustments[AdjustmentType::OVERWRITE_COLOR]["g"] = g;
    _adjustments[AdjustmentType::OVERWRITE_COLOR]["b"] = b;
    _adjustments[AdjustmentType::OVERWRITE_COLOR]["a"] = a;
  }

  float Layer::evalCurve(string channel, float x)
  {
    if (_curves.count(channel) > 0) {
      return _curves[channel].eval(x);
    }

    return x;
  }

  RGBColor Layer::evalGradient(float x)
  {
    if (_adjustments.count(AdjustmentType::GRADIENT) > 0) {
      return _grad.eval(x);
    }

    return RGBColor();
  }

  map<string, map<string, float>> Layer::getSelectiveColor()
  {
    return _selectiveColor;
  }

  Gradient& Layer::getGradient()
  {
    return _grad;
  }

  void Layer::init(shared_ptr<Image> source)
  {
    _mode = BlendMode::NORMAL;
    _opacity = 100;
    _visible = true;

    _image = source;
  }
}
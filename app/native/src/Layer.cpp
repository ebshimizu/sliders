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
    _selectiveColor(other._selectiveColor),
    _cbChannel(other._cbChannel),
    _cbSettings(other._cbSettings),
    _mask(other._mask),
    _offsetX(other._offsetX),
    _offsetY(other._offsetY),
    _precompOrder(other._precompOrder),
    _localAdjOrderOverride(other._localAdjOrderOverride),
    _localSelectionGroupOverride(other._localSelectionGroupOverride)
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
    _cbChannel = other._cbChannel;
    _cbSettings = other._cbSettings;
    _mask = other._mask;
    _offsetX = other._offsetX;
    _offsetY = other._offsetY;
    _precompOrder = other._precompOrder;
    _localAdjOrderOverride = other._localAdjOrderOverride;
    _localSelectionGroupOverride = other._localSelectionGroupOverride;

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

  void Layer::setMask(shared_ptr<Image> mask)
  {
    _mask = mask;
  }

  bool Layer::hasMask()
  {
    return _mask != nullptr;
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

  shared_ptr<Image> Layer::getMask()
  {
    return _mask;
  }

  float Layer::getOpacity()
  {
    return _opacity;
  }

  void Layer::setOpacity(float val)
  {
    _opacity = clamp<float>(val, 0, 1);
  }

  void Layer::setConditionalBlend(string channel, map<string, float> settings)
  {
    _cbChannel = channel;
    _cbSettings = settings;
  }

  bool Layer::shouldConditionalBlend()
  {
    return _cbSettings.size() > 0;
  }

  const string& Layer::getConditionalBlendChannel()
  {
    return _cbChannel;
  }

  const map<string, float>& Layer::getConditionalBlendSettings()
  {
    return _cbSettings;
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
    _opacity = 1;
    _visible = true;
  }

  bool Layer::isAdjustmentLayer()
  {
    return _adjustment && !isPrecomp();
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

  void Layer::addAdjustment(AdjustmentType type, map<string, float> vals)
  {
    _adjustments[type] = vals;
  }

  void Layer::addHSLAdjustment(float hue, float sat, float light)
  {
    _adjustments[AdjustmentType::HSL]["hue"] = fmodf(hue, 1);
    _adjustments[AdjustmentType::HSL]["sat"] = clamp<float>(sat, 0, 1);
    _adjustments[AdjustmentType::HSL]["light"] = clamp<float>(light, 0, 1);
  }

  void Layer::addLevelsAdjustment(float inMin, float inMax, float gamma, float outMin, float outMax)
  {
    // we should sanity check this one. Min < max. If min > max, min = max and max = min + 10 clamped to 255 max

    if (inMin > inMax) {
      inMin = inMax;
      inMax = inMin + .01;
      inMax = (inMax > 1) ? 1 : inMax;
    }

    if (outMin > outMax) {
      outMin = outMax;
      outMax = outMin + .01;
      outMax = (outMax > 1) ? 1 : outMax;
    }

    _adjustments[AdjustmentType::LEVELS]["inMin"] = clamp<float>(inMin, 0, 1);
    _adjustments[AdjustmentType::LEVELS]["inMax"] = clamp<float>(inMax, 0, 1);
    _adjustments[AdjustmentType::LEVELS]["gamma"] = clamp<float>(gamma, 0, 1);
    _adjustments[AdjustmentType::LEVELS]["outMin"] = clamp<float>(outMin, 0, 1);
    _adjustments[AdjustmentType::LEVELS]["outMax"] = clamp<float>(outMax, 0, 1);
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
    _adjustments[AdjustmentType::EXPOSURE]["exposure"] = clamp<float>(exp, 0, 1);
    _adjustments[AdjustmentType::EXPOSURE]["offset"] = clamp<float>(offset, 0, 1);
    _adjustments[AdjustmentType::EXPOSURE]["gamma"] = clamp<float>(gamma, 0, 1);
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
    if (_selectiveColor.count(channel) == 0) {
      _selectiveColor[channel][param] = 0.5;
    }

    if (_selectiveColor[channel].count(param) == 0) {
      _selectiveColor[channel][param] = 0.5;
      return 0.5;
    }

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
    _adjustments[AdjustmentType::COLOR_BALANCE]["shadowR"] = clamp<float>(shadowR, 0, 1);
    _adjustments[AdjustmentType::COLOR_BALANCE]["shadowG"] = clamp<float>(shadowG, 0, 1);
    _adjustments[AdjustmentType::COLOR_BALANCE]["shadowB"] = clamp<float>(shadowB, 0, 1);
    _adjustments[AdjustmentType::COLOR_BALANCE]["midR"] = clamp<float>(midR, 0, 1);
    _adjustments[AdjustmentType::COLOR_BALANCE]["midG"] = clamp<float>(midG, 0, 1);
    _adjustments[AdjustmentType::COLOR_BALANCE]["midB"] = clamp<float>(midB, 0, 1);
    _adjustments[AdjustmentType::COLOR_BALANCE]["highR"] = clamp<float>(highR, 0, 1);
    _adjustments[AdjustmentType::COLOR_BALANCE]["highG"] = clamp<float>(highG, 0, 1);
    _adjustments[AdjustmentType::COLOR_BALANCE]["highB"] = clamp<float>(highB, 0, 1);
  }

  void Layer::addPhotoFilterAdjustment(bool preserveLuma, float r, float g, float b, float d)
  {
    _adjustments[AdjustmentType::PHOTO_FILTER]["r"] = clamp<float>(r, 0, 1);
    _adjustments[AdjustmentType::PHOTO_FILTER]["g"] = clamp<float>(g, 0, 1);
    _adjustments[AdjustmentType::PHOTO_FILTER]["b"] = clamp<float>(b, 0, 1);
    _adjustments[AdjustmentType::PHOTO_FILTER]["density"] = clamp<float>(d, 0, 1);
    _adjustments[AdjustmentType::PHOTO_FILTER]["preserveLuma"] = preserveLuma ? 1.0f : 0;
  }

  void Layer::addColorAdjustment(float r, float g, float b, float a)
  {
    _adjustments[AdjustmentType::COLORIZE]["r"] = clamp<float>(r, 0, 1);
    _adjustments[AdjustmentType::COLORIZE]["g"] = clamp<float>(g, 0, 1);
    _adjustments[AdjustmentType::COLORIZE]["b"] = clamp<float>(b, 0, 1);
    _adjustments[AdjustmentType::COLORIZE]["a"] = clamp<float>(a, 0, 1);
  }

  void Layer::addLighterColorAdjustment(float r, float g, float b, float a)
  {
    _adjustments[AdjustmentType::LIGHTER_COLORIZE]["r"] = clamp<float>(r, 0, 1);
    _adjustments[AdjustmentType::LIGHTER_COLORIZE]["g"] = clamp<float>(g, 0, 1);
    _adjustments[AdjustmentType::LIGHTER_COLORIZE]["b"] = clamp<float>(b, 0, 1);
    _adjustments[AdjustmentType::LIGHTER_COLORIZE]["a"] = clamp<float>(a, 0, 1);
  }

  void Layer::addOverwriteColorAdjustment(float r, float g, float b, float a)
  {
    _adjustments[AdjustmentType::OVERWRITE_COLOR]["r"] = clamp<float>(r, 0, 1);
    _adjustments[AdjustmentType::OVERWRITE_COLOR]["g"] = clamp<float>(g, 0, 1);
    _adjustments[AdjustmentType::OVERWRITE_COLOR]["b"] = clamp<float>(b, 0, 1);
    _adjustments[AdjustmentType::OVERWRITE_COLOR]["a"] = clamp<float>(a, 0, 1);
  }

  void Layer::addInvertAdjustment()
  {
    // just add the key, there are no settings for this
    _adjustments[AdjustmentType::INVERT]["on"] = 1;
  }

  void Layer::addBrightnessAdjustment(float b, float c)
  {
    _adjustments[AdjustmentType::BRIGHTNESS]["brightness"] = clamp<float>(b, 0, 1);
    _adjustments[AdjustmentType::BRIGHTNESS]["contrast"] = clamp<float>(c, 0, 1);
  }

  map<string, map<string, float>> Layer::getSelectiveColor()
  {
    return _selectiveColor;
  }

  Gradient& Layer::getGradient()
  {
    return _grad;
  }

  void Layer::resetImage()
  {
    _image->reset(1, 1, 1);
  }

  int Layer::prepExp(ExpContext& context, int start)
  {
    int index = start;
    string pfx = "x_" + _name + "_";

    _expOpacity = context.registerParam(ParamType::FREE_PARAM, pfx + "opacity", _opacity);
    index++;

    for (auto a : _adjustments) {
      for (auto params : a.second) {
        _expAdjustments[a.first][params.first] = context.registerParam(ParamType::FREE_PARAM, pfx + params.first, params.second);
        index++;
      }
    }

    if (_adjustments.count(SELECTIVE_COLOR) > 0) {
      // selective color
      vector<string> channels = { "reds", "yellows", "greens", "cyans", "blues", "magentas", "neutrals", "blacks", "whites" };
      vector<string> params = { "cyan", "magenta", "yellow", "black" };

      for (auto c : channels) {
        for (auto p : params) {
          _expSelectiveColor[c][p] = context.registerParam(ParamType::FREE_PARAM, pfx + "sc_" + c + "_" + p, _selectiveColor[c][p]);
          index++;
        }
      }
    }

    return index;
  }

  void Layer::prepExpParams(vector<double>& params)
  {
    params.push_back(_opacity);

    for (auto a : _adjustments) {
      for (auto p : a.second) {
        params.push_back(p.second);
      }
    }

    if (_adjustments.count(SELECTIVE_COLOR) > 0) {
      // selective color
      vector<string> channels = { "reds", "yellows", "greens", "cyans", "blues", "magentas", "neutrals", "blacks", "whites" };
      vector<string> names = { "cyan", "magenta", "yellow", "black" };

      for (auto c : channels) {
        for (auto p : names) {
          params.push_back(_selectiveColor[c][p]);
        }
      }
    }
  }

  void Layer::addParams(nlohmann::json & paramList)
  {
    // opacity, then adjustments in std::map order (numeric ascending -> alphabetical)
    nlohmann::json opacity;
    opacity["layerName"] = _name;
    opacity["paramID"] = paramList.size();
    opacity["adjustmentType"] = AdjustmentType::OPACITY;
    opacity["adjustmentName"] = "opacity";
    opacity["value"] = _opacity;
    paramList.push_back(opacity);

    // regular adjustments
    for (auto a : _adjustments) {
      for (auto p : a.second) {
        nlohmann::json param;
        param["layerName"] = _name;
        param["paramID"] = paramList.size();
        param["adjustmentType"] = a.first;
        param["adjustmentName"] = p.first;
        param["value"] = p.second;
        paramList.push_back(param);
      }
    }

    // selective color
    if (_adjustments.count(SELECTIVE_COLOR) > 0) {
      vector<string> channels = { "reds", "yellows", "greens", "cyans", "blues", "magentas", "neutrals", "blacks", "whites" };
      vector<string> names = { "cyan", "magenta", "yellow", "black" };
      
      for (auto c : channels) {
        for (auto p : names) {
          nlohmann::json param;
          param["layerName"] = _name;
          param["paramID"] = paramList.size();
          param["adjustmentType"] = SELECTIVE_COLOR;
          param["adjustmentName"] = "selectiveColor";

          if (_selectiveColor.count(c) > 0 && _selectiveColor[c].count(p) > 0) {
            param["value"] = _selectiveColor[c][p];
          }
          else {
            param["value"] = 0.5;
          }

          param["selectiveColor"] = nlohmann::json::object();
          param["selectiveColor"]["channel"] = c;
          param["selectiveColor"]["color"] = p;
          paramList.push_back(param);
        }
      }
    }
  }

  void Layer::setOffset(float x, float y)
  {
    _offsetX = x;
    _offsetY = y;
  }

  pair<float, float> Layer::getOffset()
  {
    return pair<float, float>(_offsetX, _offsetY);
  }

  void Layer::resetOffset()
  {
    setOffset(0, 0);
  }

  void Layer::setPrecompOrder(vector<string> order)
  {
    _precompOrder = order;
  }

  vector<string> Layer::getPrecompOrder()
  {
    return _precompOrder;
  }

  bool Layer::isPrecomp()
  {
    return _precompOrder.size() > 0;
  }

  void Layer::setLocalAdjOverride(vector<AdjustmentType> order)
  {
    // check for duplicates
    set<AdjustmentType> tmp;
    for (auto a : order) {
      if (tmp.count(a) > 1) {
        getLogger()->log("Invalid adjustment order. Duplicates are not allowed.", Comp::ERR);
        return;
      }
      tmp.insert(a);
    }

    _localAdjOrderOverride = order;
  }

  vector<AdjustmentType> Layer::getLocalAdjOverride()
  {
    return _localAdjOrderOverride;
  }

  bool Layer::hasLocalAdjOverride()
  {
    return _localAdjOrderOverride.size() > 0;
  }

  void Layer::setLocalSelectionGroupOverride(vector<string> order)
  {
    // duplicate check
    set<string> tmp;
    for (auto g : order) {
      if (tmp.count(g) > 1) {
        getLogger()->log("Invalid selection group order. Duplicates are not allowed.");
        return;
      }
      tmp.insert(g);
    }

    _localSelectionGroupOverride = order;
  }

  vector<string> Layer::getLocalSelectionGroupOverride()
  {
    return _localSelectionGroupOverride;
  }

  bool Layer::hasLocalSelectionGroupOverride()
  {
    return _localSelectionGroupOverride.size() > 0;
  }

  void Layer::init(shared_ptr<Image> source)
  {
    _mode = BlendMode::NORMAL;
    _opacity = 1;
    _visible = true;

    _image = source;
    _mask = nullptr;
    _offsetX = 0;
    _offsetY = 0;
  }
}
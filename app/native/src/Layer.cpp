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
    _adjustments(other._adjustments)
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
  }

  void Layer::deleteAllAdjustments()
  {
    _adjustments.clear();
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

  void Layer::init(shared_ptr<Image> source)
  {
    _mode = BlendMode::NORMAL;
    _opacity = 100;
    _visible = true;

    _image = source;
  }
}
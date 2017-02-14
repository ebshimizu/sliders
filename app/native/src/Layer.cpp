#include "Layer.h"

namespace Comp {
  Layer::Layer() : _name("")
  {
  }

  Layer::Layer(string name, shared_ptr<Image> img) : _name(name)
  {
    init(img);
  }

  Layer::Layer(const Layer & other) :
    _name(other._name),
    _mode(other._mode),
    _opacity(other._opacity),
    _visible(other._visible),
    _image(other._image)
  {
  }

  Layer & Layer::operator=(const Layer & other)
  {
    _name = other._name;
    _mode = other._mode;
    _opacity = other._opacity;
    _visible = other._visible;
    _image = other._image;

    return *this;
  }

  Layer::~Layer()
  {
  }

  void Layer::setImage(shared_ptr<Image> img)
  {
    // does not change layer settings
    _image = img;
  }

  unsigned int Layer::getWidth()
  {
    return _image->getWidth();
  }

  unsigned int Layer::getHeight()
  {
    return _image->getHeight();
  }

  shared_ptr<Image> Layer::getImage() {
    return _image;
  }

  void Layer::reset()
  {
    _mode = BlendMode::NORMAL;
    _opacity = 100;
    _visible = true;
  }

  void Layer::init(shared_ptr<Image> source)
  {
    _mode = BlendMode::NORMAL;
    _opacity = 100;
    _visible = true;

    _image = source;
  }
}
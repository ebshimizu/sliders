#include "Compositor.h"

namespace Comp {
  Compositor::Compositor()
  {
  }

  Compositor::~Compositor()
  {
  }

  void Compositor::loadImage(string name, string filename)
  {
    if (_images.count(name) > 0) {
      // overwrite existing image
      delete _images[name];
    }

    _images[name] = new Image(filename);

    getLogger()->log("Added new image " + name + " to compositor");
  }

  Image * Compositor::getImage(string name)
  {
    if (_images.count(name) > 0) {
      return _images[name];
    }

    return nullptr;
  }
}

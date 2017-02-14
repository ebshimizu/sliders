#include "Compositor.h"

namespace Comp {
  Compositor::Compositor()
  {
  }

  Compositor::~Compositor()
  {
  }

  bool Compositor::addLayer(string name, string file)
  {
    // check for existence in primary context
    if (_primary.count(name) > 0) {
      getLogger()->log("Failed to add layer " + name + ". Layer already exists.");
      return false;
    }

    // load image data
    _imageData[name] = shared_ptr<Image>(new Image(file));
    addLayer(name);
    return true;
  }

  bool Compositor::addLayer(string name, Image & img)
  {
    // check for existence in primary context
    if (_primary.count(name) > 0) {
      getLogger()->log("Failed to add layer " + name + ". Layer already exists.");
      return false;
    }

    // load image data
    _imageData[name] = shared_ptr<Image>(new Image(img));
    addLayer(name);
    
    return true;
  }

  bool Compositor::copyLayer(string src, string dest)
  {
    if (_primary.count(dest) > 0) {
      getLogger()->log("Failed to add layer " + dest + ". Layer already exists.");
      return false;
    }

    _primary[dest] = Layer(_primary[src]);
    _primary[dest].setName(dest);

    // save a ref to the image data
    _imageData[dest] = _primary[dest].getImage();

    // place at end of order
    _layerOrder.push_back(dest);

    getLogger()->log("Copied layer " + src + " to layer " + dest);
    return true;
  }

  bool Compositor::deleteLayer(string name)
  {
    if (_primary.count(name) <= 0) {
      getLogger()->log("Failed to delete layer " + name + ". Layer does not exist.");
      return false;
    }

    // delete from primary context
    _primary.erase(name);

    // delete from layer order
    for (auto it = _layerOrder.begin(); it != _layerOrder.end(); it++) {
      if (*(it) == name) {
        _layerOrder.erase(it);
        break;
      }
    }

    // erase from image data
    _imageData.erase(name);

    getLogger()->log("Erased layer " + name);
    return true;
  }

  void Compositor::reorderLayer(int from, int to)
  {
    // range check
    if (from < 0 || to < 0 || from >= _layerOrder.size() || to >= _layerOrder.size()) {
      getLogger()->log("Failed to move layer. Index out of bounds.");
      return;
    }
    if (from == to) {
      getLogger()->log("Failed to move layer. Destination same as source.");
      return;
    }

    string target = _layerOrder[from];
    _layerOrder.insert(_layerOrder.begin() + to, target);

    if (from > to) {
      // there is now an extra element between the start and the source
      _layerOrder.erase(_layerOrder.begin() + from + 1);
    }
    else if (from < to) {
      // there is an extra element after the source
      _layerOrder.erase(_layerOrder.begin() + from);
    }

    stringstream ss;
    ss << "Moved layer " << _layerOrder[to] << " from " << from << " to " << to;
    getLogger()->log(ss.str());
  }

  Layer & Compositor::getLayer(int id)
  {
    return _primary[_layerOrder[id]];
  }

  Layer & Compositor::getLayer(string name)
  {
    return _primary[name];
  }

  Context Compositor::getNewContext()
  {
    return Context(_primary);
  }

  Context & Compositor::getPrimaryContext()
  {
    return _primary;
  }

  vector<string> Compositor::getLayerOrder()
  {
    return _layerOrder;
  }

  int Compositor::size()
  {
    return _primary.size();
  }

  Image* Compositor::render()
  {
    return render(getNewContext());
  }

  Image* Compositor::render(Context c)
  {
    if (c.size() == 0) {
      return new Image();
    }

    Image* comp = new Image(c.begin()->second.getWidth(), c.begin()->second.getHeight());
    vector<unsigned char>& compPx = comp->getData();

    // blend the layers
    for (auto id : _layerOrder) {
      Layer l = c[id];

      if (!l._visible)
        continue;

      // pre-process layer if necessary due to adjustments, requires creating
      // new image
      vector<unsigned char>& layerPx = _imageData[l.getName()]->getData();

      // blend the layer
      for (int i = 0; i < comp->numPx(); i++) {
        // pixel data is a flat array, rgba interlaced format
        if (l._mode == BlendMode::NORMAL) {
          // layer alpha is combo of opacity and pixel alpha
          float aa = (layerPx[i * 4 + 3] / 255.0f) * (l.getOpacity() / 100.0f);
          
          // composite running alpha
          float ab = compPx[i * 4 + 3] / 255.0f;

          float ad = aa + ab * (1 - aa);

          // photoshop doesn't do premult alpha, so we compute fully here
          compPx[i * 4] = (unsigned char)((layerPx[i * 4] * aa + compPx[i * 4] * ab * (1 - aa)) / ad);
          compPx[i * 4 + 1] = (unsigned char)((layerPx[i * 4 + 1] * aa + compPx[i * 4 + 1] * ab * (1 - aa)) / ad);
          compPx[i * 4 + 2] = (unsigned char)((layerPx[i * 4 + 2] * aa + compPx[i * 4 + 2] * ab * (1 - aa)) / ad);

          // RGB
          //compPx[i * 4] = (unsigned char) (premult(layerPx[i * 4], aa) + premult(compPx[i * 4], ab) * (1 - aa));
          //compPx[i * 4 + 1] = (unsigned char) (premult(layerPx[i * 4 + 1], aa) + premult(compPx[i * 4 + 1], ab) * (1 - aa));
          //compPx[i * 4 + 2] = (unsigned char) (premult(layerPx[i * 4 + 2], aa) + premult(compPx[i * 4 + 2], ab) * (1 - aa));

          // alpha update
          compPx[i * 4 + 3] = (unsigned char)((aa + ab * (1 - aa)) * 255);
        }
        else {
          // ???
        }
      }
    }

    return comp;

  }
  string Compositor::renderToBase64()
  {
    Image* i = render();
    string ret = i->getBase64();
    delete i;

    return ret;
  }

  string Compositor::renderToBase64(Context c)
  {
    Image* i = render(c);
    string ret = i->getBase64();
    delete i;

    return ret;
  }

  bool Compositor::setLayerOrder(vector<string> order)
  {
    for (auto id : order) {
      if (_primary.count(id) == 0) {
        getLogger()->log("Unable to set layer order. Missing Layer named " + id, LogLevel::WARN);
        return false;
      }
    }

    _layerOrder = order;
    return true;
  }

  void Compositor::addLayer(string name)
  {
    _primary[name] = Layer(name, _imageData[name]);

    // place at end of order
    _layerOrder.push_back(name);

    getLogger()->log("Added new layer named " + name);
  }

  inline unsigned char Compositor::premult(unsigned char px, float a)
  {
    return (unsigned char)(px * a);
  }

}

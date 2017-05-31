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
    _imageData[name]["full"] = shared_ptr<Image>(new Image(file));
    addLayer(name);
    cacheScaled(name);
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
    _imageData[name]["full"] = shared_ptr<Image>(new Image(img));
    addLayer(name);
    cacheScaled(name);
    
    return true;
  }

  bool Compositor::addAdjustmentLayer(string name)
  {
    if (_primary.count(name) > 0) {
      getLogger()->log("Failed to add layer " + name + ". Layer already exists.");
      return false;
    }

    // create the layer
    _primary[name] = Layer(name);
    _layerOrder.push_back(name);

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
    _imageData[dest]["full"] = _primary[dest].getImage();
    cacheScaled(dest);

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

  void Compositor::setContext(Context c)
  {
    // copy all layers that exist in primary
    for (auto l : c) {
      if (_primary.count(l.first) > 0)
        _primary[l.first] = l.second;
    }
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
    return (int)_primary.size();
  }

  Image* Compositor::render(string size)
  {
    return render(getNewContext(), size);
  }

  Image* Compositor::render(Context c, string size)
  {
    if (c.size() == 0) {
      return new Image();
    }

    // pick a size to use in the cache
    int width, height;
    bool useCache = false;

    if (size == "") {
      size = "full";
    }

    if (_imageData.begin()->second.count(size) > 0) {
      width = _imageData.begin()->second[size]->getWidth();
      height = _imageData.begin()->second[size]->getHeight();
      useCache = true;
    }
    else {
      getLogger()->log("No render size named " + size + " found. Rendering at full size.", LogLevel::WARN);
      width = _imageData.begin()->second["full"]->getWidth();
      height = _imageData.begin()->second["full"]->getHeight();
    }

    Image* comp = new Image(width, height);
    vector<unsigned char>& compPx = comp->getData();

    // Photoshop appears to blend using an all white alpha 0 image
    for (int i = 0; i < compPx.size(); i++) {
      if (i % 4 == 3)
        continue;

      compPx[i] = 255;
    }

    // blend the layers
    for (auto id : _layerOrder) {
      Layer& l = c[id];

      if (!l._visible)
        continue;

      vector<unsigned char>* layerPx;
      Image* tmpLayer = nullptr;

      // handle adjustment layers
      if (l.isAdjustmentLayer()) {
        // ok so here we adjust the current composition, then blend it as normal below
        // create duplicate of current composite
        tmpLayer = new Image(*comp);
        adjust(tmpLayer, l);
        layerPx = &tmpLayer->getData();
      }
      else if (l.getAdjustments().size() > 0) {
        // so a layer may have other things clipped to it, in which case we apply the
        // specified adjustment only to the source layer and the composite as normal
        tmpLayer = new Image(*_imageData[l.getName()][size].get());
        adjust(tmpLayer, l);
        layerPx = &tmpLayer->getData();
      }
      else {
        layerPx = &_imageData[l.getName()][size]->getData();
      }

      // blend the layer
      for (unsigned int i = 0; i < comp->numPx(); i++) {
        // pixel data is a flat array, rgba interlaced format
        // a = background, b = new layer
        // alphas
        float ab = ((*layerPx)[i * 4 + 3] / 255.0f) * (l.getOpacity() / 100.0f);
        float aa = compPx[i * 4 + 3] / 255.0f;
        float ad = aa + ab - aa * ab;

        compPx[i * 4 + 3] = (unsigned char)(ad * 255);

        // premult colors
        float rb = premult((*layerPx)[i * 4], ab);
        float gb = premult((*layerPx)[i * 4 + 1], ab);
        float bb = premult((*layerPx)[i * 4 + 2], ab);

        float ra = premult(compPx[i * 4], aa);
        float ga = premult(compPx[i * 4 + 1], aa);
        float ba = premult(compPx[i * 4 + 2], aa);

        // blend modes
        if (l._mode == BlendMode::NORMAL) {
          // b over a, standard alpha blend
          compPx[i * 4] = cvt(normal(ra, rb, aa, ab), ad);
          compPx[i * 4 + 1] = cvt(normal(ga, gb, aa, ab), ad);
          compPx[i * 4 + 2] = cvt(normal(ba, bb, aa, ab), ad);
        }
        else if (l._mode == BlendMode::MULTIPLY) {
          compPx[i * 4] = cvt(multiply(ra, rb, aa, ab), ad);
          compPx[i * 4 + 1] = cvt(multiply(ga, gb, aa, ab), ad);
          compPx[i * 4 + 2] = cvt(multiply(ba, bb, aa, ab), ad);
        }
        else if (l._mode == BlendMode::SCREEN) {
          compPx[i * 4] = cvt(screen(ra, rb, aa, ab), ad);
          compPx[i * 4 + 1] = cvt(screen(ga, gb, aa, ab), ad);
          compPx[i * 4 + 2] = cvt(screen(ba, bb, aa, ab), ad);
        }
        else if (l._mode == BlendMode::OVERLAY) {
          compPx[i * 4] = cvt(overlay(ra, rb, aa, ab), ad);
          compPx[i * 4 + 1] = cvt(overlay(ga, gb, aa, ab), ad);
          compPx[i * 4 + 2] = cvt(overlay(ba, bb, aa, ab), ad);
        }
        else if (l._mode == BlendMode::HARD_LIGHT) {
          compPx[i * 4] = cvt(hardLight(ra, rb, aa, ab), ad);
          compPx[i * 4 + 1] = cvt(hardLight(ga, gb, aa, ab), ad);
          compPx[i * 4 + 2] = cvt(hardLight(ba, bb, aa, ab), ad);
        }
        else if (l._mode == BlendMode::SOFT_LIGHT) {
          compPx[i * 4] = cvt(softLight(ra, rb, aa, ab), ad);
          compPx[i * 4 + 1] = cvt(softLight(ga, gb, aa, ab), ad);
          compPx[i * 4 + 2] = cvt(softLight(ba, bb, aa, ab), ad);
        }
        else if (l._mode == BlendMode::LINEAR_DODGE) {
          // special override for alpha here
          ad = (aa + ab > 1) ? 1 : (aa + ab);
          compPx[i * 4 + 3] = (unsigned char)(ad * 255);

          compPx[i * 4] = cvt(linearDodge(ra, rb, aa, ab), ad);
          compPx[i * 4 + 1] = cvt(linearDodge(ga, gb, aa, ab), ad);
          compPx[i * 4 + 2] = cvt(linearDodge(ba, bb, aa, ab), ad);
        }
        else if (l._mode == BlendMode::COLOR_DODGE) {
          compPx[i * 4] = cvt(colorDodge(ra, rb, aa, ab), ad);
          compPx[i * 4 + 1] = cvt(colorDodge(ga, gb, aa, ab), ad);
          compPx[i * 4 + 2] = cvt(colorDodge(ba, bb, aa, ab), ad);
        }
        else if (l._mode == BlendMode::LINEAR_BURN) {
          // need unmultiplied colors for this one
          compPx[i * 4] = cvt(linearBurn(compPx[i * 4] / 255.0f, (*layerPx)[i * 4] / 255.0f, aa, ab), ad);
          compPx[i * 4 + 1] = cvt(linearBurn(compPx[i * 4 + 1] / 255.0f, (*layerPx)[i * 4 + 1] / 255.0f, aa, ab), ad);
          compPx[i * 4 + 2] = cvt(linearBurn(compPx[i * 4 + 2] / 255.0f, (*layerPx)[i * 4 + 2] / 255.0f, aa, ab), ad);
        }
        else if (l._mode == BlendMode::LINEAR_LIGHT) {
          compPx[i * 4] = cvt(linearLight(compPx[i * 4] / 255.0f, (*layerPx)[i * 4] / 255.0f, aa, ab), ad);
          compPx[i * 4 + 1] = cvt(linearLight(compPx[i * 4 + 1] / 255.0f, (*layerPx)[i * 4 + 1] / 255.0f, aa, ab), ad);
          compPx[i * 4 + 2] = cvt(linearLight(compPx[i * 4 + 2] / 255.0f, (*layerPx)[i * 4 + 2] / 255.0f, aa, ab), ad);
        }
        else if (l._mode == BlendMode::COLOR) {
          // also no premult colors
          RGBColor dest;
          dest._r = compPx[i * 4] / 255.0f;
          dest._g = compPx[i * 4 + 1] / 255.0f;
          dest._b = compPx[i * 4 + 2] / 255.0f;

          RGBColor src;
          src._r = (*layerPx)[i * 4] / 255.0f;
          src._g = (*layerPx)[i * 4 + 1] / 255.0f;
          src._b = (*layerPx)[i * 4 + 2] / 255.0f;

          RGBColor res = color(dest, src, aa, ab);
          compPx[i * 4] = cvt(res._r, ad);
          compPx[i * 4 + 1] = cvt(res._g, ad);;
          compPx[i * 4 + 2] = cvt(res._b, ad);;
        }
        else if (l._mode == BlendMode::LIGHTEN) {
          compPx[i * 4] = cvt(lighten(ra, rb, aa, ab), ad);
          compPx[i * 4 + 1] = cvt(lighten(ga, gb, aa, ab), ad);
          compPx[i * 4 + 2] = cvt(lighten(ba, bb, aa, ab), ad);
        }
        else if (l._mode == BlendMode::DARKEN) {
          compPx[i * 4] = cvt(darken(ra, rb, aa, ab), ad);
          compPx[i * 4 + 1] = cvt(darken(ga, gb, aa, ab), ad);
          compPx[i * 4 + 2] = cvt(darken(ba, bb, aa, ab), ad);
        }
        else if (l._mode == BlendMode::PIN_LIGHT) {
          compPx[i * 4] = cvt(pinLight(ra, rb, aa, ab), ad);
          compPx[i * 4 + 1] = cvt(pinLight(ga, gb, aa, ab), ad);
          compPx[i * 4 + 2] = cvt(pinLight(ba, bb, aa, ab), ad);
        }
      }

      // adjustment layer clean up, if applicable
      if (tmpLayer != nullptr) {
        delete tmpLayer;
      }
    }

    return comp;

  }

  RGBAColor Compositor::renderPixel(Context & c, int i, string size)
  {
    // photoshop appears to start with all white alpha 0 image
    RGBAColor compPx = { 1, 1, 1, 0 };

    if (size == "") {
      size = "full";
    }

    // blend the layers
    for (auto id : _layerOrder) {
      Layer& l = c[id];

      if (!l._visible)
        continue;

      RGBAColor layerPx;

      // handle adjustment layers
      if (l.isAdjustmentLayer()) {
        // ok so here we adjust the current composition, then blend it as normal below
        // create duplicate of current composite
        layerPx = adjustPixel(compPx, l);
      }
      else if (l.getAdjustments().size() > 0) {
        // so a layer may have other things clipped to it, in which case we apply the
        // specified adjustment only to the source layer and the composite as normal
        layerPx = adjustPixel(_imageData[l.getName()][size]->getPixel(i), l);
      }
      else {
        layerPx = _imageData[l.getName()][size]->getPixel(i);
      }

      // blend the layer
      // a = background, b = new layer
      // alphas
      float ab = layerPx._a * (l.getOpacity() / 100.0f);
      float aa = compPx._a;
      float ad = aa + ab - aa * ab;

      compPx._a = ad;

      // premult colors
      float rb = layerPx._r * ab;
      float gb = layerPx._g * ab;
      float bb = layerPx._b * ab;

      float ra = compPx._r * aa;
      float ga = compPx._g * aa;
      float ba = compPx._b * aa;

      // blend modes
      if (l._mode == BlendMode::NORMAL) {
        // b over a, standard alpha blend
        compPx._r = cvtf(normal(ra, rb, aa, ab), ad);
        compPx._g = cvtf(normal(ga, gb, aa, ab), ad);
        compPx._b = cvtf(normal(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::MULTIPLY) {
        compPx._r = cvtf(multiply(ra, rb, aa, ab), ad);
        compPx._g = cvtf(multiply(ga, gb, aa, ab), ad);
        compPx._b = cvtf(multiply(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::SCREEN) {
        compPx._r = cvtf(screen(ra, rb, aa, ab), ad);
        compPx._g = cvtf(screen(ga, gb, aa, ab), ad);
        compPx._b = cvtf(screen(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::OVERLAY) {
        compPx._r = cvtf(overlay(ra, rb, aa, ab), ad);
        compPx._g = cvtf(overlay(ga, gb, aa, ab), ad);
        compPx._b = cvtf(overlay(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::HARD_LIGHT) {
        compPx._r = cvtf(hardLight(ra, rb, aa, ab), ad);
        compPx._g = cvtf(hardLight(ga, gb, aa, ab), ad);
        compPx._b = cvtf(hardLight(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::SOFT_LIGHT) {
        compPx._r = cvtf(softLight(ra, rb, aa, ab), ad);
        compPx._g = cvtf(softLight(ga, gb, aa, ab), ad);
        compPx._b = cvtf(softLight(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::LINEAR_DODGE) {
        // special override for alpha here
        ad = (aa + ab > 1) ? 1 : (aa + ab);
        compPx._a = ad;

        compPx._r = cvtf(linearDodge(ra, rb, aa, ab), ad);
        compPx._g = cvtf(linearDodge(ga, gb, aa, ab), ad);
        compPx._b = cvtf(linearDodge(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::COLOR_DODGE) {
        compPx._r = cvtf(colorDodge(ra, rb, aa, ab), ad);
        compPx._g = cvtf(colorDodge(ga, gb, aa, ab), ad);
        compPx._b = cvtf(colorDodge(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::LINEAR_BURN) {
        // need unmultiplied colors for this one
        compPx._r = cvtf(linearBurn(compPx._r, layerPx._r, aa, ab), ad);
        compPx._g = cvtf(linearBurn(compPx._g, layerPx._g, aa, ab), ad);
        compPx._b = cvtf(linearBurn(compPx._b, layerPx._b, aa, ab), ad);
      }
      else if (l._mode == BlendMode::LINEAR_LIGHT) {
        compPx._r = cvtf(linearLight(compPx._r, layerPx._r, aa, ab), ad);
        compPx._g = cvtf(linearLight(compPx._g, layerPx._g, aa, ab), ad);
        compPx._b = cvtf(linearLight(compPx._b, layerPx._b, aa, ab), ad);
      }
      else if (l._mode == BlendMode::COLOR) {
        // also no premult colors
        RGBColor dest;
        dest._r = compPx._r;
        dest._g = compPx._g;
        dest._b = compPx._b;

        RGBColor src;
        src._r = layerPx._r;
        src._g = layerPx._g;
        src._b = layerPx._b;

        RGBColor res = color(dest, src, aa, ab);
        compPx._r = cvtf(res._r, ad);
        compPx._g = cvtf(res._g, ad);
        compPx._b = cvtf(res._b, ad);
      }
      else if (l._mode == BlendMode::LIGHTEN) {
        compPx._r = cvtf(lighten(ra, rb, aa, ab), ad);
        compPx._g = cvtf(lighten(ga, gb, aa, ab), ad);
        compPx._b = cvtf(lighten(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::DARKEN) {
        compPx._r = cvtf(darken(ra, rb, aa, ab), ad);
        compPx._g = cvtf(darken(ga, gb, aa, ab), ad);
        compPx._b = cvtf(darken(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::PIN_LIGHT) {
        compPx._r = cvtf(pinLight(ra, rb, aa, ab), ad);
        compPx._g = cvtf(pinLight(ga, gb, aa, ab), ad);
        compPx._b = cvtf(pinLight(ba, bb, aa, ab), ad);
      }
    }

    return compPx;
  }

  RGBAColor Compositor::renderPixel(Context & c, int x, int y, string size)
  {
    int width, height;

    if (_imageData.begin()->second.count(size) > 0) {
      width = _imageData.begin()->second[size]->getWidth();
      height = _imageData.begin()->second[size]->getHeight();
    }
    else {
      getLogger()->log("No render size named " + size + " found. Rendering at full size.", LogLevel::WARN);
      width = _imageData.begin()->second["full"]->getWidth();
      height = _imageData.begin()->second["full"]->getHeight();
    }

    int index = x + y * width;

    return renderPixel(c, index, size);
  }

  RGBAColor Compositor::renderPixel(Context & c, float x, float y, string size)
  {
    int width, height;

    if (_imageData.begin()->second.count(size) > 0) {
      width = _imageData.begin()->second[size]->getWidth();
      height = _imageData.begin()->second[size]->getHeight();
    }
    else {
      getLogger()->log("No render size named " + size + " found. Rendering at full size.", LogLevel::WARN);
      width = _imageData.begin()->second["full"]->getWidth();
      height = _imageData.begin()->second["full"]->getHeight();
    }

    int index = (int)(x * width) + (int)(y * height) * width;

    return renderPixel(c, index, size);
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

  vector<string> Compositor::getCacheSizes()
  {
    vector<string> sizes;

    for (auto kvp : _imageData.begin()->second) {
      sizes.push_back(kvp.first);
    }

    return sizes;
  }

  bool Compositor::addCacheSize(string name, float scaleFactor)
  {
    // not allowed to change default sizes
    if (name == "thumb" || name == "small" || name == "medium" || name == "full") {
      getLogger()->log("Changing default cache sizes is not allowed. Attempted to change " + name + ".", LogLevel::ERR);
      return false;
    }

    // delete existing
    for (auto kvp : _imageData) {
      kvp.second.erase(name);
    }

    // recompute
    for (auto i : _imageData) {
      _imageData[i.first][name] = i.second["full"]->resize(scaleFactor);
    }

    return true;
  }

  bool Compositor::deleteCacheSize(string name)
  {
    // not allowed to change default sizes
    if (name == "thumb" || name == "small" || name == "medium" || name == "full") {
      getLogger()->log("Changing default cache sizes is not allowed. Attempted to change " + name + ".", LogLevel::ERR);
      return false;
    }

    // delete existing
    for (auto kvp : _imageData) {
      kvp.second.erase(name);
    }

    return true;
  }

  shared_ptr<Image> Compositor::getCachedImage(string id, string size)
  {
    if (_imageData.count(id) > 0) {
      if (_imageData[id].count(size) > 0) {
        return _imageData[id][size];
      }
    }

    return nullptr;
  }

  void Compositor::startSearch(searchCallback cb, SearchMode mode, map<string, float> settings,
    int threads, string searchRenderSize)
  {
    if (_searchRunning) {
      stopSearch();
    }

    _searchThreads.clear();
    _searchThreads.resize(threads);

    _activeCallback = cb;
    _searchMode = mode;
    _searchSettings = settings;
    _searchRunning = true;
    _searchRenderSize = searchRenderSize;

    stringstream ss;
    ss << "Starting search with " << threads << " threads";
    getLogger()->log(ss.str(), LogLevel::INFO);

    // precompute ops, if any
    if (mode == RANDOM) {
      _affectedLayers.clear();

      // precompute affected layers
      if (settings["useVisibleLayersOnly"] > 0) {
        for (auto l : getPrimaryContext()) {
          if (l.second._visible)
            _affectedLayers.insert(l.first);
        }
      }
      else {
        // add everything
        for (auto l : getPrimaryContext())
          _affectedLayers.insert(l.first);
      }

      // check settings, initialize defaults if not exist
      if (_searchSettings.count("modifyLayerBlendModes") == 0) {
        _searchSettings["modifyLayerBlendModes"] = 0;
      }
    }

    // start threads
    for (int i = 0; i < threads; i++) {
      _searchThreads[i] = thread(&Compositor::runSearch, this);
    }

    getLogger()->log("Search threads launched.", LogLevel::INFO);
    return;
  }

  void Compositor::stopSearch()
  {
    // if the stop process has already been initiated return
    if (!_searchRunning) {
      return;
    }

    _searchRunning = false;
    getLogger()->log("Waiting for all threads to finish...", LogLevel::INFO);

    // wait for threads to finish
    for (int i = 0; i < _searchThreads.size(); i++) {
      stringstream ss;
      ss << "Waiting for thread " << i << "...";
      getLogger()->log(ss.str(), LogLevel::INFO);

      _searchThreads[i].join();

      ss.str("");
      ss << "Thread " << i << " stopped";
      getLogger()->log(ss.str(), LogLevel::INFO);
    }

    getLogger()->log("Search stopped");
    return;
  }

  void Compositor::runSearch()
  {
    while (_searchRunning) {
      // do stuff
      if (_searchMode == SEARCH_DEBUG) {
        // debug: returns a render of the current context every time it runs
        Image* r = render(_searchRenderSize);
        _activeCallback(r, Context(getPrimaryContext()), map<string, float>());
        this_thread::sleep_for(5s);
      }
      else if (_searchMode == RANDOM) {
        // random will take the available parameters of the layer and just
        // randomly change them
        randomSearch(Context(getPrimaryContext()));
      }
    }
  }

  void Compositor::resetImages(string name)
  {
    if (_imageData.count(name) == 0)
      return;

    for (auto img : _imageData[name])
      img.second->reset(1, 1, 1);
  }

  void Compositor::addLayer(string name)
  {
    _primary[name] = Layer(name, _imageData[name]["full"]);

    // place at end of order
    _layerOrder.push_back(name);

    getLogger()->log("Added new layer named " + name);
  }

  void Compositor::cacheScaled(string name)
  {
    _imageData[name]["thumb"] = _imageData[name]["full"]->resize(0.15f);
    _imageData[name]["small"] = _imageData[name]["full"]->resize(0.25f);
    _imageData[name]["medium"] = _imageData[name]["full"]->resize(0.5f);
  }

  void Compositor::randomSearch(Context start)
  {
    // note: this function runs in a threaded context. start is assumed to
    // be an independent context that can be freely manipulated.
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<float> dist(0, 1);
    uniform_real_distribution<float> range(-1, 1);

    for (auto& name : _affectedLayers) {
      Layer& l = start[name];
      
      // randomize opacity
      l.setOpacity(dist(gen) * 100);

      // randomize visibility
      l._visible = (dist(gen) >= 0.5) ? true : false;

      // if its not visible anymore stop
      if (!l._visible)
        continue;

      // randomize blend mode?
      if (_searchSettings["modifyLayerBlendModes"] > 0) {
        uniform_int_distribution<int> modeDist(0, 13);
        l._mode = (BlendMode)(modeDist(gen));
      }

      // randomize adjustments
      for (auto a : l.getAdjustments()) {
        // each adjustment has its own parameter limits...
        switch (a) {
        case HSL:
          l.addHSLAdjustment(range(gen) * 180, range(gen) * 100, range(gen) * 100);
          break;
        case LEVELS:
          l.addLevelsAdjustment(dist(gen) * 255, dist(gen) * 255, dist(gen) * 2, dist(gen) * 255, dist(gen) * 255);
          break;
        case CURVES:
          // here for completion but it doesn't make much sense to touch this
          break;
        case EXPOSURE:
          l.addExposureAdjustment(range(gen) * 2.5f, range(gen) * 0.5f, dist(gen) * 2);
          break;
        case GRADIENT:
          // also here for completion but also doesn't make much sense, maybe more sense thatn levels though
          break;
        case SELECTIVE_COLOR:
          // skipping for now there are like 36 parameters here
          break;
        case COLOR_BALANCE:
        {
          bool luma = (l.getAdjustment(COLOR_BALANCE)["preserveLuma"] > 0) ? true : false;
          l.addColorBalanceAdjustment(luma, dist(gen), dist(gen), dist(gen), dist(gen), dist(gen), dist(gen), dist(gen), dist(gen), dist(gen));
          break;
        }
        case PHOTO_FILTER:
        {
          bool luma = (l.getAdjustment(PHOTO_FILTER)["preserveLuma"] > 0) ? true : false;
          l.addPhotoFilterAdjustment(luma, dist(gen), dist(gen), dist(gen), dist(gen));
          break;
        }
        case COLORIZE:
        case LIGHTER_COLORIZE:
        case OVERWRITE_COLOR:
        {
          // group these together they have the same interface
          l.addAdjustment(a, "r", dist(gen));
          l.addAdjustment(a, "g", dist(gen));
          l.addAdjustment(a, "b", dist(gen));
          l.addAdjustment(a, "a", dist(gen));
          break;
        }
        default:
          break;
        }
      }
    }

    // render
    Image* r = render(start, _searchRenderSize);

    // callback
    _activeCallback(r, start, map<string, float>());
  }

  inline float Compositor::premult(unsigned char px, float a)
  {
    return (float)((px / 255.0f) * a);
  }

  inline unsigned char Compositor::cvt(float px, float a)
  {
    float v = ((px / a) * 255);
    return (unsigned char)((v > 255) ? 255 : (v < 0) ? 0 : v);
  }

  inline float Compositor::cvtf(float px, float a)
  {
    float v = px / a;
    return (v > 1) ? 1 : (v < 0) ? 0 : v;
  }

  inline float Compositor::normal(float a, float b, float alpha1, float alpha2)
  {
    return b + a * (1 - alpha2);
  }

  inline float Compositor::multiply(float a, float b, float alpha1, float alpha2)
  {
    return b * a + b * (1 - alpha1) + a * (1 - alpha2);
  }

  inline float Compositor::screen(float a, float b, float alpha1, float alpha2)
  {
    return b + a - b * a;
  }

  inline float Compositor::overlay(float a, float b, float alpha1, float alpha2)
  {
    if (2 * a <= alpha1) {
      return b * a * 2 + b * (1 - alpha1) + a * (1 - alpha2);
    }
    else {
      return b * (1 + alpha1) + a * (1 + alpha2) - 2 * a * b - alpha1 * alpha2;
    }
  }

  inline float Compositor::hardLight(float a, float b, float alpha1, float alpha2)
  {
    if (2 * b <= alpha2) {
      return 2 * b * a + b * (1 - alpha1) + a * (1 - alpha2);
    }
    else {
      return b * (1 + alpha1) + a * (1 + alpha2) - alpha1 * alpha2 - 2 * a * b;
    }
  }

  inline float Compositor::softLight(float Dca, float Sca, float Da, float Sa)
  {
    float m = (Da == 0) ? 0 : Dca / Da;

    if (2 * Sca <= Sa) {
      return Dca * (Sa + (2 * Sca - Sa) * (1 - m)) + Sca * (1 - Da) + Dca * (1 - Sa);
    }
    else if (2 * Sca > Sa && 4 * Dca <= Da) {
      return Da * (2 * Sca - Sa) * (16 * m * m * m - 12 * m * m - 3 * m) + Sca - Sca * Da + Dca;
    }
    else if (2 * Sca > Sa && 4 * Dca > Da){
      return Da * (2 * Sca - Sa) * (sqrt(m) - m) + Sca - Sca * Da + Dca;
    }
    else {
      return normal(Dca, Sca, Da, Sa);
    }
    
  }

  inline float Compositor::linearDodge(float Dca, float Sca, float Da, float Sa)
  {
    return Sca + Dca;
  }

  inline float Compositor::colorDodge(float Dca, float Sca, float Da, float Sa)
  {
    if (Sca == Sa && Dca == 0) {
      return Sca * (1 - Da);
    }
    else if (Sca == Sa) {
      return Sa * Da + Sca * (1 - Da) + Dca * (1 - Sa);
    }
    else if (Sca < Sa) {
      return Sa * Da * min(1.0f, Dca / Da * Sa / (Sa - Sca)) + Sca * (1 - Da) + Dca * (1 - Sa);
    }

    // probably never get here but compiler is yelling at me
    return 0;
  }

  inline float Compositor::linearBurn(float Dc, float Sc, float Da, float Sa)
  {
    // special case for handling background with alpha 0
    if (Da == 0)
      return Sc;

    float burn = Dc + Sc - 1;

    // normal blend
    return burn * Sa + Dc * (1 - Sa);
  }

  inline float Compositor::linearLight(float Dc, float Sc, float Da, float Sa)
  {
    if (Da == 0)
      return Sc;
    
    float light = Dc + 2 * Sc - 1;
    return light * Sa + Dc * (1 - Sa);
  }

  inline RGBColor Compositor::color(RGBColor & dest, RGBColor & src, float Da, float Sa)
  {
    // so it's unclear if this composition operation happens in the full LCH color space or
    // if we can approximate it with a jank HSY color space so let's try it before doing the full
    // implementation
    if (Da == 0) {
      src._r *= Sa;
      src._g *= Sa;
      src._b *= Sa;

      return src;
    }

    if (Sa == 0) {
      dest._r *= Da;
      dest._g *= Da;
      dest._b *= Da;

      return dest;
    }

    // color keeps dest luma and keeps top hue and chroma
    HSYColor dc = RGBToHSY(dest);
    HSYColor sc = RGBToHSY(src);
    dc._h = sc._h;
    dc._s = sc._s;

    RGBColor res = HSYToRGB(dc);

    // actually have to blend here...
    res._r = res._r * Sa + dest._r * Da * (1 - Sa);
    res._g = res._g * Sa + dest._g * Da * (1 - Sa);
    res._b = res._b * Sa + dest._b * Da * (1 - Sa);

    return res;
  }

  inline float Compositor::lighten(float Dca, float Sca, float Da, float Sa)
  {
    if (Sca > Dca) {
      return Sca + Dca * (1 - Sa);
    }
    else {
      return Dca + Sca * (1 - Da);
    }
  }

  inline float Compositor::darken(float Dca, float Sca, float Da, float Sa)
  {
    if (Sca > Dca) {
      return Dca + Sca * (1 - Da);
    }
    else {
      return Sca + Dca * (1 - Sa);
    }
  }

  inline float Compositor::pinLight(float Dca, float Sca, float Da, float Sa)
  {
    if (Da == 0)
      return Sca;

    if (Sca < 0.5f) {
      return darken(Dca, Sca * 2, Da, Sa);
    }
    else {
      return lighten(Dca, 2 * (Sca - 0.5f), Da, Sa);
    }
  }

  void Compositor::adjust(Image * adjLayer, Layer& l)
  {
    // only certain modes are recognized
    for (auto type : l.getAdjustments()) {
      if (type == AdjustmentType::HSL) {
        hslAdjust(adjLayer, l.getAdjustment(type));
      }
      else if (type == AdjustmentType::LEVELS) {
        levelsAdjust(adjLayer, l.getAdjustment(type));
      }
      else if (type == AdjustmentType::CURVES) {
        curvesAdjust(adjLayer, l.getAdjustment(type), l);
      }
      else if (type == AdjustmentType::EXPOSURE) {
        exposureAdjust(adjLayer, l.getAdjustment(type));
      }
      else if (type == AdjustmentType::GRADIENT) {
        gradientMap(adjLayer, l.getAdjustment(type), l);
      }
      else if (type == AdjustmentType::SELECTIVE_COLOR) {
        selectiveColor(adjLayer, l.getAdjustment(type), l);
      }
      else if (type == AdjustmentType::COLOR_BALANCE) {
        colorBalanceAdjust(adjLayer, l.getAdjustment(type));
      }
      else if (type == AdjustmentType::PHOTO_FILTER) {
        photoFilterAdjust(adjLayer, l.getAdjustment(type));
      }
      else if (type == AdjustmentType::COLORIZE) {
        colorizeAdjust(adjLayer, l.getAdjustment(type));
      }
      else if (type == AdjustmentType::LIGHTER_COLORIZE) {
        lighterColorizeAdjust(adjLayer, l.getAdjustment(type));
      }
      else if (type == AdjustmentType::OVERWRITE_COLOR) {
        overwriteColorAdjust(adjLayer, l.getAdjustment(type));
      }
    }
  }

  RGBAColor Compositor::adjustPixel(RGBAColor comp, Layer & l)
  {
    for (auto type : l.getAdjustments()) {
      if (type == AdjustmentType::HSL) {
        hslAdjust(comp, l.getAdjustment(type));
      }
      else if (type == AdjustmentType::LEVELS) {
        levelsAdjust(comp, l.getAdjustment(type));
      }
      else if (type == AdjustmentType::CURVES) {
        curvesAdjust(comp, l.getAdjustment(type), l);
      }
      else if (type == AdjustmentType::EXPOSURE) {
        exposureAdjust(comp, l.getAdjustment(type));
      }
      else if (type == AdjustmentType::GRADIENT) {
        gradientMap(comp, l.getAdjustment(type), l);
      }
      else if (type == AdjustmentType::SELECTIVE_COLOR) {
        selectiveColor(comp, l.getAdjustment(type), l);
      }
      else if (type == AdjustmentType::COLOR_BALANCE) {
        colorBalanceAdjust(comp, l.getAdjustment(type));
      }
      else if (type == AdjustmentType::PHOTO_FILTER) {
        photoFilterAdjust(comp, l.getAdjustment(type));
      }
      else if (type == AdjustmentType::COLORIZE) {
        colorizeAdjust(comp, l.getAdjustment(type));
      }
      else if (type == AdjustmentType::LIGHTER_COLORIZE) {
        lighterColorizeAdjust(comp, l.getAdjustment(type));
      }
      else if (type == AdjustmentType::OVERWRITE_COLOR) {
        overwriteColorAdjust(comp, l.getAdjustment(type));
      }
    }

    return comp;
  }

  inline void Compositor::hslAdjust(Image * adjLayer, map<string, float> adj)
  {
    // Right now the bare-bones hsl adjustment is here. PS has a lot of options for carefully
    // crafting remappings, but we don't do that for now.
    // basically we convert to hsl, add the proper adjustment, convert back to rgb8
    vector<unsigned char>& img = adjLayer->getData();

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor layerPx = adjLayer->getPixel(i);
      hslAdjust(layerPx, adj);

      // convert to char
      img[i * 4] = (unsigned char) (layerPx._r * 255);
      img[i * 4 + 1] = (unsigned char) (layerPx._g * 255);
      img[i * 4 + 2] = (unsigned char) (layerPx._b * 255);
    }
  }

  inline void Compositor::hslAdjust(RGBAColor& adjPx, map<string, float>& adj)
  {
    float h = adj["hue"];
    float s = adj["sat"];
    float l = adj["light"];

    HSLColor c = RGBToHSL(adjPx._r, adjPx._g, adjPx._b);

    // modify hsl. h is in degrees, and s and l will be out of 100 due to how photoshop represents that
    c._h += h;
    c._s += s / 100.0f;
    c._l += l / 100.0f;

    // convert back
    RGBColor c2 = HSLToRGB(c);
    adjPx._r = c2._r;
    adjPx._g = c2._g;
    adjPx._b = c2._b;
  }

  inline void Compositor::levelsAdjust(Image* adjLayer, map<string, float> adj) {
    vector<unsigned char>& img = adjLayer->getData();

    // so sometimes these values are missing and we should use defaults.
    float inMin = (adj.count("inMin") > 0) ? adj["inMin"] : 0;
    float inMax = (adj.count("inMax") > 0) ? adj["inMax"] : 255;
    float gamma = (adj.count("gamma") > 0) ? adj["gamma"] : 1;
    float outMin = (adj.count("outMin") > 0) ? adj["outMin"] : 0;
    float outMax = (adj.count("outMax") > 0) ? adj["outMax"] : 255;

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor layerPx = adjLayer->getPixel(i);
      levelsAdjust(layerPx, adj);

      // convert to char
      img[i * 4] = (unsigned char)(layerPx._r * 255);
      img[i * 4 + 1] = (unsigned char)(layerPx._g * 255);
      img[i * 4 + 2] = (unsigned char)(layerPx._b * 255);
    }
  }

  inline void Compositor::levelsAdjust(RGBAColor & adjPx, map<string, float>& adj)
  {
    // so sometimes these values are missing and we should use defaults.
    float inMin = (adj.count("inMin") > 0) ? adj["inMin"] : 0;
    float inMax = (adj.count("inMax") > 0) ? adj["inMax"] : 255;
    float gamma = (adj.count("gamma") > 0) ? adj["gamma"] : 1;
    float outMin = (adj.count("outMin") > 0) ? adj["outMin"] : 0;
    float outMax = (adj.count("outMax") > 0) ? adj["outMax"] : 255;

    adjPx._r = levels(adjPx._r, inMin / 255, inMax / 255, gamma, outMin / 255, outMax / 255);
    adjPx._g = levels(adjPx._g, inMin / 255, inMax / 255, gamma, outMin / 255, outMax / 255);
    adjPx._b = levels(adjPx._b, inMin / 255, inMax / 255, gamma, outMin / 255, outMax / 255);
  }

  inline float Compositor::levels(float px, float inMin, float inMax, float gamma, float outMin, float outMax)
  {
    // input remapping
    float out = min(max(px - inMin, 0.0f) / (inMax - inMin), 1.0f);

    // gamma correction
    out = pow(out, 1 / gamma);

    // output remapping
    out = out * (outMax - outMin) + outMin;

    return out;
  }

  inline void Compositor::curvesAdjust(Image * adjLayer, map<string, float> adj, Layer & l)
  {
    // fairly simple adjustment
    // take each channel, modify the pixel values by the specified curve
    // after per-channel adjustments are done, then apply the RGB (brightness) curve which really
    // should be labeled value because that's what that curve seems to affect
    vector<unsigned char>& img = adjLayer->getData();

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor adjPx = adjLayer->getPixel(i);
      curvesAdjust(adjPx, adj, l);

      img[i * 4] = (unsigned char)(clamp(adjPx._r, 0, 1) * 255);
      img[i * 4 + 1] = (unsigned char)(clamp(adjPx._g, 0, 1) * 255);
      img[i * 4 + 2] = (unsigned char)(clamp(adjPx._b, 0, 1) * 255);
    }
  }

  inline void Compositor::curvesAdjust(RGBAColor & adjPx, map<string, float>& adj, Layer & l)
  {
    adjPx._r = l.evalCurve("red", adjPx._r);
    adjPx._g = l.evalCurve("green", adjPx._g);
    adjPx._b = l.evalCurve("blue", adjPx._b);

    // short circuit this to avoid unnecessary conversion if the curve
    // doesn't actually exist
    if (adj.count("RGB") > 0) {
      adjPx._r = l.evalCurve("RGB", adjPx._r);
      adjPx._g = l.evalCurve("RGB", adjPx._g);
      adjPx._b = l.evalCurve("RGB", adjPx._b);
    }
  }

  inline void Compositor::exposureAdjust(Image * adjLayer, map<string, float> adj)
  {
    vector<unsigned char>& img = adjLayer->getData();

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor adjPx = adjLayer->getPixel(i);
      exposureAdjust(adjPx, adj);

      img[i * 4] = (unsigned char)(adjPx._r * 255);
      img[i * 4 + 1] = (unsigned char)(adjPx._g * 255);
      img[i * 4 + 2] = (unsigned char)(adjPx._b * 255);
    }
  }

  inline void Compositor::exposureAdjust(RGBAColor & adjPx, map<string, float>& adj)
  {
    float exposure = adj["exposure"];
    float offset = adj["offset"];
    float gamma = adj["gamma"];

    adjPx._r = clamp(pow(adjPx._r * pow(2, exposure) + offset, 1 / gamma), 0, 1);
    adjPx._g = clamp(pow(adjPx._g * pow(2, exposure) + offset, 1 / gamma), 0, 1);
    adjPx._b = clamp(pow(adjPx._b * pow(2, exposure) + offset, 1 / gamma), 0, 1);
  }

  inline void Compositor::gradientMap(Image * adjLayer, map<string, float> adj, Layer& l)
  {
    vector<unsigned char>& img = adjLayer->getData();

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor adjPx = adjLayer->getPixel(i);
      gradientMap(adjPx, adj, l);

      img[i * 4] = (unsigned char)(adjPx._r * 255);
      img[i * 4 + 1] = (unsigned char)(adjPx._g * 255);
      img[i * 4 + 2] = (unsigned char)(adjPx._b * 255);
    }
  }

  inline void Compositor::gradientMap(RGBAColor & adjPx, map<string, float>& adj, Layer & l)
  {
    float y = 0.299f * adjPx._r + 0.587f * adjPx._g + 0.114f * adjPx._b;

    // map L to an rgb color. L is between 0 and 1.
    RGBColor grad = l.evalGradient(y);

    adjPx._r = clamp(grad._r, 0, 1);
    adjPx._g = clamp(grad._g, 0, 1);
    adjPx._b = clamp(grad._b, 0, 1);
  }

  inline void Compositor::selectiveColor(Image * adjLayer, map<string, float> adj, Layer & l)
  {
    // so according to the PS docs this will apply per-channel adjustments relative to
    // "how close the color is to an option in the menu" which implies a barycentric weighting
    // of the colors. Photoshop's internal color space is probably Lab so that seems like the right
    // approach, however the conversion to Lab and computation of barycentric coords is really expensive.
    // For now we'll approximate by using the HCL cone and some hacky interpolation because barycentric
    // coordinates get rather odd in a cone
    vector<unsigned char>& img = adjLayer->getData();

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor adjPx = adjLayer->getPixel(i);
      selectiveColor(adjPx, adj, l);

      img[i * 4] = (unsigned char)(clamp(adjPx._r, 0, 1) * 255);
      img[i * 4 + 1] = (unsigned char)(clamp(adjPx._g, 0, 1) * 255);
      img[i * 4 + 2] = (unsigned char)(clamp(adjPx._b, 0, 1) * 255);
    }
  }

  inline void Compositor::selectiveColor(RGBAColor & adjPx, map<string, float>& adj, Layer & l)
  {
    map<string, map<string, float>> data = l.getSelectiveColor();

    // convert to hsl
    HSLColor hslColor = RGBToHSL(adjPx._r, adjPx._g, adjPx._b);
    float chroma = max(adjPx._r, max(adjPx._g, adjPx._b)) - min(adjPx._r, min(adjPx._g, adjPx._b));

    // determine which set of parameters we're using to adjust
    // determine chroma interval
    int interval = (int)(hslColor._h / 60);
    string c1, c2, c3, c4;
    c1 = intervalNames[interval];

    if (interval == 5) {
      // wrap around for magenta
      c2 = intervalNames[0];
    }
    else {
      c2 = intervalNames[interval + 1];
    }

    c3 = "neutrals";

    // non-chromatic colors
    if (hslColor._l < 0.5) {
      c4 = "blacks";
    }
    else {
      c4 = "whites";
    }

    // compute weights
    float w1, w2, w3, w4, wc;

    // chroma
    wc = chroma / 1.0f;

    // hue - always 60 deg intervals
    w1 = 1 - ((hslColor._h - interval * 60.0f) / 60.0f);  // distance from low interval
    w2 = 1 - w1;

    // luma - measure distance from midtones, w3 is always midtone
    w3 = 1 - abs(hslColor._l - 0.5f);
    w4 = w3 - 1;

    // do the adjustment
    CMYKColor cmykColor = RGBToCMYK(adjPx._r, adjPx._g, adjPx._b);

    if (adj["relative"] > 0) {
      // relative
      cmykColor._c += cmykColor._c * (w1 * data[c1]["cyan"] + w2 * data[c2]["cyan"]) * wc + (w3 * data[c3]["cyan"] + w4 * data[c4]["cyan"]) * (1 - wc);
      cmykColor._m += cmykColor._m * (w1 * data[c1]["magenta"] + w2 * data[c2]["magenta"]) * wc + (w3 * data[c3]["magenta"] + w4 * data[c4]["magenta"]) * (1 - wc);
      cmykColor._y += cmykColor._y * (w1 * data[c1]["yellow"] + w2 * data[c2]["yellow"]) * wc + (w3 * data[c3]["yellow"] + w4 * data[c4]["yellow"]) * (1 - wc);
      cmykColor._k += cmykColor._k * (w1 * data[c1]["black"] + w2 * data[c2]["black"]) * wc + (w3 * data[c3]["black"] + w4 * data[c4]["black"]) * (1 - wc);
    }
    else {
      // absolute
      cmykColor._c += (w1 * data[c1]["cyan"] + w2 * data[c2]["cyan"]) * wc + (w3 * data[c3]["cyan"] + w4 * data[c4]["cyan"]) * (1 - wc);
      cmykColor._m += (w1 * data[c1]["magenta"] + w2 * data[c2]["magenta"]) * wc + (w3 * data[c3]["magenta"] + w4 * data[c4]["magenta"]) * (1 - wc);
      cmykColor._y += (w1 * data[c1]["yellow"] + w2 * data[c2]["yellow"]) * wc + (w3 * data[c3]["yellow"] + w4 * data[c4]["yellow"]) * (1 - wc);
      cmykColor._k += (w1 * data[c1]["black"] + w2 * data[c2]["black"]) * wc + (w3 * data[c3]["black"] + w4 * data[c4]["black"]) * (1 - wc);
    }

    RGBColor res = CMYKToRGB(cmykColor);
    adjPx._r = res._r;
    adjPx._g = res._g;
    adjPx._b = res._b;
  }

  inline void Compositor::colorBalanceAdjust(Image * adjLayer, map<string, float> adj)
  {
    // the color balance adjustment is based on the GIMP implementation due to the
    // complexities involved with reverse-engineering the photoshop balance adjustment
    // from scratch (it appears non-linear? dunno)
    // source: https://github.com/liovch/GPUImage/commit/fcc85db4fdafae1d4e41313c96bb1cac54dc93b4#diff-5acce76055236dedac2284353170c24aR117
    vector<unsigned char>& img = adjLayer->getData();

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor adjPx = adjLayer->getPixel(i);
      colorBalanceAdjust(adjPx, adj);

      img[i * 4] = (unsigned char)(clamp(adjPx._r, 0, 1) * 255);
      img[i * 4 + 1] = (unsigned char)(clamp(adjPx._g, 0, 1) * 255);
      img[i * 4 + 2] = (unsigned char)(clamp(adjPx._b, 0, 1) * 255);
    }
  }

  inline void Compositor::colorBalanceAdjust(RGBAColor & adjPx, map<string, float>& adj)
  {
    RGBColor balanced;
    balanced._r = colorBalance(adjPx._r, adj["shadowR"], adj["midR"], adj["highR"]);
    balanced._g = colorBalance(adjPx._g, adj["shadowG"], adj["midG"], adj["highG"]);
    balanced._b = colorBalance(adjPx._b, adj["shadowB"], adj["midB"], adj["highB"]);

    if (adj["preserveLuma"] > 0) {
      HSLColor l = RGBToHSL(balanced);
      float originalLuma = 0.5f * (max(adjPx._r, max(adjPx._g, adjPx._b)) + min(adjPx._r, min(adjPx._g, adjPx._b)));
      balanced = HSLToRGB(l._h, l._s, originalLuma);
    }

    adjPx._r = balanced._r;
    adjPx._g = balanced._g;
    adjPx._b = balanced._b;
  }

  inline float Compositor::colorBalance(float px, float shadow, float mid, float high)
  {
    // some arbitrary constants...?
    float a = 0.25f;
    float b = 0.333f;
    float scale = 0.7f;

    float s = shadow * (clamp((px - b) / -a + 0.5f, 0, 1.0f) * scale);
    float m = mid * (clamp((px - b) / a + 0.5f, 0, 1.0f) * clamp((px + b - 1.0f) / -a + 0.5f, 0, 1.0f) * scale);
    float h = high * (clamp((px + b - 1.0f) / a + 0.5f, 0, 1.0f) * scale);

    return clamp(px + s + m + h, 0, 1.0f);
  }

  inline void Compositor::photoFilterAdjust(Image * adjLayer, map<string, float> adj)
  {
    // so photoshop does some weird stuff here and I don't actually have any idea how
    // it manages to add color by "simulating a color filter" for certain colors
    // someone should explain to me how/why it actually adds things to specific colors but for now
    // we'll do the simple version
    vector<unsigned char>& img = adjLayer->getData();

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor adjPx = adjLayer->getPixel(i);
      photoFilterAdjust(adjPx, adj);

      img[i * 4] = (unsigned char)(adjPx._r * 255);
      img[i * 4 + 1] = (unsigned char)(adjPx._g * 255);
      img[i * 4 + 2] = (unsigned char)(adjPx._b * 255);
    }
  }

  inline void Compositor::photoFilterAdjust(RGBAColor & adjPx, map<string, float>& adj)
  {
    float d = adj["density"];
    float fr = adjPx._r * adj["r"];
    float fg = adjPx._g * adj["g"];
    float fb = adjPx._b * adj["b"];

    if (adj["preserveLuma"] > 0) {
      HSLColor l = RGBToHSL(fr, fg, fb);
      float originalLuma = 0.5f * (max(adjPx._r, max(adjPx._g, adjPx._b)) + min(adjPx._r, min(adjPx._g, adjPx._b)));
      RGBColor rgb = HSLToRGB(l._h, l._s, originalLuma);
      fr = rgb._r;
      fg = rgb._g;
      fb = rgb._b;
    }

    // weight by density
    adjPx._r = clamp(fr * d + adjPx._r * (1 - d), 0, 1);
    adjPx._g = clamp(fg * d + adjPx._g * (1 - d), 0, 1);
    adjPx._b = clamp(fb * d + adjPx._b * (1 - d), 0, 1);
  }

  inline void Compositor::colorizeAdjust(Image * adjLayer, map<string, float> adj)
  {
    // identical to color layer blend mode, assuming a solid color input layer
    vector<unsigned char>& img = adjLayer->getData();

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor adjPx = adjLayer->getPixel(i);
      colorizeAdjust(adjPx, adj);

      img[i * 4] = (unsigned char)(adjPx._r * 255);
      img[i * 4 + 1] = (unsigned char)(adjPx._g * 255);
      img[i * 4 + 2] = (unsigned char)(adjPx._b * 255);
    }
  }

  inline void Compositor::colorizeAdjust(RGBAColor & adjPx, map<string, float>& adj)
  { 
    float sr = adj["r"];
    float sg = adj["g"];
    float sb = adj["b"];
    float a = adj["a"];
    HSYColor sc = RGBToHSY(sr, sg, sb);

    // color keeps dest luma and keeps top hue and chroma
    HSYColor dc = RGBToHSY(adjPx._r, adjPx._g, adjPx._b);
    dc._h = sc._h;
    dc._s = sc._s;

    RGBColor res = HSYToRGB(dc);

    // blend the resulting colors according to alpha
    adjPx._r = clamp(res._r * a + adjPx._r * (1 - a), 0, 1);
    adjPx._g = clamp(res._g * a + adjPx._g * (1 - a), 0, 1);
    adjPx._b = clamp(res._b * a + adjPx._b * (1 - a), 0, 1);
  }

  inline void Compositor::lighterColorizeAdjust(Image * adjLayer, map<string, float> adj)
  {
    // identical to lighter color layer blend mode, assuming a solid color input layer
    // I'm fairly sure this is literally just max(dest.L, src.L)
    vector<unsigned char>& img = adjLayer->getData();

    

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor adjPx = adjLayer->getPixel(i);
      lighterColorizeAdjust(adjPx, adj);

      img[i * 4] = (unsigned char)(adjPx._r * 255);
      img[i * 4 + 1] = (unsigned char)(adjPx._g * 255);
      img[i * 4 + 2] = (unsigned char)(adjPx._b * 255);
    }
  }

  inline void Compositor::lighterColorizeAdjust(RGBAColor & adjPx, map<string, float>& adj)
  {
    float sr = adj["r"];
    float sg = adj["g"];
    float sb = adj["b"];
    float a = adj["a"];
    float y = 0.299f * sr + 0.587f * sg + 0.114f * sb;

    float yp = 0.299f * adjPx._r + 0.587f * adjPx._g + 0.114f * adjPx._b;

    adjPx._r = (yp > y) ? adjPx._r : sr;
    adjPx._g = (yp > y) ? adjPx._g : sg;
    adjPx._b = (yp > y) ? adjPx._b : sb;

    // blend the resulting colors according to alpha
    adjPx._r = clamp(adjPx._r * a + adjPx._r * (1 - a), 0, 1);
    adjPx._g = clamp(adjPx._g * a + adjPx._g * (1 - a), 0, 1);
    adjPx._b = clamp(adjPx._b * a + adjPx._b * (1 - a), 0, 1);
  }

  inline void Compositor::overwriteColorAdjust(Image * adjLayer, map<string, float> adj)
  {
    vector<unsigned char>& img = adjLayer->getData();

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor adjPx = adjLayer->getPixel(i);
      overwriteColorAdjust(adjPx, adj);

      img[i * 4] = (unsigned char)(adjPx._r * 255);
      img[i * 4 + 1] = (unsigned char)(adjPx._g * 255);
      img[i * 4 + 2] = (unsigned char)(adjPx._b * 255);
    }
  }

  inline void Compositor::overwriteColorAdjust(RGBAColor & adjPx, map<string, float>& adj)
  {
    float sr = adj["r"];
    float sg = adj["g"];
    float sb = adj["b"];
    float a = adj["a"];

    // blend the resulting colors according to alpha
    adjPx._r = clamp(sr * a + adjPx._r * (1 - a), 0, 1);
    adjPx._g = clamp(sg * a + adjPx._g * (1 - a), 0, 1);
    adjPx._b = clamp(sb * a + adjPx._b * (1 - a), 0, 1);
  }

}

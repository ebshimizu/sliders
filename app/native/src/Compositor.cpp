#include "Compositor.h"
#include "searchData.h"
#include "third_party/json/src/json.hpp"

namespace Comp {

  Compositor::Compositor() : _searchRunning(false)
  {
  }

  Compositor::Compositor(string filename, string imageDir)
  {
    // two iterations, file load and then layer load
    nlohmann::json data;
    ifstream input(imageDir + filename);

    if (!input.is_open()) {
      // failing to load will return an empty context
      getLogger()->log("Failed to open file " + imageDir + filename, LogLevel::ERR);
      return;
    }

    input >> data;

    if (data.count("layers") == 0) {
      // this is not a darkroom file, return
      getLogger()->log(imageDir + filename + " is not a Darkroom file", LogLevel::ERR);
      return;
    }

    for (nlohmann::json::iterator it = data["layers"].begin(); it != data["layers"].end(); ++it) {
      if (it.value()["filename"] != "") {
        addLayer(it.key(), imageDir + it.value()["filename"].get<string>());
      }
      else {
        addAdjustmentLayer(it.key());
      }
    }

    // layer order
    vector<string> order;
    for (int i = 0; i < data["layerOrder"].size(); i++) {
      order.push_back(data["layerOrder"][i]);
    }
    setLayerOrder(order);

    input.close();

    _primary = contextFromDarkroom(imageDir + filename);
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

  bool Compositor::addLayerMask(string name, string file)
  {
    // check for existence in primary context
    if (_primary.count(name) == 0) {
      getLogger()->log("Failed to add layer mask to " + name + ". Layer does not exist.");
      return false;
    }

    // load image data
    _layerMasks[name]["full"] = shared_ptr<Image>(new Image(file));
    addLayerMask(name);
    return true;
  }

  bool Compositor::addLayerMask(string name, Image & img)
  {
    // check for existence in primary context
    if (_primary.count(name) == 0) {
      getLogger()->log("Failed to add layer mask to " + name + ". Layer does not exist.");
      return false;
    }

    // load image data
    _layerMasks[name]["full"] = shared_ptr<Image>(new Image(img));
    addLayerMask(name);
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

    // update serialization key
    contextToVector(getNewContext(), _vectorKey);

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

    // update serialization key
    contextToVector(getNewContext(), _vectorKey);

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

    // update serialization key
    contextToVector(getNewContext(), _vectorKey);

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
    return (int)_layerOrder.size();
  }

  int Compositor::getWidth(string size)
  {
    if (size == "")
      size = "full";

    if (_imageData.size() == 0)
      return 0;
    
    return _imageData.begin()->second[size]->getWidth();
  }

  int Compositor::getHeight(string size)
  {
    if (size == "")
      size = "full";

    if (_imageData.size() == 0)
      return 0;

    return _imageData.begin()->second[size]->getHeight();
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

    // default mask (all white)
    Image* defaultMask = new Image(width, height);
    vector<unsigned char>& defaultMaskPx = defaultMask->getData();

    // Photoshop appears to blend using an all white alpha 0 image
    for (int i = 0; i < compPx.size(); i++) {
      if (i % 4 == 3) {
        defaultMaskPx[i] = 255;
        continue;
      }

      compPx[i] = 255;
      defaultMaskPx[i] = 255;
    }


    // blend the layers
    for (int lOrder = 0; lOrder < _layerOrder.size(); lOrder++) {
      string id = _layerOrder[lOrder];
      Layer& l = c[id];

      if (!l._visible)
        continue;

      vector<unsigned char>* layerPx;
      Image* tmpLayer = nullptr;
      vector<unsigned char>* layerMaskPx;
      bool shouldConditionalBlend = l.shouldConditionalBlend();
      auto cbData = l.getConditionalBlendSettings();
      float sbMin = cbData["srcBlackMin"];
      float sbMax = cbData["srcBlackMax"];
      float swMin = cbData["srcWhiteMin"];
      float swMax = cbData["srcWhiteMax"];
      float dbMin = cbData["destBlackMin"];
      float dbMax = cbData["destBlackMax"];
      float dwMin = cbData["destWhiteMin"];
      float dwMax = cbData["destWhiteMax"];
      bool hasMask = l.hasMask();

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

      // check for layer mask
      if (hasMask) {
        layerMaskPx = &_layerMasks[l.getName()][size]->getData();
      }
      else {
        layerMaskPx = &defaultMaskPx;
      }

      // blend the layer
      for (unsigned int i = 0; i < comp->numPx(); i++) {
        // pixel data is a flat array, rgba interlaced format
        // a = background, b = new layer
        // alphas
        float ab = ((*layerPx)[i * 4 + 3] / 255.0f) * l.getOpacity();
        float aa = compPx[i * 4 + 3] / 255.0f;

        // alpha ab is modulated by layer mask
        // layer mask is assumed greyscale, pull red channel as representative and premult with mask alpha
        float maskAlpha = ((*layerMaskPx)[i * 4] / 255.0f) * ((*layerMaskPx)[i * 4 + 3] / 255.0f);
        ab *= maskAlpha;

        // short circuit here if ab == 0
        if (ab == 0)
          continue;

        if (shouldConditionalBlend) {
          // i'm unsure if it works literally just on the layer below it or the composition up to this point
          float abScale = conditionalBlend(l.getConditionalBlendChannel(), sbMin,
            sbMax, swMin, swMax, dbMin, dbMax, dwMin, dwMax,
            (*layerPx)[i * 4] / 255.0f, (*layerPx)[i * 4 + 1] / 255.0f, (*layerPx)[i * 4 + 2] / 255.0f,
            compPx[i * 4] / 255.0f, compPx[i * 4 + 1] / 255.0f, compPx[i * 4 + 2] / 255.0f);

          ab = ab * abScale;
        }

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
          compPx[i * 4 + 1] = cvt(res._g, ad);
          compPx[i * 4 + 2] = cvt(res._b, ad);
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
        else if (l._mode == BlendMode::COLOR_BURN) {
          // also unmultiplied colors here
          compPx[i * 4] = cvt(colorBurn(compPx[i * 4] / 255.0f, (*layerPx)[i * 4] / 255.0f, aa, ab), ad);
          compPx[i * 4 + 1] = cvt(colorBurn(compPx[i * 4 + 1] / 255.0f, (*layerPx)[i * 4 + 1] / 255.0f, aa, ab), ad);
          compPx[i * 4 + 2] = cvt(colorBurn(compPx[i * 4 + 2] / 255.0f, (*layerPx)[i * 4 + 2] / 255.0f, aa, ab), ad);
        }
        else if (l._mode == BlendMode::VIVID_LIGHT) {
          compPx[i * 4] = cvt(vividLight(compPx[i * 4] / 255.0f, (*layerPx)[i * 4] / 255.0f, aa, ab), ad);
          compPx[i * 4 + 1] = cvt(vividLight(compPx[i * 4 + 1] / 255.0f, (*layerPx)[i * 4 + 1] / 255.0f, aa, ab), ad);
          compPx[i * 4 + 2] = cvt(vividLight(compPx[i * 4 + 2] / 255.0f, (*layerPx)[i * 4 + 2] / 255.0f, aa, ab), ad);
        }
      }

      // adjustment layer clean up, if applicable
      if (tmpLayer != nullptr) {
        delete tmpLayer;
      }
    }

    return comp;

  }

  vector<Importance> Compositor::localImportance(Context c, string size)
  {
    if (size == "") {
      size = "full";
    }

    vector<Importance> imp;
    Image* base = render(c, size);
    
    // do the expensive stuff first, mssim & derivatives
    // we're only going to look at opacity for now
    // other adjustments are important but right now it's not clear how color should be handled
    for (int i = 0; i < _layerOrder.size(); i++) {
      Layer l = c[_layerOrder[i]];

      // opacity (every layer has one)
      Importance li;
      li._depth = i;
      li._layerName = l.getName();
      li._adjType = AdjustmentType::OPACITY;
      li._param = "opacity";

      Context dc(c);
      float dval = l.getOpacity();
      dval = (dval <= 0.01) ? (dval + 0.01) : (dval - 0.01);
      dc[l.getName()].setOpacity(dval);

      Image* dbase = render(dc, size);

      // mssim
      li._mssim = dbase->MSSIM(base, 16, 1, 1, 1);

      // derivative
      li._deltaMag = finiteDifference(base, dbase, 0.01);
      
      // total alpha/luma
      if (l.isAdjustmentLayer()) {
        li._totalAlpha = 0;
        li._totalLuma = 0;
      }
      else {
        li._totalAlpha = getCachedImage(l.getName(), size)->totalAlpha();
        li._totalLuma = getCachedImage(l.getName(), size)->totalLuma();
      }

      getLogger()->log("Computed importance measures for " + li._layerName + ":" + li._param + "[" + to_string(li._adjType) + "]");

      imp.push_back(li);
      delete dbase;
    }

    delete base;
    return imp;
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

    // update serialization key
    contextToVector(getNewContext(), _vectorKey);

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

    _constraints._locked = true;
    _searchThreads.clear();
    _searchThreads.resize(threads);

    _activeCallback = cb;
    _searchMode = mode;
    _searchSettings = settings;
    _searchRunning = true;
    _searchRenderSize = searchRenderSize;
    _initSearchContext = getNewContext();

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

    if (mode == EXPLORATORY) {
      // TODO: Maybe temporary? Exploratory starts in its own single thread
      _searchThreads.resize(1);
      _searchThreads[0] = thread(&Compositor::exploratorySearch, this);
    }
    else {
      // start threads
      for (int i = 0; i < threads; i++) {
        _searchThreads[i] = thread(&Compositor::runSearch, this);
      }
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

    _constraints._locked = false;
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
        _activeCallback(r, Context(getPrimaryContext()), map<string, float>(), map<string, string>());
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

  void Compositor::computeExpContext(Context & c, int px, string functionName, string size)
  {
    getLogger()->log("Starting code generation...");

    if (size == "") {
      size = "full";
    }

    // ok so here we want to build the expression context that represents the entire render pipeline
    ExpContext ctx;

    // register functions
    ctx.registerFunc("linearDodgeAlpha", 2, 1);
    ctx.registerFunc("overlay", 4, 1);
    ctx.registerFunc("hardLight", 4, 1);
    ctx.registerFunc("softLight", 4, 1);
    ctx.registerFunc("linearBurn", 4, 1);
    ctx.registerFunc("linearLight", 4, 1);
    ctx.registerFunc("colorDodge", 4, 1);
    ctx.registerFunc("color", 8, 3);
    ctx.registerFunc("lighten", 4, 1);
    ctx.registerFunc("darken", 4, 1);
    ctx.registerFunc("pinLight", 4, 1);
    ctx.registerFunc("cvtT", 2, 1);
    ctx.registerFunc("clamp", 3, 1);
    ctx.registerFunc("RGBToHSL", 3, 3);
    ctx.registerFunc("HSLToRGB", 3, 3);
    ctx.registerFunc("HSYToRGB", 3, 3);
    ctx.registerFunc("LabToRGB", 3, 3);
    ctx.registerFunc("RGBToCMYK", 3, 4);
    ctx.registerFunc("RGBToHSY", 3, 3);
    ctx.registerFunc("levels", 6, 1);
    ctx.registerFunc("rgbCompand", 1, 1);
    ctx.registerFunc("selectiveColor", 9 * 4 + 3, 3);
    ctx.registerFunc("colorBalanceAdjust", 12, 3);
    ctx.registerFunc("photoFilter", 7, 3);
    ctx.registerFunc("lighterColorizeAdjust", 7, 3);

    // create variables (they get stored in the layer and image structures)
    int index = 0;
    
    for (auto& name : _layerOrder) {
      Layer& l = c[name];

      // skip invisible layers
      if (!l._visible)
        continue;

      if (_imageData.count(name) > 0 && !l.isAdjustmentLayer()) {
        getLogger()->log("Initializing pixel data for " + name);

        // create layer pixel vars
        index = _imageData[name][size]->initExp(ctx, name, index, px);
      }

      getLogger()->log("Initializing layer data for " + name);

      // create layer adjustment vars
      index = l.prepExp(ctx, index);
    }

    getLogger()->log("Context initialized");

    // get the result
    Utils<ExpStep>::RGBAColorT res = renderPixel<ExpStep>(c, px, size);

    getLogger()->log("Trace complete");

    ctx.registerResult(res._r, 0, "R");
    ctx.registerResult(res._g, 1, "G");
    ctx.registerResult(res._b, 2, "B");
    ctx.registerResult(res._a, 3, "A");

    getLogger()->log("Saving file");

    vector<string> sc = ctx.toSourceCode(functionName);
    ofstream file(functionName + ".h");
    for (auto &s : sc) {
      file << s << endl;
    }

    getLogger()->log("Code Generation Complete");
  }

  void Compositor::computeExpContext(Context & c, int x, int y, string functionName, string size)
  {
    int width, height;

    if (_imageData.begin()->second.count(size) > 0) {
      width = _imageData.begin()->second[size]->getWidth();
      height = _imageData.begin()->second[size]->getHeight();
    }
    else {
      getLogger()->log("No render size named " + size + " found. Using full size.", LogLevel::WARN);
      width = _imageData.begin()->second["full"]->getWidth();
      height = _imageData.begin()->second["full"]->getHeight();
    }

    int index = clamp(x, 0, width) + clamp(y, 0, height) * width;

    computeExpContext(c, index, functionName, size);
  }

  ConstraintData& Compositor::getConstraintData()
  {
    return _constraints;
  }

  void Compositor::paramsToCeres(Context& c, vector<Point> pts, vector<RGBColor> targetColor,
    vector<double> weights, string outputPath)
  {
    if (pts.size() != targetColor.size() || pts.size() != weights.size()) {
      getLogger()->log("paramsToCeres requires the same number of points, targets, and weights.", ERR);
      return;
    }

    nlohmann::json freeParams = nlohmann::json::array();

    for (auto& l : _layerOrder) {
      if (!c[l]._visible)
        continue;

      // gather parameter info
      c[l].addParams(freeParams);
    }

    nlohmann::json residuals = nlohmann::json::array();

    // gather layer color info
    for (int i = 0; i < pts.size(); i++) {
      Point& p = pts[i];

      int x = (int)p._x;
      int y = (int)p._y;

      nlohmann::json pixel;
      pixel["x"] = x;
      pixel["y"] = y;
      pixel["flat"] = x + y * getWidth();

      nlohmann::json tc;
      tc["r"] = targetColor[i]._r;
      tc["g"] = targetColor[i]._g;
      tc["b"] = targetColor[i]._b;

      // layer pixel colors
      nlohmann::json layers = nlohmann::json::array();
      for (auto& l : _layerOrder) {
        // adjustment layers have no pixel data
        if (_imageData.count(l) == 0)
          continue;

        // skip invisible
        if (!c[l]._visible)
          continue;

        RGBAColor color = c[l].getImage()->getPixel(x, y);

        nlohmann::json layerColor;
        layerColor["r"] = color._r;
        layerColor["g"] = color._g;
        layerColor["b"] = color._b;
        layerColor["a"] = color._a;
        layerColor["name"] = l;
        layers.push_back(layerColor);
      }

      nlohmann::json residual;
      residual["pixel"] = pixel;
      residual["layers"] = layers;
      residual["target"] = tc;
      residual["weight"] = weights[i];

      residuals.push_back(residual);
    }

    nlohmann::json data;
    data["params"] = freeParams;
    data["residuals"] = residuals;

    // write json to file
    ofstream file(outputPath);
    file << data.dump(4);
  }

  bool Compositor::ceresToContext(string file, map<string, float>& metadata, Context& c)
  {
    nlohmann::json data;
    ifstream input(file);

    getLogger()->log("Reading data from " + file);

    if (!input.is_open()) {
      getLogger()->log("File not open. Aborting...");
      return false;
    }

    input >> data;

    if (data.count("params") == 0) {
      // this is not acutally a ceres output function so return
      getLogger()->log("File is not a configuration output. Skipping...", WARN);
      return false;
    }

    c = getNewContext();

    getLogger()->log("Importing parameters...");

    for (int i = 0; i < data["params"].size(); i++) {
      auto param = data["params"][i];
      string layerName = param["layerName"].get<string>();

      getLogger()->log("Importing parameter " + param["adjustmentName"].get<string>() + " for layer " + layerName);

      if (param["adjustmentType"] == AdjustmentType::OPACITY) {
        c[layerName].setOpacity(param["value"].get<float>());
      }
      else if (param["adjustmentType"] == AdjustmentType::SELECTIVE_COLOR) {
        if (param["adjustmentName"].get<string>() == "relative") {
          c[layerName].addAdjustment((AdjustmentType)param["adjustmentType"].get<int>(), param["adjustmentName"].get<string>(), param["value"].get<float>());
        }
        else {
          c[layerName].setSelectiveColorChannel(param["selectiveColor"]["channel"], param["selectiveColor"]["color"], param["value"]);
        }
      }
      else {
        c[layerName].addAdjustment((AdjustmentType)param["adjustmentType"].get<int>(), param["adjustmentName"].get<string>(), param["value"].get<float>());
      }
    }

    getLogger()->log("Importing Metadata...");

    if (data.count("score") > 0) {
      float score = data["score"];
      metadata.insert(make_pair("score", score));
    }

    getLogger()->log("Import complete.");

    return true;
  }

  void Compositor::addSearchGroup(SearchGroup g)
  {
    _searchGroups.push_back(g);
  }

  void Compositor::clearSearchGroups()
  {
    _searchGroups.clear();
  }

  void Compositor::regionalImportance(string mode, vector<string>& names, vector<double>& scores, int x, int y, int w, int h)
  {
    names.clear();
    scores.clear();

    for (auto& id : _layerOrder) {
      names.push_back(id);

      if (mode == "alpha") {
        if (!_primary[id].isAdjustmentLayer()) {
          auto img = getCachedImage(id, "full");
          scores.push_back(img->avgAlpha(x, y, w, h));
        }
        else {
          scores.push_back(0);
        }
      }
    }
  }

  void Compositor::pointImportance(string mode, map<string, double>& scores, int x, int y, Context & c)
  {
    scores.clear();

    // store the current pixel color
    RGBAColor srcPixel = renderPixel<float>(c, x, y);

    for (auto& id : _layerOrder) {
      if (mode == "alpha") {
        if (!_primary[id].isAdjustmentLayer()) {
          auto img = getCachedImage(id, "full");

          scores[id] = (img->getPixel(x, y)._a);
        }
        else {
          scores[id] = 0;
        }
      }
      else if (mode == "visibilityDelta") {
        // the visibility delta is the magnitude of the pixel color difference
        // with the layer's visibility toggled
        Context toggle(c);
        toggle[id]._visible = !toggle[id]._visible;
        RGBAColor modPixel = renderPixel<float>(toggle, x, y, "full");

        // calculate difference
        // premultiplied alpha
        float rd = (srcPixel._r * srcPixel._a) - (modPixel._r * modPixel._a);
        float gd = (srcPixel._g * srcPixel._a) - (modPixel._g * modPixel._a);
        float bd = (srcPixel._b * srcPixel._a) - (modPixel._b * modPixel._a);
        float diff = sqrt(rd * rd + gd * gd + bd * bd);

        scores[id] = diff;

        // log it 
        getLogger()->log("visibilityDelta for " + id + ": " + to_string(diff));
      }
      else if (mode == "specVisibilityDelta") {
        // the speculative visibility delta is basically the same as the visibility
        // delta, but with some corner case handling
        // there are two visibility conditions: opacity > 0, visility on
        // - if layer is visible but opacity is 0, the delta is compared to opacity 1
        // - if layer is invisible but opacity is 0, the delta is comapred to visible + opacity 1
        // - if layer is visible, the delta is compared to invisible
        Context toggle(c);

        if (toggle[id].getOpacity() == 0) {
          toggle[id].setOpacity(1);

          if (!toggle[id]._visible) {
            toggle[id]._visible = true;
          }
        }
        else {
          toggle[id]._visible = !toggle[id]._visible;
        }

        RGBAColor modPixel = renderPixel<float>(toggle, x, y, "full");

        // calculate difference
        // premultiplied alpha
        float rd = (srcPixel._r * srcPixel._a) - (modPixel._r * modPixel._a);
        float gd = (srcPixel._g * srcPixel._a) - (modPixel._g * modPixel._a);
        float bd = (srcPixel._b * srcPixel._a) - (modPixel._b * modPixel._a);
        float diff = sqrt(rd * rd + gd * gd + bd * bd);

        scores[id] = diff;

        // log it 
        getLogger()->log("specVisibilityDelta for " + id + ": " + to_string(diff));
      }
    }
  }

  double Compositor::pointImportance(ImportanceMapMode mode, string layer, int x, int y, Context & c)
  {
    // store the current pixel color
    RGBAColor srcPixel = renderPixel<float>(c, x, y, "full");

    if (mode == ImportanceMapMode::ALPHA) {
      if (!_primary[layer].isAdjustmentLayer()) {
        shared_ptr<Image> img = getCachedImage(layer, "full");

        return img->getPixel(x, y)._a;
      }
      else {
        return 0;
      }
    }
    else if (mode == ImportanceMapMode::VISIBILITY_DELTA) {
      // the visibility delta is the magnitude of the pixel color difference
      // with the layer's visibility toggled
      Context toggle(c);
      toggle[layer]._visible = !toggle[layer]._visible;
      RGBAColor modPixel = renderPixel<float>(toggle, x, y);

      // calculate difference
      // premultiplied alpha
      float rd = (srcPixel._r * srcPixel._a) - (modPixel._r * modPixel._a);
      float gd = (srcPixel._g * srcPixel._a) - (modPixel._g * modPixel._a);
      float bd = (srcPixel._b * srcPixel._a) - (modPixel._b * modPixel._a);
      float diff = sqrt(rd * rd + gd * gd + bd * bd);

      // don'T LOG IT IT DOES THIS FOR EVERY PIXEL
      //getLogger()->log("visibilityDelta for " + id + ": " + to_string(diff));
      return diff;
    }
    else if (mode == ImportanceMapMode::SPEC_VISIBILITY_DELTA) {
      // the speculative visibility delta is basically the same as the visibility
      // delta, but with some corner case handling
      // there are two visibility conditions: opacity > 0, visility on
      // - if layer is visible but opacity is 0, the delta is compared to opacity 1
      // - if layer is invisible but opacity is 0, the delta is comapred to visible + opacity 1
      // - if layer is visible, the delta is compared to invisible
      Context toggle(c);

      if (toggle[layer].getOpacity() == 0) {
        toggle[layer].setOpacity(1);

        if (!toggle[layer]._visible) {
          toggle[layer]._visible = true;
        }
      }
      else {
        toggle[layer]._visible = !toggle[layer]._visible;
      }

      RGBAColor modPixel = renderPixel<float>(toggle, x, y, "full");

      // calculate difference
      // premultiplied alpha
      float rd = (srcPixel._r * srcPixel._a) - (modPixel._r * modPixel._a);
      float gd = (srcPixel._g * srcPixel._a) - (modPixel._g * modPixel._a);
      float bd = (srcPixel._b * srcPixel._a) - (modPixel._b * modPixel._a);
      float diff = sqrt(rd * rd + gd * gd + bd * bd);

      return diff;
    }
  }

  shared_ptr<ImportanceMap> Compositor::computeImportanceMap(string layer, ImportanceMapMode mode, Context& current)
  {
    getLogger()->log("Computing importance map for " + layer + " type " + to_string(mode));

    // point importance is slow so like render the entire image i guess
    int maxW = getWidth();
    int maxH = getHeight();
    shared_ptr<ImportanceMap> newMap = shared_ptr<ImportanceMap>(new ImportanceMap(maxW, maxH));
    shared_ptr<Image> currentImg = shared_ptr<Image>(render(current));

    if (mode == ImportanceMapMode::ALPHA) {
      if (!_primary[layer].isAdjustmentLayer()) {
        shared_ptr<Image> img = getCachedImage(layer, "full");

        for (int y = 0; y < maxH; y++) {
          for (int x = 0; x < maxW; x++) {
            newMap->setVal(img->getPixel(x, y)._a, x, y);
          }
        }
      }
    }
    else if (mode == ImportanceMapMode::VISIBILITY_DELTA || mode == ImportanceMapMode::SPEC_VISIBILITY_DELTA) {
      shared_ptr<Image> img;

      if (mode == ImportanceMapMode::VISIBILITY_DELTA) {
        // the visibility delta is the magnitude of the pixel color difference
        // with the layer's visibility toggled
        Context toggle(current);
        toggle[layer]._visible = !toggle[layer]._visible;
        img = shared_ptr<Image>(render(toggle));
      }
      else if (mode == ImportanceMapMode::SPEC_VISIBILITY_DELTA) {
        // the speculative visibility delta is basically the same as the visibility
        // delta, but with some corner case handling
        // there are two visibility conditions: opacity > 0, visility on
        // - if layer is visible but opacity is 0, the delta is compared to opacity 1
        // - if layer is invisible but opacity is 0, the delta is comapred to visible + opacity 1
        // - if layer is visible, the delta is compared to invisible
        Context toggle(current);

        if (toggle[layer].getOpacity() == 0) {
          toggle[layer].setOpacity(1);

          if (!toggle[layer]._visible) {
            toggle[layer]._visible = true;
          }
        }
        else {
          toggle[layer]._visible = !toggle[layer]._visible;
        }

        img = shared_ptr<Image>(render(toggle));
      }

      for (int y = 0; y < maxH; y++) {
        for (int x = 0; x < maxW; x++) {
          RGBAColor srcPixel = currentImg->getPixel(x, y);
          RGBAColor modPixel = img->getPixel(x, y);

          float rd = (srcPixel._r * srcPixel._a) - (modPixel._r * modPixel._a);
          float gd = (srcPixel._g * srcPixel._a) - (modPixel._g * modPixel._a);
          float bd = (srcPixel._b * srcPixel._a) - (modPixel._b * modPixel._a);
          float diff = sqrt(rd * rd + gd * gd + bd * bd);

          newMap->setVal(diff, x, y);
        }
      }
    }

    _importanceMapCache[layer][mode] = newMap;
    return newMap;
  }

  void Compositor::computeAllImportanceMaps(ImportanceMapMode mode, Context & current)
  {
    for (auto& l : _layerOrder) {
      computeImportanceMap(l, mode, current);
    }
  }

  shared_ptr<ImportanceMap> Compositor::getImportanceMap(string layer, ImportanceMapMode mode)
  {
    if (_importanceMapCache.count(layer) > 0) {
      if (_importanceMapCache[layer].count(mode) > 0) {
        return _importanceMapCache[layer][mode];
      }
    }

    return nullptr;
  }

  void Compositor::deleteImportanceMap(string layer, ImportanceMapMode mode)
  {
    if (_importanceMapCache.count(layer) > 0) {
      _importanceMapCache[layer].erase(mode);
      getLogger()->log("Deleted map " + to_string(mode) + " for layer " + layer);
    }
  }

  void Compositor::deleteLayerImportanceMaps(string layer)
  {
    _importanceMapCache.erase(layer);
    getLogger()->log("Deleted all maps for layer " + layer);
  }

  void Compositor::deleteImportanceMapType(ImportanceMapMode mode)
  {
    for (auto& kvp : _importanceMapCache) {
      kvp.second.erase(mode);
    }

    getLogger()->log("Deleted all maps of type " + to_string(mode));
  }

  void Compositor::deleteAllImportanceMaps()
  {
    _importanceMapCache.clear();
    getLogger()->log("Deleted all importance maps.");
  }

  void Compositor::dumpImportanceMaps(string folder)
  {
    // exports both an image and a raw json file containing the info about the importance maps
    for (auto& kvp : _importanceMapCache) {
      // for each type
      for (auto& type : kvp.second) {
        getLogger()->log("Exporting layer " + kvp.first + " map " + to_string(type.first), LogLevel::INFO);

        string base = kvp.first + "_" + to_string(type.first);
        type.second->dump(folder + "/", base);
      }
    }
  }

  bool Compositor::importanceMapExists(string layer, ImportanceMapMode mode)
  {
    if (_importanceMapCache.count(layer) > 0) {
      if (_importanceMapCache[layer].count(mode) > 0) {
        return true;
      }
    }

    return false;
  }

  map<string, map<ImportanceMapMode, shared_ptr<ImportanceMap>>> Compositor::getImportanceMapCache()
  {
    return _importanceMapCache;
  }

  ClickMap * Compositor::createClickMap(ImportanceMapMode mode, Context current)
  {
    // generate importance maps
    computeAllImportanceMaps(mode, current);

    // gather maps
    map<string, shared_ptr<ImportanceMap>> impMaps;
    for (auto& kvp : _importanceMapCache) {
      impMaps[kvp.first] = kvp.second[mode];
    }

    // adjustment layers
    vector<bool> adjustments;
    for (int i = 0; i < _layerOrder.size(); i++) {
      adjustments.push_back(_primary[_layerOrder[i]].isAdjustmentLayer());
    }

    // create the click map and run the computations
    ClickMap* ret = new ClickMap(getWidth(), getHeight(), _layerOrder, impMaps, adjustments);
    
    return ret;
  }

  void Compositor::analyzeAndTag()
  {
    for (int i = 0; i < _layerOrder.size(); i++) {
      string name = _layerOrder[i];
      Layer l = _primary[name];

      // really really simple tags for now, I expect them to get more complicated later
      if (l.isAdjustmentLayer()) {
        _layerTags[name].insert("Global Adjustment");
      }
      else {
        _layerTags[name].insert("Structure");
      }

      // adjustments check
      // ther's a number of points to be made here
      // specifically
      // - that the gradient effects depends heavily on blend mode and gradient content
      // - currently you can't edit curves
      // - where does invert go
      // - and we are ignoring layer specific blend options and possible color stuff
      auto adjustments = l.getAdjustments();
      for (auto& adj : adjustments) {
        switch (adj) {
        case HSL:
        case GRADIENT:
          _layerTags[name].insert("Color");
          _layerTags[name].insert("Brightness");
          break;
        case LEVELS:
        case EXPOSURE:
          _layerTags[name].insert("Brightness");
          _layerTags[name].insert("Contrast");
          break;
        case CURVES:
          _layerTags[name].insert("Color");
          _layerTags[name].insert("Brightness");
          _layerTags[name].insert("Contrast");
          break;
        case SELECTIVE_COLOR:
        case COLOR_BALANCE:
        case PHOTO_FILTER:
        case COLORIZE:
        case LIGHTER_COLORIZE:
        case OVERWRITE_COLOR:
          _layerTags[name].insert("Color");
          break;
        case BRIGHTNESS:
          _layerTags[name].insert("Brightness");
          break;
        case INVERT:
          _layerTags[name].insert("Brightness");
          break;
        default:
          break;
        }
      }
    }
  }

  set<string> Compositor::uniqueTags()
  {
    set<string> ret;

    for (auto& kvp : _layerTags) {
      for (auto& tag : kvp.second) {
        ret.insert(tag);
      }
    }

    return ret;
  }

  set<string> Compositor::getTags(string layer)
  {
    return _layerTags[layer];
  }

  map<string, set<string>> Compositor::allTags()
  {
    return _layerTags;
  }

  void Compositor::deleteTags(string layer)
  {
    _layerTags.erase(layer);
  }

  void Compositor::deleteAllTags()
  {
    _layerTags.clear();
  }

  bool Compositor::hasTag(string layer, string tag)
  {
    return (_layerTags[layer].count(tag) > 0);
  }

  map<string, map<AdjustmentType, vector<GoalResult>>> Compositor::goalSelect(Goal g, Context & c, int x, int y, int maxLevel)
  {
    vector<int> xv = { x };
    vector<int> yv = { y };
    return goalSelect(g, c, xv, yv, maxLevel);
  }

  map<string, map<AdjustmentType, vector<GoalResult>>> Compositor::goalSelect(Goal g, Context & c, vector<int> x, vector<int> y, int maxLevel)
  {
    // right now this function will select _individual parameters_ (NOT ENTIRE ADJUSTMENTS)
    // that satisfy the given goal constraint.
    // i expect this functionality to get substantially more complicated but we're starting small here.
    map<string, map<AdjustmentType, vector<GoalResult>>> ret;

    // automatic bail if vector sizes are different
    if (x.size() != y.size())
      return ret;

    // ok so this method will only work for single params for reasons that will qucikly become apparent
    if (g.getType() == GoalType::SELECT_ANY) {
      // avg speculative difference with threshold
      map<string, double> avgScores;
      map<string, int> affectedPixels;

      for (int i = 0; i < x.size(); i++) {
        map<string, double> ptScores;
        pointImportance("specVisibilityDelta", ptScores, x[i], y[i], c);

        for (auto& s : ptScores) {
          bool counted = true;
          if (!_primary[s.first].isAdjustmentLayer()) {
            auto px = _primary[s.first].getImage()->getPixel(x[i], y[i]);
            if (px._a == 0)
              counted = false;
          }
          else {
            if (_primary[s.first].hasMask()) {
              auto px = _primary[s.first].getMask()->getPixel(x[i], y[i]);
              if (px._r == 1)
                counted = false;
            }
          }

          // first run
          if (i == 0) {
            avgScores[s.first] = s.second;
            affectedPixels[s.first] = (counted ? 1 : 0);
          }
          else {
            avgScores[s.first] += s.second;
            affectedPixels[s.first] += (counted ? 1 : 0);
          }
        }
      }

      // adjustment type is opacity, "opacity" for return values
      // also take the average of all the spec deltas
      for (auto& s : avgScores) {
        // TODO: maybe? allow custom threshold if needed
        float val = s.second / affectedPixels[s.first];

        if (affectedPixels[s.first] == 0)
          val = 0;

        if (val > 0.01) {
          GoalResult r;
          r._param = "opacity";
          r._val = 1;

          getLogger()->log("layer " + s.first + " accepted with average " + to_string(val));

          ret[s.first][AdjustmentType::OPACITY].push_back(r);
        }
      }
    }
    else if (g.getType() == GoalType::SELECT_GROUP) {
      // tbd
    }
    else {
      // set up goal color vector
      vector<RGBAColor> current;
      for (int i = 0; i < x.size(); i++) {
        current.push_back(renderPixel<float>(c, x[0], y[0], "full"));
      }
      g.setOriginalColors(current);

      // scan each parameter from 0 to 1 (they're all normalized!) to see if goal gets satisfied.
      for (auto& layer : _layerOrder) {
        getLogger()->log("Testing layer " + layer);

        // sanity check
        if (!_primary[layer].isAdjustmentLayer()) {
          // if a transparent pixel is at all of the specified location
          // then this layer can't do anything really so skip it
          bool testLayer = false;

          for (int i = 0; i < x.size(); i++) {
            auto px = _primary[layer].getImage()->getPixel(x[i], y[i]);
            if (px._a != 0) {
              testLayer = true;
              break;
            }
          }

          // if the layer has no non-zero alpha pixels, skip
          if (!testLayer)
            continue;
        }

        // coarse scan of the opacity to see if it can satisfy the goal
        // temporary rendering context
        Context temp(c);

        for (int i = 0; i <= 100; i++) {
          float val = 0.01f * i;
          temp[layer].setOpacity(val);

          // render all of the pixels
          vector<RGBAColor> testPixels;
          for (int i = 0; i < x.size(); i++) {
            testPixels.push_back(renderPixel<float>(temp, x[i], y[i], "full"));
          }

          if (g.meetsGoal(testPixels)) {
            // add param to ret, continue
            GoalResult r;
            r._param = "opacity";
            r._val = val;
            ret[layer][AdjustmentType::OPACITY].push_back(r);

            getLogger()->log("Layer " + layer + " adjustment " + to_string(AdjustmentType::OPACITY) + " parameter opacity satisfies goal with value " + to_string(val));

            break;
          }
        }

        // this should really be an optimization process (if using multiple, may need ceres in on this)
        auto adjustments = _primary[layer].getAdjustments();

        // TODO: opacity / visibility might want to be up and on for adjustment checking, will test
        // hey also i'm skipping selective color for now because I just don't want to deal with it
        for (auto& a : adjustments) {
          float min;
          map<string, float> vals;
          bool meetsGoal = adjustmentMeetsGoal(layer, a, g, c, x, y, maxLevel, min, vals);

          // check goal success
          if (meetsGoal) {
            // accept n stuff
            for (auto& p : vals) {
              GoalResult r;
              r._param = p.first;
              r._val = p.second;
              ret[layer][a].push_back(r);

              getLogger()->log("Layer " + layer + " adjustment " + to_string(a) + " parameter " + p.first + " satisfies goal with value " + to_string(p.second));
            }

            getLogger()->log("Layer " + layer + " adjustment " + to_string(a) + " accepted with score " + to_string(min));
          }
          else {
            getLogger()->log("Layer " + layer + " adjustment " + to_string(a) + " failed goal with score " + to_string(min));
          }
        }
      }
    }

    return ret;
  }

  bool Compositor::adjustmentMeetsGoal(string layer, AdjustmentType adj, Goal & g, Context c,
    vector<int> x, vector<int> y, int maxLevel, float & minimum, map<string, float>& paramVals)
  {
    // this assumes the stuff in the poisson disc cache is initialized, if not it'll probably crash?
    // set up adjustment settings
    map<string, float> params = c[layer].getAdjustment(adj);

    // some adjustments have params that should be ignored, or are simply unable to be edited
    if (adj == AdjustmentType::CURVES || adj == AdjustmentType::GRADIENT || adj == AdjustmentType::INVERT) {
      minimum = 0;
      return false;
    }
    else if (adj == AdjustmentType::SELECTIVE_COLOR) {
      getLogger()->log("Selective color is not supported yet", LogLevel::WARN);
      minimum = 0;
      return false;
    }
    else if (adj == AdjustmentType::COLOR_BALANCE) {
      getLogger()->log("Color Balance is not supported yet", LogLevel::WARN);
      minimum = 0;
      return false;
    }
    else if (adj == AdjustmentType::PHOTO_FILTER) {
      // preserve luma is a binary option here and should not be used during optimization
      params.erase("preserveLuma");
    }

    // ok now determine the dimensionality of the thing
    int n = params.size();

    // check for sample pattern existence
    if (_pdiskCache.count(n) == 0) {
      getLogger()->log("No sample patterns initialized for dimensionality " + to_string(n) + ". Aborting...", LogLevel::ERR);
      minimum = 0;
      return false;
    }

    int level = 0;
    map<int, shared_ptr<PoissonDisk>> disks = _pdiskCache[n];

    // check that level exists, if not clamp
    int maxAvailableLevel = 0;
    for (auto& levels : disks) {
      if (levels.first > maxAvailableLevel)
        maxAvailableLevel = levels.first;
    }

    if (maxLevel > maxAvailableLevel)
      maxLevel = maxAvailableLevel;

    // initialize points with all of level 0
    vector<vector<float>> pts;
    minimum = FLT_MAX;
    paramVals = params;
    Context renderContext(c);

    // sorts by score
    multimap<float, vector<float>> sortedPts;

    // ok so until we hit max level, do this repeatedly
    while (level <= maxLevel) {
      if (level == 0) {
        pts = disks[level]->allPoints();
      }
      else {
        // take the top 25%, max 1000 pts, min 
        int ptsToNextLevel = min((int)(sortedPts.size() * 0.25), 1000);
        vector<vector<float>> nextPts;

        int i = 0;
        for (auto& p : sortedPts) {
          if (i >= ptsToNextLevel)
            break;

          nextPts.push_back(p.second);
          i++;
        }
        pts = disks[level]->nearby(nextPts);
      }

      sortedPts.clear();

      // evaluate all points
      for (auto& p : pts) {
        // apply points
        int i = 0;
        map<string, float> currentParamVals;
        for (auto& param : params) {
          renderContext[layer].addAdjustment(adj, param.first, p[i]);
          currentParamVals[param.first] = p[i];
          i++;
        }

        // render the pixels
        vector<RGBAColor> testPixels;
        for (int j = 0; j < x.size(); j++) {
          testPixels.push_back(renderPixel<float>(renderContext, x[j], y[j], "full"));
        }

        // get the objective
        float fx = g.goalObjective(testPixels);

        if (fx < minimum) {
          minimum = fx;
          paramVals = currentParamVals;
        }

        // put in the sorted points
        sortedPts.insert(make_pair(fx, p));
      }

      getLogger()->log("Layer " + layer + " adjustment " + to_string(adj) + " current minimum is " + to_string(minimum) + " at level " + to_string(level));

      // get the points that are close in the next level
      level++;
    }

    return g.meetsGoal(minimum);
  }

  map<string, map<AdjustmentType, vector<GoalResult>>> Compositor::goalSelect(Goal g, Context & c, int x, int y, int w, int h, int maxLevel)
  {
    // choose the pixels to sample
    // right now here's how it works
    // - maximum allowed number of pixels is an 8 x 8 patch
    // - if the selected region is larger than 8 x 8, subsample in a grid pattern
    vector<int> xs;
    vector<int> ys;

    if (w > 8) {
      // subsample
      for (int i = 0; i < 8; i++) {
        // w/8 should be > 1 so no repeats should happen
        xs.push_back(x + (w / 8.0f) * i);
      }
    }
    else {
      for (int i = 0; i < w; i++) {
        xs.push_back(x + i);
      }
    }

    if (h > 8) {
      // subsample
      for (int i = 0; i < 8; i++) {
        // w/8 should be > 1 so no repeats should happen
        ys.push_back(y + (h / 8.0f) * i);
      }
    }
    else {
      for (int i = 0; i < w; i++) {
        ys.push_back(y + i);
      }
    }

    vector<int> xpts;
    vector<int> ypts;

    // create the vectors, basically repeat x values and cycle through ys
    for (int i = 0; i < xs.size(); i++) {
      for (int j = 0; j < ys.size(); j++) {
        xpts.push_back(xs[i]);
        ypts.push_back(ys[j]);
      }
    }

    return goalSelect(g, c, xpts, ypts, maxLevel);
  }

  void Compositor::addLayer(string name)
  {
    _primary[name] = Layer(name, _imageData[name]["full"]);

    // place at end of order
    _layerOrder.push_back(name);

    // update the key
    contextToVector(getNewContext(), _vectorKey);

    getLogger()->log("Added new layer named " + name);
  }

  void Compositor::addLayerMask(string name)
  {
    _primary[name].setMask(_layerMasks[name]["full"]);

    // rescale
    _layerMasks[name]["thumb"] = _layerMasks[name]["full"]->resize(0.15f);
    _layerMasks[name]["small"] = _layerMasks[name]["full"]->resize(0.25f);
    _layerMasks[name]["medium"] = _layerMasks[name]["full"]->resize(0.5f);

    getLogger()->log("Added mask " + _layerMasks[name]["full"]->getFilename() + " to layer " + name);
  }

  void Compositor::cacheScaled(string name)
  {
    _imageData[name]["thumb"] = _imageData[name]["full"]->resize(0.15f);
    _imageData[name]["small"] = _imageData[name]["full"]->resize(0.25f);
    _imageData[name]["medium"] = _imageData[name]["full"]->resize(0.5f);
  }

  float Compositor::finiteDifference(Image * a, Image * b, float delta)
  {
    float sum = 0;

    vector<unsigned char>& pxa = a->getData();
    vector<unsigned char>& pxb = b->getData();

    for (int i = 0; i < pxa.size() / 4; i++) {
      int index = i * 4;

      // premultiply after converting to [0,1]
      float alphaA = pxa[i + 3] / 255.0f;
      Eigen::Vector3f pa(pxa[i] / 255.0f, pxa[i + 1] / 255.0f, pxa[i + 2] / 255.0f);
      pa *= alphaA;

      float alphaB = pxb[i + 3] / 255.0f;
      Eigen::Vector3f pb(pxb[i] / 255.0f, pxb[i + 1] / 255.0f, pxb[i + 2] / 255.0f);
      pb *= alphaB;

      // compute derivative magnitude
      float mag = ((pb - pa) / delta).norm();
      sum += mag;
    }

    return sum;
  }

  map<string, float> Compositor::getDelta(Goal & g, Context & r, float fx, map<string, float>& currentVals,
    string layer, AdjustmentType t, vector<int>& x, vector<int>& y)
  {
    map<string, float> results;

    // TODO: make variable
    float delta = 0.01;

    // r is the current render context
    for (auto& p : currentVals) {
      Context temp(r);

      float dp;
      if (p.second >= 1) {
        // yeah like i mean it's gonna be clamped and set to 1 - 0.01 so?
        dp = 1 - delta;
      }
      else {
        dp = p.second + delta;
      }

      temp[layer].addAdjustment(t, p.first, dp);

      // compute
      vector<RGBAColor> testPixels;
      for (int i = 0; i < x.size(); i++) {
        testPixels.push_back(renderPixel<float>(temp, x[i], y[i], "full"));
      }

      float fxp = g.goalObjective(testPixels);
      results[p.first] = (fxp - fx) / 0.01;
    }

    return results;
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
    _activeCallback(r, start, map<string, float>(), map<string, string>());
  }

  void Compositor::initPoissonDisk(int n, int level, int k) {
    float r;
    if (n < 4) {
      r = 0.2 * pow(2, -level);
    }
    else {
      r = 0.5 * pow(1.5, -level);
    }

    _pdiskCache[n][level] = shared_ptr<PoissonDisk>(new PoissonDisk(r, n, k));
    _pdiskCache[n][level]->sample();
  }

  void Compositor::initPoissonDisks()
  {
    // maximum depth is 3, dimensions for now are 2, 3, 4, 5
    for (int i = 0; i <= 4; i++) {
      getLogger()->log("2D Poisson Disks Level " + to_string(i));
      initPoissonDisk(2, i);
      getLogger()->log("3D Poisson Disks Level " + to_string(i));
      initPoissonDisk(3, i);
      getLogger()->log("4D Poisson Disks Level " + to_string(i));
      initPoissonDisk(4, i);
      if (i <= 3) {
        getLogger()->log("5D Poisson Disks Level " + to_string(i));
        initPoissonDisk(5, i);
      }
    }
  }

  void Compositor::exploratorySearch()
  {
    getLogger()->log("Exploratory search started", LogLevel::INFO);

    // this runs in a threaded context. It should check if _searchRunning at times
    // to ensure the entire thing doesn't freeze
    ExpSearchSet activeSet(_searchSettings);
    shared_ptr<Image> currentRender = shared_ptr<Image>(render(_initSearchContext, _searchRenderSize));

    Context c = _initSearchContext;
    nlohmann::json key;
    vector<double> cv = contextToVector(c, key);

    // set the initial configuration
    activeSet.setInitial(shared_ptr<ExpSearchSample>(new ExpSearchSample(currentRender, _initSearchContext, cv)));

    // run structural search, then everything else
    expStructSearch(activeSet);

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> zeroOne(0, 1);
    
    // keep a single sample and repeatedly mutate it, similar to genetic op but without
    // a population since we don't have an objective here. Crossover samples are
    // pulled from the active set
    // termination condition: consecutive failures (up for debate)
    int failures = 0;
    int sample = 0;
    while (failures < _searchSettings["maxFailures"]) {
      // skip to the end and return results if search is no longer running
      if (!_searchRunning)
        break;
      
      stringstream log;
      log << "[" << sample << "]\t";

      // crossover
      if (zeroOne(gen) < _searchSettings["crossoverRate"]) {
        shared_ptr<ExpSearchSample> xover;
        int xsample;

        // 50% chance to crossover with the start config, 100% if activeSet is size 0
        if (activeSet.size() == 0 || zeroOne(gen) < 0.5) {
          xsample = -1;
          xover = activeSet.getInitial();
        }
        else {
          xsample = (int)(zeroOne(gen) * activeSet.size());
          xover = activeSet.get(xsample);
        }

        vector<double> xvec = xover->getContextVector();

        log << "Crossover (" << xsample << ")";

        for (int i = 0; i < cv.size(); i++) {
          if (zeroOne(gen) < _searchSettings["crossoverChance"]) {
            cv[i] = xvec[i];
            log << " [" << i << "]";
          }
        }
      }

      for (int i = 0; i < cv.size(); i++) {
        // mutate the current context, which has been translated to a vector
        // mutate here just means randomize between 0 and 1
        // note: some parameters are toggles (like relative for selective color) may have to deal with them later
        if (zeroOne(gen) < _searchSettings["mutationRate"]) {
          cv[i] = zeroOne(gen);
          log << " Mutation [" << i << "]";
        }
      }

      getLogger()->log(log.str());

      // option to only use struct results as a base
      if (_searchSettings["useStructOnly"] > 0) {
        // randomly select a base
        int ind = (int)(zeroOne(gen) * _structResults.size());
        vector<double> base = _structResults[ind];

        for (int id : _structParams) {
          cv[id] = base[id];
        }
      }

      // attempt to add the thing
      Context newCtx = vectorToContext(cv, key);

      // sanity check for levels, restore to default if invalid
      for (auto& l : newCtx) {
        auto adj = l.second.getAdjustment(AdjustmentType::LEVELS);
        if (adj.size() > 0) {
          if (adj["inMin"] > adj["inMax"] || adj["outMin"] > adj["outMax"]) {
            // restore to initial conditions
            l.second.addAdjustment(AdjustmentType::LEVELS, _initSearchContext[l.first].getAdjustment(AdjustmentType::LEVELS));

            // update vector
            cv = contextToVector(newCtx, key);
          }
        }
      }

      shared_ptr<Image> img = shared_ptr<Image>(render(newCtx, _searchRenderSize));
      shared_ptr<ExpSearchSample> newSample = shared_ptr<ExpSearchSample>(new ExpSearchSample(img, newCtx, cv));

      // check that the result is "reasonable"
      bool good = activeSet.isGood(newSample);
      // failing to be a reasonable sample isn't necsesarily a failure to find diversity, so it won't be counted as such

      if (good) {
        bool success = activeSet.add(newSample);

        if (!success) {
          failures++;
        }
        else {
          map<string, string> m2;
          m2["reason"] = activeSet.getReason(activeSet.size() - 1);

          _activeCallback(new Image(*img.get()), newCtx, map<string, float>(), m2);
          failures = 0;
        }
      }

      getLogger()->log("Failures: " + to_string(failures) + "/" + to_string(_searchSettings["maxFailures"]));
      sample++;
    }

    // right now things that are added to the set can't be removed, so we return as we find at the moment.
    // return everything to the calling function with the callback
    //for (int i = 0; i < activeSet.size(); i++) {
    //  auto x = activeSet.get(i);

      // string metadata things
    //  map<string, string> m2;
    //  m2["reason"] = activeSet.getReason(i);

    //  _activeCallback(new Image(*x->getImg().get()), x->getContext(), map<string, float>(), m2);
    //}
  }

  void Compositor::expStructSearch(ExpSearchSet & activeSet)
  {
    getLogger()->log("Starting structural search");

    // first collect information from the search groups
    set<string> structLayers;

    for (auto& g : _searchGroups) {
      if (g._type == G_STRUCTURE) {
        for (auto& name : g._layerNames) {
          structLayers.insert(name);
        }
      }
    }
    
    // data setup
    Context c = _initSearchContext;
    nlohmann::json key;
    vector<double> cv = contextToVector(c, key);
    _structParams.clear();
    _structResults.clear();

    // determine which parameters are the opacity ones
    for (int i = 0; i < key.size(); i++) {
      AdjustmentType t = (AdjustmentType)key[i]["adjustmentType"];
      if (t == AdjustmentType::OPACITY) {
        string name = key[i]["layerName"].get<string>();
        if (structLayers.count(name) > 0) {
          _structParams.push_back(i);
        }
      }
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> zeroOne(0, 1);

    // for now structure is assumed to imply opacity only
    int failures = 0;
    int sample = 0;
    while (failures < _searchSettings["maxFailures"]) {
      if (!_searchRunning)
        break;
      
      stringstream log;
      log << "[" << sample << "]\t";

      // crossover is eliminated here in favor of toggling
      // but maybe we do want crossover back to the original config

      // mutate
      for (auto& id : _structParams) {
        // mutate the current context, which has been translated to a vector
        // mutate here just means randomize between 0 and 1
        if (zeroOne(gen) < _searchSettings["mutationRate"]) {
          cv[id] = zeroOne(gen);
          log << " Mutation [" << id << "]";
        }
      }

      // toggle
      // this randomly sets an opacity layer to either on (100%) or off (0%)
      // with a 50% chance
      for (auto& id : _structParams) {
        if (zeroOne(gen) < _searchSettings["toggleRate"]) {
          log << " Toggle [" << id << "]";
          cv[id] = (zeroOne(gen) < 0.5) ? 1 : 0;
        }
      }

      getLogger()->log(log.str());

      // attempt to add the thing
      Context newCtx = vectorToContext(cv, key);
      shared_ptr<Image> img = shared_ptr<Image>(render(newCtx, _searchRenderSize));
      shared_ptr<ExpSearchSample> newSample = shared_ptr<ExpSearchSample>(new ExpSearchSample(img, newCtx, cv));

      // check that the result is "reasonable"
      bool good = activeSet.isGood(newSample);
      // failing to be a reasonable sample isn't necsesarily a failure to find diversity, so it won't be counted as such

      if (good) {
        bool success = activeSet.add(newSample, true);

        if (!success) {
          failures++;
        }
        else {
          map<string, string> m2;
          m2["reason"] = activeSet.getReason(activeSet.size() - 1);

          _activeCallback(new Image(*img.get()), newCtx, map<string, float>(), m2);
          _structResults.push_back(cv);
          failures = 0;
        }
      }

      getLogger()->log("Failures: " + to_string(failures) + "/" + to_string(_searchSettings["maxFailures"]));
      sample++;
    }
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
      else if (type == AdjustmentType::INVERT) {
        invertAdjust(adjLayer);
      }
      else if (type == AdjustmentType::BRIGHTNESS) {
        brightnessAdjust(adjLayer, l.getAdjustment(type));
      }
    }
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

      img[i * 4] = (unsigned char)(clamp(adjPx._r, 0.0f, 1.0f) * 255);
      img[i * 4 + 1] = (unsigned char)(clamp(adjPx._g, 0.0f, 1.0f) * 255);
      img[i * 4 + 2] = (unsigned char)(clamp(adjPx._b, 0.0f, 1.0f) * 255);
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

  inline void Compositor::selectiveColor(Image * adjLayer, map<string, float> adj, Layer & l)
  {
    // so according to the PS docs this will apply per-channel adjustments relative to
    // "how close the color is to an option in the menu" which implies a barycentric weighting
    // of the colors. Photoshop's internal color space is probably Lab so that seems like the right
    // approach, however the conversion to Lab and computation of barycentric coords is really expensive.
    // For now we'll approximate by using the HCL cone and some hacky interpolation because barycentric
    // coordinates get rather odd in a cone
    vector<unsigned char>& img = adjLayer->getData();

    map<string, map<string, float>> data = l.getSelectiveColor();

    for (auto& c : data) {
      for (auto& p : c.second) {
        p.second = (p.second - 0.5) * 2;
      }
    }

    bool rel = adj["relative"] > 0;

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor adjPx = adjLayer->getPixel(i);
      selectiveColor<float>(adjPx, data, rel);

      img[i * 4] = (unsigned char)(adjPx._r * 255);
      img[i * 4 + 1] = (unsigned char)(adjPx._g * 255);
      img[i * 4 + 2] = (unsigned char)(adjPx._b * 255);
    }
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

      img[i * 4] = (unsigned char)(adjPx._r * 255);
      img[i * 4 + 1] = (unsigned char)(adjPx._g * 255);
      img[i * 4 + 2] = (unsigned char)(adjPx._b * 255);
    }
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

  inline void Compositor::invertAdjust(Image * adjLayer)
  {
    vector<unsigned char>& img = adjLayer->getData();

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor adjPx = adjLayer->getPixel(i);
      invertAdjustT<float>(adjPx);

      img[i * 4] = (unsigned char)(adjPx._r * 255);
      img[i * 4 + 1] = (unsigned char)(adjPx._g * 255);
      img[i * 4 + 2] = (unsigned char)(adjPx._b * 255);
    }
  }

  inline void Compositor::brightnessAdjust(Image * adjLayer, map<string, float> adj)
  {
    vector<unsigned char>& img = adjLayer->getData();

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor adjPx = adjLayer->getPixel(i);
      brightnessAdjust(adjPx, adj);

      img[i * 4] = (unsigned char)(adjPx._r * 255);
      img[i * 4 + 1] = (unsigned char)(adjPx._g * 255);
      img[i * 4 + 2] = (unsigned char)(adjPx._b * 255);
    }
  }

  vector<double> Compositor::contextToVector(Context c, nlohmann::json& key)
  {
    // add the parameter data to the key
    key = nlohmann::json::array();

    for (auto& l : _layerOrder) {
      // gather parameter info for all layers
      c[l].addParams(key);
    }

    // dump values into a vector
    vector<double> vals;
    for (int i = 0; i < key.size(); i++) {
      vals.push_back(key[i]["value"]);
    }

    // some extremely verbose logging
    // dump the json file
    //stringstream ss;
    //getLogger()->log(key.dump(2), DBG);

    return vals;
  }

  vector<double> Compositor::contextToVector(Context c)
  {
    return contextToVector(c, _vectorKey);
  }

  Context Compositor::vectorToContext(vector<double> v)
  {
    return vectorToContext(v, _vectorKey);
  }

  Context Compositor::contextFromDarkroom(string file)
  {
    nlohmann::json data;
    ifstream input(file);

    if (!input.is_open()) {
      // failing to load will return an empty context
      getLogger()->log("Failed to open file " + file, LogLevel::ERR);
      return Context();
    }

    input >> data;

    if (data.count("layers") == 0) {
      // this is not a darkroom file, return
      getLogger()->log(file + " is not a Darkroom file", LogLevel::ERR);
      return Context();
    }

    Context ctx = getNewContext();
    for (nlohmann::json::iterator it = data["layers"].begin(); it != data["layers"].end(); ++it) {
      string layerName = it.key();

      // check layer existence
      if (ctx.count(layerName) > 0) {
        ctx[layerName].setOpacity(it.value()["opacity"]);
        ctx[layerName]._visible = it.value()["visible"];
        ctx[layerName]._mode = (BlendMode)(it.value()["blendMode"].get<int>());
        ctx[layerName]._psType = it.value()["type"].get<string>();

        map<string, float> cbs;
        auto cbData = it.value()["conditionalBlend"]["params"];
        for (auto cbit = cbData.begin(); cbit != cbData.end(); ++cbit) {
          cbs[cbit.key()] = cbit.value();
        }
        ctx[layerName].setConditionalBlend(it.value()["conditionalBlend"]["channel"], cbs);

        // adjustments
        auto adjData = it.value()["adjustments"];
        for (auto ait = adjData.begin(); ait != adjData.end(); ++ait) {
          AdjustmentType t = (AdjustmentType)stoi(ait.key());

          // now iterate though the settings
          for (auto apit = ait.value().begin(); apit != ait.value().end(); ++apit) {
            ctx[layerName].addAdjustment(t, apit.key(), apit.value());
          }

          if (t == AdjustmentType::SELECTIVE_COLOR) {
            // selective color
            auto scData = it.value()["selectiveColor"];

            // scData will always have relative in it, but if it has nothing else,
            // skip it
            if (scData.size() > 1) {
              map<string, map<string, float>> sc;
              bool rel = false;
              for (auto scit = scData.begin(); scit != scData.end(); ++scit) {
                string key = scit.key();

                if (key == "relative") {
                  rel = true;
                }
                else {
                  sc[key]["black"] = scit.value()["black"];
                  sc[key]["cyan"] = scit.value()["cyan"];
                  sc[key]["magenta"] = scit.value()["magenta"];
                  sc[key]["yellow"] = scit.value()["yellow"];
                }
              }
              ctx[layerName].addSelectiveColorAdjustment(rel, sc);
            }
          }
        }

        // curves
        if (it.value().count("curves") > 0) {
          auto curveData = it.value()["curves"];

          // curves are basically arrays and should be fairly easy to reconstruct
          for (auto cit = curveData.begin(); cit != curveData.end(); ++cit) {
            string channel = cit.key();
            auto curve = cit.value();

            vector<Point> pts;
            for (int i = 0; i < curve.size(); i++) {
              pts.push_back(Point(curve[i]["x"], curve[i]["y"]));
            }
            ctx[layerName].addCurvesChannel(channel, Curve(pts));
          }
        }

        // gradient
        if (it.value().count("gradient") > 0) {
          auto gradData = it.value()["gradient"];
          vector<float> x;
          vector<RGBColor> colors;

          for (int i = 0; i < gradData.size(); i++) {
            x.push_back(gradData[i]["x"]);

            RGBColor color;
            color._r = gradData[i]["color"]["r"];
            color._g = gradData[i]["color"]["g"];
            color._b = gradData[i]["color"]["b"];

            colors.push_back(color);
          }

          ctx[layerName].addGradientAdjustment(Gradient(x, colors));
        }
      }
      else {
        getLogger()->log("Current compositor has no layer named " + it.key(), LogLevel::WARN);
      }
    }

    getLogger()->log("Loaded file " + file, LogLevel::INFO);
    return ctx;
  }

  Context Compositor::vectorToContext(vector<double> x, nlohmann::json & key)
  {
    Context c = getNewContext();

    for (int i = 0; i < key.size(); i++) {
      AdjustmentType t = (AdjustmentType)key[i]["adjustmentType"];
      string layerName = key[i]["layerName"];
      double val = x[i];
      string adjName = key[i]["adjustmentName"];

      //getLogger()->log("processing " + layerName + " adj: " + adjName);

      if (t == AdjustmentType::OPACITY) {
        c[layerName].setOpacity(val);
      }
      else if (t == AdjustmentType::SELECTIVE_COLOR && adjName != "relative") {
        string channel = key[i]["selectiveColor"]["channel"];
        string color = key[i]["selectiveColor"]["color"];
        c[layerName].setSelectiveColorChannel(channel, color, val);
      }
      else {
        c[layerName].addAdjustment(t, adjName, val);
      }
    }

    return c;
  }
}

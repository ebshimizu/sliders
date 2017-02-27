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
      Image* adjLayer = nullptr;

      // handle adjustment layers
      if (l.isAdjustmentLayer()) {
        // ok so here we adjust the current layer, then blend it as normal below
        // create duplicate of current composite
        adjLayer = new Image(*comp);
        adjust(adjLayer, l);
        layerPx = &adjLayer->getData();
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
      }

      // adjustment layer clean up, if applicable
      if (adjLayer != nullptr) {
        delete adjLayer;
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

  inline float Compositor::premult(unsigned char px, float a)
  {
    return (float)((px / 255.0f) * a);
  }

  inline unsigned char Compositor::cvt(float px, float a)
  {
    float v = ((px / a) * 255);
    return (unsigned char)((v > 255) ? 255 : (v < 0) ? 0 : v);
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
    }
  }

  inline void Compositor::hslAdjust(Image * adjLayer, map<string, float> adj)
  {
    // Right now the bare-bones hsl adjustment is here. PS has a lot of options for carefully
    // crafting remappings, but we don't do that for now.
    // basically we convert to hsl, add the proper adjustment, convert back to rgb8
    vector<unsigned char>& img = adjLayer->getData();
    float h = adj["hue"];
    float s = adj["sat"];
    float l = adj["light"];

    for (int i = 0; i < img.size() / 4; i++) {
      // 4 pixel stride here, ignoring alpha
      float r = img[i * 4] / 255.0f;
      float g = img[i * 4 + 1] / 255.0f;
      float b = img[i * 4 + 2] / 255.0f;

      HSLColor c = RGBToHSL(r, g, b);

      // modify hsl. h is in degrees, and s and l will be out of 100 due to how photoshop represents that
      c._h += h;
      c._s += s / 100.0f;
      c._l += l / 100.0f;

      // convert back
      RGBColor c2 = HSLToRGB(c);
      img[i * 4] = (unsigned char) (c2._r * 255);
      img[i * 4 + 1] = (unsigned char) (c2._g * 255);
      img[i * 4 + 2] = (unsigned char) (c2._b * 255);
    }
  }

  inline void Compositor::levelsAdjust(Image* adjLayer, map<string, float> adj) {
    vector<unsigned char>& img = adjLayer->getData();

    // so sometimes these values are missing and we should use defaults.
    float inMin = (adj.count("inMin") > 0) ? adj["inMin"] : 0;
    float inMax = (adj.count("inMax") > 0) ? adj["inMax"] : 255;
    float gamma = (adj.count("gamma") > 0) ? adj["gamma"] : 1;
    float outMin = (adj.count("outMin") > 0) ? adj["outMin"] : 0;
    float outMax = (adj.count("outMax") > 0) ? adj["outMax"] : 255;

    for (int i = 0; i < img.size(); i++) {
      if (i % 4 == 3)
        continue;

      img[i] = levels(img[i], inMin, inMax, gamma, outMin, outMax);
    }
  }

  inline unsigned char Compositor::levels(unsigned char px, float inMin, float inMax, float gamma, float outMin, float outMax)
  {
    // input remapping
    float out = min(max(px - inMin, 0.0f) / (inMax - inMin), 1.0f);

    // gamma correction
    out = pow(out, 1 / gamma);

    // output remapping
    out = out * (outMax - outMin) + outMin;

    return (unsigned char)out;
  }

  inline void Compositor::curvesAdjust(Image * adjLayer, map<string, float> adj, Layer & l)
  {
    // fairly simple adjustment
    // take each channel, modify the pixel values by the specified curve
    // after per-channel adjustments are done, then apply the RGB (brightness) curve which really
    // should be labeled value because that's what that curve seems to affect
    vector<unsigned char>& img = adjLayer->getData();

    for (int i = 0; i < img.size() / 4; i++) {
      float r = l.evalCurve("red", img[i * 4] / 255.0f);
      float g = l.evalCurve("green", img[i * 4 + 1] / 255.0f);
      float b = l.evalCurve("blue", img[i * 4 + 2] / 255.0f);

      // short circuit this to avoid unnecessary conversion if the curve
      // doesn't actually exist
      if (adj.count("RGB") > 0) {
        r = l.evalCurve("RGB", r);
        g = l.evalCurve("RGB", g);
        b = l.evalCurve("RGB", b);
      }

      img[i * 4] = (unsigned char)(clamp(r, 0, 1) * 255);
      img[i * 4 + 1] = (unsigned char)(clamp(g, 0, 1) * 255);
      img[i * 4 + 2] = (unsigned char)(clamp(b, 0, 1) * 255);
    }
  }

}

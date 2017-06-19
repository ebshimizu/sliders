/*
Compositor.h - An image compositing library
author: Evan Shimizu
*/

#pragma once

#include <vector>
#include <map>
#include <functional>
#include <algorithm>
#include <thread>
#include <set>
#include <random>

#include "Image.h"
#include "Layer.h"
#include "util.h"
#include "ConstraintData.h"

using namespace std;

namespace Comp {
  typedef function<void(Image*, Context, map<string, float> metadata)> searchCallback;

  enum SearchMode {
    SEARCH_DEBUG,          // returns the same image at intervals for testing app functionality
    RANDOM,         // randomly adjusts the available parameters to do things
    SMART_RANDOM,
    MCMC,
    NLS
  };

  // the compositor for now assumes that every layer it contains have the same dimensions.
  // having unequal layer sizes will likely lead to crashes or other undefined behavior
  class Compositor {
  public:
    Compositor();
    ~Compositor();

    // adds a layer, true on success, false if layer already exists
    bool addLayer(string name, string file);
    bool addLayer(string name, Image& img);

    // adds an adjustment layer
    bool addAdjustmentLayer(string name);
    
    bool copyLayer(string src, string dest);

    bool deleteLayer(string name);

    // changes the sort order of a layer
    void reorderLayer(int from, int to);

    Layer& getLayer(int id);
    Layer& getLayer(string name);

    // Returns a copy of the current context
    Context getNewContext();

    // Sets the primary context to the specified context.
    // Will only copy over layers that are in the primary to avoid problems with rendering
    void setContext(Context c);

    // returns a ref to the primary context
    Context& getPrimaryContext();

    vector<string> getLayerOrder();

    // number of layers present in the compositor
    int size();

    int getWidth(string size = "");
    int getHeight(string size = "");

    // Render the primary context
    // since the render functions do hit the js interface, images are allocated and 
    // memory is explicity handled to prevent scope issues
    // The render functions should be used for GUI render calls.
    Image* render(string size = "");

    // render with a given context
    Image* render(Context c, string size = "");

    // Couple functions for rendering specific pixels. Mostly used by optimizers
    // i is the Pixel Number (NOT the array start number, as in i * 4 = pixel number)
    template <typename T>
    inline typename Utils<T>::RGBAColorT renderPixel(Context& c, int i, string size = "");

    // (x, y) version
    template <typename T>
    inline typename Utils<T>::RGBAColorT renderPixel(Context& c, int x, int y, string size = "");

    // floating point coords version
    template <typename T>
    inline typename Utils<T>::RGBAColorT renderPixel(Context& c, float x, float y, string size = "");

    // render directly to a string with the primary context
    string renderToBase64();

    // render directly to a string with a given context
    string renderToBase64(Context c);

    // sets the layer order. True on success.
    // checks to make sure all layers exist in the primary context before overwrite
    bool setLayerOrder(vector<string> order);

    // cache status and manipulation
    vector<string> getCacheSizes();
    bool addCacheSize(string name, float scaleFactor);
    bool deleteCacheSize(string name);
    shared_ptr<Image> getCachedImage(string id, string size);

    // main entry point for starting the search process.
    void startSearch(searchCallback cb, SearchMode mode, map<string, float> settings,
      int threads = 1, string searchRenderSize = "");
    void stopSearch();
    void runSearch();

    // resets images to full white alpha 1
    void resetImages(string name);

    // computes a function for use in the ceres optimizer
    void computeExpContext(Context& c, int px, string functionName, string size = "");
    void computeExpContext(Context& c, int x, int y, string functionName, string size = "");

    // returns the constraint data for modification
    ConstraintData& getConstraintData();

    /*
    outputs a json file containing the following:
     - freeParams
        An array of objects containing information about the parameters used by the optimizer
        Fields: paramName, paramID, layerName, adjustmentType, adjustmentName, selectiveColor.channel, selectiveColor.channel.value, value
     - layerColors
        An array of objects contining information about the layer colors, the target colors,
        and the weights of each color. These are the points at which the residuals are calculated.
        Fields: pixel.x, pixel.y, pixel.flat, array of layer color info (needs consistent order), targetColor, weight
    */
    void paramsToCeres(Context& c, vector<Point> pts, vector<RGBColor> targetColor,
      vector<double> weights, string outputPath);

    /*
    Imports a ceres results file (should be the same format as paramsToCeres) and into
    a new context
    If not null, metadata will import the extra data contained in the file
    */
    Context ceresToContext(string file, map<string, float>& metadata);

  private:
    void addLayer(string name);

    // stores standard scales of the image in the cache
    // standard sizes are: thumb - 15%, small - 25%, medium - 50%
    void cacheScaled(string name);

    // search modes
    void randomSearch(Context start);

    inline float premult(unsigned char px, float a);
    inline unsigned char cvt(float px, float a);

    template <typename T>
    inline T cvtT(T px, T a);
    
    template <typename T>
    inline T normal(T a, T b, T alpha1, T alpha2);

    template <typename T>
    inline T multiply(T a, T b, T alpha1, T alpha2);

    template <typename T>
    inline T screen(T a, T b, T alpha1, T alpha2);

    template <typename T>
    inline T overlay(T a, T b, T alpha1, T alpha2);

    template <typename T>
    inline T hardLight(T a, T b, T alpha1, T alpha2);

    template <typename T>
    inline T softLight(T Dca, T Sca, T Da, T Sa);

    template <typename T>
    inline T linearDodge(T Dca, T Sca, T Da, T Sa);

    template <typename T>
    inline T colorDodge(T Dca, T Sca, T Da, T Sa);

    template <typename T>
    inline T linearBurn(T Dc, T Sc, T Da, T Sa);

    template <typename T>
    inline T linearLight(T Dc, T Sc, T Da, T Sa);

    template <typename T>
    inline typename Utils<T>::RGBColorT color(typename Utils<T>::RGBColorT& dest, typename Utils<T>::RGBColorT& src, T Da, T Sa);
    
    template <typename T>
    inline T lighten(T Dca, T Sca, T Da, T Sa);

    template <typename T>
    inline T darken(T Dca, T Sca, T Da, T Sa);

    template <typename T>
    inline T pinLight(T Dca, T Sca, T Da, T Sa);

    void adjust(Image* adjLayer, Layer& l);

    // adjusts a single pixel according to the given adjustment layer
    template <typename T>
    inline typename Utils<T>::RGBAColorT adjustPixel(typename Utils<T>::RGBAColorT comp, Layer& l);

    // HSL
    inline void hslAdjust(Image* adjLayer, map<string, float> adj);

    template <typename T>
    inline void hslAdjust(typename Utils<T>::RGBAColorT& adjPx, map<string, T>& adj);

    // Levels
    inline void levelsAdjust(Image* adjLayer, map<string, float> adj);

    template <typename T>
    inline void levelsAdjust(typename Utils<T>::RGBAColorT& adjPx, map<string, T>& adj);

    template <typename T>
    inline T levels(T px, T inMin, T inMax, T gamma, T outMin, T outMax);

    // Curves
    inline void curvesAdjust(Image* adjLayer, map<string, float> adj, Layer& l);

    template <typename T>
    inline void curvesAdjust(typename Utils<T>::RGBAColorT& adjPx, map<string, T>& adj, Layer& l);

    // Exposure
    inline void exposureAdjust(Image* adjLayer, map<string, float> adj);

    template <typename T>
    inline void exposureAdjust(typename Utils<T>::RGBAColorT& adjPx, map<string, T>& adj);

    // Gradient Map
    inline void gradientMap(Image* adjLayer, map<string, float> adj, Layer& l);

    template <typename T>
    inline void gradientMap(typename Utils<T>::RGBAColorT& adjPx, map<string, T>& adj, Layer& l);

    // selective color
    inline void selectiveColor(Image* adjLayer, map<string, float> adj, Layer& l);

    template <typename T>
    inline void selectiveColor(typename Utils<T>::RGBAColorT& adjPx, map<string, T>& adj, Layer& l);

    // Color Balance
    inline void colorBalanceAdjust(Image* adjLayer, map<string, float> adj);

    template <typename T>
    inline void colorBalanceAdjust(typename Utils<T>::RGBAColorT& adjPx, map<string, T>& adj);

    template <typename T>
    inline T colorBalance(T px, T shadow, T mid, T high);

    // Photo filter
    inline void photoFilterAdjust(Image* adjLayer, map<string, float> adj);
    
    template<typename T>
    inline void photoFilterAdjust(typename Utils<T>::RGBAColorT& adjPx, map<string, T>& adj);

    // Colorize
    inline void colorizeAdjust(Image* adjLayer, map<string, float> adj);

    template <typename T>
    inline void colorizeAdjust(typename Utils<T>::RGBAColorT& adjPx, map<string, T>& adj);

    // Lighter Colorize
    inline void lighterColorizeAdjust(Image* adjLayer, map<string, float> adj);

    template <typename T>
    inline void lighterColorizeAdjust(typename Utils<T>::RGBAColorT& adjPx, map<string, T>& adj);

    // Overwrite Color
    inline void overwriteColorAdjust(Image* adjLayer, map<string, float> adj);

    template <typename T>
    inline void overwriteColorAdjust(typename Utils<T>::RGBAColorT& adjPx, map<string, T>& adj);

    // compositing order for layers
    vector<string> _layerOrder;

    // Primary rendering context. This is the persistent state of the compositor.
    // Keyed by IDs in the context.
    Context _primary;

    // cached of scaled images for rendering at different sizes
    map<string, map<string, shared_ptr<Image> > > _imageData;

    bool _searchRunning;
    searchCallback _activeCallback;
    vector<thread> _searchThreads;
    string _searchRenderSize;
    SearchMode _searchMode;

    // in an attempt to keep the signature of search mostly the same, additional
    // settings should be stored in this map
    // a list of available settings can be found at the end of this file
    map<string, float> _searchSettings;

    // various search things that should be cached

    // Layers that are allowed to change during the search process
    // Associated settings: "useVisibleLayersOnly"
    // Used by modes: RANDOM
    set<string> _affectedLayers;

    // Bitmaps of each mask layer present in the interface
    // Note: at some point this might need to be converted to an object to handle different types of constraints
    // right now its meant to just be color
    ConstraintData _constraints;
  };

  template<typename T>
  inline typename Utils<T>::RGBAColorT Compositor::renderPixel(Context & c, int i, string size)
  {
    // photoshop appears to start with all white alpha 0 image
    Utils<T>::RGBAColorT compPx = { 1, 1, 1, 0 };

    if (size == "") {
      size = "full";
    }

    // blend the layers
    for (auto id : _layerOrder) {
      Layer& l = c[id];

      if (!l._visible)
        continue;

      Utils<T>::RGBAColorT layerPx;

      // handle adjustment layers
      if (l.isAdjustmentLayer()) {
        // ok so here we adjust the current composition, then blend it as normal below
        // create duplicate of current composite
        layerPx = adjustPixel<T>(compPx, l);
      }
      else if (l.getAdjustments().size() > 0) {
        // so a layer may have other things clipped to it, in which case we apply the
        // specified adjustment only to the source layer and the composite as normal
        layerPx = adjustPixel<T>(_imageData[l.getName()][size]->getPixel(i), l);
      }
      else {
        layerPx = _imageData[l.getName()][size]->getPixel(i);
      }

      // blend the layer
      // a = background, b = new layer
      // alphas
      T ab = layerPx._a * l.getOpacity();
      T aa = compPx._a;
      T ad = aa + ab - aa * ab;

      compPx._a = ad;

      // premult colors
      T rb = layerPx._r * ab;
      T gb = layerPx._g * ab;
      T bb = layerPx._b * ab;

      T ra = compPx._r * aa;
      T ga = compPx._g * aa;
      T ba = compPx._b * aa;

      // blend modes
      if (l._mode == BlendMode::NORMAL) {
        // b over a, standard alpha blend
        compPx._r = cvtT(normal(ra, rb, aa, ab), ad);
        compPx._g = cvtT(normal(ga, gb, aa, ab), ad);
        compPx._b = cvtT(normal(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::MULTIPLY) {
        compPx._r = cvtT(multiply(ra, rb, aa, ab), ad);
        compPx._g = cvtT(multiply(ga, gb, aa, ab), ad);
        compPx._b = cvtT(multiply(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::SCREEN) {
        compPx._r = cvtT(screen(ra, rb, aa, ab), ad);
        compPx._g = cvtT(screen(ga, gb, aa, ab), ad);
        compPx._b = cvtT(screen(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::OVERLAY) {
        compPx._r = cvtT(overlay(ra, rb, aa, ab), ad);
        compPx._g = cvtT(overlay(ga, gb, aa, ab), ad);
        compPx._b = cvtT(overlay(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::HARD_LIGHT) {
        compPx._r = cvtT(hardLight(ra, rb, aa, ab), ad);
        compPx._g = cvtT(hardLight(ga, gb, aa, ab), ad);
        compPx._b = cvtT(hardLight(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::SOFT_LIGHT) {
        compPx._r = cvtT(softLight(ra, rb, aa, ab), ad);
        compPx._g = cvtT(softLight(ga, gb, aa, ab), ad);
        compPx._b = cvtT(softLight(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::LINEAR_DODGE) {
        // special override for alpha here
        ad = (aa + ab > 1) ? 1 : (aa + ab);
        compPx._a = ad;

        compPx._r = cvtT(linearDodge(ra, rb, aa, ab), ad);
        compPx._g = cvtT(linearDodge(ga, gb, aa, ab), ad);
        compPx._b = cvtT(linearDodge(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::COLOR_DODGE) {
        compPx._r = cvtT(colorDodge(ra, rb, aa, ab), ad);
        compPx._g = cvtT(colorDodge(ga, gb, aa, ab), ad);
        compPx._b = cvtT(colorDodge(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::LINEAR_BURN) {
        // need unmultiplied colors for this one
        compPx._r = cvtT(linearBurn(compPx._r, layerPx._r, aa, ab), ad);
        compPx._g = cvtT(linearBurn(compPx._g, layerPx._g, aa, ab), ad);
        compPx._b = cvtT(linearBurn(compPx._b, layerPx._b, aa, ab), ad);
      }
      else if (l._mode == BlendMode::LINEAR_LIGHT) {
        compPx._r = cvtT(linearLight(compPx._r, layerPx._r, aa, ab), ad);
        compPx._g = cvtT(linearLight(compPx._g, layerPx._g, aa, ab), ad);
        compPx._b = cvtT(linearLight(compPx._b, layerPx._b, aa, ab), ad);
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
        compPx._r = cvtT(res._r, ad);
        compPx._g = cvtT(res._g, ad);
        compPx._b = cvtT(res._b, ad);
      }
      else if (l._mode == BlendMode::LIGHTEN) {
        compPx._r = cvtT(lighten(ra, rb, aa, ab), ad);
        compPx._g = cvtT(lighten(ga, gb, aa, ab), ad);
        compPx._b = cvtT(lighten(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::DARKEN) {
        compPx._r = cvtT(darken(ra, rb, aa, ab), ad);
        compPx._g = cvtT(darken(ga, gb, aa, ab), ad);
        compPx._b = cvtT(darken(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::PIN_LIGHT) {
        compPx._r = cvtT(pinLight(ra, rb, aa, ab), ad);
        compPx._g = cvtT(pinLight(ga, gb, aa, ab), ad);
        compPx._b = cvtT(pinLight(ba, bb, aa, ab), ad);
      }
    }

    return compPx;
  }

  template <typename T>
  inline typename Utils<T>::RGBAColorT Compositor::renderPixel(Context & c, int x, int y, string size)
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

    return renderPixel<T>(c, index, size);
  }

  template <typename T>
  inline typename Utils<T>::RGBAColorT Compositor::renderPixel(Context & c, float x, float y, string size)
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

    return renderPixel<T>(c, index, size);
  }

  template<typename T>
  inline T Compositor::cvtT(T px, T a)
  {
    // 0 alpha special case
    if (a == 0)
      return 0;

    T v = px / a;
    return (v > 1) ? 1 : (v < 0) ? 0 : v;
  }

  template<>
  inline ExpStep Compositor::cvtT(ExpStep px, ExpStep a)
  {
    vector<ExpStep> res = px.context->callFunc("cvtT", px, a);
    return res[0];
  }

  template<typename T>
  inline T Compositor::normal(T a, T b, T alpha1, T alpha2)
  {
    return b + a * (1 - alpha2);
  }

  template<typename T>
  inline T Compositor::multiply(T a, T b, T alpha1, T alpha2)
  {
    return b * a + b * (1 - alpha1) + a * (1 - alpha2);
  }

  template<typename T>
  inline T Compositor::screen(T a, T b, T alpha1, T alpha2)
  {
    return b + a - b * a;
  }

  template<typename T>
  inline T Compositor::overlay(T a, T b, T alpha1, T alpha2)
  {
    if (2 * a <= alpha1) {
      return b * a * 2 + b * (1 - alpha1) + a * (1 - alpha2);
    }
    else {
      return b * (1 + alpha1) + a * (1 + alpha2) - 2 * a * b - alpha1 * alpha2;
    }
  }

  template<>
  inline ExpStep Compositor::overlay<ExpStep>(ExpStep a, ExpStep b, ExpStep alpha1, ExpStep alpha2) {
    vector<ExpStep> res = a.context->callFunc("overlay", a, b, alpha1, alpha2);
    return res[0];
  }

  template<typename T>
  inline T Compositor::hardLight(T a, T b, T alpha1, T alpha2)
  {
    if (2 * b <= alpha2) {
      return 2 * b * a + b * (1 - alpha1) + a * (1 - alpha2);
    }
    else {
      return b * (1 + alpha1) + a * (1 + alpha2) - alpha1 * alpha2 - 2 * a * b;
    }
  }

  template<>
  inline ExpStep Compositor::hardLight<ExpStep>(ExpStep a, ExpStep b, ExpStep alpha1, ExpStep alpha2) {
    vector<ExpStep> res = a.context->callFunc("hardLight", a, b, alpha1, alpha2);
    return res[0];
  }

  template<typename T>
  inline T Compositor::softLight(T Dca, T Sca, T Da, T Sa)
  {
    T m = (Da == 0) ? 0 : Dca / Da;

    if (2 * Sca <= Sa) {
      return Dca * (Sa + (2 * Sca - Sa) * (1 - m)) + Sca * (1 - Da) + Dca * (1 - Sa);
    }
    else if (2 * Sca > Sa && 4 * Dca <= Da) {
      return Da * (2 * Sca - Sa) * (16 * m * m * m - 12 * m * m - 3 * m) + Sca - Sca * Da + Dca;
    }
    else if (2 * Sca > Sa && 4 * Dca > Da) {
      return Da * (2 * Sca - Sa) * (sqrt(m) - m) + Sca - Sca * Da + Dca;
    }
    else {
      return normal(Dca, Sca, Da, Sa);
    }
  }

  template<>
  inline ExpStep Compositor::softLight<ExpStep>(ExpStep Dca, ExpStep Sca, ExpStep Da, ExpStep Sa) {
    vector<ExpStep> res = Sca.context->callFunc("softLight", Dca, Sca, Da, Sa);
    return res[0];
  }

  template<typename T>
  inline T Compositor::linearDodge(T Dca, T Sca, T Da, T Sa)
  {
    return Sca + Dca;
  }

  template<typename T>
  inline T Compositor::colorDodge(T Dca, T Sca, T Da, T Sa)
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

  template<>
  inline ExpStep Compositor::colorDodge<ExpStep>(ExpStep Dca, ExpStep Sca, ExpStep Da, ExpStep Sa) {
    vector<ExpStep> res = Sca.context->callFunc("colorDodge", Dca, Sca, Da, Sa);
    return res[0];
  }

  template<typename T>
  inline T Compositor::linearBurn(T Dc, T Sc, T Da, T Sa)
  {
    // special case for handling background with alpha 0
    if (Da == 0)
      return Sc;

    T burn = Dc + Sc - 1;

    // normal blend
    return burn * Sa + Dc * (1 - Sa);
  }

  template<>
  inline ExpStep Compositor::linearBurn<ExpStep>(ExpStep Dc, ExpStep Sc, ExpStep Da, ExpStep Sa) {
    vector<ExpStep> res = Sc.context->callFunc("linearBurn", Dc, Sc, Da, Sa);
    return res[0];
  }

  template<typename T>
  inline T Compositor::linearLight(T Dc, T Sc, T Da, T Sa)
  {
    if (Da == 0)
      return Sc;

    T light = Dc + 2 * Sc - 1;
    return light * Sa + Dc * (1 - Sa);
  }

  template<>
  inline ExpStep Compositor::linearLight<ExpStep>(ExpStep Dc, ExpStep Sc, ExpStep Da, ExpStep Sa) {
    vector<ExpStep> res = Sc.context->callFunc("linearLight", Dc, Sc, Da, Sa);
    return res[0];
  }


  template<typename T>
  inline typename Utils<T>::RGBColorT Compositor::color(typename Utils<T>::RGBColorT & dest, typename Utils<T>::RGBColorT & src, T Da, T Sa)
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
    Utils<T>::HSYColorT dc = Utils<T>::RGBToHSY(dest);
    Utils<T>::HSYColorT sc = Utils<T>::RGBToHSY(src);
    dc._h = sc._h;
    dc._s = sc._s;

    Utils<T>::RGBColorT res = Utils<T>::HSYToRGB(dc);

    // actually have to blend here...
    res._r = res._r * Sa + dest._r * Da * (1 - Sa);
    res._g = res._g * Sa + dest._g * Da * (1 - Sa);
    res._b = res._b * Sa + dest._b * Da * (1 - Sa);

    return res;
  }

  template<>
  inline typename Utils<ExpStep>::RGBColorT Compositor::color<ExpStep>(typename Utils<ExpStep>::RGBColorT & dest,
    typename Utils<ExpStep>::RGBColorT & src, ExpStep Da, ExpStep Sa)
  {
    vector<ExpStep> params;
    params.push_back(dest._r);
    params.push_back(dest._g);
    params.push_back(dest._b);
    params.push_back(src._r);
    params.push_back(src._g);
    params.push_back(src._b);
    params.push_back(Da);
    params.push_back(Sa);

    vector<ExpStep> res = Sa.context->callFunc("color", params);
    Utils<ExpStep>::RGBColorT c;
    c._r = res[0];
    c._g = res[1];
    c._b = res[2];

    return c;
  }

  template<typename T>
  inline T Compositor::lighten(T Dca, T Sca, T Da, T Sa)
  {
    if (Sca > Dca) {
      return Sca + Dca * (1 - Sa);
    }
    else {
      return Dca + Sca * (1 - Da);
    }
  }

  template<>
  inline ExpStep Compositor::lighten<ExpStep>(ExpStep Dca, ExpStep Sca, ExpStep Da, ExpStep Sa) {
    vector<ExpStep> res = Sca.context->callFunc("lighten", Dca, Sca, Da, Sa);
    return res[0];
  }

  template<typename T>
  inline T Compositor::darken(T Dca, T Sca, T Da, T Sa)
  {
    if (Sca > Dca) {
      return Dca + Sca * (1 - Da);
    }
    else {
      return Sca + Dca * (1 - Sa);
    }
  }

  template<>
  inline ExpStep Compositor::darken<ExpStep>(ExpStep Dca, ExpStep Sca, ExpStep Da, ExpStep Sa) {
    vector<ExpStep> res = Sca.context->callFunc("darken", Dca, Sca, Da, Sa);
    return res[0];
  }

  template<typename T>
  inline T Compositor::pinLight(T Dca, T Sca, T Da, T Sa)
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

  template<>
  inline ExpStep Compositor::pinLight<ExpStep>(ExpStep Dca, ExpStep Sca, ExpStep Da, ExpStep Sa) {
    vector<ExpStep> res = Sca.context->callFunc("pinLight", Dca, Sca, Da, Sa);
    return res[0];
  }

  template<typename T>
  inline typename Utils<T>::RGBAColorT Compositor::adjustPixel(typename Utils<T>::RGBAColorT comp, Layer & l)
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

  template<typename T>
  inline void Compositor::hslAdjust(typename Utils<T>::RGBAColorT & adjPx, map<string, T>& adj)
  {
    // all params are between 0 and 1
    T h = adj["hue"];
    T s = adj["sat"];
    T l = adj["light"];

    Utils<T>::HSLColorT c = Utils<T>::RGBToHSL(adjPx._r, adjPx._g, adjPx._b);

    // modify hsl
    c._h = c._h + ((h - 0.5f) * 360);
    c._s = c._s + (s - 0.5f) * 2;
    c._l = c._l + (l - 0.5f) * 2;

    // convert back
    Utils<T>::RGBColorT c2 = Utils<T>::HSLToRGB(c);
    adjPx._r = clamp<T>(c2._r, 0, 1);
    adjPx._g = clamp<T>(c2._g, 0, 1);
    adjPx._b = clamp<T>(c2._b, 0, 1);
  }

  template<typename T>
  inline void Compositor::levelsAdjust(typename Utils<T>::RGBAColorT & adjPx, map<string, T>& adj)
  {
    // all of these keys should exist. if they don't there could be problems...
    T inMin = adj["inMin"];
    T inMax = adj["inMax"];
    T gamma = (adj["gamma"] * 10);
    T outMin = adj["outMin"];
    T outMax = adj["outMax"];

    adjPx._r = clamp<T>(levels(adjPx._r, inMin, inMax, gamma, outMin, outMax), 0.0, 1.0);
    adjPx._g = clamp<T>(levels(adjPx._g, inMin, inMax, gamma, outMin, outMax), 0.0, 1.0);
    adjPx._b = clamp<T>(levels(adjPx._b, inMin, inMax, gamma, outMin, outMax), 0.0, 1.0);
  }

  template<typename T>
  inline T Compositor::levels(T px, T inMin, T inMax, T gamma, T outMin, T outMax)
  {
    // input remapping
    T out = min(max(px - inMin, (T)0.0f) / (inMax - inMin), (T)1.0f);

    if (inMax == inMin)
      out = (T)1;

    if (gamma < 1e-6)
      gamma = 1e-6;   // clamp to small

    // gamma correction
    out = pow(out, 1 / gamma);

    // output remapping
    out = out * (outMax - outMin) + outMin;

    return out;
  }

  template <>
  inline ExpStep Compositor::levels<ExpStep>(ExpStep px, ExpStep inMin, ExpStep inMax, ExpStep gamma, ExpStep outMin, ExpStep outMax) {
    vector<ExpStep> args;
    args.push_back(px);
    args.push_back(inMin);
    args.push_back(inMax);
    args.push_back(gamma);
    args.push_back(outMin);
    args.push_back(outMax);

    vector<ExpStep> res = px.context->callFunc("levels", args);

    return res[0];
  }

  template<typename T>
  inline void Compositor::curvesAdjust(typename Utils<T>::RGBAColorT & adjPx, map<string, T>& adj, Layer & l)
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

  template<typename T>
  inline void Compositor::exposureAdjust(typename Utils<T>::RGBAColorT & adjPx, map<string, T>& adj)
  {
    T exposure = (adj["exposure"] - 0.5f) * 10;
    T offset = adj["offset"] - 0.5f;
    T gamma = adj["gamma"] * 10;

    adjPx._r = clamp<T>(pow(adjPx._r * pow(2, exposure) + offset, 1 / gamma), 0.0, 1.0);
    adjPx._g = clamp<T>(pow(adjPx._g * pow(2, exposure) + offset, 1 / gamma), 0.0, 1.0);
    adjPx._b = clamp<T>(pow(adjPx._b * pow(2, exposure) + offset, 1 / gamma), 0.0, 1.0);
  }

  template<typename T>
  inline void Compositor::gradientMap(typename Utils<T>::RGBAColorT & adjPx, map<string, T>& adj, Layer & l)
  {
    T y = 0.299f * adjPx._r + 0.587f * adjPx._g + 0.114f * adjPx._b;

    // map L to an rgb color. L is between 0 and 1.
    Utils<T>::RGBColorT grad = l.evalGradient(y);

    adjPx._r = clamp<T>(grad._r, 0.0, 1.0);
    adjPx._g = clamp<T>(grad._g, 0.0, 1.0);
    adjPx._b = clamp<T>(grad._b, 0.0, 1.0);
  }

  template<typename T>
  inline void Compositor::selectiveColor(typename Utils<T>::RGBAColorT & adjPx, map<string, T>& adj, Layer & l)
  {
    map<string, map<string, float>> data = l.getSelectiveColor();

    for (auto& c : data) {
      for (auto& p : c.second) {
        p.second = (p.second - 0.5) * 2;
      }
    }

    // convert to hsl
    Utils<T>::HSLColorT hslColor = Utils<T>::RGBToHSL(adjPx._r, adjPx._g, adjPx._b);
    T chroma = max(adjPx._r, max(adjPx._g, adjPx._b)) - min(adjPx._r, min(adjPx._g, adjPx._b));

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
    T w1, w2, w3, w4, wc;

    // chroma
    wc = chroma / 1.0f;

    // hue - always 60 deg intervals
    w1 = 1 - ((hslColor._h - (interval * 60.0f)) / 60.0f);  // distance from low interval
    w2 = 1 - w1;

    // luma - measure distance from midtones, w3 is always midtone
    w3 = 1 - abs(hslColor._l - 0.5f);
    w4 = 1 - w3;

    // do the adjustment
    Utils<T>::CMYKColorT cmykColor = Utils<T>::RGBToCMYK(adjPx._r, adjPx._g, adjPx._b);

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

    Utils<T>::RGBColorT res = Utils<T>::CMYKToRGB(cmykColor);
    adjPx._r = clamp<T>(res._r, 0, 1);
    adjPx._g = clamp<T>(res._g, 0, 1);
    adjPx._b = clamp<T>(res._b, 0, 1);
  }

  template<>
  inline void Compositor::selectiveColor<ExpStep>(typename Utils<ExpStep>::RGBAColorT& adjPx, map<string, ExpStep>& adj, Layer& l) {
    // there are 9*4 + 3 params here
    vector<ExpStep> params;
    params.push_back(adjPx._r);
    params.push_back(adjPx._g);
    params.push_back(adjPx._b);

    for (auto c : l._expSelectiveColor) {
      for (auto p : c.second) {
        params.push_back(p.second);
      }
    }

    vector<ExpStep> res = adjPx._r.context->callFunc("selectiveColor", params);

    adjPx._r = res[0];
    adjPx._g = res[1];
    adjPx._b = res[2];
  }

  template<typename T>
  inline void Compositor::colorBalanceAdjust(typename Utils<T>::RGBAColorT & adjPx, map<string, T>& adj)
  {
    Utils<T>::RGBColorT balanced;
    balanced._r = colorBalance(adjPx._r, (adj["shadowR"] - 0.5f) * 2, (adj["midR"] - 0.5f) * 2, (adj["highR"] - 0.5f) * 2);
    balanced._g = colorBalance(adjPx._g, (adj["shadowG"] - 0.5f) * 2, (adj["midG"] - 0.5f) * 2, (adj["highG"] - 0.5f) * 2);
    balanced._b = colorBalance(adjPx._b, (adj["shadowB"] - 0.5f) * 2, (adj["midB"] - 0.5f) * 2, (adj["highB"] - 0.5f) * 2);

    if (adj["preserveLuma"] > 0) {
      Utils<T>::HSLColorT l = Utils<T>::RGBToHSL(balanced);
      T originalLuma = 0.5f * (max(adjPx._r, max(adjPx._g, adjPx._b)) + min(adjPx._r, min(adjPx._g, adjPx._b)));
      balanced = Utils<T>::HSLToRGB(l._h, l._s, originalLuma);
    }

    adjPx._r = clamp<T>(balanced._r, 0, 1);
    adjPx._g = clamp<T>(balanced._g, 0, 1);
    adjPx._b = clamp<T>(balanced._b, 0, 1);
  }

  template<>
  inline void Compositor::colorBalanceAdjust<ExpStep>(typename Utils<ExpStep>::RGBAColorT& adjPx, map<string, ExpStep>& adj) {
    vector<ExpStep> params;
    params.push_back(adjPx._r);
    params.push_back(adjPx._g);
    params.push_back(adjPx._b);

    params.push_back(adj["shadowR"]);
    params.push_back(adj["shadowG"]);
    params.push_back(adj["shadowB"]);
    params.push_back(adj["midR"]);
    params.push_back(adj["midG"]);
    params.push_back(adj["midB"]);
    params.push_back(adj["highR"]);
    params.push_back(adj["highG"]);
    params.push_back(adj["highB"]);

    vector<ExpStep> res = adjPx._r.context->callFunc("colorBalanceAdjust", params);

    adjPx._r = res[0];
    adjPx._g = res[1];
    adjPx._b = res[2];
  }

  template<typename T>
  inline T Compositor::colorBalance(T px, T shadow, T mid, T high)
  {
    // some arbitrary constants...?
    T a = 0.25f;
    T b = 0.333f;
    T scale = 0.7f;

    T s = shadow * (clamp<T>((px - b) / -a + 0.5f, 0, 1.0f) * scale);
    T m = mid * (clamp<T>((px - b) / a + 0.5f, 0, 1.0f) * clamp<T>((px + b - 1.0f) / -a + 0.5f, 0, 1.0f) * scale);
    T h = high * (clamp<T>((px + b - 1.0f) / a + 0.5f, 0, 1.0f) * scale);

    return clamp<T>(px + s + m + h, 0, 1.0);
  }

  template<typename T>
  inline void Compositor::photoFilterAdjust(typename Utils<T>::RGBAColorT & adjPx, map<string, T>& adj)
  {
    T d = adj["density"];
    T fr = adjPx._r * adj["r"];
    T fg = adjPx._g * adj["g"];
    T fb = adjPx._b * adj["b"];

    if (adj["preserveLuma"] > 0) {
      Utils<T>::HSLColorT l = Utils<T>::RGBToHSL(fr, fg, fb);
      T originalLuma = 0.5 * (max(adjPx._r, max(adjPx._g, adjPx._b)) + min(adjPx._r, min(adjPx._g, adjPx._b)));
      Utils<T>::RGBColorT rgb = Utils<T>::HSLToRGB(l._h, l._s, originalLuma);
      fr = rgb._r;
      fg = rgb._g;
      fb = rgb._b;
    }

    // weight by density
    adjPx._r = clamp<T>(fr * d + adjPx._r * (1 - d), 0, 1);
    adjPx._g = clamp<T>(fg * d + adjPx._g * (1 - d), 0, 1);
    adjPx._b = clamp<T>(fb * d + adjPx._b * (1 - d), 0, 1);
  }

  template <>
  inline void Compositor::photoFilterAdjust<ExpStep>(typename Utils<ExpStep>::RGBAColorT& adjPx, map<string, ExpStep>& adj) {
    vector<ExpStep> params;
    params.push_back(adjPx._r);
    params.push_back(adjPx._g);
    params.push_back(adjPx._b);

    params.push_back(adj["density"]);
    params.push_back(adj["r"]);
    params.push_back(adj["g"]);
    params.push_back(adj["b"]);

    vector<ExpStep> res = adjPx._r.context->callFunc("photoFilter", params);
    adjPx._r = res[0];
    adjPx._g = res[1];
    adjPx._b = res[2];
  }

  template<typename T>
  inline void Compositor::colorizeAdjust(typename Utils<T>::RGBAColorT & adjPx, map<string, T>& adj)
  {
    T sr = adj["r"];
    T sg = adj["g"];
    T sb = adj["b"];
    T a = adj["a"];
    Utils<T>::HSYColorT sc = Utils<T>::RGBToHSY(sr, sg, sb);

    // color keeps dest luma and keeps top hue and chroma
    Utils<T>::HSYColorT dc = Utils<T>::RGBToHSY(adjPx._r, adjPx._g, adjPx._b);
    dc._h = sc._h;
    dc._s = sc._s;

    Utils<T>::RGBColorT res = Utils<T>::HSYToRGB(dc);

    // blend the resulting colors according to alpha
    adjPx._r = clamp<T>(res._r * a + adjPx._r * (1 - a), 0, 1);
    adjPx._g = clamp<T>(res._g * a + adjPx._g * (1 - a), 0, 1);
    adjPx._b = clamp<T>(res._b * a + adjPx._b * (1 - a), 0, 1);
  }

  template<typename T>
  inline void Compositor::lighterColorizeAdjust(typename Utils<T>::RGBAColorT & adjPx, map<string, T>& adj)
  {
    T sr = adj["r"];
    T sg = adj["g"];
    T sb = adj["b"];
    T a = adj["a"];
    T y = 0.299f * sr + 0.587f * sg + 0.114f * sb;

    T yp = 0.299f * adjPx._r + 0.587f * adjPx._g + 0.114f * adjPx._b;

    adjPx._r = (yp > y) ? adjPx._r : sr;
    adjPx._g = (yp > y) ? adjPx._g : sg;
    adjPx._b = (yp > y) ? adjPx._b : sb;

    // blend the resulting colors according to alpha
    adjPx._r = clamp<T>(adjPx._r * a + adjPx._r * (1 - a), 0, 1);
    adjPx._g = clamp<T>(adjPx._g * a + adjPx._g * (1 - a), 0, 1);
    adjPx._b = clamp<T>(adjPx._b * a + adjPx._b * (1 - a), 0, 1);
  }

  template<>
  inline void Compositor::lighterColorizeAdjust<ExpStep>(typename Utils<ExpStep>::RGBAColorT& adjPx, map<string, ExpStep>& adj) {
    vector<ExpStep> params;
    params.push_back(adjPx._r);
    params.push_back(adjPx._g);
    params.push_back(adjPx._b);

    params.push_back(adj["r"]);
    params.push_back(adj["g"]);
    params.push_back(adj["b"]);
    params.push_back(adj["a"]);

    vector<ExpStep> res = adjPx._r.context->callFunc("lighterColorizeAdjust", params);
    adjPx._r = res[0];
    adjPx._g = res[1];
    adjPx._b = res[2];
  }

  template<typename T>
  inline void Compositor::overwriteColorAdjust(typename Utils<T>::RGBAColorT & adjPx, map<string, T>& adj)
  {
    T sr = adj["r"];
    T sg = adj["g"];
    T sb = adj["b"];
    T a = adj["a"];

    // blend the resulting colors according to alpha
    adjPx._r = clamp<T>(sr * a + adjPx._r * (1 - a), 0, 1);
    adjPx._g = clamp<T>(sg * a + adjPx._g * (1 - a), 0, 1);
    adjPx._b = clamp<T>(sb * a + adjPx._b * (1 - a), 0, 1);
  }

  inline void Compositor::levelsAdjust(Image* adjLayer, map<string, float> adj) {
    vector<unsigned char>& img = adjLayer->getData();

    // gamma is between 0 and 10 usually
    // so sometimes these values are missing and we should use defaults.
    float inMin = (adj.count("inMin") > 0) ? adj["inMin"] : 0;
    float inMax = (adj.count("inMax") > 0) ? adj["inMax"] : 1;
    float gamma = (adj.count("gamma") > 0) ? adj["gamma"] : 0.1;
    float outMin = (adj.count("outMin") > 0) ? adj["outMin"] : 0;
    float outMax = (adj.count("outMax") > 0) ? adj["outMax"] : 1;

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor layerPx = adjLayer->getPixel(i);
      levelsAdjust(layerPx, adj);

      // convert to char
      img[i * 4] = (unsigned char)(layerPx._r * 255);
      img[i * 4 + 1] = (unsigned char)(layerPx._g * 255);
      img[i * 4 + 2] = (unsigned char)(layerPx._b * 255);
    }
  }

  template<>
  inline typename Utils<ExpStep>::RGBAColorT Compositor::adjustPixel<ExpStep>(typename Utils<ExpStep>::RGBAColorT comp, Layer & l)
  {
    for (auto type : l.getAdjustments()) {
      if (type == AdjustmentType::HSL) {
        hslAdjust(comp, l._expAdjustments[type]);
      }
      else if (type == AdjustmentType::LEVELS) {
        levelsAdjust(comp, l._expAdjustments[type]);
      }
      else if (type == AdjustmentType::CURVES) {
        curvesAdjust(comp, l._expAdjustments[type], l);
      }
      else if (type == AdjustmentType::EXPOSURE) {
        exposureAdjust(comp, l._expAdjustments[type]);
      }
      else if (type == AdjustmentType::GRADIENT) {
        gradientMap(comp, l._expAdjustments[type], l);
      }
      else if (type == AdjustmentType::SELECTIVE_COLOR) {
        selectiveColor(comp, l._expAdjustments[type], l);
      }
      else if (type == AdjustmentType::COLOR_BALANCE) {
        colorBalanceAdjust(comp, l._expAdjustments[type]);
      }
      else if (type == AdjustmentType::PHOTO_FILTER) {
        photoFilterAdjust(comp, l._expAdjustments[type]);
      }
      else if (type == AdjustmentType::COLORIZE) {
        colorizeAdjust(comp, l._expAdjustments[type]);
      }
      else if (type == AdjustmentType::LIGHTER_COLORIZE) {
        lighterColorizeAdjust(comp, l._expAdjustments[type]);
      }
      else if (type == AdjustmentType::OVERWRITE_COLOR) {
        overwriteColorAdjust(comp, l._expAdjustments[type]);
      }
    }

    return comp;
  }

  // specialization for exp step (pixel access different, attribute access different, etc)
  template<>
  inline typename Utils<ExpStep>::RGBAColorT Compositor::renderPixel<ExpStep>(Context & c, int i, string size)
  {
    // photoshop appears to start with all white alpha 0 image
    Utils<ExpStep>::RGBAColorT compPx = { ExpStep(1.0), ExpStep(1.0), ExpStep(1.0), ExpStep(0.0) };

    if (size == "") {
      size = "full";
    }

    // blend the layers
    for (auto id : _layerOrder) {
      Layer& l = c[id];

      if (!l._visible)
        continue;

      getLogger()->log("Compositing layer " + l.getName());

      Utils<ExpStep>::RGBAColorT layerPx;

      // handle adjustment layers
      if (l.isAdjustmentLayer()) {
        // ok so here we adjust the current composition, then blend it as normal below
        // create duplicate of current composite
        layerPx = adjustPixel<ExpStep>(compPx, l);
      }
      else if (l.getAdjustments().size() > 0) {
        // so a layer may have other things clipped to it, in which case we apply the
        // specified adjustment only to the source layer and the composite as normal
        layerPx = adjustPixel<ExpStep>(_imageData[l.getName()][size]->getPixel(), l);
      }
      else {
        layerPx = _imageData[l.getName()][size]->getPixel();
      }

      // blend the layer
      // a = background, b = new layer
      // alphas
      ExpStep ab = layerPx._a * l.getOpacity();
      ExpStep aa = compPx._a;
      ExpStep ad = aa + ab - aa * ab;

      compPx._a = ad;

      // premult colors
      ExpStep rb = layerPx._r * ab;
      ExpStep gb = layerPx._g * ab;
      ExpStep bb = layerPx._b * ab;

      ExpStep ra = compPx._r * aa;
      ExpStep ga = compPx._g * aa;
      ExpStep ba = compPx._b * aa;

      // blend modes
      if (l._mode == BlendMode::NORMAL) {
        // b over a, standard alpha blend
        compPx._r = cvtT(normal(ra, rb, aa, ab), ad);
        compPx._g = cvtT(normal(ga, gb, aa, ab), ad);
        compPx._b = cvtT(normal(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::MULTIPLY) {
        compPx._r = cvtT(multiply(ra, rb, aa, ab), ad);
        compPx._g = cvtT(multiply(ga, gb, aa, ab), ad);
        compPx._b = cvtT(multiply(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::SCREEN) {
        compPx._r = cvtT(screen(ra, rb, aa, ab), ad);
        compPx._g = cvtT(screen(ga, gb, aa, ab), ad);
        compPx._b = cvtT(screen(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::OVERLAY) {
        compPx._r = cvtT(overlay(ra, rb, aa, ab), ad);
        compPx._g = cvtT(overlay(ga, gb, aa, ab), ad);
        compPx._b = cvtT(overlay(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::HARD_LIGHT) {
        compPx._r = cvtT(hardLight(ra, rb, aa, ab), ad);
        compPx._g = cvtT(hardLight(ga, gb, aa, ab), ad);
        compPx._b = cvtT(hardLight(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::SOFT_LIGHT) {
        compPx._r = cvtT(softLight(ra, rb, aa, ab), ad);
        compPx._g = cvtT(softLight(ga, gb, aa, ab), ad);
        compPx._b = cvtT(softLight(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::LINEAR_DODGE) {
        // special override for alpha here

        //linearDodgeAlpha(aa, ab) = { return (aa + ab > 1) ? 1 : (aa + ab); }
        vector<ExpStep> res = ab.context->callFunc("linearDodgeAlpha", aa, ab);
        ad = res[0];
        compPx._a = ad;

        compPx._r = cvtT(linearDodge(ra, rb, aa, ab), ad);
        compPx._g = cvtT(linearDodge(ga, gb, aa, ab), ad);
        compPx._b = cvtT(linearDodge(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::COLOR_DODGE) {
        compPx._r = cvtT(colorDodge(ra, rb, aa, ab), ad);
        compPx._g = cvtT(colorDodge(ga, gb, aa, ab), ad);
        compPx._b = cvtT(colorDodge(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::LINEAR_BURN) {
        // need unmultiplied colors for this one
        compPx._r = cvtT(linearBurn(compPx._r, layerPx._r, aa, ab), ad);
        compPx._g = cvtT(linearBurn(compPx._g, layerPx._g, aa, ab), ad);
        compPx._b = cvtT(linearBurn(compPx._b, layerPx._b, aa, ab), ad);
      }
      else if (l._mode == BlendMode::LINEAR_LIGHT) {
        compPx._r = cvtT(linearLight(compPx._r, layerPx._r, aa, ab), ad);
        compPx._g = cvtT(linearLight(compPx._g, layerPx._g, aa, ab), ad);
        compPx._b = cvtT(linearLight(compPx._b, layerPx._b, aa, ab), ad);
      }
      else if (l._mode == BlendMode::COLOR) {
        // also no premult colors
        Utils<ExpStep>::RGBColorT dest;
        dest._r = compPx._r;
        dest._g = compPx._g;
        dest._b = compPx._b;

        Utils<ExpStep>::RGBColorT src;
        src._r = layerPx._r;
        src._g = layerPx._g;
        src._b = layerPx._b;

        Utils<ExpStep>::RGBColorT res = color(dest, src, aa, ab);
        compPx._r = cvtT(res._r, ad);
        compPx._g = cvtT(res._g, ad);
        compPx._b = cvtT(res._b, ad);
      }
      else if (l._mode == BlendMode::LIGHTEN) {
        compPx._r = cvtT(lighten(ra, rb, aa, ab), ad);
        compPx._g = cvtT(lighten(ga, gb, aa, ab), ad);
        compPx._b = cvtT(lighten(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::DARKEN) {
        compPx._r = cvtT(darken(ra, rb, aa, ab), ad);
        compPx._g = cvtT(darken(ga, gb, aa, ab), ad);
        compPx._b = cvtT(darken(ba, bb, aa, ab), ad);
      }
      else if (l._mode == BlendMode::PIN_LIGHT) {
        compPx._r = cvtT(pinLight(ra, rb, aa, ab), ad);
        compPx._g = cvtT(pinLight(ga, gb, aa, ab), ad);
        compPx._b = cvtT(pinLight(ba, bb, aa, ab), ad);
      }
    }

    return compPx;
  }
}

/*
_searchSettings - Allowed settings
Settings not on this list have no effect besides taking up a tiny bit of memory

useVisibleLayersOnly
  - Used by: RANDOM (default: 1)
  - Set to 1 to exclude hidden layers at time of initialization.
  - Set to 0 to include everything

modifyLayerBlendModes
  - Used by: RANDOM (default: 0)
  - Set to 1 to allow the random sampler to change blend modes
  - Set to 0 to disallow
*/
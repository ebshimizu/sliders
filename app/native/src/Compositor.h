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

using namespace std;

namespace Comp {
  const string version = "0.1";

  typedef map<string, Layer> Context;
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

    // Render the primary context
    // since the render functions do hit the js interface, images are allocated and 
    // memory is explicity handled to prevent scope issues
    Image* render(string size = "");

    // render with a given context
    Image* render(Context c, string size = "");

    // Couple functions for rendering specific pixels. This function is used internally
    // to render pixels for the final composite and is exposed here for use by
    // various optimizers.
    // i is the Pixel Number (NOT the array start number, as in i * 4 = pixel number)
    RGBAColor renderPixel(Context& c, int i, string size = "");

    // (x, y) version
    RGBAColor renderPixel(Context& c, int x, int y, string size = "");

    // floating point coords version
    RGBAColor renderPixel(Context& c, float x, float y, string size = "");

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

  private:
    void addLayer(string name);

    // stores standard scales of the image in the cache
    // standard sizes are: thumb - 15%, small - 25%, medium - 50%
    void cacheScaled(string name);

    // search modes
    void randomSearch(Context start);

    inline float premult(unsigned char px, float a);
    inline unsigned char cvt(float px, float a);
    inline float cvtf(float px, float a);
    inline float normal(float a, float b, float alpha1, float alpha2);
    inline float multiply(float a, float b, float alpha1, float alpha2);
    inline float screen(float a, float b, float alpha1, float alpha2);
    inline float overlay(float a, float b, float alpha1, float alpha2);
    inline float hardLight(float a, float b, float alpha1, float alpha2);
    inline float softLight(float Dca, float Sca, float Da, float Sa);
    inline float linearDodge(float Dca, float Sca, float Da, float Sa);
    inline float colorDodge(float Dca, float Sca, float Da, float Sa);
    inline float linearBurn(float Dc, float Sc, float Da, float Sa);
    inline float linearLight(float Dc, float Sc, float Da, float Sa);
    inline RGBColor color(RGBColor& dest, RGBColor& src, float Da, float Sa);
    inline float lighten(float Dca, float Sca, float Da, float Sa);
    inline float darken(float Dca, float Sca, float Da, float Sa);
    inline float pinLight(float Dca, float Sca, float Da, float Sa);

    void adjust(Image* adjLayer, Layer& l);

    // adjusts a single pixel according to the given adjustment layer
    RGBAColor adjustPixel(RGBAColor comp, Layer& l);

    inline void hslAdjust(Image* adjLayer, map<string, float> adj);
    inline void hslAdjust(RGBAColor& adjPx, map<string, float>& adj);

    inline void levelsAdjust(Image* adjLayer, map<string, float> adj);
    inline void levelsAdjust(RGBAColor& adjPx, map<string, float>& adj);
    inline float levels(float px, float inMin, float inMax, float gamma, float outMin, float outMax);

    inline void curvesAdjust(Image* adjLayer, map<string, float> adj, Layer& l);
    inline void curvesAdjust(RGBAColor& adjPx, map<string, float>& adj, Layer& l);

    inline void exposureAdjust(Image* adjLayer, map<string, float> adj);
    inline void exposureAdjust(RGBAColor& adjPx, map<string, float>& adj);

    inline void gradientMap(Image* adjLayer, map<string, float> adj, Layer& l);
    inline void gradientMap(RGBAColor& adjPx, map<string, float>& adj, Layer& l);

    inline void selectiveColor(Image* adjLayer, map<string, float> adj, Layer& l);
    inline void selectiveColor(RGBAColor& adjPx, map<string, float>& adj, Layer& l);

    inline void colorBalanceAdjust(Image* adjLayer, map<string, float> adj);
    inline void colorBalanceAdjust(RGBAColor& adjPx, map<string, float>& adj);
    inline float colorBalance(float px, float shadow, float mid, float high);

    inline void photoFilterAdjust(Image* adjLayer, map<string, float> adj);
    inline void photoFilterAdjust(RGBAColor& adjPx, map<string, float>& adj);

    inline void colorizeAdjust(Image* adjLayer, map<string, float> adj);
    inline void colorizeAdjust(RGBAColor& adjPx, map<string, float>& adj);

    inline void lighterColorizeAdjust(Image* adjLayer, map<string, float> adj);
    inline void lighterColorizeAdjust(RGBAColor& adjPx, map<string, float>& adj);

    inline void overwriteColorAdjust(Image* adjLayer, map<string, float> adj);
    inline void overwriteColorAdjust(RGBAColor& adjPx, map<string, float>& adj);

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
  };
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
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
#include "searchData.h"
#include "ClickMap.h"
#include "Selection.h"

using namespace std;

namespace Comp {
  typedef function<void(Image*, Context, map<string, float>, map<string, string>)> searchCallback;

  enum SearchMode {
    SEARCH_DEBUG,          // returns the same image at intervals for testing app functionality
    RANDOM,         // randomly adjusts the available parameters to do things
    SMART_RANDOM,
    MCMC,
    NLS,
    EXPLORATORY
  };

  enum ImportanceMapMode {
    ALPHA,
    VISIBILITY_DELTA,
    SPEC_VISIBILITY_DELTA,
    EFFECTIVE_CHANGE
  };

  enum EffectMode {
    NONE,
    STROKE
  };

  // contains importance data for the given layer and parameter
  struct Importance {
    string _layerName;
    AdjustmentType _adjType;
    string _param;

    float _depth;
    float _totalAlpha;
    float _totalLuma;
    float _deltaMag;
    float _mssim;
  };

  class ImageEffect {
  public: 
    ImageEffect();

    void stroke(RGBColor color, int width);

    EffectMode _mode;
    RGBColor _color;
    int _width;
  };

  struct Group {
    string _name;
    bool _readOnly;   // read only groups are in the inherent photoshop strucutre and cannot be removed right now
    set<string> _affectedLayers;

    ImageEffect _effect;
  };

  // the compositor for now assumes that every layer it contains have the same dimensions.
  // having unequal layer sizes will likely lead to crashes or other undefined behavior
  class Compositor {
  public:
    Compositor();
    Compositor(string filename, string imageDir);
    ~Compositor();

    // adds a layer, true on success, false if layer already exists
    bool addLayer(string name, string file);
    bool addLayer(string name, Image& img);

    // adds a layer mask to the specified layer
    bool addLayerMask(string name, string file);
    bool addLayerMask(string name, Image& img);

    // adds an adjustment layer
    bool addAdjustmentLayer(string name);
    
    bool copyLayer(string src, string dest);

    bool deleteLayer(string name);

    // changes the sort order of a layer
    void reorderLayer(int from, int to);

    Layer& getLayer(int id);
    Layer& getLayer(string name);

    // group operations
    bool addGroup(string name, set<string> layers, float priority, bool readOnly = false);

    // this is basically like add group but I'd like to make it very clear what you're doing with this function
    bool addGroupFromExistingLayer(string name, set<string> layers, float priority, bool readOnly = false);
    
    void deleteGroup(string name);
    void addLayerToGroup(string layer, string group);
    void removeLayerFromGroup(string layer, string group);
    void setGroupLayers(string group, set<string> layers);
    void setGroupOrder(multimap<float, string> order);
    void setGroupOrder(string group, float priority);
    bool layerInGroup(string layer, string group);
    bool isGroup(string group);
    Group getGroup(string name);
    void setGroupEffect(string name, ImageEffect effect);

    multimap<float, string> getGroupOrder();

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

    // wrapper for old render calls
    Image* render(Context& c, string size = "");

    // render with a given context
    Image* render(Context& c, Image* comp, vector<string> order, float co, string size = "");

    // renders the composition up to and including the specified layer.
    // additionally, the pixels unaffected by the given layer are dimmed by a maximum specified amount
    // (floor of 20% opacity)
    Image* renderUpToLayer(Context& c, string layer, string orderLayer, float dim, string size = "");

    // computes the local importance centered at the given context
    // returns a list of importance values that are _not_ sorted (since there are
    // multiple ways to sort the data)
    // image size parameter is mostly a speed control since we'll have to render a bunch of stuff
    // in this function
    vector<Importance> localImportance(Context c, string size = "");

    // Couple functions for rendering specific pixels. Mostly used by optimizers
    // i is the Pixel Number (NOT the array start number, as in i * 4 = pixel number)
    inline Utils<float>::RGBAColorT renderPixel(Context& c, int i, string size = "");

    // (x, y) version
    inline Utils<float>::RGBAColorT renderPixel(Context& c, int x, int y, string size = "");

    // floating point coords version
    inline Utils<float>::RGBAColorT renderPixel(Context& c, float x, float y, string size = "");

    inline Utils<float>::RGBAColorT renderPixel(Context& c, typename Utils<float>::RGBAColorT* compPx, vector<string> order, int i, float co, string size = "");

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
    //void computeExpContext(Context& c, int px, string functionName, string size = "");
    //void computeExpContext(Context& c, int x, int y, string functionName, string size = "");

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
    a new context. Returns false on error.
    If not null, metadata will import the extra data contained in the file
    */
    bool ceresToContext(string file, map<string, float>& metadata, Context& c);

    // Adds a search group to the compositor
    void addSearchGroup(SearchGroup g);

    // resets the search group list
    void clearSearchGroups();

    // converts a context to a normal vector, with a vector of json objects
    // acting as the key to map back to the context when needed.
    vector<double> contextToVector(Context c, nlohmann::json& key);

    // updates the cached key 
    vector<double> contextToVector(Context c);

    // uses the cached key to deserialize a vector
    Context vectorToContext(vector<double> v);

    // takes a darkroom file and loads it, returning a context
    Context contextFromDarkroom(string file);

    // computes the regional importance for the rectangle specified by x, y, w, h
    // and returns the results in names and scores
    void regionalImportance(string mode, vector<string>& names, vector<double>& scores, int x, int y, int w, int h);

    // Importance calculated for a single point
    void pointImportance(string mode, map<string, double>& scores, int x, int y, Context& c);
    double pointImportance(ImportanceMapMode mode, string layer, int x, int y, Context& c);

    // importance maps!
    // this function creates an importance map for a specific layer and stores it in the 
    // Compositor's cache. It also returns the resulting image.
    shared_ptr<ImportanceMap> computeImportanceMap(string layer, ImportanceMapMode mode, Context& current);

    // given a mode, compute all the importance maps for a particular mode
    void computeAllImportanceMaps(ImportanceMapMode mode, Context& current);

    // importance map cache manipulation
    shared_ptr<ImportanceMap> getImportanceMap(string layer, ImportanceMapMode mode);
    void deleteImportanceMap(string layer, ImportanceMapMode mode);
    void deleteLayerImportanceMaps(string layer);
    void deleteImportanceMapType(ImportanceMapMode mode);
    void deleteAllImportanceMaps();
    void dumpImportanceMaps(string folder);
    bool importanceMapExists(string layer, ImportanceMapMode mode);

    // returns a copy of the current cache
    map<string, map<ImportanceMapMode, shared_ptr<ImportanceMap>>> getImportanceMapCache();

    // clickmaps
    // this function just straight up returns a clickmap. at some point there might be a function
    // that does all of this stuff internally, but for now it should be ok to let everything
    // be scriptable for testing
    ClickMap* createClickMap(ImportanceMapMode mode, Context current);

    // tags
    // goes through each layer and adds tags to layers based on content and adjustments
    void analyzeAndTag();

    // returns a set of all the tags used by the layers
    set<string> uniqueTags();

    // gets tags for each layer
    set<string> getTags(string layer);

    // gets all tags for each layer
    map<string, set<string>> allTags();

    // deletes tags from a layer
    void deleteTags(string layer);

    // deletes tags from all layers
    void deleteAllTags();

    // determines if a layer has been tagged with the given tag
    bool hasTag(string layer, string tag);

    // selects layers and parameters based on the current goal
    // goal format is uh, changeable but for now it's limited to brightness / color
    // also just targeting a single goal right now
    map<string, map<AdjustmentType, vector<GoalResult>>> goalSelect(Goal g, Context& c, int x, int y, int maxLevel = 100);

    // the multi-pixel version of goal select
    map<string, map<AdjustmentType, vector<GoalResult>>> goalSelect(Goal g, Context& c, vector<int> x, vector<int> y, int maxLevel = 100);

    // tests a given layer's adjustment to see if it meets the goal
    bool adjustmentMeetsGoal(string layer, AdjustmentType adj, Goal& g, Context c,
      vector<int> x, vector<int> y, int maxLevel, float& minimum, map<string, float>& paramVals);

    // multi-pixel rectangular select
    map<string, map<AdjustmentType, vector<GoalResult>>> goalSelect(Goal g, Context& c, int x, int y, int w, int h, int maxLevel = 100);

    // uh, poisson disk sampling because i need a test function?
    void initPoissonDisk(int n, int level, int k = 30);

    // populates that cache
    void initPoissonDisks();

    vector<string> getFlatLayerOrder();
    void getFlatLayerOrder(vector<string> currentOrder, vector<string>& order);

    // given a layer name, returns the things that affect the layer
    // in order from bottom to top. This should end up being a combination
    // of selection groups and render groups
    vector<string> getModifierOrder(string layer);
    vector<string> findLayerInTree(string target, vector<string> currentOrder);

    // returns the parent layer name of the given layer
    // returns empty string for root.
    string getParent(string layer);

    // applies the specified offset to each layer recusively
    // as in render groups do not get an offset
    void offsetLayer(string layer, float dx, float dy);
    void resetLayerOffset(string layer);

    // histogram intersection with adjusted layers
    double layerHistogramIntersect(Context& c, string layer1, string layer2, float binSize = 0.05, string size = "full");
    double propLayerHistogramIntersect(Context& c, string layer1, string layer2, float binSize = 0.05, string size = "full");

    // given an image (with valid render map), return another image that contains
    // a map of which pixels are part of the given group
    Image* getGroupInclusionMap(Image* img, string group);

  private:
    void addLayer(string name);
    void addLayerMask(string name);

    int indexedOffset(float x, float y, string size);
    int applyIndexedOffset(int i, float dx, float dy, string size);

    // stores standard scales of the image in the cache
    // standard sizes are: thumb - 15%, small - 25%, medium - 50%
    void cacheScaled(string name);

    float finiteDifference(Image* a, Image* b, float delta);

    // this returns the numerical derivative for each of the parameters in currentVals
    // based on the objective function in the goal.
    // the context can be freely modified it's just for rendering
    map<string, float> getDelta(Goal& g, Context& r, float fx, map<string, float>& currentVals,
      string layer, AdjustmentType t, vector<int>& x, vector<int>& y);

    // search modes
    void randomSearch(Context start);

    // parent thread for the exploratory search. Right now it's single threaded with
    // no option to change, may adjust in the future.
    void exploratorySearch();

    // subroutine for the exploratory search
    void expStructSearch(ExpSearchSet& activeSet);

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

    template <typename T>
    inline T colorBurn(T Dc, T Sc, T Da, T Sa);

    template <typename T>
    inline T vividLight(T Dc, T Sc, T Da, T Sa);

    void adjust(Image* adjLayer, Layer& l);

    // adjusts a single pixel according to the given adjustment layer
    template <typename T>
    inline typename Utils<T>::RGBAColorT adjustPixel(typename Utils<T>::RGBAColorT comp, Layer& l);

    // HSL
    inline void hslAdjust(Image* adjLayer, map<string, float> adj);

    template <typename T>
    inline void hslAdjust(typename Utils<T>::RGBAColorT& adjPx, T h, T s, T l);

    // Levels
    inline void levelsAdjust(Image* adjLayer, map<string, float> adj);

    template <typename T>
    inline void levelsAdjust(typename Utils<T>::RGBAColorT& adjPx, T inMin, T inMax, T gamma, T outMin, T outMax);

    template <typename T>
    inline T levels(T px, T inMin, T inMax, T gamma, T outMin, T outMax);

    // Curves
    inline void curvesAdjust(Image* adjLayer, map<string, float> adj, Layer& l);

    template <typename T>
    inline void curvesAdjust(typename Utils<T>::RGBAColorT& adjPx, map<string, T>& adj, Layer& l);

    // Exposure
    inline void exposureAdjust(Image* adjLayer, map<string, float> adj);

    template <typename T>
    inline void exposureAdjust(typename Utils<T>::RGBAColorT& adjPx, T exposure, T offset, T gamma);

    // Gradient Map
    inline void gradientMap(Image* adjLayer, map<string, float> adj, Layer& l);

    template <typename T>
    inline void gradientMap(typename Utils<T>::RGBAColorT& adjPx, map<string, T>& adj, Layer& l);

    // selective color
    inline void selectiveColor(Image* adjLayer, map<string, float> adj, Layer& l);

    template <typename T>
    inline void selectiveColor(typename Utils<T>::RGBAColorT& adjPx, map<string, T>& adj, Layer& l);

    // specific version for standard renderer
    template <typename T>
    inline void selectiveColor(typename Utils<T>::RGBAColorT& adjPx, map<string, map<string, float>>& data, bool rel);

    // Color Balance
    inline void colorBalanceAdjust(Image* adjLayer, map<string, float> adj);

    template <typename T>
    inline void colorBalanceAdjust(typename Utils<T>::RGBAColorT& adjPx, T shadowR, T shadowG, T shadowB,
      T midR, T midG, T midB, T highR, T highG, T highB, T pl);

    template <typename T>
    inline T colorBalance(T px, T shadow, T mid, T high);

    // Photo filter
    inline void photoFilterAdjust(Image* adjLayer, map<string, float> adj);
    
    template<typename T>
    inline void photoFilterAdjust(typename Utils<T>::RGBAColorT& adjPx, T d, T r, T g, T b, T pl);

    // Colorize
    inline void colorizeAdjust(Image* adjLayer, map<string, float> adj);

    template <typename T>
    inline void colorizeAdjust(typename Utils<T>::RGBAColorT& adjPx, T sr, T sg, T sb, T a);

    // Lighter Colorize
    inline void lighterColorizeAdjust(Image* adjLayer, map<string, float> adj);

    template <typename T>
    inline void lighterColorizeAdjust(typename Utils<T>::RGBAColorT& adjPx, T sr, T sg, T sb, T a);

    // Overwrite Color
    inline void overwriteColorAdjust(Image* adjLayer, map<string, float> adj);

    template <typename T>
    inline void overwriteColorAdjust(typename Utils<T>::RGBAColorT& adjPx, T sr, T sg, T sb, T a);

    // invert
    inline void invertAdjust(Image* adjLayer);

    template<typename T>
    inline void invertAdjustT(typename Utils<T>::RGBAColorT& adjPx);

    // brightness/contrast
    inline void brightnessAdjust(Image* adjLayer, map<string, float> adj);

    template <typename T>
    inline void brightnessAdjust(typename Utils<T>::RGBAColorT& adjPx, map<string, T>& adj);

    // conditional blend
    // returns adjusted alpha of src layer, calculation happens disregarding alpha channel
    template <typename T>
    inline T conditionalBlend(const string& channel, float sbMin, float sbMax, float swMin, float swMax,
      float dbMin, float dbMax, float dwMin, float dwMax,
      T srcR, T srcG, T srcB, T destR, T destG, T destB);

    // translates a vector to a context given a key.
    // these functions are similar to what happens in sendToCeres
    Context vectorToContext(vector<double> x, nlohmann::json& key);

    // compositing order for layers
    vector<string> _layerOrder;

    // Primary rendering context. This is the persistent state of the compositor.
    // Keyed by IDs in the context.
    Context _primary;

    // compositing order for groups
    multimap<float, string> _groupOrder;
    map<string, Group> _groups;

    // layer tags
    // determined by analysis of content or by user input
    map<string, set<string>> _layerTags;

    // cached of scaled images for rendering at different sizes
    map<string, map<string, shared_ptr<Image>>> _imageData;
    map<string, map<string, shared_ptr<Image>>> _layerMasks;
    map<string, map<string, shared_ptr<Image>>> _precompRenderCache;

    bool _searchRunning;
    searchCallback _activeCallback;
    vector<thread> _searchThreads;
    string _searchRenderSize;
    SearchMode _searchMode;

    // a context storing the state of the image when start search was called
    Context _initSearchContext;

    // in an attempt to keep the signature of search mostly the same, additional
    // settings should be stored in this map
    // a list of available settings can be found at the end of this file
    map<string, float> _searchSettings;

    // the exploratory search needs a bit more info to be really useful
    // and we'll store that here
    vector<SearchGroup> _searchGroups;

    // various search things that should be cached
    // eploratory structural search stuff
    vector<vector<double>> _structResults;
    vector<int> _structParams;

    // cache of importance maps generated by the compositor
    // layer name : { importance map mode : image}
    map<string, map<ImportanceMapMode, shared_ptr<ImportanceMap> > > _importanceMapCache;

    // Layers that are allowed to change during the search process
    // Associated settings: "useVisibleLayersOnly"
    // Used by modes: RANDOM
    set<string> _affectedLayers;

    // Bitmaps of each mask layer present in the interface
    // Note: at some point this might need to be converted to an object to handle different types of constraints
    // right now its meant to just be color
    ConstraintData _constraints;

    // when serializing contexts, this key can be used to convert back into
    // a context.
    nlohmann::json _vectorKey;

    // precomputed sampling patterns for various levels of detail (0 is full res)
    map<int, map<int, shared_ptr<PoissonDisk>>> _pdiskCache;
  };

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

  template<typename T>
  inline T Compositor::colorBurn(T Dc, T Sc, T Da, T Sa)
  {
    // this is a blend that basically just happens based on color
    if (Sc == 0)
      return 0 * Sa + Dc * (1 - Sa);

    T burn = max<T>(1 - ((1 - Dc) / Sc), 0);

    // normal blend
    return burn * Sa + Dc * (1 - Sa);
  }

  template<typename T>
  inline T Compositor::vividLight(T Dc, T Sc, T Da, T Sa)
  {
    if (Sc < 0.5) {
      return colorBurn(Dc, Sc * 2, Da, Sa);
    }
    else {
      return colorDodge(Dc * Da, (T)((2 * Sc - 0.5) * Sa), Da, Sa);
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
        auto adj = l.getAdjustment(type);
        T h = adj["hue"];
        T s = adj["sat"];
        T l = adj["light"];
        hslAdjust(comp, h, s, l);
      }
      else if (type == AdjustmentType::LEVELS) {
        auto adj = l.getAdjustment(type);
        T inMin = adj["inMin"];
        T inMax = adj["inMax"];
        T gamma = (adj["gamma"] * 10);
        T outMin = adj["outMin"];
        T outMax = adj["outMax"];

        levelsAdjust(comp, inMin, inMax, gamma, outMin, outMax);
      }
      else if (type == AdjustmentType::CURVES) {
        curvesAdjust(comp, l.getAdjustment(type), l);
      }
      else if (type == AdjustmentType::EXPOSURE) {
        auto adj = l.getAdjustment(type);
        T exposure = adj["exposure"];
        T offset = adj["offset"];
        T gamma = adj["gamma"];

        exposureAdjust(comp, exposure, offset, gamma);
      }
      else if (type == AdjustmentType::GRADIENT) {
        gradientMap(comp, l.getAdjustment(type), l);
      }
      else if (type == AdjustmentType::SELECTIVE_COLOR) {
        selectiveColor(comp, l.getAdjustment(type), l);
      }
      else if (type == AdjustmentType::COLOR_BALANCE) {
        auto adj = l.getAdjustment(type);
        T shadowR = adj["shadowR"];
        T shadowG = adj["shadowG"];
        T shadowB = adj["shadowB"];
        T midR = adj["midR"];
        T midG = adj["midG"];
        T midB = adj["midB"];
        T highR = adj["highR"];
        T highG = adj["highG"];
        T highB = adj["highB"];
        T pl = adj["preserveLuma"];

        colorBalanceAdjust(comp, shadowR, shadowG, shadowB, midR, midG, midB, highR, highG, highB, pl);
      }
      else if (type == AdjustmentType::PHOTO_FILTER) {
        auto adj = l.getAdjustment(type);
        T d = adj["density"];
        T r = adj["r"];
        T g = adj["g"];
        T b = adj["b"];
        T pl = adj["preserveLuma"];
        photoFilterAdjust(comp, d, r, g, b, pl);
      }
      else if (type == AdjustmentType::COLORIZE) {
        auto adj = l.getAdjustment(type);
        T sr = adj["r"];
        T sg = adj["g"];
        T sb = adj["b"];
        T a = adj["a"];

        colorizeAdjust(comp, sr, sg, sb, a);
      }
      else if (type == AdjustmentType::LIGHTER_COLORIZE) {
        auto adj = l.getAdjustment(type);
        T sr = adj["r"];
        T sg = adj["g"];
        T sb = adj["b"];
        T a = adj["a"];
        lighterColorizeAdjust(comp, sr, sg, sb, a);
      }
      else if (type == AdjustmentType::OVERWRITE_COLOR) {
        auto adj = l.getAdjustment(type);
        T sr = adj["r"];
        T sg = adj["g"];
        T sb = adj["b"];
        T a = adj["a"];

        overwriteColorAdjust(comp, sr, sg, sb, a);
      }
      else if (type == AdjustmentType::INVERT) {
        invertAdjustT<T>(comp);
      }
      else if (type == AdjustmentType::BRIGHTNESS) {
        brightnessAdjust(comp, l.getAdjustment(type));
      }
    }

    return comp;
  }

  template<typename T>
  inline void Compositor::hslAdjust(typename Utils<T>::RGBAColorT & adjPx, T h, T s, T l)
  {
    // all params are between 0 and 1
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
  inline void Compositor::levelsAdjust(typename Utils<T>::RGBAColorT & adjPx, T inMin, T inMax, T gamma, T outMin, T outMax)
  {
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
  inline void Compositor::exposureAdjust(typename Utils<T>::RGBAColorT & adjPx, T exposure, T offset, T gamma)
  {
    exposure = (exposure - 0.5f) * 10;
    offset = offset - 0.5f;
    gamma = gamma * 10;

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
  inline void Compositor::selectiveColor(typename Utils<T>::RGBAColorT& adjPx, map<string, map<string, float>>& data, bool rel)
  {
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

    if (rel) {
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

  template<typename T>
  inline void Compositor::selectiveColor(typename Utils<T>::RGBAColorT & adjPx, map<string, T>& adj, Layer & l)
  {
    map<string, map<string, float>> data = l.getSelectiveColor();

    for (auto& c : data) {
      for (auto& p : c.second) {
        p.second = (p.second - 0.5) * 2;
      }
    }

    selectiveColor<T>(adjPx, data, adj["relative"] > 0);
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
  inline void Compositor::colorBalanceAdjust(typename Utils<T>::RGBAColorT & adjPx, T shadowR, T shadowG,
    T shadowB, T midR, T midG, T midB, T highR, T highG, T highB, T pl)
  {
    Utils<T>::RGBColorT balanced;
    balanced._r = colorBalance(adjPx._r, (shadowR - 0.5f) * 2, (midR - 0.5f) * 2, (highR - 0.5f) * 2);
    balanced._g = colorBalance(adjPx._g, (shadowG - 0.5f) * 2, (midG - 0.5f) * 2, (highG - 0.5f) * 2);
    balanced._b = colorBalance(adjPx._b, (shadowB - 0.5f) * 2, (midB - 0.5f) * 2, (highB - 0.5f) * 2);

    if (pl > 0) {
      Utils<T>::HSLColorT l = Utils<T>::RGBToHSL(balanced);
      T originalLuma = 0.5f * (max(adjPx._r, max(adjPx._g, adjPx._b)) + min(adjPx._r, min(adjPx._g, adjPx._b)));
      balanced = Utils<T>::HSLToRGB(l._h, l._s, originalLuma);
    }

    adjPx._r = clamp<T>(balanced._r, 0, 1);
    adjPx._g = clamp<T>(balanced._g, 0, 1);
    adjPx._b = clamp<T>(balanced._b, 0, 1);
  }

  template<>
  inline void Compositor::colorBalanceAdjust<ExpStep>(typename Utils<ExpStep>::RGBAColorT& adjPx, ExpStep shadowR, ExpStep shadowG, ExpStep shadowB,
    ExpStep midR, ExpStep midG, ExpStep midB, ExpStep highR, ExpStep highG, ExpStep highB, ExpStep pl) {
    vector<ExpStep> params;
    params.push_back(adjPx._r);
    params.push_back(adjPx._g);
    params.push_back(adjPx._b);

    params.push_back(shadowR);
    params.push_back(shadowG);
    params.push_back(shadowB);
    params.push_back(midR);
    params.push_back(midG);
    params.push_back(midB);
    params.push_back(highR);
    params.push_back(highG);
    params.push_back(highB);

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
  inline void Compositor::photoFilterAdjust(typename Utils<T>::RGBAColorT & adjPx, T d, T r, T g, T b, T pl)
  {
    T fr = adjPx._r * r;
    T fg = adjPx._g * g;
    T fb = adjPx._b * b;

    if (pl > 0) {
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
  inline void Compositor::photoFilterAdjust<ExpStep>(typename Utils<ExpStep>::RGBAColorT& adjPx, ExpStep d, ExpStep r, ExpStep g, ExpStep b, ExpStep pl) {
    vector<ExpStep> params;
    params.push_back(adjPx._r);
    params.push_back(adjPx._g);
    params.push_back(adjPx._b);

    params.push_back(d);
    params.push_back(r);
    params.push_back(g);
    params.push_back(b);

    vector<ExpStep> res = adjPx._r.context->callFunc("photoFilter", params);
    adjPx._r = res[0];
    adjPx._g = res[1];
    adjPx._b = res[2];
  }

  template<typename T>
  inline void Compositor::colorizeAdjust(typename Utils<T>::RGBAColorT & adjPx, T sr, T sg, T sb, T a)
  {
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
  inline void Compositor::lighterColorizeAdjust(typename Utils<T>::RGBAColorT & adjPx, T sr, T sg, T sb, T a)
  {
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
  inline void Compositor::lighterColorizeAdjust<ExpStep>(typename Utils<ExpStep>::RGBAColorT& adjPx, ExpStep sr, ExpStep sg, ExpStep sb, ExpStep a) {
    vector<ExpStep> params;
    params.push_back(adjPx._r);
    params.push_back(adjPx._g);
    params.push_back(adjPx._b);

    params.push_back(sr);
    params.push_back(sg);
    params.push_back(sb);
    params.push_back(a);

    vector<ExpStep> res = adjPx._r.context->callFunc("lighterColorizeAdjust", params);
    adjPx._r = res[0];
    adjPx._g = res[1];
    adjPx._b = res[2];
  }

  template<typename T>
  inline void Compositor::overwriteColorAdjust(typename Utils<T>::RGBAColorT & adjPx, T sr, T sg, T sb, T a)
  {
    // blend the resulting colors according to alpha
    adjPx._r = clamp<T>(sr * a + adjPx._r * (1 - a), 0, 1);
    adjPx._g = clamp<T>(sg * a + adjPx._g * (1 - a), 0, 1);
    adjPx._b = clamp<T>(sb * a + adjPx._b * (1 - a), 0, 1);
  }

  template<typename T>
  inline void Compositor::invertAdjustT(typename Utils<T>::RGBAColorT & adjPx)
  {
    adjPx._r = ((T)1 - adjPx._r);
    adjPx._g = ((T)1 - adjPx._g);
    adjPx._b = ((T)1 - adjPx._b);
  }

  // simplistic contrast / brightness approach, likely very similar to the "legacy" option in photoshop
  template<typename T>
  inline void Compositor::brightnessAdjust(typename Utils<T>::RGBAColorT & adjPx, map<string, T>& adj)
  {
    // rescale adjustments to [-1, 1] from [0, 1]
    T c = adj["contrast"] * (T)2 - (T)1;
    T b = adj["brightness"] * (T)2 - (T)1;

    // scale the adjustment value to a factor we can multiply the pixel value by
    T factor = (((T)(1.01568)) * (c + (T)1)) / ((T)1.01568 - c);

    adjPx._r = clamp<T>(factor * (adjPx._r - (T)0.5) + (T)0.5 + b, 0, 1);
    adjPx._g = clamp<T>(factor * (adjPx._g - (T)0.5) + (T)0.5 + b, 0, 1);
    adjPx._b = clamp<T>(factor * (adjPx._b - (T)0.5) + (T)0.5 + b, 0, 1);
  }

  template<typename T>
  inline T Compositor::conditionalBlend(const string& channel, float sbMin, float sbMax, float swMin, float swMax,
    float dbMin, float dbMax, float dwMin, float dwMax,
    T srcR, T srcG, T srcB, T destR, T destG, T destB)
  {
    T src;
    T dest;

    if (channel == "") {
      // unsure what photoshop is using as grey here, will do flat average first
      src = (srcR + srcG + srcB) / 3;
      dest = (destR + destG + destG) / 3;
    }
    else if (channel == "red") {
      src = srcR;
      dest = destR;
    }
    else if (channel == "green") {
      src = srcG;
      dest = destG;
    }
    else if (channel == "blue") {
      src = srcB;
      dest = srcB;
    }

    // do the blend
    T alpha;

    // check if in entire bounds
    if (src >= sbMin && src <= swMax) {
      // check if in blending bounds
      if (src >= sbMin && src < sbMax) {
        // interp
        alpha = (src - sbMin) / (sbMax - sbMin);
      }
      else if (src > swMin && src <= swMax) {
        // interp
        alpha = 1 - ((src - swMin) / (swMax - swMin));
      }
      else {
        alpha = (T)1;
      }
    }
    else {
      alpha = (T)0;
    }

    // assuming it deos source checking first then dest checking
    // also assuming this is multiplicative in which case order doesn't matter
    // check dest bounds
    if (dest >= dbMin && dest <= dwMax) {
      // check if in blending bounds
      if (dest >= dbMin && dest < dbMax) {
        // interp
        alpha *= (dest - dbMin) / (dbMax - dbMin);
      }
      else if (dest > dwMin && dest <= dwMax) {
        // interp
        alpha *= (T)1 - ((dest - dwMin) / (dwMax - dwMin));
      }
    }
    else {
      alpha *= (T)0;
    }

    return alpha;
  }

  inline void Compositor::levelsAdjust(Image* adjLayer, map<string, float> adj) {
    vector<unsigned char>& img = adjLayer->getData();

    // gamma is between 0 and 10 usually
    // so sometimes these values are missing and we should use defaults.
    float inMin = adj["inMin"];
    float inMax = adj["inMax"];
    float gamma = (adj["gamma"] * 10);
    float outMin = adj["outMin"];
    float outMax = adj["outMax"];

    for (int i = 0; i < img.size() / 4; i++) {
      RGBAColor layerPx = adjLayer->getPixel(i);
      levelsAdjust(layerPx, inMin, inMax, gamma, outMin, outMax);

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
        ExpStep h = l._expAdjustments[type]["hue"];
        ExpStep s = l._expAdjustments[type]["sat"];
        ExpStep lt = l._expAdjustments[type]["light"];
        hslAdjust(comp, h, s, lt);
      }
      else if (type == AdjustmentType::LEVELS) {
        ExpStep inMin = l._expAdjustments[type]["inMin"];
        ExpStep inMax = l._expAdjustments[type]["inMax"];
        ExpStep gamma = (l._expAdjustments[type]["gamma"] * 10);
        ExpStep outMin = l._expAdjustments[type]["outMin"];
        ExpStep outMax = l._expAdjustments[type]["outMax"];
        levelsAdjust(comp, inMin, inMax, gamma, outMin, outMax);
      }
      else if (type == AdjustmentType::CURVES) {
        curvesAdjust(comp, l._expAdjustments[type], l);
      }
      else if (type == AdjustmentType::EXPOSURE) {
        ExpStep exposure = l._expAdjustments[type]["exposure"];
        ExpStep offset = l._expAdjustments[type]["offset"];
        ExpStep gamma = l._expAdjustments[type]["gamma"];

        exposureAdjust(comp, exposure, offset, gamma);
      }
      else if (type == AdjustmentType::GRADIENT) {
        gradientMap(comp, l._expAdjustments[type], l);
      }
      else if (type == AdjustmentType::SELECTIVE_COLOR) {
        selectiveColor(comp, l._expAdjustments[type], l);
      }
      else if (type == AdjustmentType::COLOR_BALANCE) {
        ExpStep shadowR = l._expAdjustments[type]["shadowR"];
        ExpStep shadowG = l._expAdjustments[type]["shadowG"];
        ExpStep shadowB = l._expAdjustments[type]["shadowB"];
        ExpStep midR = l._expAdjustments[type]["midR"];
        ExpStep midG = l._expAdjustments[type]["midG"];
        ExpStep midB = l._expAdjustments[type]["midB"];
        ExpStep highR = l._expAdjustments[type]["highR"];
        ExpStep highG = l._expAdjustments[type]["highG"];
        ExpStep highB = l._expAdjustments[type]["highB"];
        ExpStep pl = l._expAdjustments[type]["preserveLuma"];

        colorBalanceAdjust(comp, shadowR, shadowG, shadowB, midR, midG, midB, highR, highG, highB, pl);
      }
      else if (type == AdjustmentType::PHOTO_FILTER) {
        ExpStep d = l._expAdjustments[type]["density"];
        ExpStep r = l._expAdjustments[type]["r"];
        ExpStep g = l._expAdjustments[type]["g"];
        ExpStep b = l._expAdjustments[type]["b"];
        ExpStep pl = l._expAdjustments[type]["preserveLuma"];
        photoFilterAdjust(comp, d, r, g, b, pl);
      }
      else if (type == AdjustmentType::COLORIZE) {
        ExpStep sr = l._expAdjustments[type]["r"];
        ExpStep sg = l._expAdjustments[type]["g"];
        ExpStep sb = l._expAdjustments[type]["b"];
        ExpStep a = l._expAdjustments[type]["a"];
        colorizeAdjust(comp, sr, sg, sb, a);
      }
      else if (type == AdjustmentType::LIGHTER_COLORIZE) {
        ExpStep sr = l._expAdjustments[type]["r"];
        ExpStep sg = l._expAdjustments[type]["g"];
        ExpStep sb = l._expAdjustments[type]["b"];
        ExpStep a = l._expAdjustments[type]["a"];
        lighterColorizeAdjust(comp, sr, sg, sb, a);
      }
      else if (type == AdjustmentType::OVERWRITE_COLOR) {
        ExpStep sr = l._expAdjustments[type]["r"];
        ExpStep sg = l._expAdjustments[type]["g"];
        ExpStep sb = l._expAdjustments[type]["b"];
        ExpStep a = l._expAdjustments[type]["a"];
        overwriteColorAdjust(comp, sr, sg, sb, a);
      }
      else if (type == AdjustmentType::INVERT) {
        invertAdjustT<ExpStep>(comp);
      }
      else if (type == AdjustmentType::BRIGHTNESS) {
        brightnessAdjust(comp, l._expAdjustments[type]);
      }
    }

    return comp;
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
/*
CompositorNode.h - Node.js bindings for the Compositor library
author: Evan Shimizu
*/

#pragma once

#include "Logger.h"
#include "Image.h"
#include "Compositor.h"
#include "util.h"
#include "Model.h"
#include "testHarness.h"

#include <nan.h>

using namespace std;

// quick check to see if pointer is null or not. If so, throws an error
inline void nullcheck(void* ptr, string caller);

void log(const Nan::FunctionCallbackInfo<v8::Value>& info);
void setLogLocation(const Nan::FunctionCallbackInfo<v8::Value>& info);
void setLogLevel(const Nan::FunctionCallbackInfo<v8::Value>& info);
void hardware_concurrency(const Nan::FunctionCallbackInfo<v8::Value>& info);
void runTest(const Nan::FunctionCallbackInfo<v8::Value>& info);
void runAllTest(const Nan::FunctionCallbackInfo<v8::Value>& info);

class ImageWrapper : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);
  static Nan::Persistent<v8::Function> imageConstructor;

  Comp::Image* _image;

private:
  explicit ImageWrapper(unsigned int w = 0, unsigned int h = 0);
  explicit ImageWrapper(string filename);
  explicit ImageWrapper(Comp::Image* img);
  ~ImageWrapper();
  
  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getData(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getBase64(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void width(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void height(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void save(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void filename(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void structDiff(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void structIndBinDiff(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void structBinDiff(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void structTotalBinDiff(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void structAvgBinDiff(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void structPctDiff(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void structSSIMDiff(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void stats(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void diff(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void writeToImageData(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void fill(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void histogramIntersect(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void pctHistogramIntersect(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void chamferDistance(const Nan::FunctionCallbackInfo<v8::Value>& info);

  bool _deleteOnDestruct;
};

class ImportanceMapWrapper : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);
  static Nan::Persistent<v8::Function> importanceMapConstructor;

private:
  explicit ImportanceMapWrapper(shared_ptr<Comp::ImportanceMap> m, string name, Comp::ImportanceMapMode type);
  ~ImportanceMapWrapper();

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void image(v8::Local<v8::String>, const Nan::PropertyCallbackInfo<v8::Value>& info);
  static void dump(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getVal(const Nan::FunctionCallbackInfo<v8::Value>& info);

  shared_ptr<Comp::ImportanceMap> _m;
  string _name;
  Comp::ImportanceMapMode _type;
};

// LayerRef is a wrapper around a pointer to an existing layer.
// It is not likely that we'll be instantiating layers from the JS interface,
// instead we'll ask the compositor to load things instead. Therefore, we
// only need references to the existing layers instead of a fully-featured object.
// This does mean though that a layer object may be deleted out from under this object,
// so each call will require a nullptr check.
class LayerRef : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);
  static Nan::Persistent<v8::Function> layerConstructor;

private:
  explicit LayerRef(Comp::Layer* src);
  ~LayerRef();

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void width(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void height(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void image(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void reset(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void visible(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void opacity(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void blendMode(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void name(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void isAdjustmentLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getAdjustment(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteAdjustment(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteAllAdjustments(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getAdjustments(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addAdjustment(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addHSLAdjustment(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addLevelsAdjustment(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addCurvesChannel(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteCurvesChannel(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void evalCurve(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getCurve(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addExposureAdjustment(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addGradient(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void evalGradient(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getGradient(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void selectiveColor(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void selectiveColorChannel(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void colorBalance(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addPhotoFilter(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void colorize(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void lighterColorize(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void overwriteColor(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addInvertAdjustment(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void brightnessContrast(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void resetImage(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void type(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void conditionalBlend(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getMask(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void offset(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void setPrecompOrder(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getPrecompOrder(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void isPrecomp(const Nan::FunctionCallbackInfo<v8::Value>& info);

  Comp::Layer* _layer;
};

class ContextWrapper : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);
  static Nan::Persistent<v8::Function> contextConstructor;

  // it's not a complicated object, we just have a reference here
  Comp::Context _context;
private:
  explicit ContextWrapper(Comp::Context ctx);

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void keys(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void layerVector(const Nan::FunctionCallbackInfo<v8::Value>& info);
};

/* 
class StatsWrapper : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);
  static Nan::Persistent<v8::Function> statsConstructor;

  Comp::Stats _stats;

private:
  explicit StatsWrapper(Comp::Stats stats);

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
};
*/

void asyncSampleEvent(uv_work_t* req);
void asyncNop(uv_work_t* req);

class CompositorWrapper : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);

  Comp::Compositor* _compositor;
private:
  explicit CompositorWrapper();
  ~CompositorWrapper();

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void isLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addBase64Layer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void copyLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getAllLayers(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void setLayerOrder(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getLayerNames(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void size(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void imageDimensions(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void render(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void asyncRender(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void renderContext(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void asyncRenderContext(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getContext(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getCacheSizes(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addCacheSize(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteCacheSize(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getCachedImage(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void reorderLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void startSearch(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void stopSearch(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void setContext(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void resetImages(const Nan::FunctionCallbackInfo<v8::Value>& info);
  //static void computeExpContext(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void setMaskLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getMaskLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteMaskLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void clearMask(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void paramsToCeres(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void ceresToContext(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void renderPixel(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getPixelConstraints(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void computeErrorMap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addSearchGroup(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void clearSearchGroups(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void contextFromVector(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void contextFromDarkroom(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void localImportance(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void importanceInRegion(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void pointImportance(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void computeImportanceMap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void computeAllImportanceMaps(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getImportanceMap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteImportanceMap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteLayerImportanceMaps(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteImportanceMapType(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteAllImportanceMaps(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void dumpImportanceMaps(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void availableImportanceMaps(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void createClickMap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void analyzeAndTag(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void uniqueTags(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getTags(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void allTags(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteTags(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteAllTags(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void hasTag(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void goalSelect(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addMask(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addPoissonDiskCache(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void initPoissonDisks(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addGroup(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addGroupEffect(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addGroupFromExistingLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteGroup(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addLayerToGroup(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void removeLayerFromGroup(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void setGroupOrder(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getGroupOrder(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getGroup(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void setGroupLayers(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void layerInGroup(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void isGroup(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void renderUpToLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void asyncRenderUpToLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getFlatLayerOrder(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getModifierOrder(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void offsetLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void resetLayerOffset(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void layerHistogramIntersect(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void propLayerHistogramIntersect(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getGroupInclusionMap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void renderOnlyLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static Nan::Persistent<v8::Function> compositorConstructor;
};

class ClickMapWrapper : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);
  static Nan::Persistent<v8::Function> clickMapConstructor;

private:
  explicit ClickMapWrapper(Comp::ClickMap* clmap);
  ~ClickMapWrapper();

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void init(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void compute(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void visualize(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void active(const Nan::FunctionCallbackInfo<v8::Value>& info);

  Comp::ClickMap* _map;
};

class ModelWrapper : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);
  static Nan::Persistent<v8::Function> modelConstructor;

  Comp::Model* _model;
private:
  explicit ModelWrapper();
  ~ModelWrapper();

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void analyze(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void report(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void sample(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void nonParametricSample(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addSchema(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void schemaSample(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getInputData(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addSlider(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addSliderFromExamples(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void sliderSample(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void exportSliderGraph(const Nan::FunctionCallbackInfo<v8::Value>& info);
};

class UISliderWrapper : public Nan::ObjectWrap {
public: 
  static void Init(v8::Local<v8::Object> exports);
  static Nan::Persistent<v8::Function> uiSliderConstructor;

  Comp::UISlider* _slider;

  // slider object might be owned by somethin else
  bool _deleteOnDestroy;
private:
  explicit UISliderWrapper(Comp::UISlider* s);
  ~UISliderWrapper();

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void setVal(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getVal(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void displayName(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void layer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void param(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void type(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void toJSON(const Nan::FunctionCallbackInfo<v8::Value>& info);
};

class UIMetaSliderWrapper : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);
  static Nan::Persistent<v8::Function> uiMetaSliderConstructor;

  Comp::UIMetaSlider* _mSlider;

private:
  explicit UIMetaSliderWrapper(Comp::UIMetaSlider* s);
  ~UIMetaSliderWrapper();

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addSlider(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getSlider(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void size(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void names(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteSlider(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void setPoints(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void setContext(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void displayName(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getVal(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void reassignMax(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void toJSON(const Nan::FunctionCallbackInfo<v8::Value>& info);
};

class UISamplerWrapper : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);
  static Nan::Persistent<v8::Function> uiSamplerConstructor;

  Comp::UISampler* _sampler;
private:
  explicit UISamplerWrapper(Comp::UISampler* s);
  ~UISamplerWrapper();

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void displayName(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addParam(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteParam(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void params(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void sample(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void eval(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void toJSON(const Nan::FunctionCallbackInfo<v8::Value>& info);
};

class RenderWorker : public Nan::AsyncWorker {
public:
  RenderWorker(Nan::Callback *callback, string size, Comp::Compositor* c);

  // render a specific context async
  RenderWorker(Nan::Callback *callback, string size, Comp::Compositor* c, Comp::Context ctx);
 
  // render with the renderUpToLayer function instead
  RenderWorker(Nan::Callback *callback, string size, Comp::Compositor* c, Comp::Context ctx, string layer, string pc, float dim);

  ~RenderWorker() {}

  void Execute() override;

protected:
  void HandleOKCallback() override;

private:
  Comp::Compositor* _c;
  string _size;
  Comp::Image* _img;

  bool _customContext;
  Comp::Context _ctx;
  float _dim;
  string _layer;
  string _pc;
};

class StopSearchWorker : public Nan::AsyncWorker {
public:
  StopSearchWorker(Nan::Callback* callback, Comp::Compositor* c);
  void Execute() override;

protected:
  void HandleOKCallback() override;

private:
  Comp::Compositor* _c;
};

struct asyncSampleEventData {
  uv_work_t request;

  Comp::Image* img;
  Comp::Context ctx;
  map<string, float> meta;
  map<string, string> meta2;
  CompositorWrapper* c;
};

v8::Local<v8::Value> excGet(v8::Local<v8::Object>& obj, string key);
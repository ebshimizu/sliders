/*
CompositorNode.h - Node.js bindings for the Compositor library
author: Evan Shimizu
*/

#pragma once

#include "Logger.h"
#include "Image.h"
#include "Compositor.h"

#include <nan.h>

using namespace std;

// quick check to see if pointer is null or not. If so, throws an error
inline void nullcheck(void* ptr, string caller);

void log(const Nan::FunctionCallbackInfo<v8::Value>& info);
void setLogLocation(const Nan::FunctionCallbackInfo<v8::Value>& info);
void setLogLevel(const Nan::FunctionCallbackInfo<v8::Value>& info);

class ImageWrapper : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);
  static Nan::Persistent<v8::Function> imageConstructor;

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

  Comp::Image* _image;
  bool _deleteOnDestruct;
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

  Comp::Layer* _layer;
};

class CompositorWrapper : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);

private:
  explicit CompositorWrapper();
  ~CompositorWrapper();

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void addLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void copyLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void deleteLayer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getAllLayers(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void setLayerOrder(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getLayerNames(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void size(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void render(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static Nan::Persistent<v8::Function> compositorConstructor;

  Comp::Compositor* _compositor;
};
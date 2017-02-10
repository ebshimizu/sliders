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

class ImageWrapper : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);

private:
  explicit ImageWrapper(unsigned int w = 0, unsigned int h = 0);
  explicit ImageWrapper(string filename);
  ~ImageWrapper();
  
  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getData(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void getBase64(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void width(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void height(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static Nan::Persistent<v8::Function> constructor;

  Comp::Image* _image;
};
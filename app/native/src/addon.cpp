#include <nan.h>
#include "CompositorNode.h"

void InitAll(v8::Local<v8::Object> exports) {
  ImageWrapper::Init(exports);
  ImportanceMapWrapper::Init(exports);
  LayerRef::Init(exports);
  CompositorWrapper::Init(exports);
  ContextWrapper::Init(exports);
  ModelWrapper::Init(exports);
  UISliderWrapper::Init(exports);
  UIMetaSliderWrapper::Init(exports);
  UISamplerWrapper::Init(exports);
  ClickMapWrapper::Init(exports);
  exports->Set(Nan::New("log").ToLocalChecked(), Nan::New<v8::Function>(log));
  exports->Set(Nan::New("setLogLocation").ToLocalChecked(), Nan::New<v8::Function>(setLogLocation));
  exports->Set(Nan::New("setLogLevel").ToLocalChecked(), Nan::New<v8::Function>(setLogLevel));
  exports->Set(Nan::New("hardware_concurrency").ToLocalChecked(), Nan::New<v8::Function>(hardware_concurrency));
  exports->Set(Nan::New("runTest").ToLocalChecked(), Nan::New<v8::Function>(runTest));
  exports->Set(Nan::New("runAllTest").ToLocalChecked(), Nan::New<v8::Function>(runAllTest));
}

NODE_MODULE(Compositor, InitAll)
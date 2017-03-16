#include <nan.h>
#include "CompositorNode.h"

void InitAll(v8::Local<v8::Object> exports) {
  ImageWrapper::Init(exports);
  LayerRef::Init(exports);
  CompositorWrapper::Init(exports);
  ContextWrapper::Init(exports);
  exports->Set(Nan::New("log").ToLocalChecked(), Nan::New<v8::Function>(log));
  exports->Set(Nan::New("setLogLocation").ToLocalChecked(), Nan::New<v8::Function>(setLogLocation));
  exports->Set(Nan::New("setLogLevel").ToLocalChecked(), Nan::New<v8::Function>(setLogLevel));
}

NODE_MODULE(Compositor, InitAll)
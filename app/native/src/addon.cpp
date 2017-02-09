#include <nan.h>
#include "CompositorNode.h"

void InitAll(v8::Local<v8::Object> exports) {
  ImageWrapper::Init(exports);
}

NODE_MODULE(Compositor, InitAll)
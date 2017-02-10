#include "CompositorNode.h"

// general bindings

void log(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // expects two args: str, level
  if (info.Length() < 2) {
    Nan::ThrowError("log function expects two arguments: (string, int32)");
  }

  // check types
  if (!info[0]->IsString() || !info[1]->IsInt32()) {
    Nan::ThrowError("log function argument type error. Expected (string, int32).");
  }

  // do the thing
  v8::String::Utf8Value val0(info[0]->ToString());
  string msg(*val0);

  Comp::getLogger()->log(msg, (Comp::LogLevel)info[1]->Int32Value());

  info.GetReturnValue().SetNull();
}

void setLogLocation(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // expects 1 arg: str
  if (info.Length() < 1) {
    Nan::ThrowError("setLogLocation function expects one argument: string");
  }

  // check types
  if (!info[0]->IsString()) {
    Nan::ThrowError("setLogLocation function type error. Expected string.");
  }

  // do the thing
  v8::String::Utf8Value val0(info[0]->ToString());
  string path(*val0);

  Comp::getLogger()->setLogLocation(path);

  info.GetReturnValue().SetNull();
}

void setLogLevel(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // expects 1 arg: str
  if (info.Length() < 1) {
    Nan::ThrowError("setLogLevel function expects one argument: int");
  }

  // check types
  if (!info[0]->IsInt32()) {
    Nan::ThrowError("setLogLevel function type error. Expected int.");
  }

  // do the thing
  Comp::getLogger()->setLogLevel((Comp::LogLevel)info[0]->Int32Value());

  info.GetReturnValue().SetNull();
}


// object bindings

Nan::Persistent<v8::Function> ImageWrapper::constructor;

void ImageWrapper::Init(v8::Local<v8::Object> exports)
{
  Nan::HandleScope scope;

  // constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Image").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "getData", getData);
  Nan::SetPrototypeMethod(tpl, "getBase64", getBase64);
  Nan::SetPrototypeMethod(tpl, "width", width);
  Nan::SetPrototypeMethod(tpl, "height", height);

  constructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("Image").ToLocalChecked(), tpl->GetFunction());
}

ImageWrapper::ImageWrapper(unsigned int w, unsigned int h) {
  _image = new Comp::Image(w, h);
}

ImageWrapper::ImageWrapper(string filename) {
  _image = new Comp::Image(filename);
}

ImageWrapper::~ImageWrapper() {
  delete _image;
}

void ImageWrapper::New(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* img;

  if (info[0]->IsNumber()) {
    unsigned int w = info[0]->Uint32Value();
    unsigned int h = info[1]->IsUndefined() ? 0 : info[1]->Uint32Value();
    img = new ImageWrapper(w, h);
  }
  else if (info[0]->IsString()) {
    v8::String::Utf8Value val0(info[0]->ToString());
    string path(*val0);
    img = new ImageWrapper(path);
  }
  else {
    img = new ImageWrapper();
  }

  img->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

void ImageWrapper::getData(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  v8::Local<v8::Array> ret = Nan::New<v8::Array>();
  auto data = image->_image->getData();
  for (int i = 0; i < data.size(); i++) {
    ret->Set(i, Nan::New(data[i]));
  }

  info.GetReturnValue().Set(ret);
}
void ImageWrapper::getBase64(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());
  info.GetReturnValue().Set(Nan::New(image->_image->getBase64()).ToLocalChecked());
}

void ImageWrapper::width(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());
  info.GetReturnValue().Set(Nan::New(image->_image->getWidth()));
}

void ImageWrapper::height(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());
  info.GetReturnValue().Set(Nan::New(image->_image->getHeight()));
}
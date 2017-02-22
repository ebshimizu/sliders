#include "CompositorNode.h"

// general bindings

inline void nullcheck(void * ptr, string caller)
{
  if (ptr == nullptr) {
    Nan::ThrowError(string("Null pointer exception in " + caller).c_str());
  }
}

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

Nan::Persistent<v8::Function> ImageWrapper::imageConstructor;
Nan::Persistent<v8::Function> LayerRef::layerConstructor;
Nan::Persistent<v8::Function> CompositorWrapper::compositorConstructor;

void ImageWrapper::Init(v8::Local<v8::Object> exports)
{
  Nan::HandleScope scope;

  // constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Image").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "data", getData);
  Nan::SetPrototypeMethod(tpl, "base64", getBase64);
  Nan::SetPrototypeMethod(tpl, "width", width);
  Nan::SetPrototypeMethod(tpl, "height", height);
  Nan::SetPrototypeMethod(tpl, "save", save);

  imageConstructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("Image").ToLocalChecked(), tpl->GetFunction());
}

ImageWrapper::ImageWrapper(unsigned int w, unsigned int h) {
  _image = new Comp::Image(w, h);
}

ImageWrapper::ImageWrapper(string filename) {
  _image = new Comp::Image(filename);
}

ImageWrapper::ImageWrapper(Comp::Image * img)
{
  _image = img;
}

ImageWrapper::~ImageWrapper() {
  if (_deleteOnDestruct) {
    delete _image;
  }
}

void ImageWrapper::New(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* img;

  if (info[0]->IsNumber()) {
    unsigned int w = info[0]->Uint32Value();
    unsigned int h = info[1]->IsUndefined() ? 0 : info[1]->Uint32Value();
    img = new ImageWrapper(w, h);
    img->_deleteOnDestruct = true;
  }
  else if (info[0]->IsString()) {
    v8::String::Utf8Value val0(info[0]->ToString());
    string path(*val0);
    img = new ImageWrapper(path);
    img->_deleteOnDestruct = true;
  }
  else if (info[0]->IsExternal()) {
    Comp::Image* i = static_cast<Comp::Image*>(info[0].As<v8::External>()->Value());
    img = new ImageWrapper(i);
    img->_deleteOnDestruct = info[1]->BooleanValue();
  }
  else {
    img = new ImageWrapper();
    img->_deleteOnDestruct = true;
  }

  img->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

void ImageWrapper::getData(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());
  nullcheck(image->_image, "image.data");

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
  nullcheck(image->_image, "image.base64");

  info.GetReturnValue().Set(Nan::New(image->_image->getBase64()).ToLocalChecked());
}

void ImageWrapper::width(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());
  nullcheck(image->_image, "image.width");

  info.GetReturnValue().Set(Nan::New(image->_image->getWidth()));
}

void ImageWrapper::height(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());
  nullcheck(image->_image, "image.height");

  info.GetReturnValue().Set(Nan::New(image->_image->getHeight()));
}

void ImageWrapper::save(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());
  nullcheck(image->_image, "image.save");

  v8::String::Utf8Value val0(info[0]->ToString());
  string file(*val0);

  image->_image->save(file);
  
  info.GetReturnValue().Set(Nan::Null());
}

void LayerRef::Init(v8::Local<v8::Object> exports)
{
  Nan::HandleScope scope;

  // constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Layer").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "width", width);
  Nan::SetPrototypeMethod(tpl, "height", height);
  Nan::SetPrototypeMethod(tpl, "Image", image);
  Nan::SetPrototypeMethod(tpl, "reset", reset);
  Nan::SetPrototypeMethod(tpl, "visible", visible);
  Nan::SetPrototypeMethod(tpl, "opacity", opacity);
  Nan::SetPrototypeMethod(tpl, "blendMode", blendMode);
  Nan::SetPrototypeMethod(tpl, "name", name);

  layerConstructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("Layer").ToLocalChecked(), tpl->GetFunction());
}

LayerRef::LayerRef(Comp::Layer * src)
{
  _layer = src;
}

LayerRef::~LayerRef()
{
}

void LayerRef::New(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  if (!info[0]->IsExternal()) {
    Nan::ThrowError("Internal Error: Layer pointer not found as first argument of LayerRef constructor.");
  }

  Comp::Layer* layer = static_cast<Comp::Layer*>(info[0].As<v8::External>()->Value());

  LayerRef* lr = new LayerRef(layer);
  lr->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

void LayerRef::width(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.width");

  info.GetReturnValue().Set(Nan::New(layer->_layer->getWidth()));
}

void LayerRef::height(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.height");

  info.GetReturnValue().Set(Nan::New(layer->_layer->getHeight()));
}

void LayerRef::image(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // right now this just returns the base64 string for the image
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.image");

  info.GetReturnValue().Set(Nan::New(layer->_layer->getImage()->getBase64()).ToLocalChecked());
}

void LayerRef::reset(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.reset");

  layer->_layer->reset();
  info.GetReturnValue().Set(Nan::Null());
}

void LayerRef::visible(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.visible");
  
  if (info[0]->IsBoolean()) {
    layer->_layer->_visible = info[0]->BooleanValue();
  }

  info.GetReturnValue().Set(Nan::New(layer->_layer->_visible));
}

void LayerRef::opacity(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.opacity");

  if (info[0]->IsNumber()) {
    layer->_layer->setOpacity(info[0]->NumberValue());
  }

  info.GetReturnValue().Set(Nan::New(layer->_layer->getOpacity()));
}

void LayerRef::blendMode(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.blendMode");

  if (info[0]->IsInt32()) {
    layer->_layer->_mode = (Comp::BlendMode)(info[0]->Int32Value());
  }

  info.GetReturnValue().Set(Nan::New(layer->_layer->_mode));
}

void LayerRef::name(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.name");

  info.GetReturnValue().Set(Nan::New(layer->_layer->getName()).ToLocalChecked());
}

void CompositorWrapper::Init(v8::Local<v8::Object> exports)
{
  Nan::HandleScope scope;

  // constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Compositor").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "getLayer", getLayer);
  Nan::SetPrototypeMethod(tpl, "addLayer", addLayer);
  Nan::SetPrototypeMethod(tpl, "copyLayer", copyLayer);
  Nan::SetPrototypeMethod(tpl, "deleteLayer", deleteLayer);
  Nan::SetPrototypeMethod(tpl, "getAllLayers", getAllLayers);
  Nan::SetPrototypeMethod(tpl, "setLayerOrder", setLayerOrder);
  Nan::SetPrototypeMethod(tpl, "getLayerNames", getLayerNames);
  Nan::SetPrototypeMethod(tpl, "size", size);
  Nan::SetPrototypeMethod(tpl, "render", render);

  compositorConstructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("Compositor").ToLocalChecked(), tpl->GetFunction());
}

CompositorWrapper::CompositorWrapper()
{
}

CompositorWrapper::~CompositorWrapper()
{
  delete _compositor;
}

void CompositorWrapper::New(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // no args expected, anything passed in is ignored
  CompositorWrapper* cw = new CompositorWrapper();
  cw->_compositor = new Comp::Compositor();

  cw->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

void CompositorWrapper::getLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;

  if (!info[0]->IsString()) {
    Nan::ThrowError("getLayer expects a layer name");
  }

  // expects a name
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getLayer");

  v8::String::Utf8Value val0(info[0]->ToString());
  string name(*val0);

  Comp::Layer& l = c->_compositor->getLayer(name);

  v8::Local<v8::Function> cons = Nan::New<v8::Function>(LayerRef::layerConstructor);
  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&l) };

  info.GetReturnValue().Set(cons->NewInstance(argc, argv));
}

void CompositorWrapper::addLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  if (!info[0]->IsString() || !info[1]->IsString()) {
    Nan::ThrowError("addLayer expects a layer name and a filepath");
  }

  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.addLayer");

  v8::String::Utf8Value val0(info[0]->ToString());
  string name(*val0);

  v8::String::Utf8Value val1(info[1]->ToString());
  string path(*val1);

  bool result = c->_compositor->addLayer(name, path);

  info.GetReturnValue().Set(Nan::New(result));
}

void CompositorWrapper::copyLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  if (!info[0]->IsString() || !info[1]->IsString()) {
    Nan::ThrowError("copyLayer expects two layer names");
  }

  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.copyLayer");

  v8::String::Utf8Value val0(info[0]->ToString());
  string name(*val0);

  v8::String::Utf8Value val1(info[1]->ToString());
  string dest(*val1);

  bool result = c->_compositor->copyLayer(name, dest);

  info.GetReturnValue().Set(Nan::New(result));
}

void CompositorWrapper::deleteLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  if (!info[0]->IsString()) {
    Nan::ThrowError("deleteLayer expects a layer name");
  }

  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.deleteLayer");

  v8::String::Utf8Value val0(info[0]->ToString());
  string name(*val0);

  bool result = c->_compositor->deleteLayer(name);

  info.GetReturnValue().Set(Nan::New(result));
}

void CompositorWrapper::getAllLayers(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getAllLayers");

  v8::Local<v8::Function> cons = Nan::New<v8::Function>(LayerRef::layerConstructor);

  // extract all layers into an object (map) and return that
  v8::Local<v8::Object> layers = Nan::New<v8::Object>();
  for (int i = 0; i < c->_compositor->size(); i++) {
    Comp::Layer& l = c->_compositor->getLayer(i);

    // create v8 object
    v8::Local<v8::Object> layer = Nan::New<v8::Object>();
    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&l) };

    layers->Set(Nan::New(l.getName()).ToLocalChecked(), cons->NewInstance(argc, argv));
  }

  info.GetReturnValue().Set(layers);
}

void CompositorWrapper::setLayerOrder(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.setLayerOrder");

  // unpack the array
  if (!info[0]->IsArray()) {
    Nan::ThrowError("Set Layer Order expects an array");
  }

  vector<string> names;
  v8::Local<v8::Array> args = info[0].As<v8::Array>();
  for (unsigned int i = 0; i < args->Length(); i++) {
    v8::String::Utf8Value val0(args->Get(i)->ToString());
    string name(*val0);
    names.push_back(name);
  }

  // the compositor does all the checks, we just report the result here
  bool result = c->_compositor->setLayerOrder(names);

  info.GetReturnValue().Set(Nan::New(result));
}

void CompositorWrapper::getLayerNames(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getLayerNames");

  // dump names into v8 array
  v8::Local<v8::Array> layers = Nan::New<v8::Array>();
  auto layerData = c->_compositor->getLayerOrder();
  int i = 0;
  for (auto id : layerData) {
    layers->Set(i, Nan::New(id).ToLocalChecked());
    i++;
  }

  info.GetReturnValue().Set(layers);
}

void CompositorWrapper::size(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.size");

  info.GetReturnValue().Set(Nan::New(c->_compositor->size()));
}

void CompositorWrapper::render(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.render");

  Comp::Image* img;

  if (info[0]->IsString()) {
    v8::String::Utf8Value val0(info[0]->ToString());
    string size(*val0);
    img = c->_compositor->render(size);
  }
  else {
    img = c->_compositor->render();
  }

  // construct the image
  v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImageWrapper::imageConstructor);
  const int argc = 2;
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(img), Nan::New(true) };

  info.GetReturnValue().Set(cons->NewInstance(argc, argv));
}

void CompositorWrapper::getCacheSizes(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getCacheSizes");

  // dump names into v8 array
  v8::Local<v8::Array> layers = Nan::New<v8::Array>();
  auto cacheSizes = c->_compositor->getCacheSizes();
  int i = 0;
  for (auto id : cacheSizes) {
    layers->Set(i, Nan::New(id).ToLocalChecked());
    i++;
  }

  info.GetReturnValue().Set(layers);
}

void CompositorWrapper::addCacheSize(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.addCacheSize");

  if (!info[0]->IsString() || !info[1]->IsNumber()) {
    Nan::ThrowError("addCacheSize expects (string, float)");
  }

  v8::String::Utf8Value val0(info[0]->ToString());
  string size(*val0);
  float scale = info[1]->NumberValue();

  info.GetReturnValue().Set(Nan::New(c->_compositor->addCacheSize(size, scale)));
}

void CompositorWrapper::deleteCacheSize(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.deleteCacheSize");

  if (!info[0]->IsString()) {
    Nan::ThrowError("deleteCacheSize expects (string)");
  }

  v8::String::Utf8Value val0(info[0]->ToString());
  string size(*val0);

  info.GetReturnValue().Set(Nan::New(c->_compositor->deleteCacheSize(size)));
}

void CompositorWrapper::getCachedImage(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getCachedImage");

  if (!info[0]->IsString() || !info[1]->IsString()) {
    Nan::ThrowError("getCachedImage expects (string, string)");
  }

  v8::String::Utf8Value val0(info[0]->ToString());
  string id(*val0);

  v8::String::Utf8Value val1(info[1]->ToString());
  string size(*val1);

  shared_ptr<Comp::Image> img = c->_compositor->getCachedImage(id, size);

  v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImageWrapper::imageConstructor);
  const int argc = 2;

  // this is a shared ptr, so we really do not want to delete the data when the v8 image
  // object goes bye bye
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(img.get()), Nan::New(false) };

  info.GetReturnValue().Set(cons->NewInstance(argc, argv));
}

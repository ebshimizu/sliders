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
  Nan::SetPrototypeMethod(tpl, "image", image);
  Nan::SetPrototypeMethod(tpl, "reset", reset);
  Nan::SetPrototypeMethod(tpl, "visible", visible);
  Nan::SetPrototypeMethod(tpl, "opacity", opacity);
  Nan::SetPrototypeMethod(tpl, "blendMode", blendMode);
  Nan::SetPrototypeMethod(tpl, "name", name);
  Nan::SetPrototypeMethod(tpl, "getAdjustment", getAdjustment);
  Nan::SetPrototypeMethod(tpl, "deleteAdjustment", deleteAdjustment);
  Nan::SetPrototypeMethod(tpl, "deleteAllAdjustments", deleteAllAdjustments);
  Nan::SetPrototypeMethod(tpl, "getAdjustments", getAdjustments);
  Nan::SetPrototypeMethod(tpl, "addAdjustment", addAdjustment);
  Nan::SetPrototypeMethod(tpl, "addHSLAdjustment", addHSLAdjustment);
  Nan::SetPrototypeMethod(tpl, "addLevelsAdjustment", addLevelsAdjustment);
  Nan::SetPrototypeMethod(tpl, "addCurve", addCurvesChannel);
  Nan::SetPrototypeMethod(tpl, "deleteCurve", deleteCurvesChannel);
  Nan::SetPrototypeMethod(tpl, "evalCurve", evalCurve);
  Nan::SetPrototypeMethod(tpl, "getCurve", getCurve);
  Nan::SetPrototypeMethod(tpl, "addExposureAdjustment", addExposureAdjustment);
  Nan::SetPrototypeMethod(tpl, "addGradient", addGradient);
  Nan::SetPrototypeMethod(tpl, "evalGradient", evalGradient);
  Nan::SetPrototypeMethod(tpl, "selectiveColor", selectiveColor);
  Nan::SetPrototypeMethod(tpl, "colorBalance", colorBalance);
  Nan::SetPrototypeMethod(tpl, "photoFilter", addPhotoFilter);
  Nan::SetPrototypeMethod(tpl, "colorize", colorize);
  Nan::SetPrototypeMethod(tpl, "lighterColorize", lighterColorize);

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
    layer->_layer->setOpacity((float)info[0]->NumberValue());
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

void LayerRef::getAdjustment(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.getAdjustment");

  if (!info[0]->IsInt32()) {
    Nan::ThrowError("getAdjustment expects (int)");
  }

  map<string, float> adj = layer->_layer->getAdjustment((Comp::AdjustmentType)(info[0]->Int32Value()));

  // extract all layers into an object (map) and return that
  v8::Local<v8::Object> adjustment = Nan::New<v8::Object>();
  int i = 0;

  for (auto kvp : adj) {
    // create v8 object
    adjustment->Set(Nan::New(kvp.first).ToLocalChecked(), Nan::New(kvp.second));
  }

  info.GetReturnValue().Set(adjustment);
}

void LayerRef::deleteAdjustment(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.deleteAdjustment");

  if (!info[0]->IsInt32()) {
    Nan::ThrowError("deleteAdjustment expects (int)");
  }

  layer->_layer->deleteAdjustment((Comp::AdjustmentType)(info[0]->Int32Value()));
}

void LayerRef::deleteAllAdjustments(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.deleteAllAdjustments");

  layer->_layer->deleteAllAdjustments();
}

void LayerRef::getAdjustments(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.getAdjustments");

  vector<Comp::AdjustmentType> types = layer->_layer->getAdjustments();
  v8::Local<v8::Array> ret = Nan::New<v8::Array>();
  int i = 0;
  for (auto t : types) {
    ret->Set(i, Nan::New((int)t));
    i++;
  }

  info.GetReturnValue().Set(ret);
}

void LayerRef::addAdjustment(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.addAdjustment");

  if (!info[0]->IsInt32() || !info[1]->IsString() || !info[2]->IsNumber()) {
    Nan::ThrowError("addAdjustment expects (int, string, float)");
  }

  Comp::AdjustmentType type = (Comp::AdjustmentType)info[0]->Int32Value();
  v8::String::Utf8Value val1(info[1]->ToString());
  string param(*val1);

  layer->_layer->addAdjustment(type, param, (float)info[2]->NumberValue());
}

void LayerRef::addHSLAdjustment(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.addHSLAdjustment");

  if (!info[0]->IsNumber() || !info[1]->IsNumber() || !info[2]->IsNumber()) {
    Nan::ThrowError("addHSLAdjustment expects (float, float, float)");
  }

  layer->_layer->addHSLAdjustment((float)info[0]->NumberValue(), (float)info[1]->NumberValue(), (float)info[2]->NumberValue());
}

void LayerRef::addLevelsAdjustment(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.addHSLAdjustment");

  if (!info[0]->IsNumber() || !info[1]->IsNumber()) {
    Nan::ThrowError("addHSLAdjustment expects (float, float, [float, float, float])");
  }

  float inMin = (float)info[0]->NumberValue();
  float inMax = (float)info[1]->NumberValue();
  float gamma = (info[2]->IsNumber()) ? (float)info[2]->NumberValue() : 1.0f;
  float outMin = (info[3]->IsNumber()) ? (float)info[3]->NumberValue() : 0;
  float outMax = (info[4]->IsNumber()) ? (float)info[4]->NumberValue() : 255.0f;

  layer->_layer->addLevelsAdjustment(inMin, inMax, gamma, outMin, outMax);
}

void LayerRef::addCurvesChannel(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.addCurvesChannel");

  if (!info[0]->IsString() || !info[1]->IsArray()) {
    Nan::ThrowError("addCurvesChannel(string, object[]) invalid arguments.");
  }

  // get channel name
  v8::String::Utf8Value val0(info[0]->ToString());
  string channel(*val0);

  // get points, should be an array of objects {x: ###, y: ###}
  vector<Comp::Point> pts;
  v8::Local<v8::Array> args = info[1].As<v8::Array>();
  for (unsigned int i = 0; i < args->Length(); i++) {
    v8::Local<v8::Object> pt = args->Get(i).As<v8::Object>();
    pts.push_back(Comp::Point((float)pt->Get(Nan::New("x").ToLocalChecked())->NumberValue(), (float)pt->Get(Nan::New("y").ToLocalChecked())->NumberValue()));
  }

  Comp::Curve c(pts);
  layer->_layer->addCurvesChannel(channel, c);
}

void LayerRef::deleteCurvesChannel(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.deleteCurvesChannel");

  if (!info[0]->IsString()) {
    Nan::ThrowError("deleteCurvesChannel(string) invalid arguments.");
  }

  // get channel name
  v8::String::Utf8Value val0(info[0]->ToString());
  string channel(*val0);

  layer->_layer->deleteCurvesChannel(channel);
}

void LayerRef::evalCurve(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.evalCurve");

  if (!info[0]->IsString() || !info[1]->IsNumber()) {
    Nan::ThrowError("evalCurve(string, float) invalid arguments.");
  }

  v8::String::Utf8Value val0(info[0]->ToString());
  string channel(*val0);

  info.GetReturnValue().Set(Nan::New(layer->_layer->evalCurve(channel, (float)info[1]->NumberValue())));
}

void LayerRef::getCurve(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.getCurve");

  if (!info[0]->IsString()) {
    Nan::ThrowError("getCurve(string) invalid arguments.");
  }

  // get channel name
  v8::String::Utf8Value val0(info[0]->ToString());
  string channel(*val0);

  Comp::Curve c = layer->_layer->getCurveChannel(channel);

  // create object
  v8::Local<v8::Array> pts = Nan::New<v8::Array>();
  for (int i = 0; i < c._pts.size(); i++) {
    Comp::Point pt = c._pts[i];

    // create v8 object
    v8::Local<v8::Object> point = Nan::New<v8::Object>();
    point->Set(Nan::New("x").ToLocalChecked(), Nan::New(pt._x));
    point->Set(Nan::New("y").ToLocalChecked(), Nan::New(pt._y));

    pts->Set(Nan::New(i), point);
  }

  info.GetReturnValue().Set(pts);
}

void LayerRef::addExposureAdjustment(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.getCurve");

  if (!info[0]->IsNumber() || !info[1]->IsNumber() || !info[2]->IsNumber()) {
    Nan::ThrowError("addExposureAdjustment(float, float, float) invalid arguments.");
  }

  layer->_layer->addExposureAdjustment((float)info[0]->NumberValue(), (float)info[1]->NumberValue(), (float)info[2]->NumberValue());
}

void LayerRef::addGradient(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.addGradient");

  if (!info[0]->IsArray() || !info[1]->IsArray()) {
    Nan::ThrowError("addGradient(float[], object[]) invalid arguments.");
  }

  // get point locations, just a plain array
  vector<float> pts;
  v8::Local<v8::Array> args = info[0].As<v8::Array>();
  for (unsigned int i = 0; i < args->Length(); i++) {
    pts.push_back((float)args->Get(i)->NumberValue());
  }

  // get colors, an array of objects {r, g, b}
  vector<Comp::RGBColor> colors;
  v8::Local<v8::Array> c = info[1].As<v8::Array>();
  for (unsigned int i = 0; i < c->Length(); i++) {
    v8::Local<v8::Object> co = c->Get(i).As<v8::Object>();
    Comp::RGBColor color;
    color._r = (float)co->Get(Nan::New("r").ToLocalChecked())->NumberValue();
    color._g = (float)co->Get(Nan::New("g").ToLocalChecked())->NumberValue();
    color._b = (float)co->Get(Nan::New("b").ToLocalChecked())->NumberValue();

    colors.push_back(color);
  }

  layer->_layer->addGradientAdjustment(Comp::Gradient(pts, colors));
}

void LayerRef::evalGradient(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.evalGradient");

  if (!info[0]->IsNumber()) {
    Nan::ThrowError("evalGradient(float) invalid arguments.");
  }

  Comp::RGBColor res = layer->_layer->evalGradient((float)info[0]->NumberValue());
  v8::Local<v8::Object> rgb = Nan::New<v8::Object>();
  rgb->Set(Nan::New("r").ToLocalChecked(), Nan::New(res._r));
  rgb->Set(Nan::New("g").ToLocalChecked(), Nan::New(res._g));
  rgb->Set(Nan::New("b").ToLocalChecked(), Nan::New(res._b));

  info.GetReturnValue().Set(rgb);
}

void LayerRef::selectiveColor(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.selectiveColor");

  if (info.Length() == 0) {
    // if no args, get state
    map<string, map<string, float> > sc = layer->_layer->getSelectiveColor();

    // put into object
    v8::Local<v8::Object> ret = Nan::New<v8::Object>();
    for (auto c : sc) {
      v8::Local<v8::Object> color = Nan::New<v8::Object>();
      for (auto cp : c.second) {
        color->Set(Nan::New(cp.first).ToLocalChecked(), Nan::New(cp.second));
      }
      ret->Set(Nan::New(c.first).ToLocalChecked(), color);
    }

    ret->Set(Nan::New("relative").ToLocalChecked(), Nan::New(layer->_layer->getAdjustment(Comp::AdjustmentType::SELECTIVE_COLOR)["relative"]));
    info.GetReturnValue().Set(ret);
  }
  else {
    // otherwise set the object passed in
    map<string, map<string, float> > sc;

    if (!info[0]->IsBoolean() || !info[1]->IsObject()) {
      Nan::ThrowError("selectiveColor(bool, object) argument error");
    }

    v8::Local<v8::Object> ret = info[1].As<v8::Object>();
    auto names = ret->GetOwnPropertyNames();
    for (unsigned int i = 0; i < names->Length(); i++) {
      v8::Local<v8::Object> color = ret->Get(names->Get(i)).As<v8::Object>();
      v8::Local<v8::Array> colorNames = color->GetOwnPropertyNames();

      v8::String::Utf8Value o1(names->Get(i)->ToString());
      string group(*o1);

      for (unsigned int j = 0; j < colorNames->Length(); j++) {
        v8::Local<v8::Value> colorVal = color->Get(colorNames->Get(j));

        v8::String::Utf8Value o2(colorNames->Get(j)->ToString());
        string propName(*o2);

        sc[group][propName] = (float)colorVal->NumberValue();
      }
    }

    layer->_layer->addSelectiveColorAdjustment(info[0]->BooleanValue(), sc);
  }
}

void LayerRef::colorBalance(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.colorBalance");

  if (info.Length() == 0) {
    // return status
    v8::Local<v8::Object> ret = Nan::New<v8::Object>();

    map<string, float> adj = layer->_layer->getAdjustment(Comp::AdjustmentType::COLOR_BALANCE);

    v8::Local<v8::Object> shadow;
    shadow->Set(Nan::New("r").ToLocalChecked(), Nan::New(adj["shadowR"]));
    shadow->Set(Nan::New("g").ToLocalChecked(), Nan::New(adj["shadowG"]));
    shadow->Set(Nan::New("b").ToLocalChecked(), Nan::New(adj["shadowB"]));
    ret->Set(Nan::New("shadow").ToLocalChecked(), shadow);

    v8::Local<v8::Object> mid;
    mid->Set(Nan::New("r").ToLocalChecked(), Nan::New(adj["midR"]));
    mid->Set(Nan::New("g").ToLocalChecked(), Nan::New(adj["midG"]));
    mid->Set(Nan::New("b").ToLocalChecked(), Nan::New(adj["midB"]));
    ret->Set(Nan::New("mid").ToLocalChecked(), mid);

    v8::Local<v8::Object> high;
    high->Set(Nan::New("r").ToLocalChecked(), Nan::New(adj["highR"]));
    high->Set(Nan::New("g").ToLocalChecked(), Nan::New(adj["highG"]));
    high->Set(Nan::New("b").ToLocalChecked(), Nan::New(adj["highB"]));
    ret->Set(Nan::New("high").ToLocalChecked(), high);

    ret->Set(Nan::New("preserveLuma").ToLocalChecked(), Nan::New(adj["preserveLuma"]));

    info.GetReturnValue().Set(ret);
  }
  else {
    // set object
    // so there have to be 10 arguments and all numeric except for the first one
    if (info.Length() != 10 || !info[0]->IsBoolean()) {
      Nan::ThrowError("colorBalance(bool, float...(x9)) argument error");
    }

    for (int i = 1; i < info.Length(); i++) {
      if (!info[i]->IsNumber())
      Nan::ThrowError("colorBalance(bool, float...(x9)) argument error");
    }

    // just call the function
    layer->_layer->addColorBalanceAdjustment(info[0]->BooleanValue(), (float)info[1]->NumberValue(), (float)info[2]->NumberValue(), (float)info[3]->NumberValue(),
      (float)info[4]->NumberValue(), (float)info[5]->NumberValue(), (float)info[6]->NumberValue(),
      (float)info[7]->NumberValue(), (float)info[8]->NumberValue(), (float)info[9]->NumberValue());
  }
}

void LayerRef::addPhotoFilter(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // this one sucks a bit because photoshop is inconsistent with how it exports. We expect
  // and object here and based on the existence of particular keys need to process color
  // differently

  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.addPhotoFilter");

  if (!info[0]->IsObject()) {
    Nan::ThrowError("addPhotoFilter(object) argument error.");
  }

  v8::Local<v8::Object> color = info[0].As<v8::Object>();
  bool preserveLuma = color->Get(Nan::New("preserveLuma").ToLocalChecked())->BooleanValue();
  float density = (float)color->Get(Nan::New("density").ToLocalChecked())->NumberValue();
  
  if (color->Has(Nan::New("luminance").ToLocalChecked())) {
    // convert from lab to rgb
    Comp::RGBColor rgb = Comp::LabToRGB(color->Get(Nan::New("luminance").ToLocalChecked())->NumberValue(),
      color->Get(Nan::New("a").ToLocalChecked())->NumberValue(),
      color->Get(Nan::New("b").ToLocalChecked())->NumberValue());

    layer->_layer->addPhotoFilterAdjustment(preserveLuma, rgb._r, rgb._g, rgb._b, density);
  }
  else if (color->Has(Nan::New("hue").ToLocalChecked())) {
    // convert from hsl to rgb
    Comp::RGBColor rgb = Comp::HSLToRGB(color->Get(Nan::New("hue").ToLocalChecked())->NumberValue(),
      color->Get(Nan::New("saturation").ToLocalChecked())->NumberValue(),
      color->Get(Nan::New("brightness").ToLocalChecked())->NumberValue());

    layer->_layer->addPhotoFilterAdjustment(preserveLuma, rgb._r, rgb._g, rgb._b, density);
  }
  else if (color->Has(Nan::New("r").ToLocalChecked())) {
    // just do the thing

    layer->_layer->addPhotoFilterAdjustment(preserveLuma, color->Get(Nan::New("r").ToLocalChecked())->NumberValue(), 
      color->Get(Nan::New("g").ToLocalChecked())->NumberValue(),
      color->Get(Nan::New("b").ToLocalChecked())->NumberValue(), 
      density);
  }
  else {
    Nan::ThrowError("addPhotoFilter object arg not in recognized color format");
  }
}

void LayerRef::colorize(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // check if getting status or not
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.colorize");

  if (info.Length() == 0) {
    // status
    v8::Local<v8::Object> ret = Nan::New<v8::Object>();

    map<string, float> adj = layer->_layer->getAdjustment(Comp::AdjustmentType::COLORIZE);
    ret->Set(Nan::New("r").ToLocalChecked(), Nan::New(adj["r"]));
    ret->Set(Nan::New("g").ToLocalChecked(), Nan::New(adj["g"]));
    ret->Set(Nan::New("b").ToLocalChecked(), Nan::New(adj["b"]));
    ret->Set(Nan::New("a").ToLocalChecked(), Nan::New(adj["a"]));

    info.GetReturnValue().Set(ret);
  }
  else {
    if (!info[0]->IsNumber() || !info[1]->IsNumber() || !info[2]->IsNumber() || !info[3]->IsNumber()) {
      Nan::ThrowError("colorize(float, float, float, float) argument error.");
    }

    // set value
    layer->_layer->addColorAdjustment((float)info[0]->NumberValue(), (float)info[1]->NumberValue(),
      (float)info[2]->NumberValue(), (float)info[3]->NumberValue());
  }
}

void LayerRef::lighterColorize(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // check if getting status or not
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.lighterColorize");

  if (info.Length() == 0) {
    // status
    v8::Local<v8::Object> ret = Nan::New<v8::Object>();

    map<string, float> adj = layer->_layer->getAdjustment(Comp::AdjustmentType::LIGHTER_COLORIZE);
    ret->Set(Nan::New("r").ToLocalChecked(), Nan::New(adj["r"]));
    ret->Set(Nan::New("g").ToLocalChecked(), Nan::New(adj["g"]));
    ret->Set(Nan::New("b").ToLocalChecked(), Nan::New(adj["b"]));
    ret->Set(Nan::New("a").ToLocalChecked(), Nan::New(adj["a"]));

    info.GetReturnValue().Set(ret);
  }
  else {
    if (!info[0]->IsNumber() || !info[1]->IsNumber() || !info[2]->IsNumber() || !info[3]->IsNumber()) {
      Nan::ThrowError("lighterColorize(float, float, float, float) argument error.");
    }

    // set value
    layer->_layer->addLighterColorAdjustment((float)info[0]->NumberValue(), (float)info[1]->NumberValue(),
      (float)info[2]->NumberValue(), (float)info[3]->NumberValue());
  }
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
  Nan::SetPrototypeMethod(tpl, "getCacheSizes", getCacheSizes);
  Nan::SetPrototypeMethod(tpl, "addCacheSize", addCacheSize);
  Nan::SetPrototypeMethod(tpl, "deleteCacheSize", deleteCacheSize);
  Nan::SetPrototypeMethod(tpl, "getCachedImage", getCachedImage);
  Nan::SetPrototypeMethod(tpl, "reorderLayer", reorderLayer);

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
  if (!info[0]->IsString()) {
    Nan::ThrowError("addLayer expects (string, string [opt])");
  }

  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.addLayer");

  bool result;
  v8::String::Utf8Value val0(info[0]->ToString());
  string name(*val0);

  if (info.Length() == 2) {
    v8::String::Utf8Value val1(info[1]->ToString());
    string path(*val1);

    result = c->_compositor->addLayer(name, path);
  }
  else {
    // adjustment layer
    result = c->_compositor->addAdjustmentLayer(name);
  }

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
  float scale = (float)info[1]->NumberValue();

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

void CompositorWrapper::reorderLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.reorderLayer");

  if (!info[0]->IsInt32() || !info[1]->IsInt32()) {
    Nan::ThrowError("reorderLayer expects (int, int)");
  }

  c->_compositor->reorderLayer(info[0]->Int32Value(), info[1]->Int32Value());
}

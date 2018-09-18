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

void hardware_concurrency(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  info.GetReturnValue().Set(Nan::New(thread::hardware_concurrency()));
}

void runTest(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // get the compositor
  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty! (in runTest())");
  }
  CompositorWrapper* comp = Nan::ObjectWrap::Unwrap<CompositorWrapper>(maybe1.ToLocalChecked());

  int x, y;

  if (info[1]->IsInt32() && info[2]->IsInt32()) {
    x = info[1]->Int32Value();
    y = info[2]->Int32Value();
  }
  else {
    Nan::ThrowError("runTests needs an (x, y) pixel coordinate");
  }

  info.GetReturnValue().Set(Comp::compare(comp->_compositor, x, y));
}

void runAllTest(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // get the compositor
  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty! (in runTest())");
  }
  CompositorWrapper* comp = Nan::ObjectWrap::Unwrap<CompositorWrapper>(maybe1.ToLocalChecked());

  string output;

  if (info[1]->IsString()) {
    v8::String::Utf8Value val0(info[1]->ToString());
    output = string(*val0);
  }
  else {
    Nan::ThrowError("runAllTests needs an output filename");
  }

  Comp::compareAll(comp->_compositor, output);
}

void asyncSampleEvent(uv_work_t * req)
{
  // construct the proper objects and do the callback
  Nan::HandleScope scope;

  // this is in the node thread
  asyncSampleEventData* data = static_cast<asyncSampleEventData*>(req->data);

  // create objects
  // image object
  const int argc = 2;
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(data->img), Nan::New(true) };
  v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImageWrapper::imageConstructor);
  v8::Local<v8::Object> imgInst = Nan::NewInstance(cons, argc, argv).ToLocalChecked();

  // context object
  const int argc2 = 1;
  v8::Local<v8::Value> argv2[argc2] = { Nan::New<v8::External>(&(data->ctx)) };
  v8::Local<v8::Function> cons2 = Nan::New<v8::Function>(ContextWrapper::contextConstructor);
  v8::Local<v8::Object> ctxInst = Nan::NewInstance(cons2, argc2, argv2).ToLocalChecked();

  // metadata object
  v8::Local<v8::Object> metadata = Nan::New<v8::Object>();
  for (auto k : data->meta) {
    metadata->Set(Nan::New(k.first).ToLocalChecked(), Nan::New(k.second));
  }

  // string metadata
  for (auto k : data->meta2) {
    metadata->Set(Nan::New(k.first).ToLocalChecked(), Nan::New(k.second).ToLocalChecked());
  }

  // construct emitter objects
  v8::Local<v8::Value> emitArgv[] = { Nan::New("sample").ToLocalChecked(), imgInst, ctxInst, metadata };
  Nan::MakeCallback(data->c->handle(), "emit", 4, emitArgv);
}

void asyncNop(uv_work_t * req)
{
  // yup it does a nop
}

v8::Local<v8::Value> excGet(v8::Local<v8::Object>& obj, string key)
{
  if (Nan::HasOwnProperty(obj, Nan::New(key).ToLocalChecked()).ToChecked()) {
    return obj->Get(Nan::New(key).ToLocalChecked());
  }
  else {
    Nan::ThrowError(string("Object has no key " + key).c_str());
  }
}

// object bindings
Nan::Persistent<v8::Function> ImageWrapper::imageConstructor;
Nan::Persistent<v8::Function> ImportanceMapWrapper::importanceMapConstructor;
Nan::Persistent<v8::Function> LayerRef::layerConstructor;
Nan::Persistent<v8::Function> CompositorWrapper::compositorConstructor;
Nan::Persistent<v8::Function> ContextWrapper::contextConstructor;
Nan::Persistent<v8::Function> ModelWrapper::modelConstructor;
Nan::Persistent<v8::Function> UISliderWrapper::uiSliderConstructor;
Nan::Persistent<v8::Function> UIMetaSliderWrapper::uiMetaSliderConstructor;
Nan::Persistent<v8::Function> UISamplerWrapper::uiSamplerConstructor;
Nan::Persistent<v8::Function> ClickMapWrapper::clickMapConstructor;

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
  Nan::SetPrototypeMethod(tpl, "filename", filename);
  Nan::SetPrototypeMethod(tpl, "structDiff", structDiff);
  Nan::SetPrototypeMethod(tpl, "structIndBinDiff", structIndBinDiff);
  Nan::SetPrototypeMethod(tpl, "structAvgBinDiff", structAvgBinDiff);
  Nan::SetPrototypeMethod(tpl, "structTotalBinDiff", structTotalBinDiff);
  Nan::SetPrototypeMethod(tpl, "structBinDiff", structBinDiff);
  Nan::SetPrototypeMethod(tpl, "structPctDiff", structPctDiff);
  Nan::SetPrototypeMethod(tpl, "MSSIM", structSSIMDiff);
  Nan::SetPrototypeMethod(tpl, "stats", stats);
  Nan::SetPrototypeMethod(tpl, "diff", diff);
  Nan::SetPrototypeMethod(tpl, "writeToImageData", writeToImageData);
  Nan::SetPrototypeMethod(tpl, "fill", fill);
  Nan::SetPrototypeMethod(tpl, "histogramIntersection", histogramIntersect);
  Nan::SetPrototypeMethod(tpl, "proportionalHistogramIntersection", pctHistogramIntersect);
  Nan::SetPrototypeMethod(tpl, "chamferDistance", chamferDistance);

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

  auto data = image->_image->getData();
  v8::Local<v8::Uint8ClampedArray> ret = v8::Uint8ClampedArray::New(v8::ArrayBuffer::New(v8::Isolate::GetCurrent(), data.size()), 0, data.size());
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

void ImageWrapper::filename(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  if (image->_image == nullptr) {
    info.GetReturnValue().Set(Nan::New("").ToLocalChecked());
  }
  else {
    info.GetReturnValue().Set(Nan::New(image->_image->getFilename()).ToLocalChecked());
  }
}

void ImageWrapper::structDiff(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  if (!info[0]->IsObject()) {
    Nan::ThrowError("structDiff requires an image to compare to");
  }

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }

  ImageWrapper* y = Nan::ObjectWrap::Unwrap<ImageWrapper>(maybe1.ToLocalChecked());

  double error = image->_image->structDiff(y->_image);
  info.GetReturnValue().Set(Nan::New(error));
}

void ImageWrapper::structIndBinDiff(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  if (!info[0]->IsObject() || !info[1]->IsInt32()) {
    Nan::ThrowError("structIndBinDiff arugment error (Image, int)");
  }

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }

  ImageWrapper* y = Nan::ObjectWrap::Unwrap<ImageWrapper>(maybe1.ToLocalChecked());
  int patchSize = info[1]->Int32Value();

  vector<double> binScores = image->_image->structIndBinDiff(y->_image, patchSize);

  v8::Local<v8::Array> arr = Nan::New<v8::Array>();
  for (int i = 0; i < binScores.size(); i++) {
    arr->Set(Nan::New(i), Nan::New(binScores[i]));
  }
  info.GetReturnValue().Set(arr);
}

void ImageWrapper::structBinDiff(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  if (!info[0]->IsObject() || !info[1]->IsInt32() || !info[2]->IsNumber()) {
    Nan::ThrowError("structBinDiff arugment error (Image, int, double)");
  }

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }

  ImageWrapper* y = Nan::ObjectWrap::Unwrap<ImageWrapper>(maybe1.ToLocalChecked());
  int patchSize = info[1]->Int32Value();
  double threshold = info[2]->NumberValue();

  double error = image->_image->structBinDiff(y->_image, patchSize, threshold);
  info.GetReturnValue().Set(Nan::New(error));
}

void ImageWrapper::structTotalBinDiff(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  if (!info[0]->IsObject() || !info[1]->IsInt32()) {
    Nan::ThrowError("structTotalBinDiff arugment error (Image, int)");
  }

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }

  ImageWrapper* y = Nan::ObjectWrap::Unwrap<ImageWrapper>(maybe1.ToLocalChecked());
  int patchSize = info[1]->Int32Value();

  double error = image->_image->structTotalBinDiff(y->_image, patchSize);
  info.GetReturnValue().Set(Nan::New(error));
}

void ImageWrapper::structAvgBinDiff(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  if (!info[0]->IsObject() || !info[1]->IsInt32()) {
    Nan::ThrowError("structAvglBinDiff arugment error (Image, int)");
  }

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }

  ImageWrapper* y = Nan::ObjectWrap::Unwrap<ImageWrapper>(maybe1.ToLocalChecked());
  int patchSize = info[1]->Int32Value();

  double error = image->_image->structAvgBinDiff(y->_image, patchSize);
  info.GetReturnValue().Set(Nan::New(error));
}

void ImageWrapper::structPctDiff(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  if (!info[0]->IsObject() || !info[1]->IsInt32() || !info[2]->IsNumber()) {
    Nan::ThrowError("structPctDiff arugment error (Image, int, double)");
  }

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }

  ImageWrapper* y = Nan::ObjectWrap::Unwrap<ImageWrapper>(maybe1.ToLocalChecked());
  int patchSize = info[1]->Int32Value();
  double threshold = info[2]->NumberValue();
  
  vector<Eigen::VectorXd> patches = image->_image->patches(patchSize);
  y->_image->eliminateBinsSSIM(patches, patchSize, threshold);

  int ct = 0;
  for (int i = 0; i < patches.size(); i++) {
    if (patches[i].size() > 0)
      ct++;
  }

  info.GetReturnValue().Set(Nan::New(ct / (double)patches.size()));
}

void ImageWrapper::structSSIMDiff(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  if (!info[0]->IsObject() || !info[1]->IsInt32() || 
      !info[2]->IsNumber() || !info[3]->IsNumber() || !info[4]->IsNumber()) {
    Nan::ThrowError("structSSIMDiff arugment error (Image, int, double, double, double, double)");
  }

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }

  ImageWrapper* y = Nan::ObjectWrap::Unwrap<ImageWrapper>(maybe1.ToLocalChecked());
  int patchSize = info[1]->Int32Value();
  double a = info[2]->NumberValue();
  double b = info[3]->NumberValue();
  double g = info[4]->NumberValue();

  double res = y->_image->MSSIM(image->_image, patchSize, a, b, g);

  info.GetReturnValue().Set(Nan::New(res));
}

void ImageWrapper::stats(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  v8::Local<v8::Object> ret = Nan::New<v8::Object>();

  ret->Set(Nan::New("avgAlpha").ToLocalChecked(), Nan::New(image->_image->avgAlpha()));
  ret->Set(Nan::New("avgLuma").ToLocalChecked(), Nan::New(image->_image->avgLuma()));
  ret->Set(Nan::New("totalAlpha").ToLocalChecked(), Nan::New(image->_image->totalAlpha()));
  ret->Set(Nan::New("totalLuma").ToLocalChecked(), Nan::New(image->_image->totalLuma()));

  info.GetReturnValue().Set(ret);
}

void ImageWrapper::diff(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }
  ImageWrapper* y = Nan::ObjectWrap::Unwrap<ImageWrapper>(maybe1.ToLocalChecked());

  Comp::Image* d = image->_image->diff(y->_image);

  v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImageWrapper::imageConstructor);
  const int argc = 2;
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(d), Nan::New(true) };

  info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
}

void ImageWrapper::writeToImageData(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  // get the data array
  v8::Local<v8::Uint8ClampedArray> arr = info[0].As<v8::Object>()->Get(Nan::New("data").ToLocalChecked()).As<v8::Uint8ClampedArray>();
  unsigned char *data = (unsigned char*)arr->Buffer()->GetContents().Data();

  vector<unsigned char>& imData = image->_image->getData();
  memcpy(data, &imData[0], imData.size());

  // i don't think this needs to return anything?
}

void ImageWrapper::fill(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  if (info[0]->IsNumber() && info[1]->IsNumber() && info[2]->IsNumber()) {
    Comp::Image* filled = image->_image->fill(info[0]->NumberValue(), info[1]->NumberValue(), info[2]->NumberValue());

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImageWrapper::imageConstructor);
    const int argc = 2;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(filled), Nan::New(true) };

    info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
  }
  else {
    Nan::ThrowError("fill(float, float, float) argument error");
  }
}

void ImageWrapper::histogramIntersect(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }
  ImageWrapper* y = Nan::ObjectWrap::Unwrap<ImageWrapper>(maybe1.ToLocalChecked());

  float binSize = 0.05;
  if (info[1]->IsNumber()) {
    binSize = info[0]->NumberValue();
  }

  double d = image->_image->histogramIntersection(y->_image, binSize);

  info.GetReturnValue().Set(Nan::New(d));
}

void ImageWrapper::pctHistogramIntersect(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }
  ImageWrapper* y = Nan::ObjectWrap::Unwrap<ImageWrapper>(maybe1.ToLocalChecked());

  float binSize = 0.05;
  if (info[1]->IsNumber()) {
    binSize = info[0]->NumberValue();
  }

  double d = image->_image->proportionalHistogramIntersection(y->_image, binSize);

  info.GetReturnValue().Set(Nan::New(d));
}

void ImageWrapper::chamferDistance(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImageWrapper* image = ObjectWrap::Unwrap<ImageWrapper>(info.Holder());

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }
  ImageWrapper* y = Nan::ObjectWrap::Unwrap<ImageWrapper>(maybe1.ToLocalChecked());

  float d = image->_image->chamferDistance(y->_image);
  float d2 = y->_image->chamferDistance(image->_image);

  info.GetReturnValue().Set(Nan::New(d + d2));
}

void ImportanceMapWrapper::Init(v8::Local<v8::Object> exports)
{
  Nan::HandleScope scope;

  // constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("ImportanceMap").ToLocalChecked());

  auto tplInst = tpl->InstanceTemplate();
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetAccessor(tplInst, Nan::New("image").ToLocalChecked(), image);
  Nan::SetPrototypeMethod(tpl, "dump", dump);
  Nan::SetPrototypeMethod(tpl, "getVal", getVal);

  importanceMapConstructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("ImportanceMap").ToLocalChecked(), tpl->GetFunction());
}

ImportanceMapWrapper::ImportanceMapWrapper(shared_ptr<Comp::ImportanceMap> m, string name, Comp::ImportanceMapMode type) :
  _m(m), _name(name), _type(type)
{
}

ImportanceMapWrapper::~ImportanceMapWrapper()
{
}

void ImportanceMapWrapper::New(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // this constructor is a bit unique since it's a) intended to be returned from a compositor function only
  // and b) needs a shared pointer from the compositor parent object
  // it will throw an error if the arguments are wrong.
  if (info[0]->IsExternal() && info[1]->IsString() && info[2]->IsNumber()) {
    Comp::Compositor* c = static_cast<Comp::Compositor*>(info[0].As<v8::External>()->Value());
    v8::String::Utf8Value i1(info[1]->ToString());
    string layerName(*i1);
    Comp::ImportanceMapMode type = (Comp::ImportanceMapMode)(info[2]->Int32Value());

    ImportanceMapWrapper* m = new ImportanceMapWrapper(c->getImportanceMap(layerName, type), layerName, type);
    m->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  }
  else {
    Nan::ThrowError("ImportanceMap constructor error, requires (Compositor, string, number)");
  }
}

void ImportanceMapWrapper::image(v8::Local<v8::String>, const Nan::PropertyCallbackInfo<v8::Value>& info)
{
  // idk what the string is for really?
  ImportanceMapWrapper* m = ObjectWrap::Unwrap<ImportanceMapWrapper>(info.Holder());

  // nullcheck's a bit different here right?
  if (m->_m == nullptr) {
    Nan::ThrowError("Importance Map is null!");
  }

  v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImageWrapper::imageConstructor);
  const int argc = 2;
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(m->_m->getDisplayableImage().get()), Nan::New(false) };

  info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
}

void ImportanceMapWrapper::dump(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImportanceMapWrapper* m = ObjectWrap::Unwrap<ImportanceMapWrapper>(info.Holder());
  if (m->_m == nullptr) {
    Nan::ThrowError("Importance Map is null!");
  }

  if (info[0]->IsString() && info[1]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    v8::String::Utf8Value i1(info[1]->ToString());

    m->_m->dump(string(*i0), string(*i1));
  }
  else {
    Nan::ThrowError("ImportanceMap::dump(string, string) argument error");
  }
}

void ImportanceMapWrapper::getVal(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ImportanceMapWrapper* m = ObjectWrap::Unwrap<ImportanceMapWrapper>(info.Holder());
  if (m->_m == nullptr) {
    Nan::ThrowError("Importance Map is null!");
  }

  if (info[0]->IsNumber() && info[1]->IsNumber()) {
    double val = m->_m->getVal(info[0]->Int32Value(), info[1]->Int32Value());
    info.GetReturnValue().Set(Nan::New(val));
  }
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
  Nan::SetPrototypeMethod(tpl, "getGradient", getGradient);
  Nan::SetPrototypeMethod(tpl, "overwriteColor", overwriteColor);
  Nan::SetPrototypeMethod(tpl, "isAdjustmentLayer", isAdjustmentLayer);
  Nan::SetPrototypeMethod(tpl, "selectiveColorChannel", selectiveColorChannel);
  Nan::SetPrototypeMethod(tpl, "addInvertAdjustment", addInvertAdjustment);
  Nan::SetPrototypeMethod(tpl, "brightnessContrast", brightnessContrast);
  Nan::SetPrototypeMethod(tpl, "resetImage", resetImage);
  Nan::SetPrototypeMethod(tpl, "type", type);
  Nan::SetPrototypeMethod(tpl, "conditionalBlend", conditionalBlend);
  Nan::SetPrototypeMethod(tpl, "getMask", getMask);
  Nan::SetPrototypeMethod(tpl, "offset", offset);
  Nan::SetPrototypeMethod(tpl, "setPrecompOrder", setPrecompOrder);
  Nan::SetPrototypeMethod(tpl, "getPrecompOrder", getPrecompOrder);
  Nan::SetPrototypeMethod(tpl, "isPrecomp", isPrecomp);

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
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.image");

  v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImageWrapper::imageConstructor);
  const int argc = 2;
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(layer->_layer->getImage().get()), Nan::New(false) };

  info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
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

void LayerRef::isAdjustmentLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.isAdjustmentLayer");

  info.GetReturnValue().Set(Nan::New(layer->_layer->isAdjustmentLayer()));
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
  float gamma = (info[2]->IsNumber()) ? (float)info[2]->NumberValue() : (1.0f / 10);
  float outMin = (info[3]->IsNumber()) ? (float)info[3]->NumberValue() : 0;
  float outMax = (info[4]->IsNumber()) ? (float)info[4]->NumberValue() : 1.0f;

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

  if (!info[0]->IsArray()) {
    Nan::ThrowError("addGradient(float[], object[]) or addGradient(object[]) invalid arguments.");
  }

  v8::Local<v8::Array> args = info[0].As<v8::Array>();
  vector<float> pts;
  vector<Comp::RGBColor> colors;

  if (args->Get(0)->IsObject()) {
    // object with {r, g, b, x} fields
    for (unsigned int i = 0; i < args->Length(); i++) {
      v8::Local<v8::Object> co = args->Get(i).As<v8::Object>();
      Comp::RGBColor color;
      v8::Local<v8::Object> clr = co->Get(Nan::New("color").ToLocalChecked()).As<v8::Object>();

      color._r = (float)clr->Get(Nan::New("r").ToLocalChecked())->NumberValue();
      color._g = (float)clr->Get(Nan::New("g").ToLocalChecked())->NumberValue();
      color._b = (float)clr->Get(Nan::New("b").ToLocalChecked())->NumberValue();

      pts.push_back((float)co->Get(Nan::New("x").ToLocalChecked())->NumberValue());
      colors.push_back(color);
    }
  }
  else {
    // separate arrays
    // get point locations, just a plain array
    for (unsigned int i = 0; i < args->Length(); i++) {
      pts.push_back((float)args->Get(i)->NumberValue());
    }

    // get colors, an array of objects {r, g, b}
    v8::Local<v8::Array> c = info[1].As<v8::Array>();
    for (unsigned int i = 0; i < c->Length(); i++) {
      v8::Local<v8::Object> co = c->Get(i).As<v8::Object>();
      Comp::RGBColor color;
      color._r = (float)co->Get(Nan::New("r").ToLocalChecked())->NumberValue();
      color._g = (float)co->Get(Nan::New("g").ToLocalChecked())->NumberValue();
      color._b = (float)co->Get(Nan::New("b").ToLocalChecked())->NumberValue();

      colors.push_back(color);
    }
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

void LayerRef::getGradient(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.getGradient");

  // construct object. 
  v8::Local<v8::Array> grad = Nan::New<v8::Array>();
  Comp::Gradient& g = layer->_layer->getGradient();
  for (int i = 0; i < g._x.size(); i++) {
    // construct object
    v8::Local<v8::Object> pt = Nan::New<v8::Object>();
    pt->Set(Nan::New("x").ToLocalChecked(), Nan::New(g._x[i]));

    v8::Local<v8::Object> color = Nan::New<v8::Object>();
    color->Set(Nan::New("r").ToLocalChecked(), Nan::New(g._colors[i]._r));
    color->Set(Nan::New("g").ToLocalChecked(), Nan::New(g._colors[i]._g));
    color->Set(Nan::New("b").ToLocalChecked(), Nan::New(g._colors[i]._b));

    pt->Set(Nan::New("color").ToLocalChecked(), color);
    grad->Set(Nan::New(i), pt);
  }

  info.GetReturnValue().Set(grad);
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

void LayerRef::selectiveColorChannel(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.selectiveColorChannel");

  if (info.Length() == 2) {
    if (!info[0]->IsString() || !info[1]->IsString()) {
      Nan::ThrowError("selectiveColorChannel(string, string) invalid arguments");
    }

    v8::String::Utf8Value o1(info[0]->ToString());
    string channel(*o1);

    v8::String::Utf8Value o2(info[1]->ToString());
    string param(*o2);

    info.GetReturnValue().Set(Nan::New(layer->_layer->getSelectiveColorChannel(channel, param)));
  }
  else if (info.Length() == 3) {
    if (!info[0]->IsString() || !info[1]->IsString() || !info[2]->IsNumber()) {
      Nan::ThrowError("selectiveColorChannel(string, string, float) invalid arugments");
    }

    v8::String::Utf8Value o1(info[0]->ToString());
    string channel(*o1);

    v8::String::Utf8Value o2(info[1]->ToString());
    string param(*o2);

    layer->_layer->setSelectiveColorChannel(channel, param, (float)info[2]->NumberValue());
  }
  else {
    Nan::ThrowError("selectiveColorChannel invalid number of arguments");
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
    Comp::Utils<float>::RGBColorT rgb = Comp::Utils<float>::LabToRGB((float)color->Get(Nan::New("luminance").ToLocalChecked())->NumberValue(),
      (float)color->Get(Nan::New("a").ToLocalChecked())->NumberValue(),
      (float)color->Get(Nan::New("b").ToLocalChecked())->NumberValue());

    layer->_layer->addPhotoFilterAdjustment(preserveLuma, rgb._r, rgb._g, rgb._b, density);
  }
  else if (color->Has(Nan::New("hue").ToLocalChecked())) {
    // convert from hsl to rgb
    Comp::Utils<float>::RGBColorT rgb = Comp::Utils<float>::HSLToRGB((float)color->Get(Nan::New("hue").ToLocalChecked())->NumberValue(),
      (float)color->Get(Nan::New("saturation").ToLocalChecked())->NumberValue(),
      (float)color->Get(Nan::New("brightness").ToLocalChecked())->NumberValue());

    layer->_layer->addPhotoFilterAdjustment(preserveLuma, rgb._r, rgb._g, rgb._b, density);
  }
  else if (color->Has(Nan::New("r").ToLocalChecked())) {
    // just do the thing

    layer->_layer->addPhotoFilterAdjustment(preserveLuma, (float)color->Get(Nan::New("r").ToLocalChecked())->NumberValue(),
      (float)color->Get(Nan::New("g").ToLocalChecked())->NumberValue(),
      (float)color->Get(Nan::New("b").ToLocalChecked())->NumberValue(),
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

void LayerRef::overwriteColor(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // check if getting status or not
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.overwriteColor");

  if (info.Length() == 0) {
    // status
    v8::Local<v8::Object> ret = Nan::New<v8::Object>();

    map<string, float> adj = layer->_layer->getAdjustment(Comp::AdjustmentType::OVERWRITE_COLOR);
    ret->Set(Nan::New("r").ToLocalChecked(), Nan::New(adj["r"]));
    ret->Set(Nan::New("g").ToLocalChecked(), Nan::New(adj["g"]));
    ret->Set(Nan::New("b").ToLocalChecked(), Nan::New(adj["b"]));
    ret->Set(Nan::New("a").ToLocalChecked(), Nan::New(adj["a"]));

    info.GetReturnValue().Set(ret);
  }
  else {
    if (!info[0]->IsNumber() || !info[1]->IsNumber() || !info[2]->IsNumber() || !info[3]->IsNumber()) {
      Nan::ThrowError("overwriteColor(float, float, float, float) argument error.");
    }

    // set value
    layer->_layer->addOverwriteColorAdjustment((float)info[0]->NumberValue(), (float)info[1]->NumberValue(),
      (float)info[2]->NumberValue(), (float)info[3]->NumberValue());
  }
}

void LayerRef::addInvertAdjustment(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.invertAdjustment");

  layer->_layer->addInvertAdjustment();
}

void LayerRef::brightnessContrast(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // check if getting status or not
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.brightnessContrast");

  if (info.Length() == 0) {
    // status
    v8::Local<v8::Object> ret = Nan::New<v8::Object>();

    map<string, float> adj = layer->_layer->getAdjustment(Comp::AdjustmentType::BRIGHTNESS);
    ret->Set(Nan::New("brightness").ToLocalChecked(), Nan::New(adj["brightness"]));
    ret->Set(Nan::New("contrast").ToLocalChecked(), Nan::New(adj["contrast"]));

    info.GetReturnValue().Set(ret);
  }
  else {
    if (!info[0]->IsNumber() || !info[1]->IsNumber()) {
      Nan::ThrowError("brightnessContrast(float, float) argument error.");
    }

    // set value
    layer->_layer->addBrightnessAdjustment((float)info[0]->NumberValue(), (float)info[1]->NumberValue());
  }
}

void LayerRef::resetImage(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // check if getting status or not
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.resetImage");

  layer->_layer->resetImage();
}

void LayerRef::type(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.type");

  if (info[0]->IsString()) {
    v8::String::Utf8Value o1(info[0]->ToString());
    string type(*o1);
    layer->_layer->_psType = type;
  }

  info.GetReturnValue().Set(Nan::New(layer->_layer->_psType).ToLocalChecked());
}

void LayerRef::conditionalBlend(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.conditionalBlend");

  if (info.Length() < 2) {
    // return info
    v8::Local<v8::Object> ret = Nan::New<v8::Object>();

    ret->Set(Nan::New("channel").ToLocalChecked(), Nan::New(layer->_layer->getConditionalBlendChannel()).ToLocalChecked());

    v8::Local<v8::Object> params = Nan::New<v8::Object>();
    for (auto& param : layer->_layer->getConditionalBlendSettings()) {
      params->Set(Nan::New(param.first).ToLocalChecked(), Nan::New(param.second));
    }

    ret->Set(Nan::New("params").ToLocalChecked(), params);

    info.GetReturnValue().Set(ret);
  }
  else {
    if (info[0]->IsString() && info[1]->IsObject()) {
      v8::String::Utf8Value i0(info[0]->ToString());
      string channel(*i0);

      v8::Local<v8::Object> ret = info[1].As<v8::Object>();
      auto params = ret->GetOwnPropertyNames();

      map<string, float> cbParams;

      for (unsigned int i = 0; i < params->Length(); i++) {
        float val = ret->Get(params->Get(i))->NumberValue();

        v8::String::Utf8Value o1(params->Get(i)->ToString());
        string param(*o1);

        cbParams[param] = val;
      }

      layer->_layer->setConditionalBlend(channel, cbParams);
    }
    else {
      Nan::ThrowError("conditionalBlend(string, object) argument error");
    }
  }
}

void LayerRef::getMask(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.getMask");

  shared_ptr<Comp::Image> mask = layer->_layer->getMask();

  if (mask != nullptr) {
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImageWrapper::imageConstructor);
    const int argc = 2;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(mask.get()), Nan::New(false) };

    info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
  }
}

void LayerRef::offset(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.offset");

  if (info[0]->IsNumber() && info[1]->IsNumber()) {
    layer->_layer->setOffset(info[0]->NumberValue(), info[1]->NumberValue());
  }
  else {
    auto offset = layer->_layer->getOffset();
    v8::Local<v8::Object> ret = Nan::New<v8::Object>();
    ret->Set(Nan::New("x").ToLocalChecked(), Nan::New(offset.first));
    ret->Set(Nan::New("y").ToLocalChecked(), Nan::New(offset.second));

    info.GetReturnValue().Set(ret);
  }
}

void LayerRef::setPrecompOrder(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.setPrecompOrder");

  if (info[0]->IsArray()) {
    v8::Local<v8::Array> arr = info[0].As<v8::Array>();
    vector<string> order;
    for (int i = 0; i < arr->Length(); i++) {
      v8::String::Utf8Value str(arr->Get(i)->ToString());
      order.push_back(string(*str));
    }

    layer->_layer->setPrecompOrder(order);
  }
  else {
    Nan::ThrowError("setPrecompOrder(string[]) argument error");
  }
}

void LayerRef::getPrecompOrder(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.getPrecompOrder");

  v8::Local<v8::Array> ret = Nan::New<v8::Array>();
  vector<string> order = layer->_layer->getPrecompOrder();

  for (int i = 0; i < order.size(); i++) {
    ret->Set(i, Nan::New(order[i]).ToLocalChecked());
  }

  info.GetReturnValue().Set(ret);
}

void LayerRef::isPrecomp(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  LayerRef* layer = ObjectWrap::Unwrap<LayerRef>(info.Holder());
  nullcheck(layer->_layer, "layer.isPrecomp");
 
  info.GetReturnValue().Set(Nan::New(layer->_layer->isPrecomp()));
}

void ContextWrapper::Init(v8::Local<v8::Object> exports)
{
  Nan::HandleScope scope;

  // constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Context").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "getLayer", getLayer);
  Nan::SetPrototypeMethod(tpl, "keys", keys);
  Nan::SetPrototypeMethod(tpl, "layerVector", layerVector);

  contextConstructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("Context").ToLocalChecked(), tpl->GetFunction());
}

ContextWrapper::ContextWrapper(Comp::Context ctx) : _context(ctx)
{
}

void ContextWrapper::New(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  if (!info[0]->IsExternal()) {
    Nan::ThrowError("Internal Error: Context pointer not found as first argument of ContextWrapper constructor.");
  }

  Comp::Context* c = static_cast<Comp::Context*>(info[0].As<v8::External>()->Value());

  ContextWrapper* ctx = new ContextWrapper(*c);
  ctx->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

void ContextWrapper::getLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // this function is identical to the same function for compositors
  Nan::HandleScope scope;

  if (!info[0]->IsString()) {
    Nan::ThrowError("getLayer expects a layer name");
  }

  // expects a name
  ContextWrapper* c = ObjectWrap::Unwrap<ContextWrapper>(info.Holder());
  nullcheck(c, "ContextWrapper.getLayer");

  v8::String::Utf8Value val0(info[0]->ToString());
  string name(*val0);

  if (c->_context.count(name) > 0) {
    Comp::Layer& l = c->_context[name];

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(LayerRef::layerConstructor);
    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&l) };

    info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
  }
  else {
    info.GetReturnValue().Set(Nan::Null());
  }
}

void ContextWrapper::keys(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ContextWrapper* c = ObjectWrap::Unwrap<ContextWrapper>(info.Holder());
  nullcheck(c, "ContextWrapper.keys");

  v8::Local<v8::Array> keys = Nan::New<v8::Array>();

  int i = 0;
  for (auto k : c->_context) {
    keys->Set(i, Nan::New<v8::String>(k.first).ToLocalChecked());
    i++;
  }

  info.GetReturnValue().Set(keys);
}

void ContextWrapper::layerVector(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ContextWrapper* c = ObjectWrap::Unwrap<ContextWrapper>(info.Holder());
  nullcheck(c, "ContextWrapper.layerVector");

  if (!info[0]->IsObject()) {
    Nan::ThrowError("layerVector requires a Compositor object for Layer Order");
  }

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }
  CompositorWrapper* comp = Nan::ObjectWrap::Unwrap<CompositorWrapper>(maybe1.ToLocalChecked());

  nlohmann::json key;
  vector<double> layerData = comp->_compositor->contextToVector(c->_context, key);

  v8::Local<v8::Array> vals = Nan::New<v8::Array>();
  for (int i = 0; i < layerData.size(); i++) {
    vals->Set(i, Nan::New(layerData[i]));
  }

  info.GetReturnValue().Set(vals);
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
  Nan::SetPrototypeMethod(tpl, "addBase64Layer", addLayer);
  Nan::SetPrototypeMethod(tpl, "copyLayer", copyLayer);
  Nan::SetPrototypeMethod(tpl, "deleteLayer", deleteLayer);
  Nan::SetPrototypeMethod(tpl, "getAllLayers", getAllLayers);
  Nan::SetPrototypeMethod(tpl, "setLayerOrder", setLayerOrder);
  Nan::SetPrototypeMethod(tpl, "getTopLayerOrder", getLayerNames);
  Nan::SetPrototypeMethod(tpl, "size", size);
  Nan::SetPrototypeMethod(tpl, "render", render);
  Nan::SetPrototypeMethod(tpl, "asyncRender", asyncRender);
  Nan::SetPrototypeMethod(tpl, "getCacheSizes", getCacheSizes);
  Nan::SetPrototypeMethod(tpl, "addCacheSize", addCacheSize);
  Nan::SetPrototypeMethod(tpl, "deleteCacheSize", deleteCacheSize);
  Nan::SetPrototypeMethod(tpl, "getCachedImage", getCachedImage);
  Nan::SetPrototypeMethod(tpl, "reorderLayer", reorderLayer);
  Nan::SetPrototypeMethod(tpl, "startSearch", startSearch);
  Nan::SetPrototypeMethod(tpl, "stopSearch", stopSearch);
  Nan::SetPrototypeMethod(tpl, "renderContext", renderContext);
  Nan::SetPrototypeMethod(tpl, "asyncRenderContext", asyncRenderContext);
  Nan::SetPrototypeMethod(tpl, "getContext", getContext);
  Nan::SetPrototypeMethod(tpl, "setContext", setContext);
  Nan::SetPrototypeMethod(tpl, "resetImages", resetImages);
  Nan::SetPrototypeMethod(tpl, "setMaskLayer", setMaskLayer);
  Nan::SetPrototypeMethod(tpl, "getMaskLayer", getMaskLayer);
  Nan::SetPrototypeMethod(tpl, "deleteMaskLayer", deleteMaskLayer);
  Nan::SetPrototypeMethod(tpl, "clearMask", clearMask);
  Nan::SetPrototypeMethod(tpl, "paramsToCeres", paramsToCeres);
  Nan::SetPrototypeMethod(tpl, "ceresToContext", ceresToContext);
  Nan::SetPrototypeMethod(tpl, "renderPixel", renderPixel);
  Nan::SetPrototypeMethod(tpl, "getPixelConstraints", getPixelConstraints);
  Nan::SetPrototypeMethod(tpl, "computeErrorMap", computeErrorMap);
  Nan::SetPrototypeMethod(tpl, "addSearchGroup", addSearchGroup);
  Nan::SetPrototypeMethod(tpl, "clearSearchGroups", clearSearchGroups);
  Nan::SetPrototypeMethod(tpl, "contextFromVector", contextFromVector);
  Nan::SetPrototypeMethod(tpl, "contextFromDarkroom", contextFromDarkroom);
  Nan::SetPrototypeMethod(tpl, "localImportance", localImportance);
  Nan::SetPrototypeMethod(tpl, "imageDims", imageDimensions);
  Nan::SetPrototypeMethod(tpl, "regionalImportance", importanceInRegion);
  Nan::SetPrototypeMethod(tpl, "pointImportance", pointImportance);
  Nan::SetPrototypeMethod(tpl, "computeImportanceMap", computeImportanceMap);
  Nan::SetPrototypeMethod(tpl, "computeAllImportanceMaps", computeAllImportanceMaps);
  Nan::SetPrototypeMethod(tpl, "getImportanceMap", getImportanceMap);
  Nan::SetPrototypeMethod(tpl, "deleteImportanceMap", deleteImportanceMap);
  Nan::SetPrototypeMethod(tpl, "deleteLayerImportanceMaps", deleteLayerImportanceMaps);
  Nan::SetPrototypeMethod(tpl, "deleteImportanceMapType", deleteImportanceMapType);
  Nan::SetPrototypeMethod(tpl, "deleteAllImportanceMaps", deleteAllImportanceMaps);
  Nan::SetPrototypeMethod(tpl, "dumpImportanceMaps", dumpImportanceMaps);
  Nan::SetPrototypeMethod(tpl, "getImportanceMapCache", availableImportanceMaps);
  Nan::SetPrototypeMethod(tpl, "createClickMap", createClickMap);
  Nan::SetPrototypeMethod(tpl, "analyzeAndTag", analyzeAndTag);
  Nan::SetPrototypeMethod(tpl, "uniqueTags", uniqueTags);
  Nan::SetPrototypeMethod(tpl, "getTags", getTags);
  Nan::SetPrototypeMethod(tpl, "allTags", allTags);
  Nan::SetPrototypeMethod(tpl, "deleteTags", deleteTags);
  Nan::SetPrototypeMethod(tpl, "deleteAllTags", deleteAllTags);
  Nan::SetPrototypeMethod(tpl, "hasTag", hasTag);
  Nan::SetPrototypeMethod(tpl, "goalSelect", goalSelect);
  Nan::SetPrototypeMethod(tpl, "addMask", addMask);
  Nan::SetPrototypeMethod(tpl, "addPoissonDisk", addPoissonDiskCache);
  Nan::SetPrototypeMethod(tpl, "initPoissonDisks", initPoissonDisks);
  Nan::SetPrototypeMethod(tpl, "addGroup", addGroup);
  Nan::SetPrototypeMethod(tpl, "deleteGroup", deleteGroup);
  Nan::SetPrototypeMethod(tpl, "addLayerToGroup", addLayerToGroup);
  Nan::SetPrototypeMethod(tpl, "removeLayerFromGroup", removeLayerFromGroup);
  Nan::SetPrototypeMethod(tpl, "setGroupOrder", setGroupOrder);
  Nan::SetPrototypeMethod(tpl, "getGroupOrder", getGroupOrder);
  Nan::SetPrototypeMethod(tpl, "getGroup", getGroup);
  Nan::SetPrototypeMethod(tpl, "addGroupFromExistingLayer", addGroupFromExistingLayer);
  Nan::SetPrototypeMethod(tpl, "setGroupLayers", setGroupLayers);
  Nan::SetPrototypeMethod(tpl, "layerInGroup", layerInGroup);
  Nan::SetPrototypeMethod(tpl, "isGroup", isGroup);
  Nan::SetPrototypeMethod(tpl, "renderUpToLayer", renderUpToLayer);
  Nan::SetPrototypeMethod(tpl, "asyncRenderUpToLayer", asyncRenderUpToLayer);
  Nan::SetPrototypeMethod(tpl, "getFlatLayerOrder", getFlatLayerOrder);
  Nan::SetPrototypeMethod(tpl, "getModifierOrder", getModifierOrder);
  Nan::SetPrototypeMethod(tpl, "offsetLayer", offsetLayer);
  Nan::SetPrototypeMethod(tpl, "resetLayerOffset", resetLayerOffset);
  Nan::SetPrototypeMethod(tpl, "layerHistogramIntersect", layerHistogramIntersect);
  Nan::SetPrototypeMethod(tpl, "propLayerHistogramIntersect", propLayerHistogramIntersect);
  Nan::SetPrototypeMethod(tpl, "getGroupInclusionMap", getGroupInclusionMap);
  Nan::SetPrototypeMethod(tpl, "addGroupEffect", addGroupEffect);
  Nan::SetPrototypeMethod(tpl, "renderOnlyLayer", renderOnlyLayer);
  Nan::SetPrototypeMethod(tpl, "isLayer", isLayer);

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
  // initialization might be from a file or from nothing
  CompositorWrapper* cw = new CompositorWrapper();
  if (info.Length() == 2) {
    v8::String::Utf8Value file(info[0]->ToString());
    v8::String::Utf8Value dir(info[1]->ToString());

    cw->_compositor = new Comp::Compositor(string(*file), string(*dir));
  }
  else {
    cw->_compositor = new Comp::Compositor();
  }

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

  info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
}

void CompositorWrapper::isLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.addLayer");

  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    info.GetReturnValue().Set(Nan::New(c->_compositor->getPrimaryContext().count(string(*i0)) > 0));
  }
  else {
    info.GetReturnValue().Set(Nan::New(false));
  }
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

void CompositorWrapper::addBase64Layer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  if (info.Length() != 4 || !info[0]->IsString() || !info[1]->IsString()) {
    Nan::ThrowError("addBase64Layer expects (string, string)");
  }

  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.addBase64Layer");

  bool result;
  v8::String::Utf8Value val0(info[0]->ToString());
  string name(*val0);

  v8::String::Utf8Value val1(info[1]->ToString());
  string base64(*val1);

  Comp::Image img(info[2]->Int32Value(), info[3]->Int32Value(), base64);

  result = c->_compositor->addLayer(name, img);

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
  Comp::Context ctx = c->_compositor->getNewContext();

  for (auto& kvp : ctx) {
    Comp::Layer& l = kvp.second;

    // create v8 object
    v8::Local<v8::Object> layer = Nan::New<v8::Object>();
    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&l) };

    layers->Set(Nan::New(l.getName()).ToLocalChecked(), Nan::NewInstance(cons, argc, argv).ToLocalChecked());
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

void CompositorWrapper::imageDimensions(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.imageDimensions");

  string size;
  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    size = string(*i0);
  }
  else {
    size = "full";
  }

  v8::Local<v8::Object> ret = Nan::New<v8::Object>();

  ret->Set(Nan::New("w").ToLocalChecked(), Nan::New(c->_compositor->getWidth(size)));
  ret->Set(Nan::New("h").ToLocalChecked(), Nan::New(c->_compositor->getHeight(size)));

  info.GetReturnValue().Set(ret);
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

  info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
}

void CompositorWrapper::asyncRender(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // does a render, async style
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.render");

  string size = "";
  Nan::Callback* callback;

  if (info[0]->IsString()) {
    v8::String::Utf8Value val0(info[0]->ToString());
    size = string(*val0);

    callback = new Nan::Callback(info[1].As<v8::Function>());
  }
  else if (info[0]->IsFunction()) {
    callback = new Nan::Callback(info[0].As<v8::Function>());
  }
  else {
    Nan::ThrowError("asyncRender should either have a callback or a size string and a callback.");
  }

  Nan::AsyncQueueWorker(new RenderWorker(callback, size, c->_compositor));

  info.GetReturnValue().SetUndefined();
}

void CompositorWrapper::renderContext(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.renderContext");

  if (!info[0]->IsObject()) {
    Nan::ThrowError("renderContext requires a context to render");
  }

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }
  ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

  string size = "";
  if (info[1]->IsString()) {
    v8::String::Utf8Value val0(info[1]->ToString());
    size = string(*val0);
  }

  Comp::Image* img = c->_compositor->render(ctx->_context, size);

  v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImageWrapper::imageConstructor);
  const int argc = 2;
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(img), Nan::New(true) };

  info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
}

void CompositorWrapper::asyncRenderContext(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // does a render, async style
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.asyncRenderContext");

  if (!info[0]->IsObject()) {
    Nan::ThrowError("renderContext requires a context to render");
  }

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }
  ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

  string size = "";
  Nan::Callback* callback;

  if (info[1]->IsString()) {
    v8::String::Utf8Value val0(info[1]->ToString());
    size = string(*val0);

    callback = new Nan::Callback(info[2].As<v8::Function>());
  }
  else if (info[1]->IsFunction()) {
    callback = new Nan::Callback(info[1].As<v8::Function>());
  }
  else {
    Nan::ThrowError("asyncRenderContext should either have a callback or a size string and a callback.");
  }

  Nan::AsyncQueueWorker(new RenderWorker(callback, size, c->_compositor, ctx->_context));

  info.GetReturnValue().SetUndefined();
}

void CompositorWrapper::getContext(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getContext");

  Comp::Context ctx = c->_compositor->getNewContext();

  // context object
  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&ctx) };
  v8::Local<v8::Function> cons = Nan::New<v8::Function>(ContextWrapper::contextConstructor);

  info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
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

  info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
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

void CompositorWrapper::startSearch(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.startSearch");

  unsigned int threads = 1;
  string renderSize = "";
  Comp::SearchMode mode;
  map<string, float> opt;

  // search mode
  if (info[0]->IsNumber()) {
    mode = (Comp::SearchMode)info[0]->IntegerValue();
  }
  else {
    Nan::ThrowError("startSearch must specify a search mode.");
  }

  // options
  if (info[1]->IsObject()) {
    // convert object to c++ map
    v8::Local<v8::Object> ret = info[1].As<v8::Object>();
    auto names = ret->GetOwnPropertyNames();
    for (unsigned int i = 0; i < names->Length(); i++) {
      v8::Local<v8::Value> val = ret->Get(names->Get(i));
      v8::String::Utf8Value o1(names->Get(i)->ToString());
      string prop(*o1);

      opt[prop] = (float)val->NumberValue();
    }
  }

  // threads
  if (info[2]->IsNumber()) {
    threads = info[2]->Int32Value();
  }

  // render size
  if (info[3]->IsString()) {
    v8::String::Utf8Value val(info[3]->ToString());
    renderSize = string(*val);
  }

  // clamp threads to max supported
  if (threads > thread::hardware_concurrency()) {
    threads = thread::hardware_concurrency();
  }

  // ok so this part gets complicated. 
  // hopefully what happens here is that we create a callback function for the c++ code to call
  // in order to get the image data out of c++ into js. The compositor object itself will
  // run the search loop and the node code is called through the anonymous function created here.
  Comp::searchCallback cb = [c](Comp::Image* img, Comp::Context ctx, map<string, float> meta, map<string, string> meta2) {
    // create necessary data structures
    asyncSampleEventData* asyncData = new asyncSampleEventData();
    asyncData->request.data = (void*)asyncData;
    asyncData->img = img;
    asyncData->ctx = ctx;
    asyncData->c = c;
    asyncData->meta = meta;
    asyncData->meta2 = meta2;

    uv_queue_work(uv_default_loop(), &asyncData->request, asyncNop, reinterpret_cast<uv_after_work_cb>(asyncSampleEvent));
  };

  c->_compositor->startSearch(cb, mode, opt, threads, renderSize);

  info.GetReturnValue().SetUndefined();
}

void CompositorWrapper::stopSearch(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "composior.stopSearch");

  Nan::Callback* callback = new Nan::Callback(info[0].As<v8::Function>());

  // async this, it blocks
  Nan::AsyncQueueWorker(new StopSearchWorker(callback, c->_compositor));

  info.GetReturnValue().SetUndefined();
}

void CompositorWrapper::setContext(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.setContext");

  if (!info[0]->IsObject()) {
    Nan::ThrowError("setContext(Context) argument error");
  }

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }
  ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

  c->_compositor->setContext(ctx->_context);
}

void CompositorWrapper::resetImages(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.resetImage");

  if (!info[0]->IsString()) {
    Nan::ThrowError("resetImages(string) argument error");
  }

  v8::String::Utf8Value val0(info[0]->ToString());
  string name = string(*val0);

  c->_compositor->resetImages(name);
}

void CompositorWrapper::setMaskLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // input should be string string (base64)
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.setMaskLayer");

  if (info[0]->IsString() && info[1]->IsInt32() && info[2]->IsInt32() && info[3]->IsInt32() && info[4]->IsString()) {
    v8::String::Utf8Value val0(info[0]->ToString());
    string name(*val0);

    Comp::ConstraintType type = (Comp::ConstraintType)info[1]->Int32Value();
    int w = info[2]->Int32Value();
    int h = info[3]->Int32Value();

    v8::String::Utf8Value val1(info[4]->ToString());
    string data(*val1);

    shared_ptr<Comp::Image> img = shared_ptr<Comp::Image>(new Comp::Image(w, h, data));
    c->_compositor->getConstraintData().setMaskLayer(name, img, type);
  }
  else {
    Nan::ThrowError("compositor.setMaskLayer(name:string, type:int, w:int, h:int, data:string) argument error");
  }
}

void CompositorWrapper::getMaskLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getMaskLayer");

  if (info[0]->IsString()) {
    v8::String::Utf8Value val0(info[0]->ToString());
    string name(*val0);

    shared_ptr<Comp::Image> img = c->_compositor->getConstraintData().getRawInput(name);

    if (img == nullptr) {
      info.GetReturnValue().Set(Nan::New(Nan::Null));
      return;
    }

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImageWrapper::imageConstructor);
    const int argc = 2;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(img.get()), Nan::New(true) };

    info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
  }
  else {
    Nan::ThrowError("compositor.getMaskLayer(name:string) argument error");
  }

}

void CompositorWrapper::deleteMaskLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getMaskLayer");

  if (info[0]->IsString()) {
    v8::String::Utf8Value val0(info[0]->ToString());
    string name(*val0);

    c->_compositor->getConstraintData().deleteMaskLayer(name);
  }
}

void CompositorWrapper::clearMask(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getMaskLayer");

  c->_compositor->getConstraintData().deleteAllData();
}

void CompositorWrapper::paramsToCeres(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.paramsToCeres");

  if (!info[0]->IsObject()) {
    Nan::ThrowError("paramsToCeres requires a context to evaluate");
  }

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }
  ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

  if (info[1]->IsArray() && info[2]->IsArray() && info[3]->IsArray() && info[4]->IsString()) {
    // points
    vector<Comp::Point> pts;

    v8::Local<v8::Array> ptsArr = info[1].As<v8::Array>();
    for (int i = 0; i < ptsArr->Length(); i++) {
      v8::Local<v8::Object> pt = ptsArr->Get(i).As<v8::Object>();
      pts.push_back(Comp::Point(pt->Get(Nan::New("x").ToLocalChecked())->NumberValue(),
        pt->Get(Nan::New("y").ToLocalChecked())->NumberValue()));
    }

    // colors
    vector<Comp::RGBColor> colors;
    v8::Local<v8::Array> colorArr = info[2].As<v8::Array>();
    for (int i = 0; i < colorArr->Length(); i++) {
      v8::Local<v8::Object> color = colorArr->Get(i).As<v8::Object>();

      Comp::RGBColor c;
      c._r = color->Get(Nan::New("r").ToLocalChecked())->NumberValue();
      c._g = color->Get(Nan::New("g").ToLocalChecked())->NumberValue();
      c._b = color->Get(Nan::New("b").ToLocalChecked())->NumberValue();
      colors.push_back(c);
    }

    // weights
    vector<double> weights;
    v8::Local<v8::Array> weightArr = info[3].As<v8::Array>();
    for (int i = 0; i < weightArr->Length(); i++) {
      weights.push_back(weightArr->Get(i)->NumberValue());
    }

    // output
    v8::String::Utf8Value val4(info[4]->ToString());
    string out(*val4);

    c->_compositor->paramsToCeres(ctx->_context, pts, colors, weights, out);
  }
  else {
    Nan::ThrowError("paramsToCeres(object:Context, pts:object[], targets:object[], weights:double[], outputFile:string) argument error");
  }
}

void CompositorWrapper::ceresToContext(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.ceresToContext");

  if (info[0]->IsString()) {
    v8::String::Utf8Value val0(info[0]->ToString());
    string file(*val0);

    map<string, float> metadata;

    Comp::Context ctx;
    bool res = c->_compositor->ceresToContext(file, metadata, ctx);

    if (!res) {
      // failure to load
      info.GetReturnValue().SetNull();
      return;
    }

    // context object
    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&ctx) };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(ContextWrapper::contextConstructor);
    v8::Local<v8::Object> context = Nan::NewInstance(cons, argc, argv).ToLocalChecked();

    // metadta object
    v8::Local<v8::Object> meta = Nan::New<v8::Object>();
    for (auto& kvp : metadata) {
      meta->Set(Nan::New(kvp.first).ToLocalChecked(), Nan::New(kvp.second));
    }

    // return object
    v8::Local<v8::Object> ret = Nan::New<v8::Object>();
    ret->Set(Nan::New("context").ToLocalChecked(), context);
    ret->Set(Nan::New("metadata").ToLocalChecked(), meta);

    info.GetReturnValue().Set(ret);
  }
  else {
    Nan::ThrowError("ceresToContext(file:string) argument error");
  }
}

void CompositorWrapper::renderPixel(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.ceresToContext");

  if (info[0]->IsObject() && info[1]->IsInt32() && info[2]->IsInt32()) {
    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Object found is empty!");
    }
    ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    int x = info[1]->Int32Value();
    int y = info[2]->Int32Value();

    Comp::RGBAColor px = c->_compositor->renderPixel(ctx->_context, x, y);

    v8::Local<v8::Object> color = Nan::New<v8::Object>();
    color->Set(Nan::New("r").ToLocalChecked(), Nan::New(px._r));
    color->Set(Nan::New("g").ToLocalChecked(), Nan::New(px._g));
    color->Set(Nan::New("b").ToLocalChecked(), Nan::New(px._b));
    color->Set(Nan::New("a").ToLocalChecked(), Nan::New(px._a));

    info.GetReturnValue().Set(color);
  }
  else {
    Nan::ThrowError("renderPixel(context:object, x:int, y:int) argument error");
  }
}

void CompositorWrapper::getPixelConstraints(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getPixelConstraints");

  if (info[0]->IsObject()) {
    v8::Local<v8::Object> opt = info[0].As<v8::Object>();

    if (opt->Get(Nan::New("detailedLog").ToLocalChecked())->IsBoolean()) {
      c->_compositor->getConstraintData()._verboseDebugMode = opt->Get(Nan::New("detailedLog").ToLocalChecked())->BooleanValue();
    }

    if (opt->Get(Nan::New("unconstrainedDensity").ToLocalChecked())->IsInt32()) {
      c->_compositor->getConstraintData()._unconstDensity = opt->Get(Nan::New("unconstrainedDensity").ToLocalChecked())->Int32Value();
    }

    if (opt->Get(Nan::New("constrainedDensity").ToLocalChecked())->IsInt32()) {
      c->_compositor->getConstraintData()._constDensity = opt->Get(Nan::New("constrainedDensity").ToLocalChecked())->Int32Value();
    }

    if (opt->Get(Nan::New("unconstrainedWeight").ToLocalChecked())->IsNumber()) {
      c->_compositor->getConstraintData()._totalUnconstrainedWeight = opt->Get(Nan::New("unconstrainedWeight").ToLocalChecked())->NumberValue();
    }

    if (opt->Get(Nan::New("constrainedWeight").ToLocalChecked())->IsNumber()) {
      c->_compositor->getConstraintData()._totalConstrainedWeight = opt->Get(Nan::New("constrainedWeight").ToLocalChecked())->NumberValue();
    }
  }

  // this function is a work in progress, used currently to call functions for debugging the
  // constraint generation methods
  Comp::Context ctx = c->_compositor->getNewContext();
  auto constraints = c->_compositor->getConstraintData().getPixelConstraints(ctx, shared_ptr<Comp::Image>(c->_compositor->render(ctx)));

  // put into javascript objects
  v8::Local<v8::Array> ret = Nan::New<v8::Array>();
  for (int i = 0; i < constraints.size(); i++) {
    auto& c = constraints[i];

    v8::Local<v8::Object> constraint = Nan::New<v8::Object>();
    v8::Local<v8::Object> loc = Nan::New<v8::Object>();
    v8::Local<v8::Object> color = Nan::New<v8::Object>();

    loc->Set(Nan::New("x").ToLocalChecked(), Nan::New(c._pt._x));
    loc->Set(Nan::New("y").ToLocalChecked(), Nan::New(c._pt._y));
    constraint->Set(Nan::New("pt").ToLocalChecked(), loc);

    color->Set(Nan::New("r").ToLocalChecked(), Nan::New(c._color._r));
    color->Set(Nan::New("g").ToLocalChecked(), Nan::New(c._color._g));
    color->Set(Nan::New("b").ToLocalChecked(), Nan::New(c._color._b));
    constraint->Set(Nan::New("color").ToLocalChecked(), color);

    constraint->Set(Nan::New("weight").ToLocalChecked(), Nan::New(c._weight));

    ret->Set(Nan::New(i), constraint);
  }

  info.GetReturnValue().Set(ret);
}

void CompositorWrapper::computeErrorMap(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.computeErrorMap");

  // to prevent error we render the image for the user here at full size (since the UI supports multiple sizes)
  if (info[0]->IsObject() && info[1]->IsString()) {
    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Object found is empty!");
    }
    ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    v8::String::Utf8Value val0(info[1]->ToString());
    string file(*val0);

    shared_ptr<Comp::Image> render = shared_ptr<Comp::Image>(c->_compositor->render(ctx->_context));

    c->_compositor->getConstraintData().computeErrorMap(render, file);
  }
  else {
    Nan::ThrowError("computeErrorMap(context:object, filename:string) argument error");
  }
}

void CompositorWrapper::addSearchGroup(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.addSearchGroup");

  if (info[0]->IsObject()) {
    v8::Local<v8::Object> data = info[0].As<v8::Object>();

    // type
    Comp::SearchGroup g;
    g._type = (Comp::SearchGroupType)(data->Get(Nan::New("type").ToLocalChecked())->Int32Value());

    // names
    v8::Local<v8::Array> names = data->Get(Nan::New("layers").ToLocalChecked()).As<v8::Array>();
    for (int i = 0; i < names->Length(); i++) {
      v8::String::Utf8Value val0(names->Get(Nan::New(i))->ToString());
      string name(*val0);
      g._layerNames.push_back(name);
    }
    
    c->_compositor->addSearchGroup(g);
  }
  else {
    Nan::ThrowError("addSearchGroup expects an object");
  }
}

void CompositorWrapper::clearSearchGroups(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.clearSearchGroup");

  c->_compositor->clearSearchGroups();
}

void CompositorWrapper::contextFromVector(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.contextFromVector");

  // loads a numeric vector into a context
  if (info[0]->IsArray()) {
    vector<double> data;
    v8::Local<v8::Array> v8Data = info[0].As<v8::Array>();

    for (int i = 0; i < v8Data->Length(); i++) {
      data.push_back(v8Data->Get(i)->NumberValue());
    }

    Comp::Context ctx = c->_compositor->vectorToContext(data);

    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&ctx) };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(ContextWrapper::contextConstructor);

    info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
  }
  else {
    Nan::ThrowError("contextFromVector was not given a vector");
  }
}

void CompositorWrapper::contextFromDarkroom(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.contextFromDarkroom");

  v8::String::Utf8Value f(info[0]->ToString());
  Comp::Context ctx = c->_compositor->contextFromDarkroom(string(*f));

  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&ctx) };
  v8::Local<v8::Function> cons = Nan::New<v8::Function>(ContextWrapper::contextConstructor);

  info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
}

void CompositorWrapper::localImportance(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.localImportance");

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Object found is empty!");
  }
  ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());
  
  string size = "";

  if (info[1]->IsString()) {
    v8::String::Utf8Value i1(info[1]->ToString());
    size = string(*i1);
  }

  vector<Comp::Importance> imp = c->_compositor->localImportance(ctx->_context, size);

  v8::Local<v8::Array> ret = Nan::New<v8::Array>();
  int idx = 0;

  for (auto& i : imp) {
    v8::Local<v8::Object> impData = Nan::New<v8::Object>();

    impData->Set(Nan::New("layerName").ToLocalChecked(), Nan::New(i._layerName).ToLocalChecked());
    impData->Set(Nan::New("adjType").ToLocalChecked(), Nan::New(i._adjType));
    impData->Set(Nan::New("param").ToLocalChecked(), Nan::New(i._param).ToLocalChecked());
    impData->Set(Nan::New("depth").ToLocalChecked(), Nan::New(i._depth));
    impData->Set(Nan::New("totalAlpha").ToLocalChecked(), Nan::New(i._totalAlpha));
    impData->Set(Nan::New("totalLuma").ToLocalChecked(), Nan::New(i._totalLuma));
    impData->Set(Nan::New("deltaMag").ToLocalChecked(), Nan::New(i._deltaMag));
    impData->Set(Nan::New("mssim").ToLocalChecked(), Nan::New(i._mssim));

    ret->Set(idx, impData);
    idx++;
  }

  info.GetReturnValue().Set(ret);
}

void CompositorWrapper::importanceInRegion(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.importanceInRegion");

  // mode
  if (info[0]->IsString() && info[1]->IsObject()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string mode(*i0);

    v8::Local<v8::Object> region = info[1].As<v8::Object>();
    int x = region->Get(Nan::New("x").ToLocalChecked())->Int32Value();
    int y = region->Get(Nan::New("y").ToLocalChecked())->Int32Value();
    int w = region->Get(Nan::New("w").ToLocalChecked())->Int32Value();
    int h = region->Get(Nan::New("h").ToLocalChecked())->Int32Value();

    vector<double> scores;
    vector<string> names;
    c->_compositor->regionalImportance(mode, names, scores, x, y, w, h);

    v8::Local<v8::Array> ret = Nan::New<v8::Array>();

    for (int i = 0; i < scores.size(); i++) {
      v8::Local<v8::Object> obj = Nan::New<v8::Object>();

      obj->Set(Nan::New("name").ToLocalChecked(), Nan::New(names[i]).ToLocalChecked());
      obj->Set(Nan::New("score").ToLocalChecked(), Nan::New(scores[i]));
      ret->Set(Nan::New(i), obj);
    }

    info.GetReturnValue().Set(ret);
  }
}

void CompositorWrapper::pointImportance(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.pointImportance");

  // mode
  if (info[0]->IsString() && info[1]->IsObject() && info[2]->IsObject()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string mode(*i0);

    v8::Local<v8::Object> region = info[1].As<v8::Object>();
    int x = region->Get(Nan::New("x").ToLocalChecked())->Int32Value();
    int y = region->Get(Nan::New("y").ToLocalChecked())->Int32Value();

    Nan::MaybeLocal<v8::Object> maybe2 = Nan::To<v8::Object>(info[2]);
    if (maybe2.IsEmpty()) {
      Nan::ThrowError("Object found is empty!");
    }
    ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe2.ToLocalChecked());

    map<string, double> scores;
    c->_compositor->pointImportance(mode, scores, x, y, ctx->_context);

    v8::Local<v8::Array> ret = Nan::New<v8::Array>();

    int i = 0;
    for (auto& kvp : scores) {
      v8::Local<v8::Object> obj = Nan::New<v8::Object>();

      obj->Set(Nan::New("name").ToLocalChecked(), Nan::New(kvp.first).ToLocalChecked());
      obj->Set(Nan::New("score").ToLocalChecked(), Nan::New(kvp.second));
      ret->Set(Nan::New(i), obj);
      i++;
    }

    info.GetReturnValue().Set(ret);
  }
}

void CompositorWrapper::computeImportanceMap(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.computeImportanceMap");

  if (info[0]->IsString() && info[1]->IsNumber() && info[2]->IsObject()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    Comp::ImportanceMapMode mode = (Comp::ImportanceMapMode)(info[1]->IntegerValue());

    Nan::MaybeLocal<v8::Object> maybe2 = Nan::To<v8::Object>(info[2]);
    if (maybe2.IsEmpty()) {
      Nan::ThrowError("Object found is empty!");
    }
    ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe2.ToLocalChecked());

    // actually discard the returned result instead we use the JS constructor
    c->_compositor->computeImportanceMap(string(*i0), mode, ctx->_context);

    const int argc = 3;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(c->_compositor), info[0], info[1] };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImportanceMapWrapper::importanceMapConstructor);

    info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
  }
  else {
    Nan::ThrowError("compositor.computeImportanceMap(string, int, Context) argument error");
  }
}

void CompositorWrapper::computeAllImportanceMaps(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.computeAllImportanceMaps");

  if (info[0]->IsNumber() && info[1]->IsObject()) {
    Comp::ImportanceMapMode mode = (Comp::ImportanceMapMode)(info[0]->IntegerValue());

    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[1]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Object found is empty!");
    }
    ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    c->_compositor->computeAllImportanceMaps(mode, ctx->_context);
  }
  else {
    Nan::ThrowError("compositor.computeAllImportanceMaps(int, Context) argument error");
  }
}

void CompositorWrapper::getImportanceMap(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getImportanceMap");

  // existence check
  if (info[0]->IsString() && info[1]->IsNumber()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    if (c->_compositor->importanceMapExists(string(*i0), (Comp::ImportanceMapMode)(info[1]->IntegerValue()))) {
      const int argc = 3;
      v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(c->_compositor), info[0], info[1] };
      v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImportanceMapWrapper::importanceMapConstructor);

      info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
    }
  }
}

void CompositorWrapper::deleteImportanceMap(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.deleteImportanceMap");

  if (info[0]->IsString() && info[1]->IsNumber()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    c->_compositor->deleteImportanceMap(string(*i0), (Comp::ImportanceMapMode)info[1]->Int32Value());
  }
  else {
    Nan::ThrowError("compositor.deleteImporanceMap(string, int) argument error");
  }
}

void CompositorWrapper::deleteLayerImportanceMaps(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.deleteLayerImportanceMaps");

  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    c->_compositor->deleteLayerImportanceMaps(string(*i0));
  }
  else {
    Nan::ThrowError("compositor.deleteImportanceMap(string) argument error");
  }
}

void CompositorWrapper::deleteImportanceMapType(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.deleteImportanceMapType");

  if (info[0]->IsNumber()) {
    c->_compositor->deleteImportanceMapType((Comp::ImportanceMapMode)info[0]->Int32Value());
  }
  else {
    Nan::ThrowError("compositor.deleteImporanceMapType(int) argument error");
  }
}

void CompositorWrapper::deleteAllImportanceMaps(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.deleteAllImportanceMaps");

  c->_compositor->deleteAllImportanceMaps();
}

void CompositorWrapper::dumpImportanceMaps(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.dumpImportanceMaps");

  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());

    c->_compositor->dumpImportanceMaps(string(*i0));
  }
  else {
    Nan::ThrowError("compositor.dumpImportanceMaps(string) arugmnet error");
  }
}

void CompositorWrapper::availableImportanceMaps(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.availableImportanceMaps");

  auto cache = c->_compositor->getImportanceMapCache();
  v8::Local<v8::Object> maps = Nan::New<v8::Object>();
  
  for (auto& kvp : cache) {
    v8::Local<v8::Array> typeList = Nan::New<v8::Array>();
    int i = 0;
    for (auto& types : kvp.second) {
      typeList->Set(Nan::New(i), Nan::New(types.first));
      i++;
    }

    maps->Set(Nan::New(kvp.first).ToLocalChecked(), typeList);
  }

  info.GetReturnValue().Set(maps);
}

void CompositorWrapper::createClickMap(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.createClickMap");

  if (info[0]->IsNumber() && info[1]->IsObject()) {
    Comp::ImportanceMapMode mode = (Comp::ImportanceMapMode)(info[0]->IntegerValue());

    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[1]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Object found is empty!");
    }
    ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    Comp::ClickMap* cm = c->_compositor->createClickMap(mode, ctx->_context);

    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(cm) };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(ClickMapWrapper::clickMapConstructor);
    v8::Local<v8::Object> cmap = Nan::NewInstance(cons, argc, argv).ToLocalChecked();
    info.GetReturnValue().Set(cmap);
  }
  else {
    Nan::ThrowError("compositor.computeAllImportanceMaps(int, Context) argument error");
  }
}

void CompositorWrapper::analyzeAndTag(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.analyzeAndTag");

  c->_compositor->analyzeAndTag();
}

void CompositorWrapper::uniqueTags(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.uniqueTags");

  set<string> tags = c->_compositor->uniqueTags();

  v8::Local<v8::Array> ret = Nan::New<v8::Array>();

  int i = 0;
  for (auto& t : tags) {
    ret->Set(i, Nan::New(t).ToLocalChecked());
    i++;
  }

  info.GetReturnValue().Set(ret);
}

void CompositorWrapper::getTags(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getTags");

  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    set<string> tags = c->_compositor->getTags(string(*i0));

    v8::Local<v8::Array> ret = Nan::New<v8::Array>();

    int i = 0;
    for (auto& t : tags) {
      ret->Set(i, Nan::New(t).ToLocalChecked());
      i++;
    }

    info.GetReturnValue().Set(ret);
  }
  else {
    Nan::ThrowError("compositor.getTags(string) arugment error");
  }
}

void CompositorWrapper::allTags(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.allTags");

  map<string, set<string>> tags = c->_compositor->allTags();

  v8::Local<v8::Object> ret = Nan::New<v8::Object>();

  for (auto& kvp : tags) {
    v8::Local<v8::Array> t = Nan::New<v8::Array>();
    int i = 0;

    for (auto& tag : kvp.second) {
      t->Set(i, Nan::New(tag).ToLocalChecked());
      i++;
    }

    ret->Set(Nan::New(kvp.first).ToLocalChecked(), t);
  }

  info.GetReturnValue().Set(ret);
}

void CompositorWrapper::deleteTags(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.deleteTags");

  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    c->_compositor->deleteTags(string(*i0));
  }
  else {
    Nan::ThrowError("compositor.deleteTags(string) arugment error");
  }
}

void CompositorWrapper::deleteAllTags(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.deleteAllTags");

  c->_compositor->deleteAllTags();
}

void CompositorWrapper::hasTag(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.hasTag");

  if (info[0]->IsString() && info[1]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    v8::String::Utf8Value i1(info[1]->ToString());

    bool ret = c->_compositor->hasTag(string(*i0), string(*i1));
    info.GetReturnValue().Set(Nan::New(ret));
  }
  else {
    Nan::ThrowError("compositor.hasTag(string, string) arugment error");
  }
}

void CompositorWrapper::goalSelect(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // expects a goal object, context, and selection point
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.goalSelect");

  if (info[0]->IsObject() && info[1]->IsObject() && info[2]->IsNumber() && info[3]->IsNumber()) {
    // info[0] should contain a goal object
    v8::Local<v8::Object> goal = info[0].As<v8::Object>();
    Comp::GoalType gt = (Comp::GoalType)(goal->Get(Nan::New("type").ToLocalChecked())->IntegerValue());
    Comp::GoalTarget ga = (Comp::GoalTarget)(goal->Get(Nan::New("target").ToLocalChecked())->IntegerValue());
    v8::Local<v8::Object> color = goal->Get(Nan::New("color").ToLocalChecked()).As<v8::Object>();

    Comp::RGBAColor targetColor;
    targetColor._r = color->Get(Nan::New("r").ToLocalChecked())->NumberValue();
    targetColor._g = color->Get(Nan::New("g").ToLocalChecked())->NumberValue();
    targetColor._b = color->Get(Nan::New("b").ToLocalChecked())->NumberValue();
    targetColor._a = 1;

    Comp::Goal g(gt, ga);
    g.setTargetColor(targetColor);

    // context
    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[1]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Object found is empty!");
    }
    ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    // targets
    int x = info[2]->IntegerValue();
    int y = info[3]->IntegerValue();

    map<string, map<Comp::AdjustmentType, vector<Comp::GoalResult>>> results;
    if (info[4]->IsNumber() && info[5]->IsNumber()) {
      int w = info[4]->IntegerValue();
      int h = info[5]->IntegerValue();

      int maxLevel = 100;
      if (info[6]->IsNumber()) {
        maxLevel = info[6]->IntegerValue();
      }

      results = c->_compositor->goalSelect(g, ctx->_context, x, y, w, h, maxLevel);
    }
    else {
      int maxLevel = 100;
      if (info[4]->IsNumber())
        maxLevel = info[4]->IntegerValue();

      results = c->_compositor->goalSelect(g, ctx->_context, x, y, maxLevel);
    }

    // results to javascript object fun times
    v8::Local<v8::Object> ret = Nan::New<v8::Object>();
    for (auto& r : results) {
      v8::Local<v8::Object> adjustments = Nan::New<v8::Object>();

      for (auto& a : r.second) {
        v8::Local<v8::Array> params = Nan::New<v8::Array>();

        for (int i = 0; i < a.second.size(); i++) {
          v8::Local<v8::Object> gr = Nan::New<v8::Object>();
          gr->Set(Nan::New("param").ToLocalChecked(), Nan::New(a.second[i]._param).ToLocalChecked());
          gr->Set(Nan::New("val").ToLocalChecked(), Nan::New(a.second[i]._val));

          params->Set(i, gr);
        }

        adjustments->Set(Nan::New(a.first), params);
      }

      ret->Set(Nan::New(r.first).ToLocalChecked(), adjustments);
    }

    info.GetReturnValue().Set(ret);
  }
  else {
    Nan::ThrowError("compositor.goalSelect(object, context, int, int[, int, int]) argument error");
  }
}

void CompositorWrapper::addMask(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  if (!info[0]->IsString() || !info[1]->IsString()) {
    Nan::ThrowError("addMask expects (string, string)");
  }

  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.addLayer");

  bool result;
  v8::String::Utf8Value val0(info[0]->ToString());
  string name(*val0);

  v8::String::Utf8Value val1(info[1]->ToString());
  string path(*val1);

  result = c->_compositor->addLayerMask(name, path);

  info.GetReturnValue().Set(Nan::New(result));
}

void CompositorWrapper::addPoissonDiskCache(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.addPoissonDiskCache");

  if (info[0]->IsNumber() && info[1]->IsNumber()) {
    int n = info[0]->IntegerValue();
    int level = info[1]->IntegerValue();
    int k = 30;

    if (info[2]->IsNumber()) {
      k = info[2]->NumberValue();
    }

    c->_compositor->initPoissonDisk(n, level, k);
  }
}

void CompositorWrapper::initPoissonDisks(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.addPoissonDiskCache");

  // runs the sample pattern generation function
  c->_compositor->initPoissonDisks();
}

void CompositorWrapper::addGroup(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.addGroup");

  if (info[0]->IsString() && info[1]->IsArray() && info[2]->IsNumber()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string name(*i0);

    set<string> layers;
    v8::Local<v8::Array> arr = info[1].As<v8::Array>();
    for (int i = 0; i < arr->Length(); i++) {
      v8::String::Utf8Value av(arr->Get(i)->ToString());
      string lname(*av);

      layers.insert(lname);
    }

    float priority = info[2]->NumberValue();

    bool readOnly = false;
    if (info[3]->IsBoolean())
      readOnly = info[3]->BooleanValue();

    bool res = c->_compositor->addGroup(name, layers, priority, readOnly);
    info.GetReturnValue().Set(Nan::New(res));
  }
  else {
    Nan::ThrowError("addGroup(string, string[], float[, bool) arugment error");
  }
}

void CompositorWrapper::addGroupEffect(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.addGroupEffect");

  if (info[0]->IsString() && info[1]->IsObject()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string group(*i0);

    // bunch of options here
    v8::Local<v8::Object> obj = info[1].As<v8::Object>();

    // must have mode
    if (!Nan::HasOwnProperty(obj, Nan::New("mode").ToLocalChecked()).ToChecked()) {
      Nan::ThrowError("Group effect needs a mode");
    }

    Comp::ImageEffect effect;
    int mode = obj->Get(Nan::New("mode").ToLocalChecked())->IntegerValue();
    effect._mode = (Comp::EffectMode)mode;

    if (mode == Comp::EffectMode::STROKE) {
      if (!Nan::HasOwnProperty(obj, Nan::New("width").ToLocalChecked()).ToChecked()) {
        Nan::ThrowError("Stroke effect needs a width");
      }
      if (!Nan::HasOwnProperty(obj, Nan::New("color").ToLocalChecked()).ToChecked()) {
        Nan::ThrowError("Stroke effect needs a color");
      }

      effect._width = obj->Get(Nan::New("width").ToLocalChecked())->IntegerValue();

      auto rgb = obj->Get(Nan::New("color").ToLocalChecked()).As<v8::Object>();
      effect._color._r = rgb->Get(Nan::New("r").ToLocalChecked())->NumberValue();
      effect._color._g = rgb->Get(Nan::New("g").ToLocalChecked())->NumberValue();
      effect._color._b = rgb->Get(Nan::New("b").ToLocalChecked())->NumberValue();
    }

    c->_compositor->setGroupEffect(group, effect);
  }
  else {
    Nan::ThrowError("addGroupEffect(string, object) argument error");
  }
}

void CompositorWrapper::addGroupFromExistingLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.addGroupFromExistingLayer");

  if (info[0]->IsString() && info[1]->IsArray() && info[2]->IsNumber()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string name(*i0);

    set<string> layers;
    v8::Local<v8::Array> arr = info[1].As<v8::Array>();
    for (int i = 0; i < arr->Length(); i++) {
      v8::String::Utf8Value av(arr->Get(i)->ToString());
      string lname(*av);

      layers.insert(lname);
    }

    float priority = info[2]->NumberValue();

    bool readOnly = false;
    if (info[3]->IsBoolean())
      readOnly = info[3]->BooleanValue();

    bool res = c->_compositor->addGroupFromExistingLayer(name, layers, priority, readOnly);
    info.GetReturnValue().Set(Nan::New(res));
  }
  else {
    Nan::ThrowError("addGroupFrom Existing Layer(string, string[], float[, bool) arugment error");
  }
}

void CompositorWrapper::deleteGroup(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.deleteGroup");

  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string name(*i0);

    c->_compositor->deleteGroup(name);
  }
  else {
    Nan::ThrowError("deleteGroup(string) arugment error");
  }
}

void CompositorWrapper::addLayerToGroup(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.addLayerToGroup");

  if (info[0]->IsString() && info[1]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    v8::String::Utf8Value i1(info[1]->ToString());

    string layer(*i0);
    string group(*i1);

    c->_compositor->addLayerToGroup(layer, group);
  }
  else {
    Nan::ThrowError("addLayerToGroup(string, string) argument error");
  }
}

void CompositorWrapper::removeLayerFromGroup(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.removeLayerFromGroup");

  if (info[0]->IsString() && info[1]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    v8::String::Utf8Value i1(info[1]->ToString());

    string layer(*i0);
    string group(*i1);

    c->_compositor->removeLayerFromGroup(layer, group);
  }
  else {
    Nan::ThrowError("removeLayerFromGroup(string, string) argument error");
  }
}

void CompositorWrapper::setGroupOrder(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.setGroupOrder");

  if (info[0]->IsArray()) {
    // assumes array of object pairs
    v8::Local<v8::Array> vals = info[0].As<v8::Array>();
    multimap<float, string> order;
    for (int i = 0; i < vals->Length(); i++) {
      v8::Local<v8::Object> obj = vals->Get(i).As<v8::Object>();

      float val = obj->Get(Nan::New("val").ToLocalChecked())->NumberValue();
      v8::String::Utf8Value n(obj->Get(Nan::New("group").ToLocalChecked())->ToString());
      string group(*n);

      order.insert(make_pair(val, group));
    }

    c->_compositor->setGroupOrder(order);
  }
  else if (info[0]->IsString() && info[1]->IsNumber()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string group(*i0);

    float priority = info[1]->NumberValue();

    c->_compositor->setGroupOrder(group, priority);
  }
  else {
    Nan::ThrowError("setGroupOrder argument error");
  }
}

void CompositorWrapper::getGroupOrder(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getGroupOrder");

  multimap<float, string> order = c->_compositor->getGroupOrder();

  v8::Local<v8::Array> ret = Nan::New<v8::Array>();
  int i = 0;
  for (auto& o : order) {
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    obj->Set(Nan::New("val").ToLocalChecked(), Nan::New(o.first));
    obj->Set(Nan::New("group").ToLocalChecked(), Nan::New(o.second).ToLocalChecked());
    ret->Set(i, obj);
    i++;
  }

  info.GetReturnValue().Set(ret);
}

void CompositorWrapper::getGroup(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getGroup");

  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string name(*i0);
    Comp::Group g = c->_compositor->getGroup(name);

    v8::Local<v8::Object> ret = Nan::New<v8::Object>();
    ret->Set(Nan::New("name").ToLocalChecked(), Nan::New(g._name).ToLocalChecked());
    ret->Set(Nan::New("readOnly").ToLocalChecked(), Nan::New(g._readOnly));

    v8::Local<v8::Array> layers = Nan::New<v8::Array>();
    int i = 0;
    for (auto& l : g._affectedLayers) {
      layers->Set(Nan::New(i), Nan::New(l).ToLocalChecked());
      i++;
    }
    ret->Set(Nan::New("affectedLayers").ToLocalChecked(), layers);

    v8::Local<v8::Object> effect = Nan::New<v8::Object>();
    effect->Set(Nan::New("mode").ToLocalChecked(), Nan::New(g._effect._mode));
    effect->Set(Nan::New("width").ToLocalChecked(), Nan::New(g._effect._width));

    v8::Local<v8::Object> rgb = Nan::New<v8::Object>();
    rgb->Set(Nan::New("r").ToLocalChecked(), Nan::New(g._effect._color._r));
    rgb->Set(Nan::New("g").ToLocalChecked(), Nan::New(g._effect._color._g));
    rgb->Set(Nan::New("b").ToLocalChecked(), Nan::New(g._effect._color._b));
    effect->Set(Nan::New("color").ToLocalChecked(), rgb);
    ret->Set(Nan::New("effect").ToLocalChecked(), effect);

    info.GetReturnValue().Set(ret);
  }
  else {
    Nan::ThrowError("getGroup(string) argument error");
  }
}

void CompositorWrapper::setGroupLayers(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.setGroupLayers");

  if (info[0]->IsString() && info[1]->IsArray()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string group(*i0);

    v8::Local<v8::Array> arr = info[1].As<v8::Array>();

    set<string> layers;
    for (int i = 0; i < arr->Length(); i++) {
      v8::String::Utf8Value str(arr->Get(i)->ToString());
      layers.insert(string(*str));
    }

    c->_compositor->setGroupLayers(group, layers);
  }
  else {
    Nan::ThrowError("setGroupLayers(string, string[]) arugment error");
  }
}

void CompositorWrapper::layerInGroup(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.layerInGroup");

  if (info[0]->IsString() && info[1]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string layer(*i0);

    v8::String::Utf8Value i1(info[1]->ToString());
    string group(*i1);

    info.GetReturnValue().Set(Nan::New(c->_compositor->layerInGroup(layer, group)));
  }
  else {
    Nan::ThrowError("layerInGroup(string, string) argument error");
  }
}

void CompositorWrapper::isGroup(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.isGroup");

  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string group(*i0);

    info.GetReturnValue().Set(Nan::New(c->_compositor->isGroup(group)));
  }
}

void CompositorWrapper::renderUpToLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.renderUpToLayer");
  
  if (info[0]->IsObject() && info[1]->IsString() && info[2]->IsString() && info[3]->IsNumber()) {
    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Object found is empty!");
    }
    ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    v8::String::Utf8Value i1(info[1]->ToString());
    string layer(*i1);
    v8::String::Utf8Value i2(info[2]->ToString());
    string precomp(*i2);

    float dim = info[3]->NumberValue();
    
    string size = "full";
    if (info[4]->IsString()) {
      v8::String::Utf8Value i3(info[3]->ToString());
      size = string(*i3);
    }

    Comp::Image* img = c->_compositor->renderUpToLayer(ctx->_context, layer, precomp, dim, size);

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImageWrapper::imageConstructor);
    const int argc = 2;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(img), Nan::New(true) };

    info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
  }
  else {
    Nan::ThrowError("renderUpToLayer(context, string, float[, string]) argument error");
  }
}

void CompositorWrapper::asyncRenderUpToLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.asyncRenderUpToLayer");

  if (info[0]->IsObject() && info[1]->IsString() && info[2]->IsString() && info[3]->IsNumber() && info[4]->IsString() && info[5]->IsFunction()) {
    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Object found is empty!");
    }
    ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    v8::String::Utf8Value i1(info[1]->ToString());
    string layer(*i1);
    v8::String::Utf8Value i2(info[2]->ToString());
    string pc(*i2);

    float dim = info[3]->NumberValue();

    v8::String::Utf8Value i3(info[4]->ToString());
    string size(*i3);

    Nan::Callback* callback = new Nan::Callback(info[5].As<v8::Function>());
    Nan::AsyncQueueWorker(new RenderWorker(callback, size, c->_compositor, ctx->_context, layer, pc, dim));
  }
  else {
    Nan::ThrowError("asyncRenderUpToLayer(context, string, float, string, function(image)) argument error");
  }
}

void CompositorWrapper::getFlatLayerOrder(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getFlatLayerOrder");

  vector<string> order = c->_compositor->getFlatLayerOrder();
  v8::Local<v8::Array> ret = Nan::New<v8::Array>();

  for (int i = 0; i < order.size(); i++) {
    ret->Set(i, Nan::New(order[i]).ToLocalChecked());
  }

  info.GetReturnValue().Set(ret);
}

void CompositorWrapper::getModifierOrder(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getModifierOrder");

  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());

    vector<string> order = c->_compositor->getModifierOrder(string(*i0));
    v8::Local<v8::Array> ret = Nan::New<v8::Array>();

    for (int i = 0; i < order.size(); i++) {
      ret->Set(i, Nan::New(order[i]).ToLocalChecked());
    }

    info.GetReturnValue().Set(ret);
  }
  else {
    Nan::ThrowError("getModifierOrder(string), argument error");
  }

}

void CompositorWrapper::offsetLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.offsetLayer");

  if (info[0]->IsString() && info[1]->IsNumber() && info[2]->IsNumber()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    c->_compositor->offsetLayer(string(*i0), info[1]->NumberValue(), info[2]->NumberValue());
  }
  else {
    Nan::ThrowError("offsetLayer(string, float, float) argument error");
  }
}

void CompositorWrapper::resetLayerOffset(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.offsetLayer");

  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    c->_compositor->resetLayerOffset(string(*i0));
  }
  else {
    Nan::ThrowError("resetLayerOffset(string) argument error");
  }
}

void CompositorWrapper::layerHistogramIntersect(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.layerHistogramIntersect");

  if (info[0]->IsObject() && info[1]->IsString() && info[2]->IsString()) {
    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Object found is empty!");
    }
    ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    v8::String::Utf8Value i0(info[1]->ToString());
    v8::String::Utf8Value i1(info[2]->ToString());

    float binSize = 0.05;
    if (info[3]->IsNumber()) {
      binSize = info[3]->NumberValue();
    }

    string size = "full";
    if (info[4]->IsString()) {
      v8::String::Utf8Value i3(info[4]->ToString());
      size = string(*i3);
    }

    double val = c->_compositor->layerHistogramIntersect(ctx->_context, string(*i0), string(*i1), binSize, size);
    
    if (val < 0) {
      Nan::ThrowError("LayerHistogramIntersect returned invalid result. Check Compositor log");
    }
    else {
      info.GetReturnValue().Set(Nan::New(val));
    }
  }
  else {
    Nan::ThrowError("layerHistogramIntersect(context, string, string[, float, string]) argument error");
  }
}

void CompositorWrapper::propLayerHistogramIntersect(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.propLayerHistogramIntersect");

  if (info[0]->IsObject() && info[1]->IsString() && info[2]->IsString()) {
    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Object found is empty!");
    }
    ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    v8::String::Utf8Value i0(info[1]->ToString());
    v8::String::Utf8Value i1(info[2]->ToString());

    float binSize = 0.05;
    if (info[3]->IsNumber()) {
      binSize = info[3]->NumberValue();
    }

    string size = "full";
    if (info[4]->IsString()) {
      v8::String::Utf8Value i3(info[4]->ToString());
      size = string(*i3);
    }

    double val = c->_compositor->propLayerHistogramIntersect(ctx->_context, string(*i0), string(*i1), binSize, size);

    if (val < 0) {
      Nan::ThrowError("LayerHistogramIntersect returned invalid result. Check Compositor log");
    }
    else {
      info.GetReturnValue().Set(Nan::New(val));
    }
  }
  else {
    Nan::ThrowError("propLayerHistogramIntersect(context, string, string[, float, string]) argument error");
  }
}

void CompositorWrapper::getGroupInclusionMap(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.getGroupInclusionMap");

  if (info[0]->IsObject() && info[1]->IsString()) {
    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Object found is empty!");
    }
    ImageWrapper* ctx = Nan::ObjectWrap::Unwrap<ImageWrapper>(maybe1.ToLocalChecked());

    v8::String::Utf8Value i1(info[1]->ToString());

    Comp::Image* img = c->_compositor->getGroupInclusionMap(ctx->_image, string(*i1));

    // construct the image
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImageWrapper::imageConstructor);
    const int argc = 2;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(img), Nan::New(true) };

    info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
  }
  else {
    Nan::ThrowError("getGroupInclusionMap(image, string) argument error");
  }
}

void CompositorWrapper::renderOnlyLayer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  CompositorWrapper* c = ObjectWrap::Unwrap<CompositorWrapper>(info.Holder());
  nullcheck(c->_compositor, "compositor.renderOnlyLayer");

  if (info[0]->IsObject() && info[1]->IsString()) {
    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Object found is empty!");
    }
    ContextWrapper* ctx = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    v8::String::Utf8Value i1(info[1]->ToString());
    string layer(*i1);

    string size = "";

    if (info[2]->IsString()) {
      v8::String::Utf8Value i2(info[2]->ToString());
      size = string(*i2);
    }

    vector<string> renderOrder;
    renderOrder.push_back(layer);

    Comp::Image* r = c->_compositor->render(ctx->_context, nullptr, renderOrder, 1, size);

    v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImageWrapper::imageConstructor);
    const int argc = 2;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(r), Nan::New(true) };

    info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
  }
  else {
    Nan::ThrowError("renderOnlyLayer(Context, string, string) argument error");
  }
}

RenderWorker::RenderWorker(Nan::Callback * callback, string size, Comp::Compositor * c) :
  Nan::AsyncWorker(callback), _size(size), _c(c)
{
  _customContext = false;
  _dim = -1;
}

RenderWorker::RenderWorker(Nan::Callback * callback, string size, Comp::Compositor * c, Comp::Context ctx):
  Nan::AsyncWorker(callback), _size(size), _c(c), _ctx(ctx)
{
  _customContext = true;
  _dim = -1;
}

RenderWorker::RenderWorker(Nan::Callback * callback, string size, Comp::Compositor * c, Comp::Context ctx, string layer, string pc, float dim) :
  Nan::AsyncWorker(callback), _size(size), _c(c), _ctx(ctx), _dim(dim), _layer(layer), _pc(pc)
{
  _customContext = true;
}

void RenderWorker::Execute()
{
  if (_customContext) {
    if (_dim < 0) {
      _img = _c->render(_ctx, _size);
    }
    else {
      _img = _c->renderUpToLayer(_ctx, _layer, _pc, _dim, _size);
    }
  }
  else
    _img = _c->render(_size);
}

void RenderWorker::HandleOKCallback()
{
  Nan::HandleScope scope;

  const int argc = 2;
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(_img), Nan::New(true) };
  v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImageWrapper::imageConstructor);
  v8::Local<v8::Object> imgInst = Nan::NewInstance(cons, argc, argv).ToLocalChecked();

  v8::Local<v8::Value> cb[] = { Nan::Null(), imgInst };
  callback->Call(2, cb);
}

StopSearchWorker::StopSearchWorker(Nan::Callback * callback, Comp::Compositor * c):
  Nan::AsyncWorker(callback), _c(c)
{
}

void StopSearchWorker::Execute()
{
  // this could take a while
  _c->stopSearch();
}

void StopSearchWorker::HandleOKCallback()
{
  Nan::HandleScope scope;

  v8::Local<v8::Value> cb[] = { Nan::Null() };
  callback->Call(1, cb);
}

void ClickMapWrapper::Init(v8::Local<v8::Object> exports)
{
  Nan::HandleScope scope;

  // constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("ClickMap").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "init", init);
  Nan::SetPrototypeMethod(tpl, "compute", compute);
  Nan::SetPrototypeMethod(tpl, "visualize", visualize);
  Nan::SetPrototypeMethod(tpl, "active", active);

  clickMapConstructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("ClickMap").ToLocalChecked(), tpl->GetFunction());
}

ClickMapWrapper::ClickMapWrapper(Comp::ClickMap* clmap) : _map(clmap) {

}

ClickMapWrapper::~ClickMapWrapper() {
  delete _map;
  _map = nullptr;
}

void ClickMapWrapper::New(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  if (info[0]->IsExternal()) {
    Comp::ClickMap* c = static_cast<Comp::ClickMap*>(info[0].As<v8::External>()->Value());
    ClickMapWrapper* cm = new ClickMapWrapper(c);
    cm->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  }
  else {
    Nan::ThrowError("ClickMap constructor failure. No ClickMap object found.");
  }
}

void ClickMapWrapper::init(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ClickMapWrapper* c = ObjectWrap::Unwrap<ClickMapWrapper>(info.Holder());
  nullcheck(c->_map, "ClickMap.init");

  if (info[0]->IsNumber() && info[1]->IsBoolean()) {
    c->_map->init(info[0]->NumberValue(), info[1]->BooleanValue());
  }
  else {
    Nan::ThrowError("ClickMap.init(float, bool) argument error");
  }
}

void ClickMapWrapper::compute(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ClickMapWrapper* c = ObjectWrap::Unwrap<ClickMapWrapper>(info.Holder());
  nullcheck(c->_map, "ClickMap.compute");

  if (info[0]->IsNumber()) {
    c->_map->compute(info[0]->IntegerValue());
  }
  else {
    Nan::ThrowError("ClickMap.compute(int) argument error");
  }
}

void ClickMapWrapper::visualize(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ClickMapWrapper* c = ObjectWrap::Unwrap<ClickMapWrapper>(info.Holder());
  nullcheck(c->_map, "ClickMap.visualize");

  if (info[0]->IsNumber()) {
    Comp::Image* img = c->_map->visualize((Comp::ClickMap::VisualizationType)(info[0]->IntegerValue()));

    const int argc = 2;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(img), Nan::New(true) };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(ImageWrapper::imageConstructor);
    v8::Local<v8::Object> image = Nan::NewInstance(cons, argc, argv).ToLocalChecked();
    info.GetReturnValue().Set(image);
  }
  else {
    Nan::ThrowError("ClickMap.visualize(int) argument error");
  }
}

void ClickMapWrapper::active(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ClickMapWrapper* c = ObjectWrap::Unwrap<ClickMapWrapper>(info.Holder());
  nullcheck(c->_map, "ClickMap.active");

  if (info[0]->IsNumber() && info[1]->IsNumber()) {
    vector<string> names = c->_map->activeLayers(info[0]->IntegerValue(), info[1]->IntegerValue());

    v8::Local<v8::Array> ret = Nan::New<v8::Array>();

    for (int i = 0; i < names.size(); i++) {
      ret->Set(i, Nan::New(names[i]).ToLocalChecked());
    }

    info.GetReturnValue().Set(ret);
  }
  else {
    Nan::ThrowError("ClickMap.active(int, int) argument error");
  }
}

void ModelWrapper::Init(v8::Local<v8::Object> exports)
{
  Nan::HandleScope scope;

  // constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Model").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "analyze", analyze);
  Nan::SetPrototypeMethod(tpl, "report", report);
  Nan::SetPrototypeMethod(tpl, "sample", sample);
  Nan::SetPrototypeMethod(tpl, "nonParametricLocalSample", nonParametricSample);
  Nan::SetPrototypeMethod(tpl, "addSchema", addSchema);
  Nan::SetPrototypeMethod(tpl, "schemaSample", schemaSample);
  Nan::SetPrototypeMethod(tpl, "getInputData", getInputData);
  Nan::SetPrototypeMethod(tpl, "addSlider", addSlider);
  Nan::SetPrototypeMethod(tpl, "sliderSample", sliderSample);
  Nan::SetPrototypeMethod(tpl, "exportSliderGraph", exportSliderGraph);
  Nan::SetPrototypeMethod(tpl, "addSliderFromExamples", addSliderFromExamples);

  modelConstructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("Model").ToLocalChecked(), tpl->GetFunction());
}

ModelWrapper::ModelWrapper() : _model(nullptr)
{
}

ModelWrapper::~ModelWrapper()
{
  delete _model;
}

void ModelWrapper::New(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // need a compositor arg
  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Internal Error: Compositor object found is empty!");
  }
  CompositorWrapper* c = Nan::ObjectWrap::Unwrap<CompositorWrapper>(maybe1.ToLocalChecked());

  ModelWrapper* mw = new ModelWrapper();
  mw->_model = new Comp::Model(c->_compositor);
  mw->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

void ModelWrapper::analyze(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ModelWrapper* m = ObjectWrap::Unwrap<ModelWrapper>(info.Holder());
  nullcheck(m->_model, "model.analyze");

  if (!info.Length() == 1 || !info[0]->IsObject()) {
    Nan::ThrowError("analyze(object) argument error");
  }
  else {
    // object expected format: { axisName : [array of filenames] }
    v8::Local<v8::Object> data = info[0].As<v8::Object>();

    map<string, vector<string>> analysisData;
    auto keys = data->GetOwnPropertyNames();
    for (int i = 0; i < keys->Length(); i++) {
      v8::Local<v8::Array> filenames = data->Get(keys->Get(i)).As<v8::Array>();

      vector<string> files;
      for (int j = 0; j < filenames->Length(); j++) {
        v8::String::Utf8Value val0(filenames->Get(j)->ToString());
        string file(*val0);

        files.push_back(file);
      }

      v8::String::Utf8Value axisName(keys->Get(i)->ToString());
      string axis(*axisName);

      analysisData[axis] = files;
    }

    m->_model->analyze(analysisData);
  }
}

void ModelWrapper::report(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ModelWrapper* m = ObjectWrap::Unwrap<ModelWrapper>(info.Holder());
  nullcheck(m->_model, "model.report");

  // basically returns the trainInfo structure
  const map<string, Comp::ModelInfo> data = m->_model->getModelInfo();

  v8::Local<v8::Object> rep = Nan::New<v8::Object>();

  for (auto& axis : data) {
    string name = axis.first;

    v8::Local<v8::Object> axisData = Nan::New<v8::Object>();

    for (auto& y : axis.second._activeParams) {
      // extract param info for each active parameter
      v8::Local<v8::Object> paramData = Nan::New<v8::Object>();

      paramData->Set(Nan::New("layerName").ToLocalChecked(), Nan::New(y.second._name).ToLocalChecked());
      paramData->Set(Nan::New("paramName").ToLocalChecked(), Nan::New(y.second._param).ToLocalChecked());
      paramData->Set(Nan::New("adjustmentType").ToLocalChecked(), Nan::New(y.second._type));
      paramData->Set(Nan::New("min").ToLocalChecked(), Nan::New(y.second._min));
      paramData->Set(Nan::New("max").ToLocalChecked(), Nan::New(y.second._max));

      // value array
      v8::Local<v8::Array> valArray = Nan::New<v8::Array>();
      for (int i = 0; i < y.second._vals.size(); i++) {
        valArray->Set(i, Nan::New(y.second._vals[i]));
      }

      paramData->Set(Nan::New("vals").ToLocalChecked(), valArray);

      // add the parameter info the the axis object
      axisData->Set(Nan::New(y.first).ToLocalChecked(), paramData);
    }

    rep->Set(Nan::New(name).ToLocalChecked(), axisData);
  }

  info.GetReturnValue().Set(rep);
}

void ModelWrapper::sample(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ModelWrapper* m = ObjectWrap::Unwrap<ModelWrapper>(info.Holder());
  nullcheck(m->_model, "model.sample");

  Comp::Context ctx = m->_model->sample();

  // context object
  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&ctx) };
  v8::Local<v8::Function> cons = Nan::New<v8::Function>(ContextWrapper::contextConstructor);
  v8::Local<v8::Object> ctxInst = Nan::NewInstance(cons, argc, argv).ToLocalChecked();

  info.GetReturnValue().Set(ctxInst);
}

void ModelWrapper::nonParametricSample(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ModelWrapper* m = ObjectWrap::Unwrap<ModelWrapper>(info.Holder());
  nullcheck(m->_model, "model.nonParametricSample");

  v8::Local<v8::Object> axisData = info[0].As<v8::Object>();
  map<string, Comp::Context> axes;
  v8::Local<v8::Array> props = axisData->GetOwnPropertyNames();
  
  for (int i = 0; i < props->Length(); i++) {
    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(axisData->Get(props->Get(i)));
    if (maybe1.IsEmpty()) {
      continue;
    }
    ContextWrapper* c = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());
    v8::String::Utf8Value prop(props->Get(i)->ToString());
    string propName(*prop);

    axes[propName] = c->_context;
  }

  float alpha = 1;
  if (info[1]->IsNumber()) {
    alpha = info[1]->NumberValue();
  }

  int k = -1;
  if (info[2]->IsNumber()) {
    k = info[2]->Int32Value();
  }

  Comp::Context ctx = m->_model->nonParametricLocalSample(axes, alpha, k);

  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&ctx) };
  v8::Local<v8::Function> cons = Nan::New<v8::Function>(ContextWrapper::contextConstructor);
  v8::Local<v8::Object> ctxInst = Nan::NewInstance(cons, argc, argv).ToLocalChecked();

  info.GetReturnValue().Set(ctxInst);
}

void ModelWrapper::addSchema(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ModelWrapper* m = ObjectWrap::Unwrap<ModelWrapper>(info.Holder());
  nullcheck(m->_model, "model.addSchema");

  if (info[0]->IsString()) {
    v8::String::Utf8Value val0(info[0]->ToString());
    string file(*val0);

    m->_model->addSchema(file);
  }
  else {
    Nan::ThrowError("addSchema(string) argument error");
  }
}

void ModelWrapper::schemaSample(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // second parameter is a vector of objects that need to be converted to internal rep
  ModelWrapper* m = ObjectWrap::Unwrap<ModelWrapper>(info.Holder());
  nullcheck(m->_model, "model.schemaSample");

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Internal Error: Compositor object found is empty!");
  }
  ContextWrapper* c = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

  // convert vector to c++ vector
  v8::Local<v8::Array> i1 = info[1].As<v8::Array>();
  vector<Comp::AxisConstraint> ac;
  for (int i = 0; i < i1->Length(); i++) {
    v8::Local<v8::Object> co = i1->Get(i).As<v8::Object>();
    
    Comp::AxisConstraint constraint;
    v8::Local<v8::Array> keys = co->GetOwnPropertyNames();
    for (int j = 0; j < keys->Length(); j++) {
      v8::String::Utf8Value prop(keys->Get(j)->ToString());
      string propName(*prop);

      if (propName == "mode") {
        constraint._mode = (Comp::AxisConstraintMode)co->Get(keys->Get(j))->Int32Value();
      }
      else if (propName == "axis") {
        v8::String::Utf8Value val(co->Get(keys->Get(j))->ToString());
        constraint._axis = string(*val);
      }
      else if (propName == "min") {
        constraint._min = (float)co->Get(keys->Get(j))->NumberValue();
      }
      else if (propName == "max") {
        constraint._max = (float)co->Get(keys->Get(j))->NumberValue();
      }
      else if (propName == "layer") {
        v8::String::Utf8Value val(co->Get(keys->Get(j))->ToString());
        constraint._layer = string(*val);
      }
      else if (propName == "type") {
        constraint._type = (Comp::AdjustmentType)co->Get(keys->Get(j))->Int32Value();
      }
      else if (propName == "param") {
        v8::String::Utf8Value val(co->Get(keys->Get(j))->ToString());
        constraint._param = string(*val);
      }
      else if (propName == "val") {
        constraint._val = (float)co->Get(keys->Get(j))->NumberValue();
      }
      else if (propName == "tolerance") {
        constraint._tolerance = (float)co->Get(keys->Get(j))->NumberValue();
      }
    }

    ac.push_back(constraint);
  }

  Comp::Context ctx = m->_model->schemaSample(c->_context, ac);

  map<string, float> axisVals = m->_model->schemaEval(ctx);

  v8::Local<v8::Object> meta = Nan::New<v8::Object>();
  for (auto& axis : axisVals) {
    meta->Set(Nan::New(axis.first).ToLocalChecked(), Nan::New(axis.second));
  }

  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&ctx) };
  v8::Local<v8::Function> cons = Nan::New<v8::Function>(ContextWrapper::contextConstructor);
  v8::Local<v8::Object> ctxInst = Nan::NewInstance(cons, argc, argv).ToLocalChecked();

  v8::Local<v8::Object> ret = Nan::New<v8::Object>();
  ret->Set(Nan::New("context").ToLocalChecked(), ctxInst);
  ret->Set(Nan::New("metadata").ToLocalChecked(), meta);

  info.GetReturnValue().Set(ret);
}

void ModelWrapper::getInputData(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ModelWrapper* m = ObjectWrap::Unwrap<ModelWrapper>(info.Holder());
  nullcheck(m->_model, "model.getInputData");

  v8::Local<v8::Object> ret = Nan::New<v8::Object>();
  auto data = m->_model->getInputData();
  const int argc = 1;

  for (auto& d : data) {
    v8::Local<v8::Array> contexts = Nan::New<v8::Array>();

    for (int i = 0; i < d.second.size(); i++) {
      Comp::Context c = d.second[i];

      v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&c) };
      v8::Local<v8::Function> cons = Nan::New<v8::Function>(ContextWrapper::contextConstructor);
      v8::Local<v8::Object> ctxInst = Nan::NewInstance(cons, argc, argv).ToLocalChecked();

      contexts->Set(i, ctxInst);
    }

    ret->Set(Nan::New(d.first).ToLocalChecked(), contexts);
  }

  info.GetReturnValue().Set(ret);
}

void ModelWrapper::addSlider(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // input is a name, array of objects, and an optional ordering
  ModelWrapper* m = ObjectWrap::Unwrap<ModelWrapper>(info.Holder());
  nullcheck(m->_model, "model.addSlider");

  string name;
  vector<Comp::LayerParamInfo> params;
  vector<shared_ptr<Comp::ParamFunction>> funcs;

  if (info[0]->IsString() && info[1]->IsArray()) {
    v8::String::Utf8Value sliderName(info[0]->ToString());
    name = string(*sliderName);

    v8::Local<v8::Array> paramData = info[1].As<v8::Array>();
    for (int i = 0; i < paramData->Length(); i++) {
      v8::Local<v8::Object> objData = paramData->Get(i).As<v8::Object>();
      Comp::LayerParamInfo param;

      // looking for specific params
      if (Nan::HasOwnProperty(objData, Nan::New("layerName").ToLocalChecked()).ToChecked()) {
        v8::String::Utf8Value str(objData->Get(Nan::New("layerName").ToLocalChecked())->ToString());
        param._name = string(*str);
      }
      else {
        Nan::ThrowError("Parameter objects must have a layerName field");
      }

      if (Nan::HasOwnProperty(objData, Nan::New("param").ToLocalChecked()).ToChecked()) {
        v8::String::Utf8Value str(objData->Get(Nan::New("param").ToLocalChecked())->ToString());
        param._param = string(*str);
      }
      else {
        Nan::ThrowError("Parameter objects must have a param field");
      }

      if (Nan::HasOwnProperty(objData, Nan::New("adjustmentType").ToLocalChecked()).ToChecked()) {
        param._type = (Comp::AdjustmentType)(objData->Get(Nan::New("adjustmentType").ToLocalChecked())->Int32Value());
      }
      else {
        Nan::ThrowError("Parameter objects must have an adjustmentType field");
      }

      // now looking for function data info
      v8::String::Utf8Value t(excGet(objData, "type")->ToString());
      string funcType = string(*t);

      if (funcType == "dynamicSine") {
        float f = excGet(objData, "f")->NumberValue();
        float phase = excGet(objData, "phase")->NumberValue();
        float A0 = excGet(objData, "A0")->NumberValue();
        float A1 = excGet(objData, "A1")->NumberValue();
        float D0 = excGet(objData, "D0")->NumberValue();
        float D1 = excGet(objData, "D1")->NumberValue();

        shared_ptr<Comp::ParamFunction> func = shared_ptr<Comp::ParamFunction>(new Comp::DynamicSine(f, phase, A0, A1, D0, D1));
        funcs.push_back(func);
      }
      else if (funcType == "sawtooth") {
        int cycles = excGet(objData, "cycles")->Int32Value();
        float min = excGet(objData, "min")->NumberValue();
        float max = excGet(objData, "max")->NumberValue();
        bool inverted = excGet(objData, "inverted")->BooleanValue();

        shared_ptr<Comp::ParamFunction> func = shared_ptr<Comp::ParamFunction>(new Comp::Sawtooth(cycles, min, max, inverted));
        funcs.push_back(func);
      }
      else if (funcType == "lerp") {
        vector<float> xs;
        vector<float> ys;

        auto xa = excGet(objData, "xs").As<v8::Array>();
        auto ya = excGet(objData, "ys").As<v8::Array>();

        for (int i = 0; i < xa->Length(); i++) {
          xs.push_back(xa->Get(i)->NumberValue());
          ys.push_back(ya->Get(i)->NumberValue());
        }

        shared_ptr<Comp::ParamFunction> func = shared_ptr<Comp::ParamFunction>(new Comp::LinearInterp(xs, ys));
        funcs.push_back(func);
      }
      else {
        Nan::ThrowError(string("Unrecognized function type " + funcType).c_str());
      }

      params.push_back(param);
    }
  }

  Comp::Slider s(params, funcs);
  s._name = name;

  m->_model->addSlider(name, s);
}

void ModelWrapper::addSliderFromExamples(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  // input is a name, array of strings
  ModelWrapper* m = ObjectWrap::Unwrap<ModelWrapper>(info.Holder());
  nullcheck(m->_model, "model.addSliderFromExamples");

  string name;
  vector<string> files;

  if (info[0]->IsString() && info[1]->IsArray()) {
    v8::String::Utf8Value sliderName(info[0]->ToString());
    name = string(*sliderName);

    v8::Local<v8::Array> filenames = info[1].As<v8::Array>();
    for (int i = 0; i < filenames->Length(); i++) {
      v8::String::Utf8Value fname(filenames->Get(i)->ToString());
      files.push_back(string(*fname));
    }

    m->_model->sliderFromExamples(name, files);
  }
  else {
    Nan::ThrowError("addSliderFromExample(string, array) argument error");
  }
}

void ModelWrapper::sliderSample(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ModelWrapper* m = ObjectWrap::Unwrap<ModelWrapper>(info.Holder());
  nullcheck(m->_model, "model.sliderSample");

  if (info.Length() != 3 || !info[0]->IsObject() || !info[1]->IsString() || !info[2]->IsNumber()) {
    Nan::ThrowError("sliderSample(Context, string, float) argument error");
  }

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Internal Error: Context object found is empty!");
  }
  ContextWrapper* c = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

  v8::String::Utf8Value str(info[1]->ToString());
  string id = string(*str);

  float val = info[2]->NumberValue();

  Comp::Context ctx = m->_model->getSlider(id).sample(c->_context, val);

  const int argc = 1;
  v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&ctx) };
  v8::Local<v8::Function> cons = Nan::New<v8::Function>(ContextWrapper::contextConstructor);
  v8::Local<v8::Object> ctxInst = Nan::NewInstance(cons, argc, argv).ToLocalChecked();

  v8::Local<v8::Object> ret = Nan::New<v8::Object>();
  ret->Set(Nan::New("context").ToLocalChecked(), ctxInst);

  v8::Local<v8::Object> metadata = Nan::New<v8::Object>();

  metadata->Set(Nan::New(id).ToLocalChecked(), Nan::New(val));
  ret->Set(Nan::New("metadata").ToLocalChecked(), metadata);

  info.GetReturnValue().Set(ret);
}

void ModelWrapper::exportSliderGraph(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  ModelWrapper* m = ObjectWrap::Unwrap<ModelWrapper>(info.Holder());
  nullcheck(m->_model, "model.exportSliderGraph");

  if (info.Length() < 2 || !info[0]->IsString() || !info[1]->IsString()) {
    Nan::ThrowError("exportSliderGraph(string, string, [int]) argument error");
  }

  v8::String::Utf8Value str(info[0]->ToString());
  string id = string(*str);

  v8::String::Utf8Value str2(info[1]->ToString());
  string filename = string(*str2);

  int n = 1000;

  if (info[2]->IsInt32()) {
    n = info[2]->Int32Value();
  }

  m->_model->getSlider(id).exportGraphData(filename, n);
}

void UISliderWrapper::Init(v8::Local<v8::Object> exports)
{
  Nan::HandleScope scope;

  // constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Slider").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "setVal", setVal);
  Nan::SetPrototypeMethod(tpl, "getVal", getVal);
  Nan::SetPrototypeMethod(tpl, "displayName", displayName);
  Nan::SetPrototypeMethod(tpl, "layer", layer);
  Nan::SetPrototypeMethod(tpl, "param", param);
  Nan::SetPrototypeMethod(tpl, "type", type);
  Nan::SetPrototypeMethod(tpl, "toJSON", toJSON);

  uiSliderConstructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("Slider").ToLocalChecked(), tpl->GetFunction());
}

UISliderWrapper::UISliderWrapper(Comp::UISlider* s) : _slider(s) {
  _deleteOnDestroy = true;
}

UISliderWrapper::~UISliderWrapper()
{
  if (_deleteOnDestroy)
    delete _slider;
}

void UISliderWrapper::New(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  string layer;
  string param;
  Comp::AdjustmentType t;

  if (info.Length() == 3) {
    v8::String::Utf8Value i0(info[0]->ToString());
    layer = string(*i0);

    v8::String::Utf8Value i1(info[1]->ToString());
    param = string(*i1);

    t = (Comp::AdjustmentType)(info[2]->Int32Value());

    Comp::UISlider* slider = new Comp::UISlider(layer, param, t);
    UISliderWrapper* sw = new UISliderWrapper(slider);
    sw->Wrap(info.This());

    info.GetReturnValue().Set(info.This());
  }
  else if (info[0]->IsExternal()) {
    Comp::UISlider* i = static_cast<Comp::UISlider*>(info[0].As<v8::External>()->Value());
    UISliderWrapper* sw = new UISliderWrapper(i);
    sw->_deleteOnDestroy = false;
    sw->Wrap(info.This());

    info.GetReturnValue().Set(info.This());
  }
  else if (info[0]->IsObject()) {
    // json object init
    v8::Local<v8::Object> obj = info[0].As<v8::Object>();
    v8::String::Utf8Value l(obj->Get(Nan::New("layer").ToLocalChecked())->ToString());
    v8::String::Utf8Value p(obj->Get(Nan::New("param").ToLocalChecked())->ToString());
    Comp::AdjustmentType t = (Comp::AdjustmentType)obj->Get(Nan::New("type").ToLocalChecked())->Int32Value();
    v8::String::Utf8Value dn(obj->Get(Nan::New("displayName").ToLocalChecked())->ToString());

    Comp::UISlider* i = new Comp::UISlider(string(*l), string(*p), t, string(*dn));
    i->setVal(obj->Get(Nan::New("value").ToLocalChecked())->NumberValue());

    UISliderWrapper* sw = new UISliderWrapper(i);
    sw->Wrap(info.This());

    info.GetReturnValue().Set(info.This());
  }
  else {
    Nan::ThrowError("Slider constructor argument error: requires string, string, int");
  }
}

void UISliderWrapper::setVal(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UISliderWrapper* s = ObjectWrap::Unwrap<UISliderWrapper>(info.Holder());
  nullcheck(s->_slider, "slider.setVal");

  if (info[0]->IsNumber()) {
    float val = info[0]->NumberValue();

    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[1]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Internal Error: Context object found is empty!");
    }
    ContextWrapper* c = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    Comp::Context ctx = s->_slider->setVal(val, c->_context);

    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&ctx) };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(ContextWrapper::contextConstructor);
    v8::Local<v8::Object> ctxInst = Nan::NewInstance(cons, argc, argv).ToLocalChecked();

    info.GetReturnValue().Set(ctxInst);
  }
  else if (info[0]->IsObject()) {
    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Internal Error: Context object found is empty!");
    }
    ContextWrapper* c = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    s->_slider->setVal(c->_context);
  }
  else {
    Nan::ThrowError("setVal argument error");
  }
}

void UISliderWrapper::getVal(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UISliderWrapper* s = ObjectWrap::Unwrap<UISliderWrapper>(info.Holder());
  nullcheck(s->_slider, "slider.getVal");

  info.GetReturnValue().Set(Nan::New(s->_slider->getVal()));
}

void UISliderWrapper::displayName(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UISliderWrapper* s = ObjectWrap::Unwrap<UISliderWrapper>(info.Holder());
  nullcheck(s->_slider, "slider.displayName");

  if (info.Length() == 0) {
    info.GetReturnValue().Set(Nan::New(s->_slider->_displayName).ToLocalChecked());
  }
  else {
    v8::String::Utf8Value i0(info[0]->ToString());
    s->_slider->_displayName = string(*i0);
  }
}

void UISliderWrapper::layer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UISliderWrapper* s = ObjectWrap::Unwrap<UISliderWrapper>(info.Holder());
  nullcheck(s->_slider, "slider.layer");

  info.GetReturnValue().Set(Nan::New(s->_slider->getLayer()).ToLocalChecked());
}

void UISliderWrapper::param(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UISliderWrapper* s = ObjectWrap::Unwrap<UISliderWrapper>(info.Holder());
  nullcheck(s->_slider, "slider.param");

  info.GetReturnValue().Set(Nan::New(s->_slider->getParam()).ToLocalChecked());
}

void UISliderWrapper::type(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UISliderWrapper* s = ObjectWrap::Unwrap<UISliderWrapper>(info.Holder());
  nullcheck(s->_slider, "slider.type");

  info.GetReturnValue().Set(Nan::New(s->_slider->getType()));
}

void UISliderWrapper::toJSON(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UISliderWrapper* s = ObjectWrap::Unwrap<UISliderWrapper>(info.Holder());
  nullcheck(s->_slider, "slider.toJSON");

  v8::Local<v8::Object> ret = Nan::New<v8::Object>();

  ret->Set(Nan::New("layer").ToLocalChecked(), Nan::New(s->_slider->getLayer()).ToLocalChecked());
  ret->Set(Nan::New("param").ToLocalChecked(), Nan::New(s->_slider->getParam()).ToLocalChecked());
  ret->Set(Nan::New("type").ToLocalChecked(), Nan::New(s->_slider->getType()));
  ret->Set(Nan::New("displayName").ToLocalChecked(), Nan::New(s->_slider->_displayName).ToLocalChecked());
  ret->Set(Nan::New("value").ToLocalChecked(), Nan::New(s->_slider->getVal()));
  ret->Set(Nan::New("UIType").ToLocalChecked(), Nan::New("Slider").ToLocalChecked());

  info.GetReturnValue().Set(ret);
}

void UIMetaSliderWrapper::Init(v8::Local<v8::Object> exports)
{
  Nan::HandleScope scope;

  // constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("MetaSlider").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "addSlider", addSlider);
  Nan::SetPrototypeMethod(tpl, "getSlider", getSlider);
  Nan::SetPrototypeMethod(tpl, "size", size);
  Nan::SetPrototypeMethod(tpl, "names", names);
  Nan::SetPrototypeMethod(tpl, "deleteSlider", deleteSlider);
  Nan::SetPrototypeMethod(tpl, "setPoints", setPoints);
  Nan::SetPrototypeMethod(tpl, "setContext", setContext);
  Nan::SetPrototypeMethod(tpl, "displayName", displayName);
  Nan::SetPrototypeMethod(tpl, "getVal", getVal);
  Nan::SetPrototypeMethod(tpl, "reassignMax", reassignMax);
  Nan::SetPrototypeMethod(tpl, "toJSON", toJSON);

  uiMetaSliderConstructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("MetaSlider").ToLocalChecked(), tpl->GetFunction());
}

UIMetaSliderWrapper::UIMetaSliderWrapper(Comp::UIMetaSlider* s) : _mSlider(s) {

}

UIMetaSliderWrapper::~UIMetaSliderWrapper()
{
  delete _mSlider;
}

void UIMetaSliderWrapper::New(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string name(*i0);

    Comp::UIMetaSlider* slider = new Comp::UIMetaSlider(name);
    UIMetaSliderWrapper* sw = new UIMetaSliderWrapper(slider);
    sw->Wrap(info.This());

    info.GetReturnValue().Set(info.This());
  }
  else if (info[0]->IsObject()) {
    v8::Local<v8::Object> obj = info[0].As<v8::Object>();
    v8::String::Utf8Value name(obj->Get(Nan::New("displayName").ToLocalChecked())->ToString());

    Comp::UIMetaSlider* slider = new Comp::UIMetaSlider(string(*name));

    // slider loading time
    v8::Local<v8::Object> sliders = obj->Get(Nan::New("subSliders").ToLocalChecked()).As<v8::Object>();
    auto ids = sliders->GetOwnPropertyNames();
    for (int i = 0; i < ids->Length(); i++) {
      auto sliderObj = sliders->Get(ids->Get(i)).As<v8::Object>();

      v8::String::Utf8Value layer(sliderObj->Get(Nan::New("layer").ToLocalChecked())->ToString());
      v8::String::Utf8Value param(sliderObj->Get(Nan::New("param").ToLocalChecked())->ToString());
      Comp::AdjustmentType t = (Comp::AdjustmentType)sliderObj->Get(Nan::New("type").ToLocalChecked())->Int32Value();

      vector<float> xs;
      vector<float> ys;
      auto xsa = sliderObj->Get(Nan::New("xs").ToLocalChecked()).As<v8::Array>();
      auto ysa = sliderObj->Get(Nan::New("ys").ToLocalChecked()).As<v8::Array>();

      for (int i = 0; i < xsa->Length(); i++) {
        xs.push_back(xsa->Get(i)->NumberValue());
        ys.push_back(ysa->Get(i)->NumberValue());
      }

      slider->addSlider(string(*layer), string(*param), t, xs, ys);
    }

    UIMetaSliderWrapper* sw = new UIMetaSliderWrapper(slider);
    sw->Wrap(info.This());

    info.GetReturnValue().Set(info.This());
  }
  else {
    Nan::ThrowError("MetaSlider requires a name");
  }
}

void UIMetaSliderWrapper::addSlider(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UIMetaSliderWrapper* s = ObjectWrap::Unwrap<UIMetaSliderWrapper>(info.Holder());
  nullcheck(s->_mSlider, "MetaSlider.addSlider");

  if (info.Length() == 5) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string layer(*i0);

    v8::String::Utf8Value i1(info[1]->ToString());
    string param(*i1);

    Comp::AdjustmentType t = (Comp::AdjustmentType)(info[2]->Int32Value());

    // arrays
    if (info[3]->IsArray() && info[4]->IsArray()) {
      vector<float> xs;
      vector<float> ys;

      v8::Local<v8::Array> xsr = info[3].As<v8::Array>();
      v8::Local<v8::Array> ysr = info[4].As<v8::Array>();

      if (xsr->Length() != ysr->Length()) {
        Nan::ThrowError("Array lengths do not match, slider not added");
      }

      for (int i = 0; i < xsr->Length(); i++) {
        xs.push_back(xsr->Get(i)->NumberValue());
        ys.push_back(ysr->Get(i)->NumberValue());
      }

      string sliderName = s->_mSlider->addSlider(layer, param, t, xs, ys);
      info.GetReturnValue().Set(Nan::New(sliderName).ToLocalChecked());
    }
    // or min/max floats
    else {
      float min = info[3]->NumberValue();
      float max = info[4]->NumberValue();

      string sliderName = s->_mSlider->addSlider(layer, param, t, min, max);
      info.GetReturnValue().Set(Nan::New(sliderName).ToLocalChecked());
    }
  }
  else {
    Nan::ThrowError("addSlider(string, string, int, Array, Array) argument error");
  }
}

void UIMetaSliderWrapper::getSlider(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UIMetaSliderWrapper* s = ObjectWrap::Unwrap<UIMetaSliderWrapper>(info.Holder());
  nullcheck(s->_mSlider, "MetaSlider.getSlider");

  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string id(*i0);

    Comp::UISlider* slider = s->_mSlider->getSlider(id);

    if (slider == nullptr) {
      info.GetReturnValue().Set(Nan::New(Nan::Null));
    }
    else {
      v8::Local<v8::Function> cons = Nan::New<v8::Function>(UISliderWrapper::uiSliderConstructor);
      const int argc = 1;
      v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(slider) };

      info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
    }
  }
  else {
    Nan::ThrowError("getSlider(string) arugment error");
  }
}

void UIMetaSliderWrapper::size(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UIMetaSliderWrapper* s = ObjectWrap::Unwrap<UIMetaSliderWrapper>(info.Holder());
  nullcheck(s->_mSlider, "MetaSlider.size");

  info.GetReturnValue().Set(Nan::New(s->_mSlider->size()));
}

void UIMetaSliderWrapper::names(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UIMetaSliderWrapper* s = ObjectWrap::Unwrap<UIMetaSliderWrapper>(info.Holder());
  nullcheck(s->_mSlider, "MetaSlider.names");

  v8::Local<v8::Array> ret = Nan::New<v8::Array>();
  vector<string> names = s->_mSlider->names();

  for (int i = 0; i < names.size(); i++) {
    ret->Set(i, Nan::New(names[i]).ToLocalChecked());
  }

  info.GetReturnValue().Set(ret);
}

void UIMetaSliderWrapper::deleteSlider(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UIMetaSliderWrapper* s = ObjectWrap::Unwrap<UIMetaSliderWrapper>(info.Holder());
  nullcheck(s->_mSlider, "MetaSlider.deleteSlider");

  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string id(*i0);

    s->_mSlider->deleteSlider(id);
  }
}

void UIMetaSliderWrapper::setPoints(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UIMetaSliderWrapper* s = ObjectWrap::Unwrap<UIMetaSliderWrapper>(info.Holder());
  nullcheck(s->_mSlider, "MetaSlider.setPoints");

  if (info.Length() == 3) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string id(*i0);

    vector<float> xs;
    vector<float> ys;

    if (info[1]->IsArray() && info[2]->IsArray()) {
      v8::Local<v8::Array> a1 = info[1].As<v8::Array>();
      v8::Local<v8::Array> a2 = info[2].As<v8::Array>();

      if (a1->Length() == a2->Length()) {
        for (int i = 0; i < a1->Length(); i++) {
          xs.push_back(a1->Get(i)->NumberValue());
          ys.push_back(a2->Get(i)->NumberValue());
        }

        s->_mSlider->setPoints(id, xs, ys);
      }
      else {
        Nan::ThrowError("setPoints array lengths do not match");
      }
    }
    else {
      Nan::ThrowError("setPoints(string, Array, Array) argument error");
    }
  }
  else {
    Nan::ThrowError("setPoints(string, Array, Array) argument error");
  }
}

void UIMetaSliderWrapper::setContext(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UIMetaSliderWrapper* s = ObjectWrap::Unwrap<UIMetaSliderWrapper>(info.Holder());
  nullcheck(s->_mSlider, "MetaSlider.setContext");

  if (info.Length() == 2) {
    float val = info[0]->NumberValue();

    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[1]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Internal Error: Context object found is empty!");
    }
    ContextWrapper* c = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    Comp::Context ctx = s->_mSlider->setContext(val, c->_context);

    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&ctx) };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(ContextWrapper::contextConstructor);
    v8::Local<v8::Object> ctxInst = Nan::NewInstance(cons, argc, argv).ToLocalChecked();

    info.GetReturnValue().Set(ctxInst);
  }
  else if (info.Length() == 3) {
    float val = info[0]->NumberValue();
    float scale = info[1]->NumberValue();

    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[2]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Internal Error: Context object found is empty!");
    }
    ContextWrapper* c = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    Comp::Context ctx = s->_mSlider->setContext(val, scale, c->_context);

    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&ctx) };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(ContextWrapper::contextConstructor);
    v8::Local<v8::Object> ctxInst = Nan::NewInstance(cons, argc, argv).ToLocalChecked();

    info.GetReturnValue().Set(ctxInst);
  }
  else {
    Nan::ThrowError("setContext(float[, float], context) argument error");
  }
}

void UIMetaSliderWrapper::displayName(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UIMetaSliderWrapper* s = ObjectWrap::Unwrap<UIMetaSliderWrapper>(info.Holder());
  nullcheck(s->_mSlider, "MetaSlider.displayName");

  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string name(*i0);

    s->_mSlider->_displayName = name;
  }
  else {
    info.GetReturnValue().Set(Nan::New(s->_mSlider->_displayName).ToLocalChecked());
  }
}

void UIMetaSliderWrapper::getVal(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UIMetaSliderWrapper* s = ObjectWrap::Unwrap<UIMetaSliderWrapper>(info.Holder());
  nullcheck(s->_mSlider, "MetaSlider.getVal");

  info.GetReturnValue().Set(Nan::New(s->_mSlider->getVal()));
}

void UIMetaSliderWrapper::reassignMax(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UIMetaSliderWrapper* s = ObjectWrap::Unwrap<UIMetaSliderWrapper>(info.Holder());
  nullcheck(s->_mSlider, "MetaSlider.reassignMax");

  Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
  if (maybe1.IsEmpty()) {
    Nan::ThrowError("Internal Error: Context object found is empty!");
  }
  ContextWrapper* c = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

  s->_mSlider->reassignMax(c->_context);
}

void UIMetaSliderWrapper::toJSON(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UIMetaSliderWrapper* s = ObjectWrap::Unwrap<UIMetaSliderWrapper>(info.Holder());
  nullcheck(s->_mSlider, "MetaSlider.toJSON");

  v8::Local<v8::Object> ret = Nan::New<v8::Object>();

  // metaslider info
  ret->Set(Nan::New("displayName").ToLocalChecked(), Nan::New(s->_mSlider->_displayName).ToLocalChecked());
  ret->Set(Nan::New("value").ToLocalChecked(), Nan::New(s->_mSlider->getVal()));
  ret->Set(Nan::New("UIType").ToLocalChecked(), Nan::New("MetaSlider").ToLocalChecked());

  v8::Local<v8::Object> sliders = Nan::New<v8::Object>();
  // slider data extraction
  for (auto& name : s->_mSlider->names()) {
    v8::Local<v8::Object> sld = Nan::New<v8::Object>();
    Comp::UISlider* slider = s->_mSlider->getSlider(name);
    auto func = s->_mSlider->getFunc(name);

    sld->Set(Nan::New("layer").ToLocalChecked(), Nan::New(slider->getLayer()).ToLocalChecked());
    sld->Set(Nan::New("param").ToLocalChecked(), Nan::New(slider->getParam()).ToLocalChecked());
    sld->Set(Nan::New("type").ToLocalChecked(), Nan::New(slider->getType()));

    // points
    v8::Local<v8::Array> xs = Nan::New<v8::Array>();
    v8::Local<v8::Array> ys = Nan::New<v8::Array>();

    for (int i = 0; i < func->_xs.size(); i++) {
      xs->Set(i, Nan::New(func->_xs[i]));
      ys->Set(i, Nan::New(func->_ys[i]));
    }

    sld->Set(Nan::New("xs").ToLocalChecked(), xs);
    sld->Set(Nan::New("ys").ToLocalChecked(), ys);

    sliders->Set(Nan::New(name).ToLocalChecked(), sld);
  }
  ret->Set(Nan::New("subSliders").ToLocalChecked(), sliders);

  info.GetReturnValue().Set(ret);
}

void UISamplerWrapper::Init(v8::Local<v8::Object> exports)
{
  Nan::HandleScope scope;

  // constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Sampler").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "addParam", addParam);
  Nan::SetPrototypeMethod(tpl, "deleteParam", deleteParam);
  Nan::SetPrototypeMethod(tpl, "params", params);
  Nan::SetPrototypeMethod(tpl, "sample", sample);
  Nan::SetPrototypeMethod(tpl, "eval", eval);
  Nan::SetPrototypeMethod(tpl, "displayName", displayName);
  Nan::SetPrototypeMethod(tpl, "toJSON", toJSON);

  uiSamplerConstructor.Reset(tpl->GetFunction());
  exports->Set(Nan::New("Sampler").ToLocalChecked(), tpl->GetFunction());
}

UISamplerWrapper::UISamplerWrapper(Comp::UISampler* s) {
  _sampler = s;
}

UISamplerWrapper::~UISamplerWrapper()
{
  delete _sampler;
}

void UISamplerWrapper::New(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string name(*i0);

    Comp::UISampler* slider = new Comp::UISampler(name);
    UISamplerWrapper* sw = new UISamplerWrapper(slider);
    sw->Wrap(info.This());

    info.GetReturnValue().Set(info.This());
  }
  else if (info[0]->IsObject()) {
    v8::Local<v8::Object> obj = info[0].As<v8::Object>();

    v8::String::Utf8Value dn(obj->Get(Nan::New("displayName").ToLocalChecked())->ToString());
    Comp::AxisEvalFuncType type = (Comp::AxisEvalFuncType)obj->Get(Nan::New("objEvalMode").ToLocalChecked())->Int32Value();

    Comp::UISampler* sampler = new Comp::UISampler(string(*dn));
    sampler->setObjectiveMode(type);

    // params
    v8::Local<v8::Object> params = obj->Get(Nan::New("params").ToLocalChecked()).As<v8::Object>();
    auto names = params->GetOwnPropertyNames();
    for (int i = 0; i < names->Length(); i++) {
      v8::Local<v8::Object> p = params->Get(names->Get(i)).As<v8::Object>();
      Comp::LayerParamInfo inf;

      v8::String::Utf8Value layer(p->Get(Nan::New("layer").ToLocalChecked())->ToString());
      v8::String::Utf8Value param(p->Get(Nan::New("param").ToLocalChecked())->ToString());
      Comp::AdjustmentType t = (Comp::AdjustmentType)p->Get(Nan::New("type").ToLocalChecked())->Int32Value();

      inf._name = string(*layer);
      inf._param = string(*param);
      inf._type = t;
      inf._inverted = false;

      sampler->addParam(inf);
    }

    UISamplerWrapper* sw = new UISamplerWrapper(sampler);
    sw->Wrap(info.This());

    info.GetReturnValue().Set(info.This());
  }
  else {
    Nan::ThrowError("Sampler requires a name");
  }
}

void UISamplerWrapper::displayName(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UISamplerWrapper* s = ObjectWrap::Unwrap<UISamplerWrapper>(info.Holder());
  nullcheck(s->_sampler, "Sampler.displayName");

  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string name(*i0);

    s->_sampler->_displayName = name;
  }
  else {
    info.GetReturnValue().Set(Nan::New(s->_sampler->_displayName).ToLocalChecked());
  }
}

void UISamplerWrapper::addParam(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UISamplerWrapper* s = ObjectWrap::Unwrap<UISamplerWrapper>(info.Holder());
  nullcheck(s->_sampler, "Sampler.addParam");

  if (info.Length() == 3) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string layer(*i0);

    v8::String::Utf8Value i1(info[1]->ToString());
    string param(*i1);

    Comp::AdjustmentType t = (Comp::AdjustmentType)(info[2]->Int32Value());

    Comp::LayerParamInfo p(t, layer, param);
    string id = s->_sampler->addParam(p);

    info.GetReturnValue().Set(Nan::New(id).ToLocalChecked());
  }
  else {
    Nan::ThrowError("addParam(string, string, int) argument error");
  }
}

void UISamplerWrapper::deleteParam(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UISamplerWrapper* s = ObjectWrap::Unwrap<UISamplerWrapper>(info.Holder());
  nullcheck(s->_sampler, "Sampler.deleteParam");

  if (info[0]->IsString()) {
    v8::String::Utf8Value i0(info[0]->ToString());
    string id(*i0);

    s->_sampler->deleteParam(id);
  }
}

void UISamplerWrapper::params(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UISamplerWrapper* s = ObjectWrap::Unwrap<UISamplerWrapper>(info.Holder());
  nullcheck(s->_sampler, "Sampler.params");

  v8::Local<v8::Array> ret = Nan::New<v8::Array>();
  vector<string> names = s->_sampler->params();

  for (int i = 0; i < names.size(); i++) {
    ret->Set(i, Nan::New(names[i]).ToLocalChecked());
  }

  info.GetReturnValue().Set(ret);
}

void UISamplerWrapper::sample(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UISamplerWrapper* s = ObjectWrap::Unwrap<UISamplerWrapper>(info.Holder());
  nullcheck(s->_sampler, "Sampler.sample");

  if (info.Length() == 2) {
    float val = info[0]->NumberValue();

    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[1]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Internal Error: Context object found is empty!");
    }
    ContextWrapper* c = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    Comp::Context ctx = s->_sampler->sample(val, c->_context);

    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&ctx) };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(ContextWrapper::contextConstructor);
    v8::Local<v8::Object> ctxInst = Nan::NewInstance(cons, argc, argv).ToLocalChecked();

    info.GetReturnValue().Set(ctxInst);
  }
  else {
    Nan::ThrowError("setContext(float, context) argument error");
  }
}

void UISamplerWrapper::eval(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UISamplerWrapper* s = ObjectWrap::Unwrap<UISamplerWrapper>(info.Holder());
  nullcheck(s->_sampler, "Sampler.eval");

  if (info.Length() == 1) {
    Nan::MaybeLocal<v8::Object> maybe1 = Nan::To<v8::Object>(info[0]);
    if (maybe1.IsEmpty()) {
      Nan::ThrowError("Internal Error: Context object found is empty!");
    }
    ContextWrapper* c = Nan::ObjectWrap::Unwrap<ContextWrapper>(maybe1.ToLocalChecked());

    info.GetReturnValue().Set(Nan::New(s->_sampler->eval(c->_context)));
  }
  else {
    Nan::ThrowError("eval(context) argument error");
  }
}

void UISamplerWrapper::toJSON(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  UISamplerWrapper* s = ObjectWrap::Unwrap<UISamplerWrapper>(info.Holder());
  nullcheck(s->_sampler, "Sampler.toJSON");

  v8::Local<v8::Object> ret = Nan::New<v8::Object>();

  ret->Set(Nan::New("displayName").ToLocalChecked(), Nan::New(s->_sampler->_displayName).ToLocalChecked());
  ret->Set(Nan::New("objEvalMode").ToLocalChecked(), Nan::New(s->_sampler->getEvalMode()));
  ret->Set(Nan::New("UIType").ToLocalChecked(), Nan::New("Sampler").ToLocalChecked());

  // parameter dump
  vector<string> ids = s->_sampler->params();
  v8::Local<v8::Object> params = Nan::New<v8::Object>();

  for (auto& id : ids) {
    Comp::LayerParamInfo info = s->_sampler->getParamInfo(id);

    v8::Local<v8::Object> pinfo = Nan::New<v8::Object>();
    pinfo->Set(Nan::New("layer").ToLocalChecked(), Nan::New(info._name).ToLocalChecked());
    pinfo->Set(Nan::New("param").ToLocalChecked(), Nan::New(info._param).ToLocalChecked());
    pinfo->Set(Nan::New("type").ToLocalChecked(), Nan::New(info._type));

    params->Set(Nan::New(id).ToLocalChecked(), pinfo);
  }

  ret->Set(Nan::New("params").ToLocalChecked(), params);

  info.GetReturnValue().Set(ret);
}
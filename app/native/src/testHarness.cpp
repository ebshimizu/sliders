#include "Compositor.h"
#include "testHarness.h"

namespace Comp {

// ! Here the generated function is called. For testing purposes, this framework will
// assume a fixed name for the function being called
// Name is required to be: compTest()

#include "compTest.cpp"

vector<double> cvtT(vector<double> params)
{
  vector<double> res;
  if (params[1] == 0)
    return { 0 };

  double v = params[0] / params[1];
  res.push_back((v > 1) ? 1 : (v < 0) ? 0 : v);
  return res;
}

vector<double> overlay(vector<double> params)
{
  double a = params[0];
  double b = params[1];
  double alpha1 = params[2];
  double alpha2 = params[3];

  if (2 * a <= alpha1) {
    return { b * a * 2 + b * (1 - alpha1) + a * (1 - alpha2) };
  }
  else {
    return { b * (1 + alpha1) + a * (1 + alpha2) - 2 * a * b - alpha1 * alpha2 };
  }
}

vector<double> hardLight(vector<double> params)
{
  double a = params[0];
  double b = params[1];
  double alpha1 = params[2];
  double alpha2 = params[3];

  if (2 * b <= alpha2) {
    return { 2 * b * a + b * (1 - alpha1) + a * (1 - alpha2) };
  }
  else {
    return { b * (1 + alpha1) + a * (1 + alpha2) - alpha1 * alpha2 - 2 * a * b };
  }
}

vector<double> softLight(vector<double> params)
{
  double Dca = params[0];
  double Sca = params[1];
  double Da = params[2];
  double Sa = params[3];

  double m = (Da == 0) ? 0 : Dca / Da;

  if (2 * Sca <= Sa) {
    return { Dca * (Sa + (2 * Sca - Sa) * (1 - m)) + Sca * (1 - Da) + Dca * (1 - Sa) };
  }
  else if (2 * Sca > Sa && 4 * Dca <= Da) {
    return { Da * (2 * Sca - Sa) * (16 * m * m * m - 12 * m * m - 3 * m) + Sca - Sca * Da + Dca };
  }
  else if (2 * Sca > Sa && 4 * Dca > Da) {
    return { Da * (2 * Sca - Sa) * (sqrt(m) - m) + Sca - Sca * Da + Dca };
  }
  else {
    return { Sca + Dca * (1 - Sa) };
  }
}

vector<double> linearDodgeAlpha(vector<double> params)
{
  double aa = params[0];
  double ab = params[1];

  return { (aa + ab > 1) ?  1 : (aa + ab) };
}

vector<double> linearBurn(vector<double> params)
{
  double Dc = params[0];
  double Sc = params[1];
  double Da = params[2];
  double Sa = params[3];

  if (Da == 0)
    return { Sc };

  double burn = Dc + Sc - 1;

  // normal blend
  return { burn * Sa + Dc * (1 - Sa) };
}

vector<double> colorDodge(vector<double> params)
{
  double Dca = params[0];
  double Sca = params[1];
  double Da = params[2];
  double Sa = params[3];

  if (Sca == Sa && Dca == 0) {
    return { Sca * (1 - Da) };
  }
  else if (Sca == Sa) {
    return { Sa * Da + Sca * (1 - Da) + Dca * (1 - Sa) };
  }
  else if (Sca < Sa) {
    return { Sa * Da * min(1.0, Dca / Da * Sa / (Sa - Sca)) + Sca * (1 - Da) + Dca * (1 - Sa) };
  }

  // probably never get here but compiler is yelling at me
  return { 0 };
}

vector<double> linearLight(vector<double> params)
{
  double Dc = params[0];
  double Sc = params[1];
  double Da = params[2];
  double Sa = params[3];

  if (Da == 0)
    return { Sc };

  double light = Dc + 2 * Sc - 1;
  return { light * Sa + Dc * (1 - Sa) };
}

vector<double> lighten(vector<double> params)
{
  double Dca = params[0];
  double Sca = params[1];
  double Da = params[2];
  double Sa = params[3];

  if (Sca > Dca) {
    return { Sca + Dca * (1 - Sa) };
  }
  else {
    return { Dca + Sca * (1 - Da) };
  }
}

vector<double> color(vector<double> params)
{
  Utils<double>::RGBColorT src, dest;
  dest._r = params[0];
  dest._g = params[1];
  dest._b = params[2];
  src._r = params[3];
  src._g = params[4];
  src._b = params[5];
  double Da = params[6];
  double Sa = params[7];

  if (Da == 0) {
    src._r *= Sa;
    src._g *= Sa;
    src._b *= Sa;

    return { src._r, src._g, src._b };
  }

  if (Sa == 0) {
    dest._r *= Da;
    dest._g *= Da;
    dest._b *= Da;

    return { dest._r, dest._g, dest._b };
  }

  // color keeps dest luma and keeps top hue and chroma
  Utils<double>::HSYColorT dc = Utils<double>::RGBToHSY(dest);
  Utils<double>::HSYColorT sc = Utils<double>::RGBToHSY(src);
  dc._h = sc._h;
  dc._s = sc._s;

  Utils<double>::RGBColorT res = Utils<double>::HSYToRGB(dc);

  // actually have to blend here...
  res._r = res._r * Sa + dest._r * Da * (1 - Sa);
  res._g = res._g * Sa + dest._g * Da * (1 - Sa);
  res._b = res._b * Sa + dest._b * Da * (1 - Sa);

  return { res._r, res._g, res._b };
}

vector<double> darken(vector<double> params)
{
  double Dca = params[0];
  double Sca = params[1];
  double Da = params[2];
  double Sa = params[3];

  if (Sca > Dca) {
    return { Dca + Sca * (1 - Da) };
  }
  else {
    return { Sca + Dca * (1 - Sa) };
  }
}

vector<double> pinLight(vector<double> params)
{
  double Dca = params[0];
  double Sca = params[1];
  double Da = params[2];
  double Sa = params[3];

  if (Da == 0)
    return { Sca };

  if (Sca < 0.5f) {
    return darken({ Dca, Sca * 2, Da, Sa });
  }
  else {
    return lighten({ Dca, 2 * (Sca - 0.5f), Da, Sa });
  }
}

vector<double> RGBToHSL(vector<double> params)
{
  auto res = Utils<double>::RGBToHSL(params[0], params[1], params[2]);

  return { res._h, res._s, res._l };
}

vector<double> HSLToRGB(vector<double> params)
{
  auto res = Utils<double>::HSLToRGB(params[0], params[1], params[2]);
  return { res._r, res._g, res._b };
}

vector<double> RGBToHSY(vector<double> params)
{
  auto res = Utils<double>::RGBToHSY(params[0], params[1], params[2]);
  return { res._h, res._s, res._y };
}

vector<double> HSYToRGB(vector<double> params)
{
  auto res = Utils<double>::HSYToRGB(params[0], params[1], params[2]);
  return { res._r, res._g, res._b };
}

vector<double> levels(vector<double> params)
{
  double px = params[0];
  double inMin = params[1];
  double inMax = params[2];
  double gamma = params[3];
  double outMin = params[4];
  double outMax = params[5];

  // input remapping
  double out = min(max(px - inMin, 0.0) / (inMax - inMin), 1.0);

  // gamma correction
  out = pow(out, 1 / gamma);

  // output remapping
  out = out * (outMax - outMin) + outMin;

  return { out };
}

vector<double> clamp(vector<double> params)
{
  return { clamp<double>(params[0], params[1], params[2]) };
}

vector<double> selectiveColor(vector<double> params)
{
  RGBColor adjPx;
  adjPx._r = params[0];
  adjPx._g = params[1];
  adjPx._b = params[2];

  map<string, map<string, float>> data;
  data["blacks"]["black"] = params[3];
  data["blacks"]["cyan"] = params[4];
  data["blacks"]["magenta"] = params[5];
  data["blacks"]["yellow"] = params[6];
  data["blues"]["black"] = params[7];
  data["blues"]["cyan"] = params[8];
  data["blues"]["magenta"] = params[9];
  data["blues"]["yellow"] = params[10];
  data["cyans"]["black"] = params[11];
  data["cyans"]["cyan"] = params[12];
  data["cyans"]["magenta"] = params[13];
  data["cyans"]["yellow"] = params[14];
  data["greens"]["black"] = params[15];
  data["greens"]["cyan"] = params[16];
  data["greens"]["magenta"] = params[17];
  data["greens"]["yellow"] = params[18];
  data["magentas"]["black"] = params[19];
  data["magnetas"]["cyan"] = params[20];
  data["magentas"]["magenta"] = params[21];
  data["magentas"]["yellow"] = params[22];
  data["neutrals"]["black"] = params[23];
  data["neutrals"]["cyan"] = params[24];
  data["neutrals"]["magenta"] = params[25];
  data["neutrals"]["yellow"] = params[26];
  data["reds"]["black"] = params[27];
  data["reds"]["cyan"] = params[28];
  data["reds"]["magenta"] = params[29];
  data["reds"]["yellow"] = params[30];
  data["whites"]["black"] = params[31];
  data["whites"]["cyan"] = params[32];
  data["whites"]["magenta"] = params[33];
  data["whites"]["yellow"] = params[34];
  data["yellows"]["black"] = params[35];
  data["yellows"]["cyan"] = params[36];
  data["yellows"]["magenta"] = params[37];
  data["yellows"]["yellow"] = params[38];

  // convert to hsl
  Utils<double>::HSLColorT hslColor = Utils<double>::RGBToHSL(adjPx._r, adjPx._g, adjPx._b);
  double chroma = max(adjPx._r, max(adjPx._g, adjPx._b)) - min(adjPx._r, min(adjPx._g, adjPx._b));

  // determine which set of parameters we're using to adjust
  // determine chroma interval
  int interval = (int)(hslColor._h / 60);
  string c1, c2, c3, c4;
  c1 = intervalNames[interval];

  if (interval == 5) {
    // wrap around for magenta
    c2 = intervalNames[0];
  }
  else {
    c2 = intervalNames[interval + 1];
  }

  c3 = "neutrals";

  // non-chromatic colors
  if (hslColor._l < 0.5) {
    c4 = "blacks";
  }
  else {
    c4 = "whites";
  }

  // compute weights
  double w1, w2, w3, w4, wc;

  // chroma
  wc = chroma / 1.0f;

  // hue - always 60 deg intervals
  w1 = 1 - ((hslColor._h - (interval * 60.0f)) / 60.0f);  // distance from low interval
  w2 = 1 - w1;

  // luma - measure distance from midtones, w3 is always midtone
  w3 = 1 - abs(hslColor._l - 0.5f);
  w4 = 1 - w3;

  // do the adjustment
  Utils<double>::CMYKColorT cmykColor = Utils<double>::RGBToCMYK(adjPx._r, adjPx._g, adjPx._b);

  // we assume relative is true always here.
  //if (adj["relative"] > 0) {
    // relative
  cmykColor._c += cmykColor._c * (w1 * data[c1]["cyan"] + w2 * data[c2]["cyan"]) * wc + (w3 * data[c3]["cyan"] + w4 * data[c4]["cyan"]) * (1 - wc);
  cmykColor._m += cmykColor._m * (w1 * data[c1]["magenta"] + w2 * data[c2]["magenta"]) * wc + (w3 * data[c3]["magenta"] + w4 * data[c4]["magenta"]) * (1 - wc);
  cmykColor._y += cmykColor._y * (w1 * data[c1]["yellow"] + w2 * data[c2]["yellow"]) * wc + (w3 * data[c3]["yellow"] + w4 * data[c4]["yellow"]) * (1 - wc);
  cmykColor._k += cmykColor._k * (w1 * data[c1]["black"] + w2 * data[c2]["black"]) * wc + (w3 * data[c3]["black"] + w4 * data[c4]["black"]) * (1 - wc);
  //}
  //else {
    // absolute
  //  cmykColor._c += (w1 * data[c1]["cyan"] + w2 * data[c2]["cyan"]) * wc + (w3 * data[c3]["cyan"] + w4 * data[c4]["cyan"]) * (1 - wc);
  //  cmykColor._m += (w1 * data[c1]["magenta"] + w2 * data[c2]["magenta"]) * wc + (w3 * data[c3]["magenta"] + w4 * data[c4]["magenta"]) * (1 - wc);
  //  cmykColor._y += (w1 * data[c1]["yellow"] + w2 * data[c2]["yellow"]) * wc + (w3 * data[c3]["yellow"] + w4 * data[c4]["yellow"]) * (1 - wc);
  //  cmykColor._k += (w1 * data[c1]["black"] + w2 * data[c2]["black"]) * wc + (w3 * data[c3]["black"] + w4 * data[c4]["black"]) * (1 - wc);
  //}

  Utils<double>::RGBColorT res = Utils<double>::CMYKToRGB(cmykColor);
  adjPx._r = clamp<double>(res._r, 0, 1);
  adjPx._g = clamp<double>(res._g, 0, 1);
  adjPx._b = clamp<double>(res._b, 0, 1);

  return { adjPx._r, adjPx._g, adjPx._b };
}

double colorBalance(double px, double shadow, double mid, double high)
{
  double a = 0.25f;
  double b = 0.333f;
  double scale = 0.7f;

  double s = shadow * (clamp<double>((px - b) / -a + 0.5f, 0, 1.0f) * scale);
  double m = mid * (clamp<double>((px - b) / a + 0.5f, 0, 1.0f) * clamp<double>((px + b - 1.0f) / -a + 0.5f, 0, 1.0f) * scale);
  double h = high * (clamp<double>((px + b - 1.0f) / a + 0.5f, 0, 1.0f) * scale);

  return clamp<double>(px + s + m + h, 0, 1.0);
}

vector<double> colorBalanceAdjust(vector<double> params)
{
  Utils<double>::RGBColorT adjPx;
  adjPx._r = params[0];
  adjPx._g = params[1];
  adjPx._b = params[2];

  Utils<double>::RGBColorT balanced;
  balanced._r = colorBalance(adjPx._r, params[3], params[6], params[9]);
  balanced._g = colorBalance(adjPx._g, params[4], params[7], params[10]);
  balanced._b = colorBalance(adjPx._b, params[5], params[8], params[11]);

  // assume preserve luma true
  //if (adj["preserveLuma"] > 0) {
  Utils<double>::HSLColorT l = Utils<double>::RGBToHSL(balanced);
  double originalLuma = 0.5f * (max(adjPx._r, max(adjPx._g, adjPx._b)) + min(adjPx._r, min(adjPx._g, adjPx._b)));
  balanced = Utils<double>::HSLToRGB(l._h, l._s, originalLuma);
  //}

  adjPx._r = clamp<double>(balanced._r, 0, 1);
  adjPx._g = clamp<double>(balanced._g, 0, 1);
  adjPx._b = clamp<double>(balanced._b, 0, 1);

  return { adjPx._r, adjPx._g, adjPx._b };
}

vector<double> photoFilter(vector<double> params)
{
  Utils<double>::RGBColorT adjPx;
  adjPx._r = params[0];
  adjPx._g = params[1];
  adjPx._b = params[2];

  double d = params[3];
  double fr = adjPx._r * params[4];
  double fg = adjPx._g * params[5];
  double fb = adjPx._b * params[6];

  // assuming preserve luma always
  //if (adj["preserveLuma"] > 0) {
  Utils<double>::HSLColorT l = Utils<double>::RGBToHSL(fr, fg, fb);
  double originalLuma = 0.5 * (max(adjPx._r, max(adjPx._g, adjPx._b)) + min(adjPx._r, min(adjPx._g, adjPx._b)));
  Utils<double>::RGBColorT rgb = Utils<double>::HSLToRGB(l._h, l._s, originalLuma);
  fr = rgb._r;
  fg = rgb._g;
  fb = rgb._b;
  //}

  // weight by density
  adjPx._r = clamp<double>(fr * d + adjPx._r * (1 - d), 0, 1);
  adjPx._g = clamp<double>(fg * d + adjPx._g * (1 - d), 0, 1);
  adjPx._b = clamp<double>(fb * d + adjPx._b * (1 - d), 0, 1);

  return { adjPx._r, adjPx._g, adjPx._b };
}

vector<double> lighterColorizeAdjust(vector<double> params)
{
  Utils<double>::RGBColorT adjPx;
  adjPx._r = params[0];
  adjPx._g = params[1];
  adjPx._b = params[2];

  double sr = params[3];
  double sg = params[4];
  double sb = params[5];
  double a = params[6];
  double y = 0.299f * sr + 0.587f * sg + 0.114f * sb;

  double yp = 0.299f * adjPx._r + 0.587f * adjPx._g + 0.114f * adjPx._b;

  adjPx._r = (yp > y) ? adjPx._r : sr;
  adjPx._g = (yp > y) ? adjPx._g : sg;
  adjPx._b = (yp > y) ? adjPx._b : sb;

  // blend the resulting colors according to alpha
  adjPx._r = clamp<double>(adjPx._r * a + adjPx._r * (1 - a), 0, 1);
  adjPx._g = clamp<double>(adjPx._g * a + adjPx._g * (1 - a), 0, 1);
  adjPx._b = clamp<double>(adjPx._b * a + adjPx._b * (1 - a), 0, 1);

  return { adjPx._r, adjPx._g, adjPx._b };
}

  // This function must be used on full size images
double compare(Compositor* c, int x, int y) {
  int width, height;

  width = c->getLayer(0).getImage()->getWidth();
  height = c->getLayer(0).getImage()->getHeight();

  int index = clamp(x, 0, width) + clamp(y, 0, height) * width;

  Context ctx = c->getNewContext();

  // to actually test this we need to give the compTest function the parameters in the same order 
  // they were created in.
  vector<double> params;

  for (auto l : ctx) {
    // layer colors
    auto p = l.second.getImage()->getPixel(index);
    params.push_back(p._r);
    params.push_back(p._g);
    params.push_back(p._b);
    params.push_back(p._a);

    l.second.prepExpParams(params);
  }

  // the rendered pixel
  RGBAColor pix = c->renderPixel<float>(ctx, index);

  // the test harness result
  vector<double> res = compTest(params);
  double l2 = sqrt(pow(pix._r - res[0], 2) + pow(pix._g - res[1], 2) + pow(pix._b - res[2], 2) + pow(pix._a - res[3], 2));

  stringstream ss;
  ss << "Test Results for (" << x << "," << y << ")\n";
  ss << "L2 Error: " << l2 << endl;
  ss << "Render: (" << pix._r << "," << pix._g << "," << pix._b << "," << pix._a << ")\n";
  ss << "Generated Func: (" << res[0] << "," << res[1] << "," << res[2] << "," << res[3] << ")\n";

  getLogger()->log(ss.str());

  return l2;
}

void compareAll(Compositor* c, string filename) {
  int width, height;

  width = c->getLayer(0).getImage()->getWidth();
  height = c->getLayer(0).getImage()->getHeight();

  Image img(width, height);

  vector<unsigned char>& data = img.getData();

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int index = x + y * width;
      double err = compare(c, x, y);

      data[index * 4] = (unsigned char)(255 * err * 100);
      data[index * 4 + 1] = 0;
      data[index * 4 + 2] = 0;
      data[index * 4 + 3] = 255;
    }
  }
  
  img.save(filename);
}

}
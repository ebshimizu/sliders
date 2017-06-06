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

      data[index * 4] = 255 * err;
      data[index * 4 + 1] = 0;
      data[index * 4 + 2] = 0;
      data[index * 4 + 3] = 255;
    }
  }
  
  img.save(filename);
}

}
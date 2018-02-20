#include "Compositor.h"
#include "ceresFunctions.h"
#include "testHarness.h"

namespace Comp {

#include "ceresFunc.h"

// ! Here the generated function is called. For testing purposes, this framework will
// assume a fixed name for the function being called
// Name is required to be: compTest()

  // This function must be used on full size images
double compare(Compositor* c, int x, int y) {
  int width, height;

  width = c->getWidth();
  height = c->getHeight();

  int index = clamp(x, 0, width) + clamp(y, 0, height) * width;

  Context ctx = c->getNewContext();

  // to actually test this we need to give the compTest function the parameters in the same order 
  // they were created in.
  vector<double> paramsA;
  vector<double> paramsB;

  for (auto name : c->getLayerOrder()) {
    Layer& l = ctx[name];

    if (!l._visible)
      continue;

    if (!l.isAdjustmentLayer()) {
      // layer colors
      auto p = l.getImage()->getPixel(index);
      paramsB.push_back(p._r);
      paramsB.push_back(p._g);
      paramsB.push_back(p._b);
      paramsB.push_back(p._a);
    }

    l.prepExpParams(paramsA);
  }

  // the rendered pixel
  RGBAColor pix = c->renderPixel(ctx, index);

  // the test harness result
  vector<double> res = ceresFunc(paramsA.data(), paramsB);
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

  width = c->getWidth();
  height = c->getHeight();

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
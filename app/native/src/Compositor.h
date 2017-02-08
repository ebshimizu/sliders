/*
Compositor.h - An image compositing library
author: Evan Shimizu
*/

#include <vector>

#include "Image.h"

using namespace std;

namespace Comp {
  class Compositor {
  public:
    Compositor();
    ~Compositor();

    // debug function just for testing if node interop works
    Image testLoadBase64();
  };
}
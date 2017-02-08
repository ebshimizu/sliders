/*
Compositor.h - An image compositing library
author: Evan Shimizu
*/

#pragma once

#ifndef _COMP_COMPOSITOR_H_
#define _COMP_COMPOSITOR_H_

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

#endif
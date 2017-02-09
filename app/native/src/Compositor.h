/*
Compositor.h - An image compositing library
author: Evan Shimizu
*/

#pragma once

#include <vector>
#include <map>

#include "Image.h"

using namespace std;

namespace Comp {
  class Compositor {
  public:
    Compositor();
    ~Compositor();

    void loadImage(string name, string filename);
    Image* getImage(string name);

  private:
    // really this should be a list of layers but for testing we'll just do this
    map<string, Image*> _images;
  };
}
/*
Image.h - Object containing a bitmap used in the compositor
author: Evan Shimizu
*/

#pragma once
#ifndef _COMP_IMAGE_H_
#define _COMP_IMAGE_H_

#include <vector>
#include <iostream>

#include "third_party/lodepng/lodepng.h"
#include "third_party/cpp-base64/base64.h"

#include "Logger.h"

using namespace std;

namespace Comp {
  class Image {
  public:
    // creates a blank image of arbitrary size
    Image(unsigned int w = 0, unsigned int h = 0);

    // loads an image from a file
    Image(string filename);

    // destructor
    ~Image();

    // gets the raw image data
    vector<unsigned char>& getData();

    // returns the image as a base64 encoded png
    string getBase64();

    unsigned int getWidth() { return _w;}
    unsigned int getHeight() { return _h; }

  private:
    // loads an image from a file
    void loadFromFile(string filename);

    unsigned int _w;
    unsigned int _h;

    // originating filename
    string _filename;

    // raw pixel data
    vector<unsigned char> _data;
  };
}

#endif
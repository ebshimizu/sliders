/*
Image.h - Object containing a bitmap used in the compositor
author: Evan Shimizu
*/

#pragma once
#ifndef _COMP_IMAGE_H_
#define _COMP_IMAGE_H_

#include <vector>
#include <iostream>

// warnings from external libs suppressed 
#pragma warning(push)
// C4267 - type conversions possible loss of data
// C4334 - 32 bit shift implicitly converted to 64 bit shift
#pragma warning(disable : 4267 4334)

#include "third_party/lodepng/lodepng.h"
#include "third_party/cpp-base64/base64.h"

#pragma warning(pop)

#include "Logger.h"
#include "util.h"

using namespace std;

namespace Comp {
  class Image {
  public:
    // creates a blank image of arbitrary size
    Image(unsigned int w = 0, unsigned int h = 0);

    // loads an image from a file
    Image(string filename);

    // copy constructor
    Image(const Image& other);

    // assignment op
    Image& operator=(const Image& other);

    // destructor
    ~Image();

    // gets the raw image data (RGBA order)
    vector<unsigned char>& getData();

    // returns the image as a base64 encoded png
    string getBase64();

    unsigned int getWidth() { return _w;}
    unsigned int getHeight() { return _h; }

    // number of pixels in the image (convenience for w * h)
    unsigned int numPx();

    // saves the image to a file
    void save(string path);
    void save();

    string getFilename();

    shared_ptr<Image> resize(unsigned int w, unsigned int h);
    shared_ptr<Image> resize(float scale);

    void reset(float r, float g, float b);

    // get the specified pixel
    RGBAColor getPixel(int index);

    int initExp(ExpContext context, string name, int index, int pxPos);

    // i don't think the exp step cares really where the pixel comes from?
    // we'll initialize the ExpStep color with the specified color but it shouldn't actually matter
    Utils<ExpStep>::RGBAColorT& getPixel();

  private:
    // loads an image from a file
    void loadFromFile(string filename);

    unsigned int _w;
    unsigned int _h;

    // originating filename
    string _filename;

    // raw pixel data
    vector<unsigned char> _data;

    // variables for the expression context
    Utils<ExpStep>::RGBAColorT _vars;
  };
}

#endif
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
#include "third_party/Eigen/Dense"
#include <flann/flann.hpp>

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

    // loads from base64 string
    Image(unsigned int w, unsigned int h, string& data);

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
    RGBAColor getPixel(int x, int y);

    // checks for pixel equality within the same image
    bool pxEq(int x1, int y1, int x2, int y2);

    void setPixel(int x, int y, float r, float g, float b, float a);
    void setPixel(int x, int y, RGBAColor color);

    int initExp(ExpContext& context, string name, int index, int pxPos);

    // i don't think the exp step cares really where the pixel comes from?
    // we'll initialize the ExpStep color with the specified color but it shouldn't actually matter
    Utils<ExpStep>::RGBAColorT& getPixel();

    // structural comparison functions
    // this function by default uses the entire image.
    double structDiff(Image* y);

    // returns a list of vectorized image patches (L channel only)
    vector<Eigen::VectorXd> patches(int patchSize);

    // there are a couple functions to deal with binned comparison functions
    // counts the number of bins that image y is better than image x in (above some threshold)
    double structBinDiff(Image* y, int patchSize, double threshold = 0.01);

    // adds up the numerical difference in each bin instead of doing it one way
    double structTotalBinDiff(Image* y, int patchSize);

    // it's like the total but its divided by the total number of bins like some sort of average
    double structAvgBinDiff(Image* y, int patchSize);

    // just in case you want the numbers for yourself
    vector<double> structIndBinDiff(Image* y, int patchSize);

    double MSSIM(Image* y, int patchSize, double a = 0, double b = 0, double g = 1);

    // computes a SSIM with the specifed weights. Since we're only really interested in
    // structure here, the default weights are not the same as the SSIM paper
    double SSIMBinDiff(Eigen::VectorXd x, Eigen::VectorXd y, double a = 0, double b = 0, double g = 1);

    // will eliminate bins that are too similar from bins
    void eliminateBins(vector<Eigen::VectorXd>& bins, int patchSize, double threshold);

    // SSIM version of eliminate bins
    void eliminateBinsSSIM(vector<Eigen::VectorXd>& bins, int patchSize, double threshold,
      double a = 0, double b = 0, double g = 1);

    // returns the histogram intersection of the two images
    double histogramIntersection(Image* y, float binSize = 0.05);
    double proportionalHistogramIntersection(Image* y, float binSize = 0.05);

    // performs an analysis of the image contents and fills in a number of stats
    void analyze();

    // specialized analysis functions
    double avgAlpha(int x, int y, int w, int h);

    Image* diff(Image* other);

    // return a new image filled with the specified solid color
    // alpha is unaffected
    Image* fill(float r, float g, float b);

    // strokes the current image
    void stroke(Image* inclusionMap, int size, RGBColor color);

    // returns the chamfer distance (asymmetric for now) between the two images
    float chamferDistance(Image* y);

    float totalAlpha() { return _totalAlpha; }
    float avgAlpha() { return _avgAlpha; }
    float totalLuma() { return _totalLuma; }
    float avgLuma() { return _avgLuma; }

    vector<string>& getRenderMap();

  private:
    // loads an image from a file
    void loadFromFile(string filename);

    // returns true if the given pixel borders an outside pixel (8-way)
    bool bordersOutside(Image* inclusionMap, int x, int y);

    // returns true if the given pixel borders a zero alpha pixel
    bool bordersZeroAlpha(int x, int y);

    // returns the distance to the closest point in y
    float closestLabDist(LabColor& x, vector<LabColor>& y);

    unsigned int _w;
    unsigned int _h;

    // some stats
    float _totalAlpha;
    float _avgAlpha;
    float _totalLuma;
    float _avgLuma;
    // spatial frequencies?

    // originating filename
    string _filename;

    // raw pixel data
    vector<unsigned char> _data;
    
    // maps out which layer was the most recent to affect the pixel. mostly used internally
    vector<string> _renderLayerMap;

    // variables for the expression context
    Utils<ExpStep>::RGBAColorT _vars;
  };

  // I'm putting this in image because it's small enough to fit and 
  // it's sort of an image extension
  class ImportanceMap {
  public:
    ImportanceMap(int w, int h);
    ImportanceMap(const ImportanceMap& other);
    ~ImportanceMap();

    void setVal(float val, int x, int y);
    double getVal(int x, int y);

    // direct access is enabled
    vector<double> _data;

    // converts values in _data to renderable form
    shared_ptr<Image> getDisplayableImage();

    // since we allow direct access (for now) we need to compute
    // min/max on demand instead of keeping a running total
    double getMin();
    double getMax();
    double nonZeroMin();

    // path: location, base: filename (before extension)
    void dump(string path, string base);

    // normalizes the values in data to [0, 1]
    void normalize();

  private:
    shared_ptr<Image> _display;

    int _w;
    int _h;
  };
}



#endif
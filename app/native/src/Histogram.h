// Histogram.h - a histogram class

#pragma once

#include <map>

using namespace std;

// simple histogram class. Only parameter needed is the bin size. Bins are assumed to start at 0
// and the range of the bin is [min,max)
class Histogram {
public:
  Histogram(float binSize);
  Histogram(const Histogram& other);
  ~Histogram();

  // adds a value to the bin 
  void add(double x, unsigned int amt = 1);

  // if working with image data (common) will automatically convert,
  // and place an 8-bit pixel value in the histogram
  void add8BitPixel(char x, unsigned int amt = 1);

  // remove values from the bins
  void remove(double x, unsigned int amt);
  void remove8BitPixel(char x, unsigned int amt);

  // get values from the bins
  unsigned int get(unsigned int id);
  unsigned int get(double x);

  // Returns the distance between this histogram and another histogram
  double diff(Histogram& other);

  // Returns an average of values contained in the histogram bins
  double avg();

  // variance and st deviation
  double variance();
  double stdev();

private:
  inline unsigned int closestBin(double val);

  // Maps bins to the number of pixels in each bin
  // bin numbers are stored as unsigned int to avoid floating point indexing errors
  map<unsigned int, unsigned int> _data;

  // total number of elements in the histogram
  unsigned int _count;

  // bin size
  float _binSize;
};
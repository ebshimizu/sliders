// Histogram.h - a histogram class

#pragma once

#include <map>
#include <set>
#include <string>

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
  void add8BitPixel(unsigned char x, unsigned int amt = 1);

  // remove values from the bins
  void remove(double x, unsigned int amt);
  void remove8BitPixel(unsigned char x, unsigned int amt);

  // get values from the bins
  unsigned int get(unsigned int id);
  unsigned int get(double x);
  double getPercent(unsigned int id);
  double getPercent(double x);

  // retrieves info about which bin has the most stuff in it
  // (optional) returns the id of the largest bin
  unsigned int largestBin(unsigned int* id = nullptr);
  double largestBinPercent(unsigned int* id = nullptr);

  // returns the number of elements at or above the specified bin
  unsigned int countAbove(unsigned int id);
  unsigned int countAbove(double x);
  double countAbovePct(unsigned int id);
  double countAbovePct(double x);

  // same as count above just in the other direction
  unsigned int countBelow(unsigned int id);
  unsigned int countBelow(double x);
  double countBelowPct(unsigned int id);
  double countBelowPct(double x);

  // there are a variety of distance metrics that can be used
  // Euclidean per-bin distance, not a great metric but fairly quick to calculate
  double l2(Histogram& other);

  // Chi-squared distance
  double chiSq(Histogram& other);

  // intersection
  double intersection(Histogram& other);

  // is this a useful function?
  double proportionalIntersection(Histogram& other);

  // Earth-Mover's Distance (if needed)
  //double emd(Histogram& other);

  // Returns an average of values contained in the histogram bins
  double avg();

  // variance and st deviation
  double variance();
  double stdev();

  // string version of the histogram
  string toString();

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
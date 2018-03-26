#include "Histogram.h"
#include <algorithm>

Histogram::Histogram(float binSize) : _binSize(binSize), _count(0)
{
}

Histogram::Histogram(const Histogram & other) : _binSize(other._binSize), _count(other._count),
  _data(other._data)
{
}

Histogram::~Histogram() {
}

void Histogram::add(double x, unsigned int amt)
{
  _data[closestBin(x)] += amt;
  _count += amt;
}

void Histogram::add8BitPixel(unsigned char x, unsigned int amt)
{
  add(x / 255.0, amt);
}

void Histogram::remove(double x, unsigned int amt)
{
  // underflow protect
  if (_data[closestBin(x)] < amt) {
    _count -= _data[closestBin(x)];
    _data[closestBin(x)] = 0;
  }
  else {
    _data[closestBin(x)] -= amt;
    _count -= amt;
  }
}

void Histogram::remove8BitPixel(unsigned char x, unsigned int amt)
{
  remove(x / 255.0, amt);
}

unsigned int Histogram::get(unsigned int id)
{
  if (_data.count(id) > 0)
    return _data[id];
  
  return 0;
}

unsigned int Histogram::get(double x)
{
  return _data[closestBin(x)];
}

double Histogram::getPercent(unsigned int id)
{
  return (get(id) / (double)_count);
}

double Histogram::getPercent(double x)
{
  return getPercent(closestBin(x));
}

unsigned int Histogram::largestBin(unsigned int* id)
{
  unsigned int largest = 0;
  unsigned int lid = 0;

  for (auto& bin : _data) {
    if (bin.second > largest) {
      largest = bin.second;
      lid = bin.first;
    }
  }

  if (id != nullptr) {
    *id = lid;
  }

  return largest;
}

double Histogram::largestBinPercent(unsigned int * id)
{
  unsigned int unNorm = largestBin(id);

  return (unNorm / (double)_count);
}

unsigned int Histogram::countAbove(unsigned int id)
{
  unsigned int count = 0;
  for (auto& bin : _data) {
    if (bin.first >= id) {
      count += bin.second;
    }
  }

  return count;
}

unsigned int Histogram::countAbove(double x)
{
  return countAbove(closestBin(x));
}

double Histogram::countAbovePct(unsigned int id)
{
  return countAbove(id) / (double)_count;
}

double Histogram::countAbovePct(double x)
{
  return countAbovePct(closestBin(x));
}

unsigned int Histogram::countBelow(unsigned int id)
{
  unsigned int count = 0;
  for (auto& bin : _data) {
    if (bin.first <= id) {
      count += bin.second;
    }
  }

  return count;
}

unsigned int Histogram::countBelow(double x)
{
  return countBelow(closestBin(x));
}

double Histogram::countBelowPct(unsigned int id)
{
  return countBelow(id) / (double)_count;
}

double Histogram::countBelowPct(double x)
{
  return countBelow(x) / (double)_count;
}

double Histogram::l2(Histogram & other)
{
  // histograms are sparse, so we need a set of bins that each one has
  set<unsigned int> activeBins;

  for (auto& bin : _data)
    activeBins.insert(bin.first);

  for (auto& bin : other._data)
    activeBins.insert(bin.first);

  double sum = 0;
  for (auto& binID : activeBins) {
    double x = get(binID) / (double)_count;
    double y = other.get(binID) / (double)other._count;

    sum += pow(x - y, 2);
  }

  return sqrt(sum);
}

double Histogram::chiSq(Histogram & other)
{
  // histograms are sparse, so we need a set of bins that each one has
  // also everything should be normalized before doing distances
  set<unsigned int> activeBins;

  for (auto& bin : _data)
    activeBins.insert(bin.first);

  for (auto& bin : other._data)
    activeBins.insert(bin.first);

  double sum = 0;
  for (auto& binID : activeBins) {
    double x = get(binID) / (double)_count;
    double y = other.get(binID) / (double)other._count;

    // for some reason if the bins are 0, just continue
    if (x == 0 && y == 0)
      continue;

    sum += pow(x - y, 2) / (x + y);
  }

  return sqrt(0.5 * sum);
}

double Histogram::intersection(Histogram & other)
{
  // assert same size
  if (other._count != _count) {
    return -1;
  }

  double sum = 0;

  // ok to do single iteration (min of a missing bin is 0)
  for (auto& bin : _data) {
    unsigned int val = min(bin.second, other.get(bin.first));
    sum += val;
  }

  // normalize
  return sum / _count;
}

double Histogram::proportionalIntersection(Histogram & other)
{
  // identical size doesn't matter here

  double sum = 0;

  for (auto& bin : _data) {
    unsigned int val = min(getPercent(bin.first), other.getPercent(bin.first));
    sum += val;
  }

  // this should already be normalized (max 1)
  return sum;
}

double Histogram::avg()
{
  double sum = 0;

  for (auto& bin : _data) {
    sum += (bin.first * _binSize);
  }

  return sum / _count;
}

double Histogram::variance()
{
  double a = avg();

  double sum = 0;

  for (const auto& vals : _data) {
    // bin deviation
    double dev = (vals.first * _binSize) - a;
    dev *= dev;

    // count the number of elements in the bin, add to running total
    sum += dev * vals.second;
  }

  // return mean of sum
  return sum / _count;
}

double Histogram::stdev()
{
  return sqrt(variance());
}

string Histogram::toString()
{
  // prints the normalized histogram
  string hist = "";

  for (auto& bin : _data) {
    hist += to_string(bin.first) + ": " + to_string(bin.second / (double)_count) + "\n";
  }

  return hist;
}

inline unsigned int Histogram::closestBin(double val)
{
  return (unsigned int)(val / _binSize);
}
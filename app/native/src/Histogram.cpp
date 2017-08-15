#include "Histogram.h"

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

void Histogram::add8BitPixel(char x, unsigned int amt)
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

void Histogram::remove8BitPixel(char x, unsigned int amt)
{
  remove(x / 255.0, amt);
}

unsigned int Histogram::get(unsigned int id)
{
  return _data[id];
}

unsigned int Histogram::get(double x)
{
  return _data[closestBin(x)];
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

inline unsigned int Histogram::closestBin(double val)
{
  return (unsigned int)(val / _binSize);
}
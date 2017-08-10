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
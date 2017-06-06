#include "Compositor.h"

/*
Functions for testing generated code. They are placed in the main project for easier access
through Node (most of the program is debugged with node since that's the actual interface
used to call the functions)
*/

namespace Comp {
  // Functions for the compTest to actually use
  vector<double> cvtT(vector<double> params);

  // test function
  vector<double> compTest(const vector<double> &params);

  // compares the test function against the compositor settings at the given pixel
  double compare(Compositor* c, int x, int y);

  // compares all pixels and writes the results to an image
  void compareAll(Compositor* c, string filename);
}
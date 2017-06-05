#include "Compositor.h"

/*
Functions for testing generated code. They are placed in the main project for easier access
through Node (most of the program is debugged with node since that's the actual interface
used to call the functions)
*/

namespace Comp {
  // test function
  vector<double> compTest(const vector<double> &params);

  // compares the test function against the compositor settings at the given pixel
  bool compare(Compositor* c, int x, int y);
}
#include "Compositor.h"

/*
Functions for testing generated code. They are placed in the main project for easier access
through Node (most of the program is debugged with node since that's the actual interface
used to call the functions)
*/

namespace Comp {
  // Functions for the compTest to actually use
  vector<double> cvtT(vector<double> params);
  vector<double> overlay(vector<double> params);
  vector<double> hardLight(vector<double> params);
  vector<double> softLight(vector<double> params);
  vector<double> linearDodgeAlpha(vector<double> params);
  vector<double> linearBurn(vector<double> params);
  vector<double> colorDodge(vector<double> params);
  vector<double> linearLight(vector<double> params);
  vector<double> lighten(vector<double> params);
  vector<double> color(vector<double> params);
  vector<double> darken(vector<double> params);
  vector<double> pinLight(vector<double> params);
  vector<double> RGBToHSL(vector<double> params);
  vector<double> HSLToRGB(vector<double> params);
  vector<double> RGBToHSY(vector<double> params);
  vector<double> HSYToRGB(vector<double> params);
  vector<double> levels(vector<double> params);
  vector<double> clamp(vector<double> params);
  vector<double> selectiveColor(vector<double> params);
  double colorBalance(double px, double shadow, double mid, double high);
  vector<double> colorBalanceAdjust(vector<double> params);
  vector<double> photoFilter(vector<double> params);
  vector<double> lighterColorizeAdjust(vector<double> params);

  // test function
  vector<double> compTest(const vector<double> &params);

  // compares the test function against the compositor settings at the given pixel
  double compare(Compositor* c, int x, int y);

  // compares all pixels and writes the results to an image
  void compareAll(Compositor* c, string filename);
}
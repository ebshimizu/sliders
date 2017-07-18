#include "Compositor.h"
#pragma once

namespace Comp {
  // Functions for the compTest to actually use
  template<class T>
  vector<T> cvtT(vector<T> params) {
    vector<T> res;
    if (params[1] == 0.0)
      return { (T) 0 };

    T v = params[0] / params[1];
    res.push_back((v > 1.0) ? (T)1.0 : (v < 0.0) ? (T)0.0 : v);
    return res;
  }

  template<class T>
  vector<T> overlay(vector<T> params) {
    T a = params[0];
    T b = params[1];
    T alpha1 = params[2];
    T alpha2 = params[3];

    if ((T)2 * a <= alpha1) {
      return { b * a * (T)2 + b * ((T)1 - alpha1) + a * ((T)1 - alpha2) };
    }
    else {
      return { b * ((T)1 + alpha1) + a * ((T)1 + alpha2) - (T)2 * a * b - alpha1 * alpha2 };
    }
  }

  template<class T>
  vector<T> hardLight(vector<T> params) {
    T a = params[0];
    T b = params[1];
    T alpha1 = params[2];
    T alpha2 = params[3];

    if (2 * b <= alpha2) {
      return { 2 * b * a + b * (1 - alpha1) + a * (1 - alpha2) };
    }
    else {
      return { b * (1 + alpha1) + a * (1 + alpha2) - alpha1 * alpha2 - 2 * a * b };
    }
  }

  template<class T>
  vector<T> softLight(vector<T> params) {
    T Dca = params[0];
    T Sca = params[1];
    T Da = params[2];
    T Sa = params[3];

    T m = (Da == (T)0) ? (T)0 : Dca / Da;

    if ((T)2 * Sca <= Sa) {
      return { Dca * (Sa + ((T)2 * Sca - Sa) * ((T)1 - m)) + Sca * ((T)1 - Da) + Dca * ((T)1 - Sa) };
    }
    else if ((T)2 * Sca > Sa && (T)4 * Dca <= Da) {
      return { Da * ((T)2 * Sca - Sa) * ((T)16 * m * m * m - (T)12 * m * m - (T)3 * m) + Sca - Sca * Da + Dca };
    }
    else if ((T)2 * Sca > Sa && (T)4 * Dca > Da) {
      return { Da * ((T)2 * Sca - Sa) * (sqrt(m) - m) + Sca - Sca * Da + Dca };
    }
    else {
      return { Sca + Dca * ((T)1 - Sa) };
    }
  }

  template<class T>
  vector<T> linearDodgeAlpha(vector<T> params) {
    T aa = params[0];
    T ab = params[1];

    return { (aa + ab > (T)1) ? (T)1 : (aa + ab) };
  }

  template<class T>
  vector<T> linearBurn(vector<T> params) {
    T Dc = params[0];
    T Sc = params[1];
    T Da = params[2];
    T Sa = params[3];

    if (Da == (T)0)
      return { Sc };

    T burn = Dc + Sc - (T)1;

    // normal blend
    return { burn * Sa + Dc * ((T)1 - Sa) };
  }

  template<class T>
  vector<T> colorDodge(vector<T> params) {
    T Dca = params[0];
    T Sca = params[1];
    T Da = params[2];
    T Sa = params[3];

    if (Sca == Sa && Dca == 0) {
      return { Sca * (1 - Da) };
    }
    else if (Sca == Sa) {
      return { Sa * Da + Sca * (1 - Da) + Dca * (1 - Sa) };
    }
    else if (Sca < Sa) {
      return { Sa * Da * min(1.0, Dca / Da * Sa / (Sa - Sca)) + Sca * (1 - Da) + Dca * (1 - Sa) };
    }

    // probably never get here but compiler is yelling at me
    return { 0 };
  }

  template<class T>
  vector<T> linearLight(vector<T> params) {
    T Dc = params[0];
    T Sc = params[1];
    T Da = params[2];
    T Sa = params[3];

    if (Da == (T)0)
      return { Sc };

    T light = Dc + (T)2 * Sc - (T)1;
    return { light * Sa + Dc * ((T)1 - Sa) };
  }

  template<class T>
  vector<T> lighten(vector<T> params) {
    T Dca = params[0];
    T Sca = params[1];
    T Da = params[2];
    T Sa = params[3];

    if (Sca > Dca) {
      return { Sca + Dca * ((T)1 - Sa) };
    }
    else {
      return { Dca + Sca * ((T)1 - Da) };
    }
  }

  template<class T>
  vector<T> color(vector<T> params) {
    Utils<T>::RGBColorT src, dest;
    dest._r = params[0];
    dest._g = params[1];
    dest._b = params[2];
    src._r = params[3];
    src._g = params[4];
    src._b = params[5];
    T Da = params[6];
    T Sa = params[7];

    if (Da == (T)0) {
      src._r *= Sa;
      src._g *= Sa;
      src._b *= Sa;

      return { src._r, src._g, src._b };
    }

    if (Sa == (T)0) {
      dest._r *= Da;
      dest._g *= Da;
      dest._b *= Da;

      return { dest._r, dest._g, dest._b };
    }

    // color keeps dest luma and keeps top hue and chroma
    Utils<T>::HSYColorT dc = Utils<T>::RGBToHSY(dest);
    Utils<T>::HSYColorT sc = Utils<T>::RGBToHSY(src);
    dc._h = sc._h;
    dc._s = sc._s;

    Utils<T>::RGBColorT res = Utils<T>::HSYToRGB(dc);

    // actually have to blend here...
    res._r = res._r * Sa + dest._r * Da * ((T)1 - Sa);
    res._g = res._g * Sa + dest._g * Da * ((T)1 - Sa);
    res._b = res._b * Sa + dest._b * Da * ((T)1 - Sa);

    return { res._r, res._g, res._b };
  }

  template<class T>
  vector<T> darken(vector<T> params) {
    T Dca = params[0];
    T Sca = params[1];
    T Da = params[2];
    T Sa = params[3];

    if (Sca > Dca) {
      return { Dca + Sca * (1 - Da) };
    }
    else {
      return { Sca + Dca * (1 - Sa) };
    }
  }

  template<class T>
  vector<T> pinLight(vector<T> params) {
    T Dca = params[0];
    T Sca = params[1];
    T Da = params[2];
    T Sa = params[3];

    if (Da == 0)
      return { Sca };

    if (Sca < 0.5f) {
      return darken<T>({ Dca, Sca * 2, Da, Sa });
    }
    else {
      return lighten<T>({ Dca, 2 * (Sca - 0.5f), Da, Sa });
    }
  }

  template<class T>
  vector<T> RGBToHSL(vector<T> params) {
    auto res = Utils<T>::RGBToHSL(params[0], params[1], params[2]);

    return { res._h, res._s, res._l };
  }

  template<class T>
  vector<T> HSLToRGB(vector<T> params) {
    auto res = Utils<T>::HSLToRGB(params[0], params[1], params[2]);
    return { res._r, res._g, res._b };
  }

  template<class T>
  vector<T> RGBToHSY(vector<T> params) {
    auto res = Utils<T>::RGBToHSY(params[0], params[1], params[2]);
    return { res._h, res._s, res._y };
  }

  template<class T>
  vector<T> HSYToRGB(vector<T> params) {
    auto res = Utils<T>::HSYToRGB(params[0], params[1], params[2]);
    return { res._r, res._g, res._b };
  }

  template<class T>
  vector<T> levels(vector<T> params) {
    T px = params[0];
    T inMin = params[1];
    T inMax = params[2];
    T gamma = params[3];
    T outMin = params[4];
    T outMax = params[5];

    // input remapping
    T out = min(max(px - inMin, (T)0.0) / (inMax - inMin), (T)1.0);

    if (inMax == inMin) {
      out = (T)1;
    }

    if (out > (T)0) {
      // gamma correction
      if (gamma < (T)1e-6) {
        gamma = (T)1e-6; // just stick this at some really small value
      }

      out = pow(out, (T)1 / gamma);
    }

    // output remapping
    out = out * (outMax - outMin) + outMin;

    return { out };
  }

  template<class T>
  vector<T> clamp(vector<T> params) {
    return { clamp<T>(params[0], params[1], params[2]) };
  }

  template<class T>
  vector<T> selectiveColor(vector<T> params) {
    Utils<T>::RGBColorT adjPx;
    adjPx._r = (T)params[0];
    adjPx._g = (T)params[1];
    adjPx._b = (T)params[2];

    map<string, map<string, T>> data;
    data["blacks"]["black"] = params[3];
    data["blacks"]["cyan"] = params[4];
    data["blacks"]["magenta"] = params[5];
    data["blacks"]["yellow"] = params[6];
    data["blues"]["black"] = params[7];
    data["blues"]["cyan"] = params[8];
    data["blues"]["magenta"] = params[9];
    data["blues"]["yellow"] = params[10];
    data["cyans"]["black"] = params[11];
    data["cyans"]["cyan"] = params[12];
    data["cyans"]["magenta"] = params[13];
    data["cyans"]["yellow"] = params[14];
    data["greens"]["black"] = params[15];
    data["greens"]["cyan"] = params[16];
    data["greens"]["magenta"] = params[17];
    data["greens"]["yellow"] = params[18];
    data["magentas"]["black"] = params[19];
    data["magnetas"]["cyan"] = params[20];
    data["magentas"]["magenta"] = params[21];
    data["magentas"]["yellow"] = params[22];
    data["neutrals"]["black"] = params[23];
    data["neutrals"]["cyan"] = params[24];
    data["neutrals"]["magenta"] = params[25];
    data["neutrals"]["yellow"] = params[26];
    data["reds"]["black"] = params[27];
    data["reds"]["cyan"] = params[28];
    data["reds"]["magenta"] = params[29];
    data["reds"]["yellow"] = params[30];
    data["whites"]["black"] = params[31];
    data["whites"]["cyan"] = params[32];
    data["whites"]["magenta"] = params[33];
    data["whites"]["yellow"] = params[34];
    data["yellows"]["black"] = params[35];
    data["yellows"]["cyan"] = params[36];
    data["yellows"]["magenta"] = params[37];
    data["yellows"]["yellow"] = params[38];

    for (auto& c : data) {
      for (auto& p : c.second) {
        p.second = (p.second - (T)0.5) * (T)2;
      }
    }

    // convert to hsl
    Utils<T>::HSLColorT hslColor = Utils<T>::RGBToHSL(adjPx._r, adjPx._g, adjPx._b);
    T chroma = max(adjPx._r, max(adjPx._g, adjPx._b)) - min(adjPx._r, min(adjPx._g, adjPx._b));

    // determine which set of parameters we're using to adjust
    // determine chroma interval
    T interval = floor(hslColor._h / (T)60);
    string c1, c2, c3, c4;

    if (interval >= (T)0 && interval < (T)1) {
      c1 = intervalNames[0];
      c2 = intervalNames[1];
    }
    if (interval >= (T)1 && interval < (T)2) {
      c1 = intervalNames[1];
      c2 = intervalNames[2];
    }
    if (interval >= (T)2 && interval < (T)3) {
      c1 = intervalNames[2];
      c2 = intervalNames[3];
    }
    if (interval >= (T)3 && interval < (T)4) {
      c1 = intervalNames[3];
      c2 = intervalNames[4];
    }
    if (interval >= (T)4 && interval < (T)5) {
      c1 = intervalNames[4];
      c2 = intervalNames[5];
    }
    if (interval >= (T)5 && interval < (T)6) {
      c1 = intervalNames[5];
      c2 = intervalNames[0];
    }

    c3 = "neutrals";

    // non-chromatic colors
    if (hslColor._l < (T)0.5) {
      c4 = "blacks";
    }
    else {
      c4 = "whites";
    }

    // compute weights
    T w1, w2, w3, w4, wc;

    // chroma
    wc = chroma / (T)1.0f;

    // hue - always 60 deg intervals
    w1 = (T)1 - ((hslColor._h - (interval * (T)60.0f)) / (T)60.0f);  // distance from low interval
    w2 = (T)1 - w1;

    // luma - measure distance from midtones, w3 is always midtone
    w3 = (T)1 - abs(hslColor._l - (T)0.5f);
    w4 = (T)1 - w3;

    // do the adjustment
    Utils<T>::CMYKColorT cmykColor = Utils<T>::RGBToCMYK(adjPx._r, adjPx._g, adjPx._b);

    // we assume relative is true always here.
    //if (adj["relative"] > 0) {
    // relative
    cmykColor._c += cmykColor._c * (w1 * data[c1]["cyan"] + w2 * data[c2]["cyan"]) * wc + (w3 * data[c3]["cyan"] + w4 * data[c4]["cyan"]) * ((T)1 - wc);
    cmykColor._m += cmykColor._m * (w1 * data[c1]["magenta"] + w2 * data[c2]["magenta"]) * wc + (w3 * data[c3]["magenta"] + w4 * data[c4]["magenta"]) * ((T)1 - wc);
    cmykColor._y += cmykColor._y * (w1 * data[c1]["yellow"] + w2 * data[c2]["yellow"]) * wc + (w3 * data[c3]["yellow"] + w4 * data[c4]["yellow"]) * ((T)1 - wc);
    cmykColor._k += cmykColor._k * (w1 * data[c1]["black"] + w2 * data[c2]["black"]) * wc + (w3 * data[c3]["black"] + w4 * data[c4]["black"]) * ((T)1 - wc);
    //}
    //else {
    // absolute
    //  cmykColor._c += (w1 * data[c1]["cyan"] + w2 * data[c2]["cyan"]) * wc + (w3 * data[c3]["cyan"] + w4 * data[c4]["cyan"]) * (1 - wc);
    //  cmykColor._m += (w1 * data[c1]["magenta"] + w2 * data[c2]["magenta"]) * wc + (w3 * data[c3]["magenta"] + w4 * data[c4]["magenta"]) * (1 - wc);
    //  cmykColor._y += (w1 * data[c1]["yellow"] + w2 * data[c2]["yellow"]) * wc + (w3 * data[c3]["yellow"] + w4 * data[c4]["yellow"]) * (1 - wc);
    //  cmykColor._k += (w1 * data[c1]["black"] + w2 * data[c2]["black"]) * wc + (w3 * data[c3]["black"] + w4 * data[c4]["black"]) * (1 - wc);
    //}

    Utils<T>::RGBColorT res = Utils<T>::CMYKToRGB(cmykColor);
    adjPx._r = clamp<T>(res._r, (T)0, (T)1);
    adjPx._g = clamp<T>(res._g, (T)0, (T)1);
    adjPx._b = clamp<T>(res._b, (T)0, (T)1);

    return { adjPx._r, adjPx._g, adjPx._b };
  }

  template<class T>
  T colorBalance(T px, T shadow, T mid, T high) {
    T a = (T)0.25;
    T b = (T)0.333;
    T scale = (T)0.7;

    T s = shadow * (clamp<T>((px - b) / -a + (T)0.5f, (T)0, (T)1.0f) * scale);
    T m = mid * (clamp<T>((px - b) / a + (T)0.5f, (T)0, (T)1.0f) * clamp<T>((px + b - (T)1.0f) / -a + (T)0.5f, (T)0, (T)1.0f) * scale);
    T h = high * (clamp<T>((px + b - (T)1.0f) / a + (T)0.5f, (T)0, (T)1.0f) * scale);

    return clamp<T>(px + s + m + h, (T)0, (T)1.0);
  }

  template<class T>
  vector<T> colorBalanceAdjust(vector<T> params) {
    Utils<T>::RGBColorT adjPx;
    adjPx._r = params[0];
    adjPx._g = params[1];
    adjPx._b = params[2];

    Utils<T>::RGBColorT balanced;
    balanced._r = colorBalance(adjPx._r, (params[3] - (T)0.5) * (T)2, (params[6] - (T)0.5) * (T)2, (params[9] - (T)0.5) * (T)2);
    balanced._g = colorBalance(adjPx._g, (params[4] - (T)0.5) * (T)2, (params[7] - (T)0.5) * (T)2, (params[10] - (T)0.5) * (T)2);
    balanced._b = colorBalance(adjPx._b, (params[5] - (T)0.5) * (T)2, (params[8] - (T)0.5) * (T)2, (params[11] - (T)0.5) * (T)2);

    // assume preserve luma true
    //if (adj["preserveLuma"] > 0) {
    Utils<T>::HSLColorT l = Utils<T>::RGBToHSL(balanced);
    T originalLuma = (T)0.5 * (max(adjPx._r, max(adjPx._g, adjPx._b)) + min(adjPx._r, min(adjPx._g, adjPx._b)));
    balanced = Utils<T>::HSLToRGB(l._h, l._s, originalLuma);
    //}

    adjPx._r = clamp<T>(balanced._r, (T)0, (T)1);
    adjPx._g = clamp<T>(balanced._g, (T)0, (T)1);
    adjPx._b = clamp<T>(balanced._b, (T)0, (T)1);

    return { adjPx._r, adjPx._g, adjPx._b };
  }

  template<class T>
  vector<T> photoFilter(vector<T> params) {
    Utils<T>::RGBColorT adjPx;
    adjPx._r = params[0];
    adjPx._g = params[1];
    adjPx._b = params[2];

    T d = params[3];
    T fr = adjPx._r * params[4];
    T fg = adjPx._g * params[5];
    T fb = adjPx._b * params[6];

    // assuming preserve luma always
    //if (adj["preserveLuma"] > 0) {
    Utils<T>::HSLColorT l = Utils<T>::RGBToHSL(fr, fg, fb);
    T originalLuma = 0.5 * (max(adjPx._r, max(adjPx._g, adjPx._b)) + min(adjPx._r, min(adjPx._g, adjPx._b)));
    Utils<T>::RGBColorT rgb = Utils<T>::HSLToRGB(l._h, l._s, originalLuma);
    fr = rgb._r;
    fg = rgb._g;
    fb = rgb._b;
    //}

    // weight by density
    adjPx._r = clamp<T>(fr * d + adjPx._r * ((T)1 - d), (T)0, (T)1);
    adjPx._g = clamp<T>(fg * d + adjPx._g * ((T)1 - d), (T)0, (T)1);
    adjPx._b = clamp<T>(fb * d + adjPx._b * ((T)1 - d), (T)0, (T)1);

    return { adjPx._r, adjPx._g, adjPx._b };
  }

  template<class T>
  vector<T> lighterColorizeAdjust(vector<T> params) {
    Utils<T>::RGBColorT adjPx;
    adjPx._r = params[0];
    adjPx._g = params[1];
    adjPx._b = params[2];

    T sr = params[3];
    T sg = params[4];
    T sb = params[5];
    T a = params[6];
    T y = 0.299f * sr + 0.587f * sg + 0.114f * sb;

    T yp = 0.299f * adjPx._r + 0.587f * adjPx._g + 0.114f * adjPx._b;

    adjPx._r = (yp > y) ? adjPx._r : sr;
    adjPx._g = (yp > y) ? adjPx._g : sg;
    adjPx._b = (yp > y) ? adjPx._b : sb;

    // blend the resulting colors according to alpha
    adjPx._r = clamp<T>(adjPx._r * a + adjPx._r * (1 - a), 0, 1);
    adjPx._g = clamp<T>(adjPx._g * a + adjPx._g * (1 - a), 0, 1);
    adjPx._b = clamp<T>(adjPx._b * a + adjPx._b * (1 - a), 0, 1);

    return { adjPx._r, adjPx._g, adjPx._b };
  }

}
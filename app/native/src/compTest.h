template <class T>
vector<T> compTest(const T* const paramsA, const vector<double> &paramsB)
{
    vector<T> result(4);
    
    const T s0 = (T) paramsA[0]; // 1_r;
    const T s1 = (T) paramsA[1]; // 1_g;
    const T s2 = (T) paramsA[2]; // 1_b;
    const T s3 = (T) paramsA[3]; // 1_a;
    const T s4 = (T) paramsB[0]; // x_1_opacity;
    const T s5 = (T) paramsA[4]; // 2_r;
    const T s6 = (T) paramsA[5]; // 2_g;
    const T s7 = (T) paramsA[6]; // 2_b;
    const T s8 = (T) paramsA[7]; // 2_a;
    const T s9 = (T) paramsB[1]; // x_2_opacity;
    const T s10 = (T) paramsA[8]; // 3_r;
    const T s11 = (T) paramsA[9]; // 3_g;
    const T s12 = (T) paramsA[10]; // 3_b;
    const T s13 = (T) paramsA[11]; // 3_a;
    const T s14 = (T) paramsB[2]; // x_3_opacity;
    const T s15 = (T)1.000000;
    const T s16 = s3 * s15;
    const T s17 = (T)0.000000;
    const T s18 = s17 * s16;
    const T s19 = (T)0.000000;
    const T s20 = s19 + s16;
    const T s21 = s20 - s18;
    const T s22 = s0 * s16;
    const T s23 = s1 * s16;
    const T s24 = s2 * s16;
    const T s25 = (T)1.000000;
    const T s26 = s25 - s16;
    const T s27 = (T)0.000000;
    const T s28 = s27 * s26;
    const T s29 = s22 + s28;
    vector<T> v30 = { s29, s21 };
    auto s30 = cvtT(v30);
    const T s31 = s30[0];
    const T s32 = (T)1.000000;
    const T s33 = s32 - s16;
    const T s34 = (T)0.000000;
    const T s35 = s34 * s33;
    const T s36 = s23 + s35;
    vector<T> v37 = { s36, s21 };
    auto s37 = cvtT(v37);
    const T s38 = s37[0];
    const T s39 = (T)1.000000;
    const T s40 = s39 - s16;
    const T s41 = (T)0.000000;
    const T s42 = s41 * s40;
    const T s43 = s24 + s42;
    vector<T> v44 = { s43, s21 };
    auto s44 = cvtT(v44);
    const T s45 = s44[0];
    const T s46 = (T)1.000000;
    const T s47 = s8 * s46;
    const T s48 = s21 * s47;
    const T s49 = s21 + s47;
    const T s50 = s49 - s48;
    const T s51 = s5 * s47;
    const T s52 = s6 * s47;
    const T s53 = s7 * s47;
    const T s54 = s31 * s21;
    const T s55 = s38 * s21;
    const T s56 = s45 * s21;
    const T s57 = (T)1.000000;
    const T s58 = s57 - s47;
    const T s59 = s54 * s58;
    const T s60 = s51 + s59;
    vector<T> v61 = { s60, s50 };
    auto s61 = cvtT(v61);
    const T s62 = s61[0];
    const T s63 = (T)1.000000;
    const T s64 = s63 - s47;
    const T s65 = s55 * s64;
    const T s66 = s52 + s65;
    vector<T> v67 = { s66, s50 };
    auto s67 = cvtT(v67);
    const T s68 = s67[0];
    const T s69 = (T)1.000000;
    const T s70 = s69 - s47;
    const T s71 = s56 * s70;
    const T s72 = s53 + s71;
    vector<T> v73 = { s72, s50 };
    auto s73 = cvtT(v73);
    const T s74 = s73[0];
    const T s75 = (T)1.000000;
    const T s76 = s13 * s75;
    const T s77 = s50 * s76;
    const T s78 = s50 + s76;
    const T s79 = s78 - s77;
    const T s80 = s10 * s76;
    const T s81 = s11 * s76;
    const T s82 = s12 * s76;
    const T s83 = s62 * s50;
    const T s84 = s68 * s50;
    const T s85 = s74 * s50;
    const T s86 = (T)1.000000;
    const T s87 = s86 - s76;
    const T s88 = s83 * s87;
    const T s89 = s80 + s88;
    vector<T> v90 = { s89, s79 };
    auto s90 = cvtT(v90);
    const T s91 = s90[0];
    const T s92 = (T)1.000000;
    const T s93 = s92 - s76;
    const T s94 = s84 * s93;
    const T s95 = s81 + s94;
    vector<T> v96 = { s95, s79 };
    auto s96 = cvtT(v96);
    const T s97 = s96[0];
    const T s98 = (T)1.000000;
    const T s99 = s98 - s76;
    const T s100 = s85 * s99;
    const T s101 = s82 + s100;
    vector<T> v102 = { s101, s79 };
    auto s102 = cvtT(v102);
    const T s103 = s102[0];
    result[0] = s91; // R;
    result[1] = s97; // G;
    result[2] = s103; // B;
    result[3] = s79; // A;
    
    return result;
}

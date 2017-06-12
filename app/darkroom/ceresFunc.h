static const int ceresFunc_paramACount = 11;
static const int ceresFunc_paramBCount = 12;

template <class T>
vector<T> ceresFunc(const T* const paramsA, const vector<double> &paramsB)
{
    vector<T> result(4);
    
    const T s0 = (T) paramsB[0]; // Rectangle 1_r;
    const T s1 = (T) paramsB[1]; // Rectangle 1_g;
    const T s2 = (T) paramsB[2]; // Rectangle 1_b;
    const T s3 = (T) paramsB[3]; // Rectangle 1_a;
    const T s4 = (T) paramsA[0]; // x_Rectangle 1_opacity;
    const T s5 = (T) paramsB[4]; // Ellipse 1_r;
    const T s6 = (T) paramsB[5]; // Ellipse 1_g;
    const T s7 = (T) paramsB[6]; // Ellipse 1_b;
    const T s8 = (T) paramsB[7]; // Ellipse 1_a;
    const T s9 = (T) paramsA[1]; // x_Ellipse 1_opacity;
    const T s10 = (T) paramsA[2]; // x_Ellipse 1_a;
    const T s11 = (T) paramsA[3]; // x_Ellipse 1_b;
    const T s12 = (T) paramsA[4]; // x_Ellipse 1_g;
    const T s13 = (T) paramsA[5]; // x_Ellipse 1_r;
    const T s14 = (T) paramsB[8]; // Ellipse 2_r;
    const T s15 = (T) paramsB[9]; // Ellipse 2_g;
    const T s16 = (T) paramsB[10]; // Ellipse 2_b;
    const T s17 = (T) paramsB[11]; // Ellipse 2_a;
    const T s18 = (T) paramsA[6]; // x_Ellipse 2_opacity;
    const T s19 = (T) paramsA[7]; // x_Ellipse 2_a;
    const T s20 = (T) paramsA[8]; // x_Ellipse 2_b;
    const T s21 = (T) paramsA[9]; // x_Ellipse 2_g;
    const T s22 = (T) paramsA[10]; // x_Ellipse 2_r;
    const T s23 = (T)1.000000;
    const T s24 = s3 * s23;
    const T s25 = (T)0.000000;
    const T s26 = s25 * s24;
    const T s27 = (T)0.000000;
    const T s28 = s27 + s24;
    const T s29 = s28 - s26;
    const T s30 = s0 * s24;
    const T s31 = s1 * s24;
    const T s32 = s2 * s24;
    const T s33 = (T)0.000000;
    vector<T> v34 = { s33, s24 };
    auto s34 = linearDodgeAlpha(v34);
    const T s35 = s34[0];
    const T s36 = (T)0.000000;
    const T s37 = s30 + s36;
    vector<T> v38 = { s37, s35 };
    auto s38 = cvtT(v38);
    const T s39 = s38[0];
    const T s40 = (T)0.000000;
    const T s41 = s31 + s40;
    vector<T> v42 = { s41, s35 };
    auto s42 = cvtT(v42);
    const T s43 = s42[0];
    const T s44 = (T)0.000000;
    const T s45 = s32 + s44;
    vector<T> v46 = { s45, s35 };
    auto s46 = cvtT(v46);
    const T s47 = s46[0];
    const T s48 = (T)1.000000;
    const T s49 = s48 - s10;
    const T s50 = s5 * s49;
    const T s51 = s13 * s10;
    const T s52 = s51 + s50;
    const T s53 = (T)0.000000;
    const T s54 = (T)1.000000;
    vector<T> v55 = { s52, s53, s54 };
    auto s55 = clamp(v55);
    const T s56 = s55[0];
    const T s57 = (T)1.000000;
    const T s58 = s57 - s10;
    const T s59 = s6 * s58;
    const T s60 = s12 * s10;
    const T s61 = s60 + s59;
    const T s62 = (T)0.000000;
    const T s63 = (T)1.000000;
    vector<T> v64 = { s61, s62, s63 };
    auto s64 = clamp(v64);
    const T s65 = s64[0];
    const T s66 = (T)1.000000;
    const T s67 = s66 - s10;
    const T s68 = s7 * s67;
    const T s69 = s11 * s10;
    const T s70 = s69 + s68;
    const T s71 = (T)0.000000;
    const T s72 = (T)1.000000;
    vector<T> v73 = { s70, s71, s72 };
    auto s73 = clamp(v73);
    const T s74 = s73[0];
    const T s75 = (T)1.000000;
    const T s76 = s8 * s75;
    const T s77 = s35 * s76;
    const T s78 = s35 + s76;
    const T s79 = s78 - s77;
    const T s80 = s56 * s76;
    const T s81 = s65 * s76;
    const T s82 = s74 * s76;
    const T s83 = s39 * s35;
    const T s84 = s43 * s35;
    const T s85 = s47 * s35;
    vector<T> v86 = { s39, s56, s35, s76 };
    auto s86 = linearBurn(v86);
    const T s87 = s86[0];
    vector<T> v88 = { s87, s79 };
    auto s88 = cvtT(v88);
    const T s89 = s88[0];
    vector<T> v90 = { s43, s65, s35, s76 };
    auto s90 = linearBurn(v90);
    const T s91 = s90[0];
    vector<T> v92 = { s91, s79 };
    auto s92 = cvtT(v92);
    const T s93 = s92[0];
    vector<T> v94 = { s47, s74, s35, s76 };
    auto s94 = linearBurn(v94);
    const T s95 = s94[0];
    vector<T> v96 = { s95, s79 };
    auto s96 = cvtT(v96);
    const T s97 = s96[0];
    const T s98 = (T)1.000000;
    const T s99 = s98 - s19;
    const T s100 = s14 * s99;
    const T s101 = s22 * s19;
    const T s102 = s101 + s100;
    const T s103 = (T)0.000000;
    const T s104 = (T)1.000000;
    vector<T> v105 = { s102, s103, s104 };
    auto s105 = clamp(v105);
    const T s106 = s105[0];
    const T s107 = (T)1.000000;
    const T s108 = s107 - s19;
    const T s109 = s15 * s108;
    const T s110 = s21 * s19;
    const T s111 = s110 + s109;
    const T s112 = (T)0.000000;
    const T s113 = (T)1.000000;
    vector<T> v114 = { s111, s112, s113 };
    auto s114 = clamp(v114);
    const T s115 = s114[0];
    const T s116 = (T)1.000000;
    const T s117 = s116 - s19;
    const T s118 = s16 * s117;
    const T s119 = s20 * s19;
    const T s120 = s119 + s118;
    const T s121 = (T)0.000000;
    const T s122 = (T)1.000000;
    vector<T> v123 = { s120, s121, s122 };
    auto s123 = clamp(v123);
    const T s124 = s123[0];
    const T s125 = (T)1.000000;
    const T s126 = s17 * s125;
    const T s127 = s79 * s126;
    const T s128 = s79 + s126;
    const T s129 = s128 - s127;
    const T s130 = s106 * s126;
    const T s131 = s115 * s126;
    const T s132 = s124 * s126;
    const T s133 = s89 * s79;
    const T s134 = s93 * s79;
    const T s135 = s97 * s79;
    const T s136 = (T)1.000000;
    const T s137 = s136 - s126;
    const T s138 = s133 * s137;
    const T s139 = s130 + s138;
    vector<T> v140 = { s139, s129 };
    auto s140 = cvtT(v140);
    const T s141 = s140[0];
    const T s142 = (T)1.000000;
    const T s143 = s142 - s126;
    const T s144 = s134 * s143;
    const T s145 = s131 + s144;
    vector<T> v146 = { s145, s129 };
    auto s146 = cvtT(v146);
    const T s147 = s146[0];
    const T s148 = (T)1.000000;
    const T s149 = s148 - s126;
    const T s150 = s135 * s149;
    const T s151 = s132 + s150;
    vector<T> v152 = { s151, s129 };
    auto s152 = cvtT(v152);
    const T s153 = s152[0];
    const T s154 = (T)1.000000;
    const T s155 = s129 * s154;
    const T s156 = s129 * s155;
    const T s157 = s129 + s155;
    const T s158 = s157 - s156;
    const T s159 = s141 * s155;
    const T s160 = s147 * s155;
    const T s161 = s153 * s155;
    const T s162 = s141 * s129;
    const T s163 = s147 * s129;
    const T s164 = s153 * s129;
    const T s165 = (T)1.000000;
    const T s166 = s165 - s155;
    const T s167 = s162 * s166;
    const T s168 = s159 + s167;
    vector<T> v169 = { s168, s158 };
    auto s169 = cvtT(v169);
    const T s170 = s169[0];
    const T s171 = (T)1.000000;
    const T s172 = s171 - s155;
    const T s173 = s163 * s172;
    const T s174 = s160 + s173;
    vector<T> v175 = { s174, s158 };
    auto s175 = cvtT(v175);
    const T s176 = s175[0];
    const T s177 = (T)1.000000;
    const T s178 = s177 - s155;
    const T s179 = s164 * s178;
    const T s180 = s161 + s179;
    vector<T> v181 = { s180, s158 };
    auto s181 = cvtT(v181);
    const T s182 = s181[0];
    result[0] = s170; // R;
    result[1] = s176; // G;
    result[2] = s182; // B;
    result[3] = s158; // A;
    
    return result;
}

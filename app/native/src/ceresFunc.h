static const int ceresFunc_paramACount = 11;
static const int ceresFunc_paramBCount = 12;

template <class T>
vector<T> ceresFunc(const T* const paramsA, const vector<double> &paramsB)
{
    vector<T> result(4);
    
    const T s0 = (T) paramsB[0]; // 1_r;
    const T s1 = (T) paramsB[1]; // 1_g;
    const T s2 = (T) paramsB[2]; // 1_b;
    const T s3 = (T) paramsB[3]; // 1_a;
    const T s4 = (T) paramsA[0]; // x_1_opacity;
    const T s5 = (T) paramsA[1]; // x_1_a;
    const T s6 = (T) paramsA[2]; // x_1_b;
    const T s7 = (T) paramsA[3]; // x_1_g;
    const T s8 = (T) paramsA[4]; // x_1_r;
    const T s9 = (T) paramsB[4]; // 2_r;
    const T s10 = (T) paramsB[5]; // 2_g;
    const T s11 = (T) paramsB[6]; // 2_b;
    const T s12 = (T) paramsB[7]; // 2_a;
    const T s13 = (T) paramsA[5]; // x_2_opacity;
    const T s14 = (T) paramsB[8]; // 3_r;
    const T s15 = (T) paramsB[9]; // 3_g;
    const T s16 = (T) paramsB[10]; // 3_b;
    const T s17 = (T) paramsB[11]; // 3_a;
    const T s18 = (T) paramsA[6]; // x_3_opacity;
    const T s19 = (T) paramsA[7]; // x_3_a;
    const T s20 = (T) paramsA[8]; // x_3_b;
    const T s21 = (T) paramsA[9]; // x_3_g;
    const T s22 = (T) paramsA[10]; // x_3_r;
    const T s23 = (T)1.000000;
    const T s24 = s23 - s5;
    const T s25 = s0 * s24;
    const T s26 = s8 * s5;
    const T s27 = s26 + s25;
    const T s28 = (T)0.000000;
    const T s29 = (T)1.000000;
    vector<T> v30 = { s27, s28, s29 };
    auto s30 = clamp(v30);
    const T s31 = s30[0];
    const T s32 = (T)1.000000;
    const T s33 = s32 - s5;
    const T s34 = s1 * s33;
    const T s35 = s7 * s5;
    const T s36 = s35 + s34;
    const T s37 = (T)0.000000;
    const T s38 = (T)1.000000;
    vector<T> v39 = { s36, s37, s38 };
    auto s39 = clamp(v39);
    const T s40 = s39[0];
    const T s41 = (T)1.000000;
    const T s42 = s41 - s5;
    const T s43 = s2 * s42;
    const T s44 = s6 * s5;
    const T s45 = s44 + s43;
    const T s46 = (T)0.000000;
    const T s47 = (T)1.000000;
    vector<T> v48 = { s45, s46, s47 };
    auto s48 = clamp(v48);
    const T s49 = s48[0];
    const T s50 = (T)1.000000;
    const T s51 = s3 * s50;
    const T s52 = (T)0.000000;
    const T s53 = s52 * s51;
    const T s54 = (T)0.000000;
    const T s55 = s54 + s51;
    const T s56 = s55 - s53;
    const T s57 = s31 * s51;
    const T s58 = s40 * s51;
    const T s59 = s49 * s51;
    const T s60 = (T)1.000000;
    const T s61 = s60 - s51;
    const T s62 = (T)0.000000;
    const T s63 = s62 * s61;
    const T s64 = s57 + s63;
    vector<T> v65 = { s64, s56 };
    auto s65 = cvtT(v65);
    const T s66 = s65[0];
    const T s67 = (T)1.000000;
    const T s68 = s67 - s51;
    const T s69 = (T)0.000000;
    const T s70 = s69 * s68;
    const T s71 = s58 + s70;
    vector<T> v72 = { s71, s56 };
    auto s72 = cvtT(v72);
    const T s73 = s72[0];
    const T s74 = (T)1.000000;
    const T s75 = s74 - s51;
    const T s76 = (T)0.000000;
    const T s77 = s76 * s75;
    const T s78 = s59 + s77;
    vector<T> v79 = { s78, s56 };
    auto s79 = cvtT(v79);
    const T s80 = s79[0];
    const T s81 = (T)1.000000;
    const T s82 = s12 * s81;
    const T s83 = s56 * s82;
    const T s84 = s56 + s82;
    const T s85 = s84 - s83;
    const T s86 = s9 * s82;
    const T s87 = s10 * s82;
    const T s88 = s11 * s82;
    const T s89 = s66 * s56;
    const T s90 = s73 * s56;
    const T s91 = s80 * s56;
    const T s92 = (T)1.000000;
    const T s93 = s92 - s82;
    const T s94 = s89 * s93;
    const T s95 = s86 + s94;
    vector<T> v96 = { s95, s85 };
    auto s96 = cvtT(v96);
    const T s97 = s96[0];
    const T s98 = (T)1.000000;
    const T s99 = s98 - s82;
    const T s100 = s90 * s99;
    const T s101 = s87 + s100;
    vector<T> v102 = { s101, s85 };
    auto s102 = cvtT(v102);
    const T s103 = s102[0];
    const T s104 = (T)1.000000;
    const T s105 = s104 - s82;
    const T s106 = s91 * s105;
    const T s107 = s88 + s106;
    vector<T> v108 = { s107, s85 };
    auto s108 = cvtT(v108);
    const T s109 = s108[0];
    const T s110 = (T)1.000000;
    const T s111 = s110 - s19;
    const T s112 = s14 * s111;
    const T s113 = s22 * s19;
    const T s114 = s113 + s112;
    const T s115 = (T)0.000000;
    const T s116 = (T)1.000000;
    vector<T> v117 = { s114, s115, s116 };
    auto s117 = clamp(v117);
    const T s118 = s117[0];
    const T s119 = (T)1.000000;
    const T s120 = s119 - s19;
    const T s121 = s15 * s120;
    const T s122 = s21 * s19;
    const T s123 = s122 + s121;
    const T s124 = (T)0.000000;
    const T s125 = (T)1.000000;
    vector<T> v126 = { s123, s124, s125 };
    auto s126 = clamp(v126);
    const T s127 = s126[0];
    const T s128 = (T)1.000000;
    const T s129 = s128 - s19;
    const T s130 = s16 * s129;
    const T s131 = s20 * s19;
    const T s132 = s131 + s130;
    const T s133 = (T)0.000000;
    const T s134 = (T)1.000000;
    vector<T> v135 = { s132, s133, s134 };
    auto s135 = clamp(v135);
    const T s136 = s135[0];
    const T s137 = (T)1.000000;
    const T s138 = s17 * s137;
    const T s139 = s85 * s138;
    const T s140 = s85 + s138;
    const T s141 = s140 - s139;
    const T s142 = s118 * s138;
    const T s143 = s127 * s138;
    const T s144 = s136 * s138;
    const T s145 = s97 * s85;
    const T s146 = s103 * s85;
    const T s147 = s109 * s85;
    const T s148 = (T)1.000000;
    const T s149 = s148 - s138;
    const T s150 = s145 * s149;
    const T s151 = s142 + s150;
    vector<T> v152 = { s151, s141 };
    auto s152 = cvtT(v152);
    const T s153 = s152[0];
    const T s154 = (T)1.000000;
    const T s155 = s154 - s138;
    const T s156 = s146 * s155;
    const T s157 = s143 + s156;
    vector<T> v158 = { s157, s141 };
    auto s158 = cvtT(v158);
    const T s159 = s158[0];
    const T s160 = (T)1.000000;
    const T s161 = s160 - s138;
    const T s162 = s147 * s161;
    const T s163 = s144 + s162;
    vector<T> v164 = { s163, s141 };
    auto s164 = cvtT(v164);
    const T s165 = s164[0];
    result[0] = s153; // R;
    result[1] = s159; // G;
    result[2] = s165; // B;
    result[3] = s141; // A;
    
    return result;
}
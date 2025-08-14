#include <string>
#include <vector>

#if ML_USING_CPLUSPLUS_EN
#ifndef _CPP_VER
#define _CPP_VER

using std::string;
using std::vector;
void nonogramHintDetect(string &image_path, vector<vector<int>> &row_hints, vector<vector<int>> &col_hints);
#endif
#endif /*ML_USING_CPLUSPLUS_EN*/
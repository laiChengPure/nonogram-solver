#pragma once
#include "../C++ part/imagedetector.h"
#if ML_USING_CPLUSPLUS_EN
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

class DigitClassifier
{
public:
    DigitClassifier(const std::wstring &model_path);
    int classify(const cv::Mat &bwimage);

private:
    Ort::Env env;
    Ort::Session session;
    Ort::AllocatorWithDefaultOptions allocator;
    std::string input_name;
    std::string output_name;

    cv::Mat resizeImage(const cv::Mat &img);
    std::vector<float> normalizeInput(const cv::Mat &img);
};
#endif /*ML_USING_CPLUSPLUS_EN*/
#include "DigitClassifier.h"

#if ML_USING_CPLUSPLUS_EN
DigitClassifier::DigitClassifier(const std::wstring &model_path)
    : env(ORT_LOGGING_LEVEL_WARNING, "DigitClassifier"),
      session(env, model_path.c_str(), Ort::SessionOptions{nullptr})
{
    Ort::AllocatedStringPtr input_name_ptr = session.GetInputNameAllocated(0, allocator);
    Ort::AllocatedStringPtr output_name_ptr = session.GetOutputNameAllocated(0, allocator);
    input_name = input_name_ptr.get();
    output_name = output_name_ptr.get();
    std::cout << "----Input name: " << input_name << std::endl;
    std::cout << "----Output name: " << output_name << std::endl;
}

cv::Mat DigitClassifier::resizeImage(const cv::Mat &img)
{
    const int target_size = 22;
    const int border = 3;

    cv::Mat resized;
    int old_w = img.cols, old_h = img.rows;
    float ratio = float(target_size) / std::max(old_w, old_h);
    int new_w = int(old_w * ratio), new_h = int(old_h * ratio);
    cv::resize(img, resized, cv::Size(new_w, new_h), 0, 0, cv::INTER_AREA);

    int top = (target_size - new_h) / 2;
    int bottom = target_size - new_h - top;
    int left = (target_size - new_w) / 2;
    int right = target_size - new_w - left;

    cv::copyMakeBorder(resized, resized, top, bottom, left, right, cv::BORDER_CONSTANT, 0);
    cv::copyMakeBorder(resized, resized, border, border, border, border, cv::BORDER_CONSTANT, 0);

    return resized;
}

std::vector<float> DigitClassifier::normalizeInput(const cv::Mat &img)
{
    cv::Mat norm;
    img.convertTo(norm, CV_32FC1, 1.0 / 255.0);
    std::vector<float> result(28 * 28);
    std::memcpy(result.data(), norm.data, 28 * 28 * sizeof(float));
    return result;
}

int DigitClassifier::classify(const cv::Mat &bwimage)
{
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(bwimage.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<cv::Rect> digit_rects;
    for (const auto &c : contours)
    {
        digit_rects.push_back(cv::boundingRect(c));
    }
    std::sort(digit_rects.begin(), digit_rects.end(), [](auto &a, auto &b)
              { return a.x < b.x; });

    int result = 0;
    for (auto &r : digit_rects)
    {
        cv::Mat digit = bwimage(r);
        cv::Mat resized = resizeImage(digit);
        resized = 255 - resized;

        auto input_tensor_values = normalizeInput(resized);
        std::array<int64_t, 4> input_shape{1, 28, 28, 1};

        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(
            OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, input_tensor_values.data(), input_tensor_values.size(),
            input_shape.data(), input_shape.size());

        float *output = nullptr;
        const char *input_names[] = {input_name.c_str()};
        const char *output_names[] = {output_name.c_str()};
        // std::cout << "Input name: " << input_name << std::endl;
        try
        {
            auto output_tensors = session.Run(Ort::RunOptions{nullptr},
                                              input_names, &input_tensor, 1,
                                              output_names, 1);
            output = output_tensors[0].GetTensorMutableData<float>();
        }
        catch (const Ort::Exception &e)
        {
            std::cerr << "ONNX Runtime error: " << e.what() << std::endl;
        }

        int pred = std::max_element(output, output + 10) - output;
        result = result * 10 + pred;
    }

    return result;
}
#endif /*ML_USING_CPLUSPLUS_EN*/
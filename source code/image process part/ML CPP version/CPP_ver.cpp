// main.cpp - Nonogram hint detection (OpenCV C++ port)
// mostly converted by ChatGPT
#include "../C++ part/imagedetector.h"
#if ML_USING_CPLUSPLUS_EN
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>
#include <map>
#include <iostream>
#include <onnxruntime_cxx_api.h>
#include "DigitClassifier.h"
#include "CPP_ver.h"

using namespace cv;
using namespace std;

#define DEBUG false

#define SKIP_AREA_SIZE 3
#define BKG_THRESH 60
#define DOWN_TO_UP 0
#define UP_TO_DOWN 1
#define RIGHT_TO_LEFT 2
#define LEFT_TO_RIGHT 3

#define REMOVE_EDGE_LINE_PORTION 0.3f
#define REMOVE_EDGE_LINE_START_RATIO 0.25f

//--------------------------------------------------//
// helper function: find all lines in the image using projection method
//--------------------------------------------------//
// reference: ChatGPT
// Detect grid lines using projection
void detect_grid_lines(const Mat &thresh, vector<int> &row_lines_no_filter, vector<int> &col_lines_no_filter)
{
    Mat binary = 255 - thresh;
    Mat hor_sum, ver_sum;

    reduce(binary, hor_sum, 1, REDUCE_SUM, CV_32S);
    reduce(binary, ver_sum, 0, REDUCE_SUM, CV_32S);

    auto find_lines = [](const Mat &proj, vector<int> &lines_no_filter)
    {
        double max_val;
        minMaxLoc(proj, nullptr, &max_val);
        bool in_line = false;
        for (int i = 0; i < proj.rows; ++i)
        {
            if (proj.at<int>(i, 0) > 0.8 * max_val)
            {
                lines_no_filter.push_back(i);
                if (!in_line)
                {
                    in_line = true;
                }
            }
            else
            {
                in_line = false;
            }
        }
    };

    find_lines(hor_sum.t(), row_lines_no_filter);
    find_lines(ver_sum, col_lines_no_filter);
}

//--------------------------------------------------//
// helper function: preprocess image
//--------------------------------------------------//
// Helper to convert to grayscale and threshold
Mat preprocess_image(const Mat &image)
{
    Mat gray, blur_img, thresh;
    cvtColor(image, gray, COLOR_BGR2GRAY);
    GaussianBlur(gray, blur_img, Size(5, 5), 0);

    int img_h = image.rows;
    int img_w = image.cols;
    uchar bkg_level = gray.at<uchar>(img_h / 100, img_w / 2);
    int thresh_level = bkg_level + BKG_THRESH;

    threshold(gray, thresh, 175, 255, THRESH_BINARY);
    return thresh;
}

//--------------------------------------------------//
// helper function: find all rects in the image
//--------------------------------------------------//
// Find rectangles among contours
void find_rect(const Mat &thresh, vector<vector<Point>> &filtered_contours)
{
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(thresh, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

    vector<int> indices(contours.size());
    iota(indices.begin(), indices.end(), 0);
    sort(indices.begin(), indices.end(), [&](int i, int j)
         { return contourArea(contours[i]) > contourArea(contours[j]); });

    for (int &idx : indices)
    {
        vector<Point> approx;
        double peri = arcLength(contours[idx], true);
        approxPolyDP(contours[idx], approx, 0.1 * peri, true);
        if (approx.size() == 4 && hierarchy[idx][2] == -1)
        {
            filtered_contours.push_back(approx);
        }
    }

    if (DEBUG)
    {
        cout << "Detected rectangles: " << filtered_contours.size() << endl;
    }
}

//--------------------------------------------------//
// helper function: find hint numbers are at which side of the grid
//--------------------------------------------------//
// Get the width and height of a rect
Size getWidthAndHeightRect(const vector<Point> &rect)
{
    Point center(0, 0);
    for (const Point &pt : rect)
        center += pt;
    center.x /= 4;
    center.y /= 4;

    int width = abs(rect[0].x - center.x) * 2;
    int height = abs(rect[0].y - center.y) * 2;
    return Size(width, height);
}

// ----- //
// Geometry helpers
Point getTopLeftOfRect(const vector<Point> &rect)
{
    return *min_element(rect.begin(), rect.end(), [](const Point &a, const Point &b)
                        { return (a.x + a.y) < (b.x + b.y); });
}

Point getTopRightOfRect(const vector<Point> &rect)
{
    return *max_element(rect.begin(), rect.end(), [](const Point &a, const Point &b)
                        { return (a.x - a.y) < (b.x - b.y); });
}

Point getBottomLeftOfRect(const vector<Point> &rect)
{
    return *min_element(rect.begin(), rect.end(), [](const Point &a, const Point &b)
                        { return (a.x - a.y) < (b.x - b.y); });
}

Point getBottomRightOfRect(const vector<Point> &rect)
{
    return *max_element(rect.begin(), rect.end(), [](const Point &a, const Point &b)
                        { return (a.x + a.y) < (b.x + b.y); });
}

// ----- //
// Returns the X coordinate of the contour centroid
int x_cord_contour(const vector<Point> &contour)
{
    Moments M = moments(contour);
    if (M.m00 != 0)
        return static_cast<int>(M.m10 / M.m00);
    else
        return 0; // fallback if area is 0
}

// Returns the Y coordinate of the contour centroid
int y_cord_contour(const vector<Point> &contour)
{
    Moments M = moments(contour);
    if (M.m00 != 0)
        return static_cast<int>(M.m01 / M.m00);
    else
        return 0; // fallback if area is 0
}

// ----- //
// Find the closest valid rectangle (not a long line)
Rect findTheClosestRectAndNotLine(const Mat &thresh_image_rev, int direction)
{
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    findContours(thresh_image_rev, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

    if (contours.empty())
        return Rect(-1, -1, -1, -1);

    vector<vector<Point>> filtered;
    for (const auto &cnt : contours)
    {
        if (contourArea(cnt) > SKIP_AREA_SIZE)
        {
            filtered.push_back(cnt);
        }
    }

    if (filtered.empty())
        return Rect(-1, -1, -1, -1);

    // Sorting
    if (direction == DOWN_TO_UP)
    {
        sort(filtered.begin(), filtered.end(), [](const auto &a, const auto &b)
             { return y_cord_contour(a) > y_cord_contour(b); });
    }
    else if (direction == UP_TO_DOWN)
    {
        sort(filtered.begin(), filtered.end(), [](const auto &a, const auto &b)
             { return y_cord_contour(a) < y_cord_contour(b); });
    }
    else if (direction == RIGHT_TO_LEFT)
    {
        sort(filtered.begin(), filtered.end(), [](const auto &a, const auto &b)
             { return x_cord_contour(a) > x_cord_contour(b); });
    }
    else
    { // LEFT_TO_RIGHT
        sort(filtered.begin(), filtered.end(), [](const auto &a, const auto &b)
             { return x_cord_contour(a) < x_cord_contour(b); });
    }

    // Return the first valid rectangle that is not a line
    for (const auto &c : filtered)
    {
        Rect bbox = boundingRect(c);
        int x = bbox.x, y = bbox.y, w = bbox.width, h = bbox.height;

        if (direction == DOWN_TO_UP || direction == UP_TO_DOWN)
        {
            if (x == 0 && x + w == thresh_image_rev.cols && h < 0.4 * w)
                continue; // likely a horizontal line
        }
        else
        {
            if (y == 0 && y + h == thresh_image_rev.rows && w < 0.4 * h)
                continue; // likely a vertical line
        }
        return bbox;
    }

    return Rect(-1, -1, -1, -1);
}

//--------------------------------------------------//
// helper function: find hint numbers
//--------------------------------------------------//
// #reference: https://github.com/chewbacca89/OpenCV-with-Python/blob/master/Lecture%204.2%20-%20Sorting%20Contours.ipynb
// Center a cropped image into a square
Mat processImgBecomeSquare(const Mat &cropped_img)
{
    int h = cropped_img.rows;
    int w = cropped_img.cols;
    int size = max(h, w);
    Mat square(size, size, cropped_img.type(), Scalar(0));
    int dy = (size - h) / 2;
    int dx = (size - w) / 2;
    cropped_img.copyTo(square(Rect(dx, dy, w, h)));
    return square;
}

// Remove possible edge lines (top-down and bottom-up)
void removePossibleEdgeLineTopDown(Mat &cropped_img)
{
    int h = cropped_img.rows;
    int w = cropped_img.cols;
    int explore_height = static_cast<int>(h * REMOVE_EDGE_LINE_PORTION);

    // Top
    bool top_line = all_of(cropped_img.ptr<uchar>(0), cropped_img.ptr<uchar>(0) + w, [](uchar v)
                           { return v == 255; });
    if (top_line)
    {
        int start_col = static_cast<int>(w * REMOVE_EDGE_LINE_START_RATIO);
        int row = 0;
        while (row <= explore_height && cropped_img.at<uchar>(row, start_col) == 255)
            ++row;
        cropped_img.rowRange(0, row).setTo(0);
    }

    // Bottom
    bool bottom_line = all_of(cropped_img.ptr<uchar>(h - 1), cropped_img.ptr<uchar>(h - 1) + w, [](uchar v)
                              { return v == 255; });
    if (bottom_line)
    {
        int start_col = static_cast<int>(w * REMOVE_EDGE_LINE_START_RATIO);
        int row = h - 1;
        while (row >= h - explore_height && cropped_img.at<uchar>(row, start_col) == 255)
            --row;
        cropped_img.rowRange(row + 1, h).setTo(0);
    }
}

// Remove possible edge lines (left-right)
void removePossibleEdgeLineLeftRight(Mat &cropped_img)
{
    int h = cropped_img.rows;
    int w = cropped_img.cols;
    int explore_width = static_cast<int>(w * REMOVE_EDGE_LINE_PORTION);

    // Left
    bool left_line = true;
    for (int r = 0; r < h; ++r)
    {
        if (cropped_img.at<uchar>(r, 0) == 0)
        {
            left_line = false;
            break;
        }
    }
    if (left_line)
    {
        int start_row = static_cast<int>(h * REMOVE_EDGE_LINE_START_RATIO);
        int col = 0;
        while (col < explore_width && cropped_img.at<uchar>(start_row, col) == 255)
            ++col;
        cropped_img.colRange(0, col).setTo(0);
    }

    // Right
    bool right_line = true;
    for (int r = 0; r < h; ++r)
    {
        if (cropped_img.at<uchar>(r, w - 1) == 0)
        {
            right_line = false;
            break;
        }
    }
    if (right_line)
    {
        int start_row = static_cast<int>(h * REMOVE_EDGE_LINE_START_RATIO);
        int col = w - 1;
        while (col >= w - explore_width && cropped_img.at<uchar>(start_row, col) == 255)
            --col;
        cropped_img.colRange(col + 1, w).setTo(0);
    }
}

// ----- //
// Find the bounding rectangle that enclose all the number in this cropped img
Mat preprocessImgCol(Mat &cropped_img, int rect_w)
{
    int cropped_h = cropped_img.rows;
    int cropped_w = cropped_img.cols;

    if (cropped_w == rect_w)
    {
        removePossibleEdgeLineTopDown(cropped_img);
        removePossibleEdgeLineLeftRight(cropped_img);
    }

    vector<vector<Point>> contours;
    findContours(cropped_img, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

    if (contours.empty())
        return cropped_img;

    vector<Rect> obj_rectangles;
    for (const auto &cnt : contours)
    {
        obj_rectangles.push_back(boundingRect(cnt));
    }

    sort(obj_rectangles.begin(), obj_rectangles.end(),
         [](const Rect &a, const Rect &b)
         {
             return a.height > b.height;
         });

    int max_h = obj_rectangles[0].height;

    int upper_y = -1, lower_y = -1, left_x = -1, right_x = -1;

    for (const auto &rect : obj_rectangles)
    {
        if (rect.height < max_h * 0.4)
            continue;

        if (upper_y == -1)
        {
            upper_y = rect.y + rect.height;
            lower_y = rect.y;
            left_x = rect.x;
            right_x = rect.x + rect.width;
            continue;
        }

        upper_y = max(upper_y, rect.y + rect.height);
        lower_y = min(lower_y, rect.y);
        left_x = min(left_x, rect.x);
        right_x = max(right_x, rect.x + rect.width);
    }

    if (upper_y == -1 || lower_y == -1 || left_x == -1 || right_x == -1)
        return cropped_img;

    return cropped_img(Rect(left_x, lower_y, right_x - left_x, upper_y - lower_y));
}

// Find the bounding rectangle that enclose all the number in this cropped img
Mat preprocessImgRow(Mat &cropped_img, int rect_h)
{
    int cropped_h = cropped_img.rows;
    int cropped_w = cropped_img.cols;

    if (cropped_h == rect_h)
    {
        removePossibleEdgeLineTopDown(cropped_img);
        removePossibleEdgeLineLeftRight(cropped_img);
    }

    vector<vector<Point>> contours;
    findContours(cropped_img, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

    if (contours.empty())
        return cropped_img;

    vector<Rect> obj_rectangles;
    for (const auto &cnt : contours)
    {
        obj_rectangles.push_back(boundingRect(cnt));
    }

    sort(obj_rectangles.begin(), obj_rectangles.end(),
         [](const Rect &a, const Rect &b)
         {
             return a.height > b.height;
         });

    int max_h = obj_rectangles[0].height;

    int upper_y = -1, lower_y = -1, left_x = -1, right_x = -1;

    for (const auto &rect : obj_rectangles)
    {
        if (rect.height < max_h * 0.4)
            continue;

        if (upper_y == -1)
        {
            upper_y = rect.y + rect.height;
            lower_y = rect.y;
            left_x = rect.x;
            right_x = rect.x + rect.width;
            continue;
        }

        upper_y = max(upper_y, rect.y + rect.height);
        lower_y = min(lower_y, rect.y);
        left_x = min(left_x, rect.x);
        right_x = max(right_x, rect.x + rect.width);
    }

    if (upper_y == -1 || lower_y == -1 || left_x == -1 || right_x == -1)
        return cropped_img;

    return cropped_img(Rect(left_x, lower_y, right_x - left_x, upper_y - lower_y));
}

// ----- //
// function to find hint numbers
vector<int> extractHintNumbers(const Mat &hint_img, int direction, int rect_h, int rect_w, DigitClassifier &clf, bool is_col)
{
    vector<vector<Point>> contours;
    findContours(hint_img, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    vector<int> hint_numbers;
    if (contours.empty())
        return hint_numbers;

    int img_h = hint_img.rows;
    int img_w = hint_img.cols;

    vector<vector<Point>> filtered;
    for (const auto &cnt : contours)
    {
        if (contourArea(cnt) > SKIP_AREA_SIZE)
        {
            filtered.push_back(cnt);
        }
    }

    auto contour_sort = [&](const vector<Point> &a, const vector<Point> &b)
    {
        if (direction == DOWN_TO_UP || direction == UP_TO_DOWN)
            return (direction == DOWN_TO_UP ? y_cord_contour(a) > y_cord_contour(b)
                                            : y_cord_contour(a) < y_cord_contour(b));
        else
            return (direction == RIGHT_TO_LEFT ? x_cord_contour(a) > x_cord_contour(b)
                                               : x_cord_contour(a) < x_cord_contour(b));
    };

    sort(filtered.begin(), filtered.end(), contour_sort);

    int upper_y = -1, lower_y = -1, left_x = -1, right_x = -1;
    vector<int> current_group;
    Rect current_rect;
    for (size_t i = 0; i < filtered.size(); ++i)
    {
        Rect r = boundingRect(filtered[i]);
        bool condition = is_col ? ((r.x == 0 && r.x + r.width == img_w && r.height < 0.4 * r.width) || (r.height < rect_h * 0.3)) : ((r.y == 0 && r.y + r.height == img_h && r.width < 0.4 * r.height) || (r.height < rect_h * 0.3));

        if (condition)
        {
            if (upper_y != -1)
            {
                Mat cropped_img = hint_img(Rect(left_x, lower_y, right_x - left_x, upper_y - lower_y));
                Mat region = is_col ? preprocessImgCol(cropped_img, rect_w) : preprocessImgRow(cropped_img, rect_h);
                region = processImgBecomeSquare(region);

                int predicted_number = clf.classify(region);
                hint_numbers.emplace_back(predicted_number);
            }
            upper_y = -1, lower_y = -1, left_x = -1, right_x = -1;
            continue;
        }
        if (upper_y == -1)
        {
            upper_y = r.y + r.height;
            lower_y = r.y;
            left_x = r.x;
            right_x = r.x + r.width;
            continue;
        }

        condition = is_col ? (abs(r.y - lower_y) <= r.height * 0.3) : (abs(r.x - left_x) <= rect_w * 0.5);

        if (condition)
        {
            upper_y = max(upper_y, r.y + r.height);
            lower_y = min(lower_y, r.y);
            left_x = min(left_x, r.x);
            right_x = max(right_x, r.x + r.width);
            continue;
        }

        Mat cropped_img = hint_img(Rect(left_x, lower_y, right_x - left_x, upper_y - lower_y));
        Mat region = is_col ? preprocessImgCol(cropped_img, rect_w) : preprocessImgRow(cropped_img, rect_h);
        region = processImgBecomeSquare(region);

        int predicted_number = clf.classify(region);
        hint_numbers.emplace_back(predicted_number);

        upper_y = r.y + r.height;
        lower_y = r.y;
        left_x = r.x;
        right_x = r.x + r.width;
    }

    if (upper_y != -1)
    {
        Mat cropped_img = hint_img(Rect(left_x, lower_y, right_x - left_x, upper_y - lower_y));
        Mat region = is_col ? preprocessImgCol(cropped_img, rect_w) : preprocessImgRow(cropped_img, rect_h);
        region = processImgBecomeSquare(region);

        int predicted_number = clf.classify(region);
        hint_numbers.emplace_back(predicted_number);
    }

    return hint_numbers;
}

//--------------------------------------------------//
// helper function: find connected rect
//--------------------------------------------------//
// find the connections of rects
Point2f contourCenter(const vector<Point> &contour)
{
    CV_Assert(contour.size() == 4); // Ensure it’s a 4-point contour (a rectangle)
    return (contour[0] + contour[1] + contour[2] + contour[3]) * 0.25f;
}

struct IntersectInfo
{
    int prev_idx;
    int next_idx;
    int intersect_cnt;
    float min_dist;
};

void findRectConnections(
    const vector<vector<Point>> &cnts_filter,
    const Mat &contour_img,
    const Mat &img,
    unordered_map<int, IntersectInfo> &dict_horizontal_intersect_info,
    unordered_map<int, IntersectInfo> &dict_vertical_intersect_info)
{
    const int dist_threshold = 10;
    const float arc_threshold_percent = 0.05f;

    for (int i = 0; i < cnts_filter.size(); ++i)
    {
        if (dict_horizontal_intersect_info.find(i) == dict_horizontal_intersect_info.end())
        {
            Point2f start_pos = contourCenter(cnts_filter[i]);
            int intersect_cnt = 1;
            double targetPerimeter = arcLength(cnts_filter[i], true);
            float min_dist = -1;
            int prev_idx = i;
            int start_x = static_cast<int>(round(start_pos.x));
            int start_y = static_cast<int>(round(start_pos.y));
            vector<int> horizontal_list = {i};

            for (int x = start_x; x < img.cols; ++x)
            {
                int cur_idx = contour_img.at<int>(start_y, x);
                if (cur_idx != -1 && cur_idx != prev_idx)
                {
                    Point2f pos = contourCenter(cnts_filter[cur_idx]);
                    float distance = norm(start_pos - pos);
                    double perimeter = arcLength(cnts_filter[cur_idx], true);
                    if (abs(targetPerimeter - perimeter) < arc_threshold_percent * targetPerimeter &&
                        (min_dist == -1 || abs(distance - min_dist) < dist_threshold))
                    {

                        intersect_cnt++;
                        min_dist = distance;
                        start_pos = pos;
                        prev_idx = cur_idx;
                        horizontal_list.push_back(cur_idx);

                        if (dict_horizontal_intersect_info.find(cur_idx) != dict_horizontal_intersect_info.end())
                        {
                            float existing_dist = dict_horizontal_intersect_info[cur_idx].min_dist;
                            if (existing_dist == -1 || abs(existing_dist - min_dist) < dist_threshold)
                            {
                                intersect_cnt += dict_horizontal_intersect_info[cur_idx].intersect_cnt - 1;
                            }
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }

            if (horizontal_list.size() == 1)
            {
                dict_horizontal_intersect_info[i] = {-1, -1, intersect_cnt, -1};
            }
            else
            {
                dict_horizontal_intersect_info[horizontal_list[0]] = {-1, horizontal_list[1], intersect_cnt, min_dist};
                intersect_cnt--;
                for (size_t j = 1; j + 1 < horizontal_list.size(); ++j)
                {
                    dict_horizontal_intersect_info[horizontal_list[j]] = {horizontal_list[j - 1], horizontal_list[j + 1], intersect_cnt, min_dist};
                    intersect_cnt--;
                }
                int last = horizontal_list.back();
                if (dict_horizontal_intersect_info.find(last) != dict_horizontal_intersect_info.end())
                {
                    dict_horizontal_intersect_info[last].prev_idx = horizontal_list[horizontal_list.size() - 2];
                }
            }
        }

        if (dict_vertical_intersect_info.find(i) == dict_vertical_intersect_info.end())
        {
            Point2f start_pos = contourCenter(cnts_filter[i]);
            int intersect_cnt = 1;
            double targetPerimeter = arcLength(cnts_filter[i], true);
            float min_dist = -1;
            int prev_idx = i;
            int start_x = static_cast<int>(round(start_pos.x));
            int start_y = static_cast<int>(round(start_pos.y));
            vector<int> vertical_list = {i};

            for (int y = start_y; y < img.rows; ++y)
            {
                int cur_idx = contour_img.at<int>(y, start_x);
                if (cur_idx != -1 && cur_idx != prev_idx)
                {
                    Point2f pos = contourCenter(cnts_filter[cur_idx]);
                    float distance = norm(start_pos - pos);
                    double perimeter = arcLength(cnts_filter[cur_idx], true);
                    if (abs(targetPerimeter - perimeter) < arc_threshold_percent * targetPerimeter &&
                        (min_dist == -1 || abs(distance - min_dist) < dist_threshold))
                    {

                        intersect_cnt++;
                        min_dist = distance;
                        start_pos = pos;
                        prev_idx = cur_idx;
                        vertical_list.push_back(cur_idx);

                        if (dict_vertical_intersect_info.find(cur_idx) != dict_vertical_intersect_info.end())
                        {
                            float existing_dist = dict_vertical_intersect_info[cur_idx].min_dist;
                            if (existing_dist == -1 || abs(existing_dist - min_dist) < dist_threshold)
                            {
                                intersect_cnt += dict_vertical_intersect_info[cur_idx].intersect_cnt - 1;
                            }
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }

            if (vertical_list.size() == 1)
            {
                dict_vertical_intersect_info[i] = {-1, -1, intersect_cnt, -1};
            }
            else
            {
                dict_vertical_intersect_info[vertical_list[0]] = {-1, vertical_list[1], intersect_cnt, min_dist};
                intersect_cnt--;
                for (size_t j = 1; j + 1 < vertical_list.size(); ++j)
                {
                    dict_vertical_intersect_info[vertical_list[j]] = {vertical_list[j - 1], vertical_list[j + 1], intersect_cnt, min_dist};
                    intersect_cnt--;
                }
                int last = vertical_list.back();
                if (dict_vertical_intersect_info.find(last) != dict_vertical_intersect_info.end())
                {
                    dict_vertical_intersect_info[last].prev_idx = vertical_list[vertical_list.size() - 2];
                }
            }
        }
    }
}

// how many row and col of the grid
void estimateGridSize(const unordered_map<int, IntersectInfo> &dict_horizontal_intersect_info,
                      const unordered_map<int, IntersectInfo> &dict_vertical_intersect_info,
                      int &row_num, int &col_num)
{
    int least_col_num_cnt = 0;
    int least_row_num_cnt = 0;

    map<int, int> possible_col_num;
    map<int, int> possible_row_num;

    // Count frequency of column intersections
    for (const auto &[key, value] : dict_horizontal_intersect_info)
    {
        int count = value.intersect_cnt;
        possible_col_num[count]++;
    }

    // Count frequency of row intersections
    for (const auto &[key, value] : dict_vertical_intersect_info)
    {
        int count = value.intersect_cnt;
        possible_row_num[count]++;
    }

    col_num = 0;
    row_num = 0;

    for (const auto &[key, val] : possible_col_num)
    {
        if (key > col_num && val >= least_col_num_cnt)
        {
            col_num = key;
        }
    }

    for (const auto &[key, val] : possible_row_num)
    {
        if (key > row_num && val >= least_row_num_cnt)
        {
            row_num = key;
        }
    }

    if (DEBUG)
    {
        cout << "number of row: " << row_num << endl;
        cout << "number of col: " << col_num << endl;
    }
}

// find the top-left rect of the grid
int findTopLeftRect(const vector<vector<Point>> &cnts_filter,
                    unordered_map<int, IntersectInfo> &dict_horizontal_intersect_info,
                    unordered_map<int, IntersectInfo> &dict_vertical_intersect_info,
                    int col_num, int row_num,
                    const string &file_path,
                    int &tl, int &tr, int &bl, int &br)
{
    tl = -1;

    // Find top-left corner
    for (int i = 0; i < cnts_filter.size(); ++i)
    {
        auto it_h = dict_horizontal_intersect_info.find(i);
        auto it_v = dict_vertical_intersect_info.find(i);
        if (it_h != dict_horizontal_intersect_info.end() &&
            it_v != dict_vertical_intersect_info.end() &&
            it_h->second.intersect_cnt == col_num &&
            it_v->second.intersect_cnt == row_num)
        {
            tl = i;
        }
    }

    if (tl == -1)
    {
        if (DEBUG)
            cout << "not found nonogram" << endl;
        return -1;
    }

    // Get TR, BL, BR
    int cur = tl;
    int step = 1;
    while (step < col_num && dict_horizontal_intersect_info.count(cur))
    {
        cur = dict_horizontal_intersect_info[cur].next_idx;
        ++step;
    }
    tr = cur;

    cur = tl;
    step = 1;
    while (step < row_num && dict_vertical_intersect_info.count(cur))
    {
        cur = dict_vertical_intersect_info[cur].next_idx;
        ++step;
    }
    bl = cur;

    step = 1;
    while (step < col_num && dict_horizontal_intersect_info.count(cur))
    {
        cur = dict_horizontal_intersect_info[cur].next_idx;
        ++step;
    }
    br = cur;

    if (DEBUG)
    {
        Mat img = imread(file_path);
        if (!img.empty() && tl >= 0 && tl < cnts_filter.size())
        {
            for (int k = 0; k < 4; ++k)
            {
                Point pt = cnts_filter[tl][k];
                circle(img, pt, 1, Scalar(0, 255, 0), 2);
            }
            imwrite("temp.jpg", img);
        }
    }

    return tl;
}

// find the hint numbers are at the top/bottom or left/right of the grid
struct HintCounts
{
    int upper = 0;
    int lower = 0;
    int left = 0;
    int right = 0;
};

HintCounts countGridHints(
    const Mat &thresh,
    const vector<vector<Point>> &cnts_filter,
    unordered_map<int, IntersectInfo> &dict_horizontal_intersect_info,
    unordered_map<int, IntersectInfo> &dict_vertical_intersect_info,
    int tl,
    int bl,
    int tr)
{
    HintCounts counts;
    Mat thresh_rev;
    bitwise_not(thresh, thresh_rev);

    int cur_idx = tl;
    while (cur_idx != -1)
    {
        Point tl_point = getTopLeftOfRect(cnts_filter[cur_idx]);
        int x = tl_point.x;
        int y = tl_point.y;
        auto [w, h] = getWidthAndHeightRect(cnts_filter[cur_idx]);

        auto [x1, y1, w1, h1] = findTheClosestRectAndNotLine(thresh_rev(Rect(x, 0, w, y)), DOWN_TO_UP);
        if (y1 != -1 && y1 >= y - 2 * h)
        {
            counts.upper++;
        }
        cur_idx = dict_horizontal_intersect_info[cur_idx].next_idx;
    }

    cur_idx = bl;
    while (cur_idx != -1)
    {
        Point bl_point = getBottomLeftOfRect(cnts_filter[cur_idx]);
        int x = bl_point.x;
        int y = bl_point.y;
        auto [w, h] = getWidthAndHeightRect(cnts_filter[cur_idx]);

        auto [x1, y1, w1, h1] = findTheClosestRectAndNotLine(thresh_rev(Rect(x, y, w, thresh_rev.rows - y)), UP_TO_DOWN);
        if (y1 != -1 && y1 <= 2 * h)
        {
            counts.lower++;
        }
        cur_idx = dict_horizontal_intersect_info[cur_idx].next_idx;
    }

    cur_idx = tl;
    while (cur_idx != -1)
    {
        Point tl_point = getTopLeftOfRect(cnts_filter[cur_idx]);
        int x = tl_point.x;
        int y = tl_point.y;
        auto [w, h] = getWidthAndHeightRect(cnts_filter[cur_idx]);

        auto [x1, y1, w1, h1] = findTheClosestRectAndNotLine(thresh_rev(Rect(0, y, x, h)), RIGHT_TO_LEFT);
        if (x1 != -1 && x1 >= x - 2 * w)
        {
            counts.left++;
        }
        cur_idx = dict_vertical_intersect_info[cur_idx].next_idx;
    }

    cur_idx = tr;
    while (cur_idx != -1)
    {
        Point tr_point = getTopRightOfRect(cnts_filter[cur_idx]);
        int x = tr_point.x;
        int y = tr_point.y;
        auto [w, h] = getWidthAndHeightRect(cnts_filter[cur_idx]);

        auto [x1, y1, w1, h1] = findTheClosestRectAndNotLine(thresh_rev(Rect(x, y, thresh_rev.cols - x, h)), LEFT_TO_RIGHT);
        if (x1 != -1 && x1 <= 2 * w)
        {
            counts.right++;
        }
        cur_idx = dict_vertical_intersect_info[cur_idx].next_idx;
    }

    if (DEBUG)
    {
        cout << "upper_cnt: " << counts.upper << endl;
        cout << "lower_cnt: " << counts.lower << endl;
        cout << "left_cnt: " << counts.left << endl;
        cout << "right_cnt: " << counts.right << endl;
    }

    return counts;
}

// get the hint numbers
pair<vector<vector<int>>, vector<vector<int>>> getGridHints(
    const Mat &thresh,
    const vector<vector<Point>> &cnts_filter,
    unordered_map<int, IntersectInfo> &dict_horizontal_intersect_info,
    unordered_map<int, IntersectInfo> &dict_vertical_intersect_info,
    int tl, int bl, int tr,
    HintCounts &counts,
    DigitClassifier &clf)
{
    Mat thresh_rev;
    bitwise_not(thresh, thresh_rev);

    vector<vector<int>> col_hint_list;
    vector<vector<int>> row_hint_list;

    int upper_cnt = counts.upper;
    int lower_cnt = counts.lower;
    int left_cnt = counts.left;
    int right_cnt = counts.right;

    if (upper_cnt > lower_cnt)
    {
        int cur_idx = tl;
        while (cur_idx != -1)
        {
            Point tl_point = getTopLeftOfRect(cnts_filter[cur_idx]);
            int x = tl_point.x;
            int y = tl_point.y;
            auto [w, h] = getWidthAndHeightRect(cnts_filter[cur_idx]);

            auto col_number_list = extractHintNumbers(thresh_rev(Rect(x, 0, w, y)), DOWN_TO_UP, h, w, clf, true);
            reverse(col_number_list.begin(), col_number_list.end());
            col_hint_list.push_back(col_number_list);

            cur_idx = dict_horizontal_intersect_info[cur_idx].next_idx;
        }
    }
    else
    {
        int cur_idx = bl;
        while (cur_idx != -1)
        {
            Point bl_point = getBottomLeftOfRect(cnts_filter[cur_idx]);
            int x = bl_point.x;
            int y = bl_point.y;
            auto [w, h] = getWidthAndHeightRect(cnts_filter[cur_idx]);

            auto col_number_list = extractHintNumbers(thresh_rev(Rect(x, y, w, thresh_rev.rows - y)), UP_TO_DOWN, h, w, clf, true);
            col_hint_list.push_back(col_number_list);

            cur_idx = dict_horizontal_intersect_info[cur_idx].next_idx;
        }
    }

    if (left_cnt > right_cnt)
    {
        int cur_idx = tl;
        while (cur_idx != -1)
        {
            Point tl_point = getTopLeftOfRect(cnts_filter[cur_idx]);
            int x = tl_point.x;
            int y = tl_point.y;
            auto [w, h] = getWidthAndHeightRect(cnts_filter[cur_idx]);

            auto row_number_list = extractHintNumbers(thresh_rev(Rect(0, y, x, h)), RIGHT_TO_LEFT, h, w, clf, false);
            reverse(row_number_list.begin(), row_number_list.end());
            row_hint_list.push_back(row_number_list);

            cur_idx = dict_vertical_intersect_info[cur_idx].next_idx;
        }
    }
    else
    {
        int cur_idx = tr;
        while (cur_idx != -1)
        {
            Point tr_point = getTopRightOfRect(cnts_filter[cur_idx]);
            int x = tr_point.x;
            int y = tr_point.y;
            auto [w, h] = getWidthAndHeightRect(cnts_filter[cur_idx]);

            auto row_number_list = extractHintNumbers(thresh_rev(Rect(x, y, thresh_rev.cols - x, h)), LEFT_TO_RIGHT, h, w, clf, false);
            row_hint_list.push_back(row_number_list);

            cur_idx = dict_vertical_intersect_info[cur_idx].next_idx;
        }
    }

    if (DEBUG)
    {
        cout << "col hint:" << endl;
        for (const auto &hint : col_hint_list)
        {
            for (int val : hint)
                cout << val << ' ';
            cout << endl;
        }
        cout << "row hint:" << endl;
        for (const auto &hint : row_hint_list)
        {
            for (int val : hint)
                cout << val << ' ';
            cout << endl;
        }
    }

    return {row_hint_list, col_hint_list};
}

//--------------------------------------------------//
// main function
//--------------------------------------------------//
void nonogramHintDetect(string &image_path, vector<vector<int>> &row_hints, vector<vector<int>> &col_hints)
{
    Mat img = imread(image_path);
    if (img.empty())
    {
        cerr << "Cannot load image: " << image_path << endl;
        return;
    }

    Mat thresh = preprocess_image(img);

    vector<int> row_lines_no_filter, col_lines_no_filter;
    detect_grid_lines(thresh, row_lines_no_filter, col_lines_no_filter);

    for (int y : row_lines_no_filter)
        line(thresh, Point(col_lines_no_filter.front(), y), Point(col_lines_no_filter.back(), y), Scalar(0), 1);
    for (int x : col_lines_no_filter)
        line(thresh, Point(x, row_lines_no_filter.front()), Point(x, row_lines_no_filter.back()), Scalar(0), 1);

    if (DEBUG)
        imwrite("grid_lines.png", thresh);

    vector<vector<Point>> filtered_contours;
    find_rect(thresh, filtered_contours);

    if (DEBUG)
    {
        Mat vis = img.clone();
        for (const auto &r : filtered_contours)
            polylines(vis, r, true, Scalar(0, 255, 0), 2);
        imwrite("detected_rects.png", vis);
    }

    // Create a matrix filled with -1 (using CV_32S to support negative values)
    Mat contour_img(img.rows, img.cols, CV_32S, Scalar(-1));

    // Draw contours with index i as the fill value
    for (int i = 0; i < filtered_contours.size(); ++i)
    {
        drawContours(contour_img, filtered_contours, i, Scalar(i), FILLED);
    }

    if (DEBUG)
    {
        // Convert contour_img to CV_8U for saving as an image (visualization purposes)
        Mat contour_img_vis;
        normalize(contour_img, contour_img_vis, 0, 255, NORM_MINMAX, CV_8U);
        imwrite("contour_img.jpg", contour_img_vis);
    }

    unordered_map<int, IntersectInfo> dict_horizontal_intersect_info;
    unordered_map<int, IntersectInfo> dict_vertical_intersect_info;
    findRectConnections(filtered_contours, contour_img, img, dict_horizontal_intersect_info, dict_vertical_intersect_info);

    int row_num = 0, col_num = 0;
    estimateGridSize(dict_horizontal_intersect_info, dict_vertical_intersect_info, row_num, col_num);

    int tl = -1, tr = -1, bl = -1, br = -1;
    int found = findTopLeftRect(filtered_contours,
                                dict_horizontal_intersect_info,
                                dict_vertical_intersect_info,
                                col_num, row_num,
                                image_path,
                                tl, tr, bl, br);
    if (found == -1)
    {
        cout << "nonogram not found" << endl;
        return;
    }

    HintCounts counts = countGridHints(thresh, filtered_contours, dict_horizontal_intersect_info, dict_vertical_intersect_info, tl, bl, tr);

    DigitClassifier clf(L"model-classifier.onnx");
    auto [row_hint_list, col_hint_list] = getGridHints(
        thresh,
        filtered_contours,
        dict_horizontal_intersect_info,
        dict_vertical_intersect_info,
        tl, bl, tr,
        counts,
        clf);

    row_hints = row_hint_list;
    col_hints = col_hint_list;
    return;
}
#endif /*ML_USING_CPLUSPLUS_EN*/
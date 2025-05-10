#pragma once

#include <opencv2/core.hpp>
#include <chrono>
#include <vector>

struct TrackedObject {
    // basic things for object
    int id = -1;
    cv::Rect bbox;
    cv::Point2f center;
    
    // time related var
    std::chrono::time_point<std::chrono::steady_clock> firstDetectedTime;
    int frames_without_match = 0;
    
    // flags
    bool isActive = false;
    bool isStationary = false;
    bool isPathValid = false;
    
    // path related var
    cv::Point pathStart;
    cv::Point pathEnd;
    double pathDistance = 0.0;
    std::vector<cv::Point> path;
};
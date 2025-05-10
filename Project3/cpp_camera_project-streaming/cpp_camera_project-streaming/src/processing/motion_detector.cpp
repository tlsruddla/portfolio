#include "../../include/processing/motion_detector.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <sstream>
#include <string>

// Image storage path
const std::string IMAGE_DIR = "images";

// HTTP server instance
static std::unique_ptr<HTTPServer> http_server = nullptr;

MotionDetector::MotionDetector(cv::VideoCapture& capture, TCPClient* tcpClient,
                               double minArea, double maxArea)
    : capture_(capture),
      tcp_client_(tcpClient),
      running_(false),
      history_(500),
      dist_threshold_(4000.0),
      detect_shadows_(false),
      min_area_(minArea),
      max_area_(maxArea),
      stationaryDistanceThreshold(20.0),
      stationaryTimeThresholdMs(3000),
      ignoreStationaryObjects(false) {
    // get camera resolution
    int frame_width = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_WIDTH));
    int frame_height =
        static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_HEIGHT));

    std::cout << "Camera resolution: " << frame_width << "x" << frame_height
              << std::endl;

    // set resolution for TCP client
    if (tcp_client_) {
        tcp_client_->setResolution(frame_width, frame_height);
    }

    // Initialize KNN background subtractor
    bg_subtractor_ = cv::createBackgroundSubtractorKNN(
        history_, dist_threshold_, detect_shadows_);

    // Create object tracker
    tracker_ = std::make_unique<ObjectTracker>();

    // Set path completed event callback
    tracker_->setPathCompletedCallback(
        [this](int objectId, const cv::Point& start, const cv::Point& end,
               double distance) { notifyPath(objectId, start, end); });

    // Start HTTP server if not already running
    if (!http_server) {
        http_server = std::make_unique<HTTPServer>(8080);
        http_server->start(IMAGE_DIR);
    }

    // Capture initial image
    cv::Mat initial_frame;
    if (capture_.read(initial_frame)) {
        // Generate timestamp for filename
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_time_t), "%Y%m%d_%H%M%S");

        std::string timestamp = ss.str();
        std::string initial_file = IMAGE_DIR + "/initial_" + timestamp + ".jpg";

        cv::imwrite(initial_file, initial_frame);

        // Update latest image
        if (http_server) {
            http_server->updateLatestImage(initial_file);
        }

        std::cout << "Initial image saved: " << initial_file << std::endl;
    }
}

MotionDetector::~MotionDetector() {
    stop();
}

void MotionDetector::run() {
    running_ = true;
    cv::Mat frame;

    while (running_) {
        bool success = capture_.read(frame);
        if (!success) {
            std::cerr << "Failed to read frame from camera" << std::endl;
            break;
        }

        // Process the current frame
        processFrame(frame);

        // Display the frame
        cv::imshow("Motion Detection", frame);

        // Exit if 'q' key is pressed
        if (cv::waitKey(1) == 'q') {
            break;
        }
    }

    stop();
}

void MotionDetector::stop() {
    running_ = false;
    cv::destroyAllWindows();
}

void MotionDetector::processFrame(const cv::Mat& frame) {
    // Create foreground mask using KNN background subtractor
    cv::Mat foreground_mask;
    std::vector<cv::Rect> motion_regions = detectMotion(frame, foreground_mask);

    // stationary object filtering
    std::vector<cv::Rect> filteredRegions;
    auto currentTime = std::chrono::steady_clock::now();

    // get tracked objects from the tracker
    const auto& trackedObjects = tracker_->getTrackedObjects();

    for (const auto& bbox : motion_regions) {
        // center point
        cv::Point center(bbox.x + bbox.width / 2, bbox.y + bbox.height / 2);

        // temporary ID for the object
        int tempId = center.x * 1000 + center.y;

        // check if the object is stationary
        // find the closest tracked object
        bool isStationary = false;
        double minDistance = std::numeric_limits<double>::max();
        int matchedId = -1;

        for (const auto& [id, obj] : trackedObjects) {
            double distance = cv::norm(cv::Point2f(center) - obj.center);
            if (distance < minDistance) {
                minDistance = distance;
                matchedId = id;
            }
        }

        // check if the matched object is stationary
        if (matchedId >= 0 && minDistance < stationaryDistanceThreshold) {
            const auto& matchedObj = trackedObjects.at(matchedId);
            auto elapsedMs =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    currentTime - matchedObj.firstDetectedTime)
                    .count();

            if (elapsedMs > stationaryTimeThresholdMs) {
                isStationary = true;
            }
        }

        // if the object is not stationary or ignoreStationaryObjects is false,
        // add it to the filtered regions
        if (!ignoreStationaryObjects || !isStationary) {
            filteredRegions.push_back(bbox);
        }
    }

    // update the tracker with the filtered regions
    tracker_->trackObjects(filteredRegions, const_cast<cv::Mat&>(frame));

    // Show the foreground mask in a separate window
    // cv::imshow("Foreground Mask", foreground_mask);
}

std::vector<cv::Rect> MotionDetector::detectMotion(const cv::Mat& frame,
                                                   cv::Mat& foreground_mask) {
    // Apply background subtraction
    cv::Mat processed_frame;
    cv::GaussianBlur(frame, processed_frame, cv::Size(5, 5), 0);
    bg_subtractor_->apply(processed_frame, foreground_mask);

    // Apply morphological operations to remove noise
    cv::Mat kernel =
        cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(foreground_mask, foreground_mask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(foreground_mask, foreground_mask, cv::MORPH_CLOSE, kernel);

    cv::threshold(foreground_mask, foreground_mask, 200, 255,
                  cv::THRESH_BINARY);

    // Find contours in the foreground mask
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(foreground_mask, contours, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);

    // Filter contours by area and create bounding rectangles
    std::vector<cv::Rect> motion_regions;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);

        // Filter objects by size using min_area_ and max_area_ parameters
        if (area >= min_area_ && area <= max_area_) {
            cv::Rect bbox = cv::boundingRect(contour);
            motion_regions.push_back(bbox);
        }
    }

    // merge overlapping boxes
    motion_regions = mergeOverlappingBoxes(motion_regions);

    return motion_regions;
}

std::vector<cv::Rect> MotionDetector::mergeOverlappingBoxes(
    const std::vector<cv::Rect>& initial_regions) {
    if (initial_regions.empty()) {
        return initial_regions;
    }

    std::vector<cv::Rect> merged_regions = initial_regions;
    bool merged = true;

    while (merged) {
        merged = false;

        for (size_t i = 0; i < merged_regions.size(); i++) {
            for (size_t j = i + 1; j < merged_regions.size(); j++) {
                cv::Point center_i(
                    merged_regions[i].x + merged_regions[i].width / 2,
                    merged_regions[i].y + merged_regions[i].height / 2);
                cv::Point center_j(
                    merged_regions[j].x + merged_regions[j].width / 2,
                    merged_regions[j].y + merged_regions[j].height / 2);

                double distance =
                    std::sqrt(std::pow(center_i.x - center_j.x, 2) +
                              std::pow(center_i.y - center_j.y, 2));

                // Merge if the distance between centers is less than 50 pixels,
                // hardcoded
                if (distance < 50) {
                    merged_regions[i] = merged_regions[i] | merged_regions[j];
                    merged_regions.erase(merged_regions.begin() + j);
                    merged = true;
                    break;
                }
            }
            if (merged) break;
        }
    }

    return merged_regions;
}

void MotionDetector::setMinPathDistance(double distance) {
    if (tracker_) {
        tracker_->setMinPathDistance(distance);
    }
}

void MotionDetector::notifyPath(int objectId, const cv::Point& start,
                                const cv::Point& end) {
    // Save current frame as image
    cv::Mat current_frame;
    if (capture_.read(current_frame)) {
        // Generate timestamp for filename
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_time_t), "%Y%m%d_%H%M%S");

        std::string timestamp = ss.str();
        std::string filename = IMAGE_DIR + "/motion_" +
                               std::to_string(objectId) + "_" + timestamp +
                               ".jpg";

        // Draw motion path on the image
        cv::line(current_frame, start, end, cv::Scalar(0, 255, 0), 2);
        cv::circle(current_frame, start, 5, cv::Scalar(0, 0, 255), -1);
        cv::circle(current_frame, end, 5, cv::Scalar(255, 0, 0), -1);

        // // Add text with object ID
        // cv::putText(current_frame, "ID: " + std::to_string(objectId),
        //             cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0,
        //             cv::Scalar(0, 255, 255), 2);

        // Add text with current time
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream time_ss;
        time_ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
        std::string current_time = time_ss.str();
        cv::putText(current_frame, current_time, cv::Point(10, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 255), 2);

        // Save image with path
        cv::imwrite(filename, current_frame);

        // Update latest image in HTTP server
        if (http_server && http_server->isRunning()) {
            http_server->updateLatestImage(filename);
        }

        std::cout << "Motion detection image saved: " << filename << std::endl;
    }

    if (tcp_client_ && tcp_client_->isConnected()) {
        bool sent = tcp_client_->sendDetection(objectId, start.x, start.y,
                                               end.x, end.y);

        if (sent) {
            // wait for server response, 150sec timeout
            bool response_received =
                tcp_client_->waitForResponse("MISSIONCOMPLETE", 150000);

            if (!response_received) {
                std::cout << "No response or denied by server" << std::endl;
            } else {
                std::cout << "Get response" << std::endl;
            }
        }
    }
}

void MotionDetector::setStationaryThresholds(double distance, int timeMs) {
    stationaryDistanceThreshold = distance;
    stationaryTimeThresholdMs = timeMs;
}

void MotionDetector::setIgnoreStationaryObjects(bool ignore) {
    ignoreStationaryObjects = ignore;
}
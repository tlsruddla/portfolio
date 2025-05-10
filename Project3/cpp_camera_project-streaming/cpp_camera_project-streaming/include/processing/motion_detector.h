#pragma once

#include <chrono>
#include <memory>
#include <opencv2/opencv.hpp>
#include <vector>

#include "../network/tcp_client.h"
#include "../network/http_server.h"
#include "object_tracker.h"

class MotionDetector {
public:
    MotionDetector(cv::VideoCapture& capture, TCPClient* tcpClient,
                   double minArea = 500, double maxArea = 20000);
    ~MotionDetector();

    /**
     * @brief Starts the motion detection process.
     * @return void
     * @note This method captures frames from the camera and processes them
     */
    void run();

    /**
     * @brief Stops the motion detection process.
     * @return void
     * @note This method stops capturing frames and releases resources.
     */
    void stop();

    /**
     * @brief Processes a single frame for motion detection.
     * @param frame The input frame.
     * @return void
     * @note This method detects motion in the given frame and notifies the TCP
     * @note client if a path is completed.
     */
    void processFrame(const cv::Mat& frame);

    /**
     * @brief Sets the minimum distance for path completion.
     * @param distance The minimum distance for path completion.
     * @return void
     */
    void setMinPathDistance(double distance);

    /**
     * @brief Sets the stationary thresholds for object detection.
     * @param distance The distance threshold for stationary objects.
     * @param timeMs The time threshold in milliseconds for stationary objects.
     * @return void
     */
    void setStationaryThresholds(double distance, int timeMs);

    /**
     * @brief Sets whether to ignore stationary objects.
     * @param ignore True to ignore stationary objects, false otherwise.
     * @return void
     */
    void setIgnoreStationaryObjects(bool ignore);

private:
    /**
     * @brief Detects motion in the given frame using background subtraction.
     * @param frame The input frame.
     * @param foreground_mask The output mask where motion is detected.
     * @return A vector of rectangles representing detected motion regions.
     */
    std::vector<cv::Rect> detectMotion(const cv::Mat& frame,
                                       cv::Mat& foreground_mask);

    /**
     * @brief Merges overlapping bounding boxes.
     * @param initial_regions The initial bounding boxes.
     * @return A vector of merged bounding boxes.
     */
    std::vector<cv::Rect> mergeOverlappingBoxes(
        const std::vector<cv::Rect>& initial_regions);

    /**
     * @brief Sends path information to the TCP client and saves the image.
     * @param objectId The ID of the object.
     * @param start The starting point of the path.
     * @param end The ending point of the path.
     * @return void
     */
    void notifyPath(int objectId, const cv::Point& start, const cv::Point& end);

    // camera, network related
    cv::VideoCapture& capture_;
    TCPClient* tcp_client_;
    std::unique_ptr<ObjectTracker> tracker_;
    bool running_;

    // background subtraction related
    cv::Ptr<cv::BackgroundSubtractor> bg_subtractor_;
    int history_;
    double dist_threshold_;
    bool detect_shadows_;

    // area threshold values
    double min_area_;
    double max_area_;

    // stationary object detection related
    double stationaryDistanceThreshold;
    int stationaryTimeThresholdMs;
    bool ignoreStationaryObjects;
};
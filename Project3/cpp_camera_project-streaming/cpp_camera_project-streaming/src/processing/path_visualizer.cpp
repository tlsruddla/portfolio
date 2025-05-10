#include "../../include/processing/path_visualizer.h"

#include <opencv2/imgproc.hpp>

#include "../../include/processing/object_tracker.h"

PathVisualizer::PathVisualizer()
    : line_thickness_(2), history_length_(30), min_path_length_to_draw_(30.0) {}

PathVisualizer::~PathVisualizer() {}

double PathVisualizer::calculateDistance(const cv::Point& p1,
                                         const cv::Point& p2) {
    return std::sqrt(std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2));
}

void PathVisualizer::drawObjects(cv::Mat& frame,
                                 const std::map<int, TrackedObject>& objects) {
    for (const auto& [id, object] : objects) {
        // Draw bounding box
        cv::Scalar color = getColorForID(id);
        cv::rectangle(frame, object.bbox, color, line_thickness_);

        // Draw ID
        std::string id_text = "ID: " + std::to_string(id);
        cv::putText(frame, id_text,
                    cv::Point(object.bbox.x, object.bbox.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 2);
    }
}

void PathVisualizer::drawPaths(cv::Mat& frame,
                               const std::map<int, TrackedObject>& objects) {
    for (const auto& [id, object] : objects) {
        cv::Scalar color = getColorForID(id);

        cv::Point start = object.pathStart;
        cv::Point end = object.pathEnd;

        double path_distance = calculateDistance(start, end);

        if (path_distance >= min_path_length_to_draw_) {
            cv::drawMarker(frame, start, cv::Scalar(0, 255, 0), cv::MARKER_STAR,
                           15, 2);
            cv::drawMarker(frame, end, cv::Scalar(0, 0, 255),
                           cv::MARKER_DIAMOND, 15, 2);

            cv::line(frame, start, end, cv::Scalar(255, 255, 0), 1,
                     cv::LINE_AA);

            if (object.path.size() > 1) {
                int start_idx = std::max(
                    0, static_cast<int>(object.path.size()) - history_length_);

                for (size_t i = start_idx; i < object.path.size() - 1; i++) {
                    cv::line(frame, object.path[i], object.path[i + 1], color,
                             line_thickness_);
                }
            }
        }
    }
}

cv::Scalar PathVisualizer::getColorForID(int id) {
    int hue = (id * 30) % 180;
    return cv::Scalar(hue, 255, 255);  // Using HSV color space
}
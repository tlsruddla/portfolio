#include "../../include/processing/object_tracker.h"

#include <algorithm>
#include <cmath>
#include <iostream>

ObjectTracker::ObjectTracker()
    : next_object_id_(0),
      max_distance_threshold_(150.0),
      max_frames_to_skip_(20),
      min_valid_path_distance_(50.0) {
    visualizer_ = std::make_unique<PathVisualizer>();
}

ObjectTracker::~ObjectTracker() {}

void ObjectTracker::trackObjects(const std::vector<cv::Rect>& detected_regions,
                                 cv::Mat& frame) {
    // set all tracked objects to inactive
    for (auto& [id, object] : tracked_objects_) {
        object.isActive = false;
    }

    // assign detections to tracked objects
    for (const auto& region : detected_regions) {
        int matched_id = assignDetectionToTracker(region);

        if (matched_id >= 0) {
            updateTrackedObject(tracked_objects_[matched_id], region);
        } else {
            TrackedObject new_object;
            new_object.id = next_object_id_;
            new_object.bbox = region;
            new_object.isActive = true;
            new_object.center = calculateCentroid(region);
            new_object.pathStart = new_object.center;
            new_object.pathEnd = new_object.center;
            new_object.firstDetectedTime = std::chrono::steady_clock::now();
            new_object.path.push_back(new_object.center);

            tracked_objects_[next_object_id_] = new_object;
            next_object_id_++;
        }
    }

    // inactive objects are removed after a certain number of frames
    for (auto it = tracked_objects_.begin(); it != tracked_objects_.end();) {
        if (!it->second.isActive) {
            it->second.frames_without_match++;

            if (it->second.frames_without_match > max_frames_to_skip_) {
                cv::Point start = it->second.pathStart;
                cv::Point end = it->second.pathEnd;

                double path_distance = calculateDistance(start, end);
                it->second.pathDistance = path_distance;

                if (path_distance >= min_valid_path_distance_) {
                    it->second.isPathValid = true;
                    std::cout << "Object " << it->first << " path: Start("
                              << start.x << "," << start.y << ") -> End("
                              << end.x << "," << end.y
                              << ") Distance: " << path_distance << std::endl;
                    // path completed callback
                    if (path_completed_callback_) {
                        path_completed_callback_(it->first, start, end,
                                                 path_distance);
                    }
                } else {
                    std::cout << "Object " << it->first
                              << " ignored (distance: " << path_distance
                              << " < " << min_valid_path_distance_ << ")"
                              << std::endl;
                }

                it = tracked_objects_.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }

    visualizer_->drawObjects(frame, tracked_objects_);
    visualizer_->drawPaths(frame, tracked_objects_);
}

void ObjectTracker::updateTrackedObject(TrackedObject& object,
                                        const cv::Rect& new_bbox) {
    object.bbox = new_bbox;
    object.center = calculateCentroid(new_bbox);
    object.pathEnd = object.center;
    object.frames_without_match = 0;
    object.isActive = true;

    object.path.push_back(object.center);

    // add isStationary update logic?
}

int ObjectTracker::assignDetectionToTracker(const cv::Rect& detection) {
    cv::Point detection_center = calculateCentroid(detection);
    int best_match_id = -1;
    double min_distance = max_distance_threshold_;

    // area weight
    const static double area_weight = 0.2;

    for (const auto& [id, object] : tracked_objects_) {
        double distance = calculateDistance(detection_center, object.center);

        double area_ratio = std::min(
            static_cast<double>(detection.area()) / object.bbox.area(),
            static_cast<double>(object.bbox.area()) / detection.area());

        double weighted_distance =
            distance * (1 - area_weight) +
            (1 - area_ratio) * area_weight * max_distance_threshold_;

        if (weighted_distance < min_distance) {
            min_distance = weighted_distance;
            best_match_id = id;
        }
    }

    return best_match_id;
}

cv::Point ObjectTracker::calculateCentroid(const cv::Rect& box) {
    return cv::Point(box.x + box.width / 2, box.y + box.height / 2);
}

double ObjectTracker::calculateDistance(const cv::Point& p1,
                                        const cv::Point& p2) {
    return std::sqrt(std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2));
}

const std::map<int, TrackedObject>& ObjectTracker::getTrackedObjects() const {
    return tracked_objects_;
}

bool ObjectTracker::getPathEndpoints(int object_id, cv::Point& start_point,
                                     cv::Point& end_point) const {
    auto it = tracked_objects_.find(object_id);
    if (it != tracked_objects_.end()) {
        start_point = it->second.pathStart;
        end_point = it->second.pathEnd;
        return true;
    }
    return false;
}

void ObjectTracker::setMinPathDistance(double distance) {
    min_valid_path_distance_ = distance;
}
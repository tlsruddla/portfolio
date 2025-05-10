#pragma once

#include <functional>
#include <map>
#include <memory>
#include <opencv2/core.hpp>
#include <vector>

#include "path_visualizer.h"
#include "tracked_object.h"

// callback type for path completion
using PathCompletedCallback =
    std::function<void(int, const cv::Point&, const cv::Point&, double)>;

class ObjectTracker {
public:
    ObjectTracker();
    ~ObjectTracker();

    /**
     * @brief track objects in the given frame
     * @param detected_regions detected regions in the current frame
     * @param frame current frame
     * @return void
     */
    void trackObjects(const std::vector<cv::Rect>& detected_regions,
                      cv::Mat& frame);

    // getter for tracked objects
    const std::map<int, TrackedObject>& getTrackedObjects() const;

    /**
     * @brief get the path endpoints of a tracked object
     * @param object_id ID of the tracked object
     * @param start_point start point of the path
     * @param end_point end point of the path
     * @return true if the path endpoints are successfully retrieved
     * @return false if the object ID is not found
     */
    bool getPathEndpoints(int object_id, cv::Point& start_point,
                          cv::Point& end_point) const;

    /**
     * @brief set the minimum path distance for valid paths
     * @param distance minimum distance for a valid path
     * @return void
     */
    void setMinPathDistance(double distance);

    /**
     * @brief set the callback function for path completion
     * @param callback callback function to be called on path completion
     * @return void
     */
    void setPathCompletedCallback(PathCompletedCallback callback) {
        path_completed_callback_ = callback;
    }

private:
    // tracked objects map
    // key: object ID, value: TrackedObject
    std::map<int, TrackedObject> tracked_objects_;

    // next object ID
    // (used for assigning unique IDs to new objects)
    int next_object_id_;

    // visualizer for path drawing
    std::unique_ptr<PathVisualizer> visualizer_;

    // tracking parameters
    double max_distance_threshold_;
    int max_frames_to_skip_;
    double min_valid_path_distance_;

    // callback for path completion
    PathCompletedCallback path_completed_callback_;

    // internal methods

    /**
     * @brief assign a detected region to an existing tracked object
     * @param detection detected region
     * @return ID of the matched tracked object, or -1 if no match is found
     */
    int assignDetectionToTracker(const cv::Rect& detection);

    /**
     * @brief update the tracked object with new bounding box
     * @param object tracked object to be updated
     * @param new_bbox new bounding box
     * @return void
     */
    void updateTrackedObject(TrackedObject& object, const cv::Rect& new_bbox);

    /**
     * @brief update the path of the tracked object
     * @param object tracked object to be updated
     * @param new_center new center point
     * @return void
     */
    cv::Point calculateCentroid(const cv::Rect& box);

    /**
     * @brief calculate the distance between two points
     * @param p1 first point
     * @param p2 second point
     * @return distance between the two points
     */
    double calculateDistance(const cv::Point& p1, const cv::Point& p2);
};
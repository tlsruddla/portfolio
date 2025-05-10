#pragma once

#include <map>
#include <opencv2/core.hpp>
#include <vector>

class PathVisualizer {
public:
    PathVisualizer();
    ~PathVisualizer();

    /**
     * @brief Draws bounding boxes and IDs for tracked objects on the frame.
     * @param frame The frame on which to draw the objects.
     * @param objects A map of tracked objects, where the key is the object ID
     *                and the value is the tracked object structure.
     * @return void
     */
    void drawObjects(cv::Mat& frame,
                     const std::map<int, struct TrackedObject>& objects);

    /**
     * @brief Draws paths for tracked objects on the frame.
     * @param frame The frame on which to draw the paths.
     * @param objects A map of tracked objects, where the key is the object ID
     *                and the value is the tracked object structure.
     * @return void
     */
    void drawPaths(cv::Mat& frame,
                   const std::map<int, struct TrackedObject>& objects);

private:
    // Drawing parameters
    int line_thickness_;
    int history_length_;

    // Minimum path length to visualize
    double min_path_length_to_draw_;

    /**
     * @brief Generates a color for a given object ID.
     * @param id The object ID.
     * @return A cv::Scalar representing the color for the object.
     */
    cv::Scalar getColorForID(int id);

    /**
     * @brief Calculates the Euclidean distance between two points.
     * @param p1 The first point.
     * @param p2 The second point.
     * @return The distance between the two points.
     */
    double calculateDistance(const cv::Point& p1, const cv::Point& p2);
};
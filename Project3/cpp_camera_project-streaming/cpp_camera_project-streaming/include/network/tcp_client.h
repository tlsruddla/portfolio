#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class TCPClient {
public:
    TCPClient();
    ~TCPClient();

    /**
     * @brief connect to the server
     * @param ip server IP address
     * @param port server port number
     * @param name client name
     * @return true if connection is successful, false otherwise
     */
    bool connect(const std::string& ip, int port, const std::string& name);

    /**
     * @brief disconnect from the server
     * @return void
     */
    void disconnect();

    /**
     * @brief check if the client is connected to the server
     * @return true if connected, false otherwise
     */
    bool isConnected() const;

    /**
     * @brief send a message to the server
     * @param message message to be sent
     * @return true if sending is successful, false otherwise
     */
    bool sendMessage(const std::string& message);

    /**
     * @brief set the resolution of the client
     * @param width width of the frame
     * @param height height of the frame
     * @return void
     * @note This method sets the resolution of the client. The default
     * resolution is 640x480. If the width or height is less than or equal to 0,
     * it will be set to the default value. The resolution is used for scaling
     * the coordinates of the detected objects.
     */
    void setResolution(int width, int height);

    /**
     * @brief send a detection message to the server
     * @param objectId ID of the detected object
     * @param startX starting X coordinate of the detected object
     * @param startY starting Y coordinate of the detected object
     * @param endX ending X coordinate of the detected object
     * @param endY ending Y coordinate of the detected object
     * @return true if sending is successful, false otherwise
     * @note This method sends a detection message to the server. The
     * coordinates are scaled
     * @note according to the resolution set by the setResolution method. The
     * message format is:
     * @note "[ROS]GOGOAL@<robotXd>@<robotYd>@<angle>\n"
     * @note where robotXd and robotYd are the scaled coordinates and angle is a
     * random value
     * @note between 0.1 and 0.9.
     */
    bool sendDetection(int objectId, int startX, int startY, int endX,
                       int endY);

    /**
     * @brief wait for a response from the server
     * @param expected_response expected response from the server
     * @param timeout_ms timeout in milliseconds
     * @return true if the expected response is received, false otherwise
     * @note MISSIONCOMPLETE should be received
     */
    bool waitForResponse(const std::string& expected_response,
                         int timeout_ms = 5000);

private:
    // buffer size
    static const int BUFFER_SIZE = 1024;

    // socket related var
    int socket_fd_;
    std::atomic<bool> connected_;
    std::string client_name_;

    // resolution
    int frame_width_ = 640;
    int frame_height_ = 480;

    // thread related var
    std::thread receive_thread_;
    std::mutex send_mutex_;

    // synchronization related var
    std::mutex response_mutex_;
    std::condition_variable response_cv_;
    std::string last_response_;
    std::atomic<bool> waiting_for_response_;

    // internal methods
    void receiveMessages();
    void handleError(const std::string& message);
};
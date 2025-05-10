#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

/**
 * @class HTTPServer
 * @brief A simple HTTP server that serves image files
 */
class HTTPServer {
public:
    /**
     * @brief Constructor for HTTPServer
     * @param port The port to listen on
     */
    HTTPServer(int port = 8080);

    /**
     * @brief Destructor for HTTPServer
     */
    ~HTTPServer();

    /**
     * @brief Start the HTTP server
     * @param imageDir The directory containing images to serve
     * @return True if server started successfully, false otherwise
     */
    bool start(const std::string& imageDir);

    /**
     * @brief Stop the HTTP server
     */
    void stop();

    /**
     * @brief Check if the server is running
     * @return True if running, false otherwise
     */
    bool isRunning() const;

    /**
     * @brief Update the latest image to be served
     * @param imagePath The path to the image file
     */
    void updateLatestImage(const std::string& imagePath);

private:
    /**
     * @brief Server thread function
     */
    void serverFunction();

    int port_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    std::condition_variable server_cv_;
    std::mutex server_mutex_;
    std::mutex image_mutex_;
    std::string image_dir_;
    std::string latest_image_;
    std::string initial_image_;
};

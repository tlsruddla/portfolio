#include "../../include/network/http_server.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

HTTPServer::HTTPServer(int port)
    : port_(port), running_(false), latest_image_(""), initial_image_("") {}

HTTPServer::~HTTPServer() {
    stop();
}

bool HTTPServer::start(const std::string& imageDir) {
    if (running_) {
        return true;  // already running
    }

    image_dir_ = imageDir;
    latest_image_ = image_dir_ + "/latest.jpg";
    initial_image_ = image_dir_ + "/initial.jpg";

    // Create images directory if it doesn't exist
    std::string command = "mkdir -p " + image_dir_;
    int result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Failed to create image directory: " << image_dir_
                  << std::endl;
        return false;
    }

    running_ = true;
    server_thread_ = std::thread(&HTTPServer::serverFunction, this);

    return true;
}

void HTTPServer::stop() {
    if (running_) {
        running_ = false;

        // Wait for server thread with condition variable
        if (server_thread_.joinable()) {
            std::cout << "Waiting for HTTP server thread to finish..."
                      << std::endl;

            {
                std::unique_lock<std::mutex> lock(server_mutex_);
                if (server_cv_.wait_for(lock, std::chrono::seconds(3),
                                        [this] { return !running_.load(); })) {
                    std::cout << "HTTP server thread terminated gracefully"
                              << std::endl;
                } else {
                    std::cout
                        << "HTTP server thread timeout - forcing termination"
                        << std::endl;
                }
            }

            if (server_thread_.joinable()) {
                server_thread_.join();
            }
        }
    }
}

bool HTTPServer::isRunning() const {
    return running_;
}

void HTTPServer::updateLatestImage(const std::string& imagePath) {
    std::lock_guard<std::mutex> lock(image_mutex_);

    // Copy the image to the latest image location
    std::ifstream src(imagePath, std::ios::binary);
    std::ofstream dst(latest_image_, std::ios::binary);

    if (src && dst) {
        dst << src.rdbuf();
    } else {
        std::cerr << "Failed to update latest image" << std::endl;
    }
}

void HTTPServer::serverFunction() {
    int server_fd = -1;

    try {
        int new_socket;
        struct sockaddr_in address;
        int addrlen = sizeof(address);

        // Creating socket file descriptor
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            std::cerr << "Socket creation failed" << std::endl;
            return;
        }

        // Allow reuse of port
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
                       sizeof(opt))) {
            std::cerr << "Setsockopt failed" << std::endl;
            close(server_fd);
            return;
        }

        // Set socket to non-blocking mode
        int flags = fcntl(server_fd, F_GETFL, 0);
        fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);

        // Bind the socket
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Bind failed" << std::endl;
            close(server_fd);
            return;
        }

        // Listen for connections
        if (listen(server_fd, 3) < 0) {
            std::cerr << "Listen failed" << std::endl;
            close(server_fd);
            return;
        }

        std::cout << "HTTP server started on port " << port_ << std::endl;

        while (running_) {
            // Accept connections with timeout
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(server_fd, &readfds);

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 500000;  // 500ms timeout

            int activity = select(server_fd + 1, &readfds, NULL, NULL, &tv);

            if (activity < 0 && errno != EINTR) {
                std::cerr << "Select error" << std::endl;
            }

            if (!running_) break;

            if (activity == 0) continue;

            if (FD_ISSET(server_fd, &readfds)) {
                if ((new_socket = accept(server_fd, (struct sockaddr*)&address,
                                         (socklen_t*)&addrlen)) < 0) {
                    continue;
                }

                // Read client request
                char buffer[1024] = {0};
                read(new_socket, buffer, 1024);

                std::string image_path;
                {
                    std::lock_guard<std::mutex> lock(image_mutex_);
                    image_path = latest_image_;
                }

                // Read image file
                std::ifstream image_file(image_path,
                                         std::ios::binary | std::ios::ate);
                if (!image_file) {
                    const char* error_response =
                        "HTTP/1.1 404 Not Found\r\nContent-Length: "
                        "9\r\n\r\nNot Found";
                    send(new_socket, error_response, strlen(error_response), 0);
                    close(new_socket);
                    continue;
                }

                std::streamsize size = image_file.tellg();
                image_file.seekg(0, std::ios::beg);

                std::vector<char> image_data(size);
                if (image_file.read(image_data.data(), size)) {
                    std::stringstream response;
                    response << "HTTP/1.1 200 OK\r\n";
                    response << "Content-Type: image/jpeg\r\n";
                    response << "Content-Length: " << size << "\r\n";
                    response << "\r\n";

                    std::string header = response.str();
                    send(new_socket, header.c_str(), header.length(), 0);
                    send(new_socket, image_data.data(), size, 0);
                }

                close(new_socket);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "HTTP server exception: " << e.what() << std::endl;
    }

    // Properly close the server socket when exiting
    if (server_fd != -1) {
        shutdown(server_fd, SHUT_RDWR);
        close(server_fd);
        server_fd = -1;
    }

    // Notify that we're done
    {
        std::lock_guard<std::mutex> lock(server_mutex_);
        std::cout << "HTTP server stopped" << std::endl;
        server_cv_.notify_all();
    }
}

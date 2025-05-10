#include "../../include/network/tcp_client.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <condition_variable>
#include <cstring>
#include <iostream>
#include <mutex>

TCPClient::TCPClient()
    : socket_fd_(-1), connected_(false), waiting_for_response_(false) {}

TCPClient::~TCPClient() {
    disconnect();
}

bool TCPClient::connect(const std::string& ip, int port,
                        const std::string& name) {
    if (connected_) disconnect();

    client_name_ = name;

    socket_fd_ = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_fd_ == -1) {
        handleError("socket() error");
        return false;
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;  // 500ms
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        handleError("setsockopt failed");
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    server_addr.sin_port = htons(port);

    std::cout << "Connecting to " << ip << ":" << port << " as '" << name
              << "'..." << std::endl;
    if (::connect(socket_fd_, (struct sockaddr*)&server_addr,
                  sizeof(server_addr)) == -1) {
        handleError("connect() error");
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    std::string auth_msg = "[" + name + ":PASSWD]";
    if (write(socket_fd_, auth_msg.c_str(), auth_msg.size()) <= 0) {
        handleError("Failed to send authentication message");
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    // start the receive thread
    connected_ = true;
    receive_thread_ = std::thread(&TCPClient::receiveMessages, this);

    std::cout << "Connected to server successfully!" << std::endl;
    return true;
}

void TCPClient::disconnect() {
    if (connected_) {
        connected_ = false;

        if (socket_fd_ != -1) {
            shutdown(socket_fd_, SHUT_RDWR);
            close(socket_fd_);
            socket_fd_ = -1;
        }

        // wake up the waiting thread
        {
            std::unique_lock<std::mutex> lock(response_mutex_);
            waiting_for_response_ = false;
            last_response_ = "DISCONNECTED";
            response_cv_.notify_all();
        }

        // wait max 3 seconds for the receive thread to finish
        if (receive_thread_.joinable()) {
            std::cout << "Waiting for receive thread to finish..." << std::endl;
            receive_thread_.join();
        }

        std::cout << "Disconnected from server." << std::endl;
    }
}

bool TCPClient::isConnected() const {
    return connected_;
}

bool TCPClient::sendMessage(const std::string& message) {
    if (!connected_ || socket_fd_ == -1) {
        return false;
    }

    std::lock_guard<std::mutex> lock(send_mutex_);

    ssize_t sent = write(socket_fd_, message.c_str(), message.size());

    if (sent <= 0) {
        handleError("Failed to send message");
        connected_ = false;
        return false;
    }

    std::cout << "Sent to server: " << message << std::endl;
    return true;
}

void TCPClient::setResolution(int width, int height) {
    frame_width_ = width > 0 ? width : 640;
    frame_height_ = height > 0 ? height : 480;
    std::cout << "TCP client using resolution: " << frame_width_ << "x"
              << frame_height_ << std::endl;
}

bool TCPClient::sendDetection(int objectId, int startX, int startY, int endX,
                              int endY) {
    char buffer[256];

    // coordinate scaling
    double robotXd = 0.8 + 0.00135417 * endX;
    double robotYd = 0.00126953 * endX - 0.00007813 * endY - 0.2375;

    // 0.1 ~ 0.9 random
    double angle = rand() % 10 / 10.0 + 0.1;

    snprintf(buffer, sizeof(buffer), "[ROS]GOGOAL@%.2f@%.2f@%.2f\n", robotXd,
             robotYd, angle);

    // set waiting_for_response_ to true
    // and clear last_response_ before sending
    {
        std::lock_guard<std::mutex> lock(response_mutex_);
        waiting_for_response_ = true;
        last_response_.clear();
    }

    bool sent = sendMessage(buffer);
    if (!sent) {
        std::lock_guard<std::mutex> lock(response_mutex_);
        waiting_for_response_ = false;
        return false;
    }

    return true;
}

bool TCPClient::waitForResponse(const std::string& expected_response,
                                int timeout_ms) {
    std::unique_lock<std::mutex> lock(response_mutex_);

    if (!waiting_for_response_) return false;

    bool result = response_cv_.wait_for(
        lock, std::chrono::milliseconds(timeout_ms),
        [this, &expected_response]() {
            return !waiting_for_response_ ||
                   (last_response_.find(expected_response) !=
                    std::string::npos);
        });

    waiting_for_response_ = false;

    return result &&
           last_response_.find(expected_response) != std::string::npos;
}

void TCPClient::receiveMessages() {
    char buffer[BUFFER_SIZE];
    ssize_t received;

    while (connected_) {
        memset(buffer, 0, BUFFER_SIZE);

        received = read(socket_fd_, buffer, BUFFER_SIZE - 1);

        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // timeout, check connected_ flag and continue
                continue;
            } else {
                if (connected_) {
                    handleError("Read error");
                    connected_ = false;
                }
                break;
            }
        } else if (received == 0) {
            // connection closed by server
            if (connected_) {
                handleError("Connection closed by server");
                connected_ = false;
            }
            break;
        } else {
            // valid data received
            buffer[received] = '\0';
            std::string response(buffer);
            std::cout << "Server: " << response << std::endl;

            // handle the response
            {
                std::lock_guard<std::mutex> lock(response_mutex_);
                if (waiting_for_response_) {
                    last_response_ = response;
                    response_cv_.notify_all();
                }
            }
        }
    }

    std::cout << "Receive thread terminated." << std::endl;
}

void TCPClient::handleError(const std::string& message) {
    std::cerr << "TCP Error: " << message << std::endl;
    perror("System");
}
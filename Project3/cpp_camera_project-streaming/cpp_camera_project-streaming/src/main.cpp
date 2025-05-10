#include <unistd.h>

#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

#include "../include/network/tcp_client.h"
#include "../include/processing/motion_detector.h"

const std::string SERVER_IP = "10.10.141.133";
const int SERVER_PORT = 5000;
const std::string CLIENT_NAME = "JETSON";

int main(int argc, char** argv) {
    double min_area = 100.0;
    double max_area = 15000.0;
    double min_path_distance = 300.0;
    int camera = 0;
    double stationaryDistance = 50.0;
    int stationaryTimeMs = 3000;

    if (argc >= 3) {
        min_area = std::stod(argv[1]);
        max_area = std::stod(argv[2]);

        if (argc >= 4) {
            min_path_distance = std::stod(argv[3]);
        }

        if (argc >= 5) {
            camera = std::stoi(argv[4]);
        }
    }

    std::cout << "Using min area: " << min_area << ", max area: " << max_area
              << ", min path distance: " << min_path_distance << std::endl;

    // TCP client initialization
    TCPClient tcpClient;
    bool connected = tcpClient.connect(SERVER_IP, SERVER_PORT, CLIENT_NAME);

    if (!connected) {
        std::cerr << "Failed to connect to server. Continuing without server "
                     "connection."
                  << std::endl;
    } else {
        // tcpClient.sendMessage("Motion detection system started\n");
    }

    // open camera
    cv::VideoCapture cap(camera);
    if (!cap.isOpened()) {
        std::cerr << "Failed to open camera." << std::endl;
        return -1;
    }
    
    std::cout << "Camera opened successfully." << std::endl;
    std::cout << "Press 'q' to quit." << std::endl;

    // MotionDetector initialization
    MotionDetector detector(cap, &tcpClient, min_area, max_area);
    detector.setMinPathDistance(min_path_distance);
    detector.setStationaryThresholds(stationaryDistance, stationaryTimeMs);
    detector.setIgnoreStationaryObjects(true);
    detector.run();

    // cleanup
    cap.release();
    tcpClient.disconnect();

    return 0;
}

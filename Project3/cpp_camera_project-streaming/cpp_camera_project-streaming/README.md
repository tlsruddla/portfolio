# 움직이는 객체 검출하기

## 환경

|**Software**|**Version**|
|---|---|
|**OS**|**Ubuntu 20.04**|
|**C++**|**C++17**|
|**GCC**|**8.4.0**|
|**CMAKE**|**3.16.3**|
|**OpenCV**|**4.8.0**|

## 프로젝트 개요
카메라를 연결하여 움직이는 객체를 추적하고, 동선을 기록하는 프로젝트입니다. 실시간으로 영상을 분석하여 움직임을 감지하고 객체의 위치와 이동 경로를 시각화합니다.

경로의 시작점을 전송할 수 있는 TCP 프로토콜이 구현되어 있으며, 본 프로젝트에서는 TCP 프로토콜을 통해 ROS 시스템에 적절한 명령을 전송합니다.

C++를 사용하였으며, CPU를 활용한 연산만을 사용하기에 ARM-cortex M cpu를 사용한 모든 보드에서 사용할 수 있습니다.

원활한 구동을 위하여 JETSON NANO 이상의 CPU를 갖는 보드에서 구동할 것을 권장합니다.

## 주요 기능
- 실시간 카메라 영상 처리
- 배경 제거를 통한 움직임 감지
- 객체 추적 및 경로 기록
- 추적 결과 시각화
- TCP 프로토콜을 통한 객체 추적 결과 전송
- 시각화 결과물 실시간 확인

## 설치 방법

### 프로젝트 빌드
```bash
git clone https://github.com/azruine/cpp_camera_project.git
cd cpp_camera_project
./init.sh
make
```

## 사용 방법
```bash
./build/release/bin/camera_app
```

## 프로젝트 구조
```
cpp_camera_project/
├── CMakeLists.txt
├── include/
│   ├── network/
│   ├   ├── http_server.h
│   │   └── tcp_client.h
│   └── processing/
│       ├── motion_detector.h
│       ├── object_tracker.h
│       ├── path_visualizer.h
│       └── tracked_object.h
├── src/
│   ├── network/
│   ├   ├── http_server.cpp
│   │   └── tcp_client.cpp
│   ├── processing/
│   │   ├── motion_detector.cpp
│   │   ├── object_tracker.cpp
│   │   └── path_visualizer.cpp
│   └── main.cpp
├── build/
└── README.md
```

## 향후 계획
- CUDA 가속화 지원 추가
- 다중 객체 추적 알고리즘 개선
- 딥러닝 기반 객체 인식 통합

# ROS Docker 환경 구축 프로젝트

## 환경

|**Software**|**Version**|
|---|---|
|**OS**|**Ubuntu 20.04**|
|**Docker**|**20.10.x**|
|**ROS**|**Noetic**|

## 프로젝트 개요
Jetson 보드에서 ROS(Robot Operating System)를 Docker 컨테이너로 실행하기 위한 프로젝트입니다. 
이 환경은 로봇 어플리케이션 개발, 테스트 및 배포를 위한 일관된 환경을 제공하며, 
Jetson 플랫폼에 최적화되어 있습니다.

Docker를 활용하여 다양한 ROS 패키지와 의존성을 간편하게 관리하고, 
개발 환경을 쉽게 복제하고 공유할 수 있습니다.

## 주요 기능
- 사전 구성된 ROS Noetic 환경
- 카메라 및 센서 드라이버 통합
- TCP/IP를 통한 외부 시스템과 통신
- 컨테이너화된 ROS 노드 실행 환경
- 볼륨 마운트를 통한 데이터 영속성

## 설치 방법

### Docker 설치
```bash
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker $USER
```
(변경사항 적용을 위해 로그아웃 후 다시 로그인)

### ROS Docker 이미지 빌드
```bash
docker-compose build -t
```

## 사용 방법

### 컨테이너 실행
```bash
docker-compose up -d
```

### ROS 노드 실행 예제
```bash
# 컨테이너 내부에서
roscore
/workspace/navi.sh
rosrun rosGoalClient rosGoalClient 5000 USER
```

## TCP/IP 통신
본 ROS 환경은 외부 시스템과 TCP/IP 통신을 통해 데이터를 주고 받을 수 있습니다.
기본 설정된 포트는 5000이며, 커스텀 TCP 서버를 통해 통신이 가능합니다.

## 향후 계획
- ROS2 Foxy/Humble 지원 추가
- 다중 로봇 제어 시스템 구현

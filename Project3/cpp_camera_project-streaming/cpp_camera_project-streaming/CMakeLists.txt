cmake_minimum_required(VERSION 3.10)
project(CudaCameraApp)

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 빌드 타입이 지정되지 않은 경우 Release로 설정
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# 빌드 타입별 컴파일러 플래그 설정
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# CUDA
find_package(CUDA REQUIRED)
include_directories(${CUDA_INCLUDE_DIRS})

# GStreamer 패키지 검색 (추가됨)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)

# Network
find_package(Threads REQUIRED)

# OpenCV
find_package(OpenCV 4 REQUIRED COMPONENTS 
    core 
    cudaarithm 
    cudafilters
    cudaimgproc
    cudawarping
    cudabgsegm
    cudafeatures2d
    video
    videoio
    imgproc
    highgui
)
include_directories(${OpenCV_INCLUDE_DIRS})

# GStreamer 헤더 포함 디렉터리 추가 (추가됨)
include_directories(${GSTREAMER_INCLUDE_DIRS} ${GST_APP_INCLUDE_DIRS})

# Main camera app
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(camera_app ${SOURCES})

# include directory
target_include_directories(camera_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# link OpenCV and CUDA libraries
target_link_libraries(camera_app ${OpenCV_LIBS} ${CUDA_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} stdc++fs)
# set output directory for the executables
set_target_properties(camera_app PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 설치 대상 설정 (선택 사항)
install(TARGETS camera_app  DESTINATION bin)

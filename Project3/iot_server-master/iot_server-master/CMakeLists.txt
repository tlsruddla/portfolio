cmake_minimum_required(VERSION 3.10)
project(IotProject)

# C11 표준을 전역으로 설정
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 빌드 타입이 지정되지 않은 경우 Release로 설정
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# 빌드 타입별 컴파일러 플래그 설정
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Network
find_package(Threads REQUIRED)

# Main camera app
add_executable(iot_server src/iot_server.c)
add_executable(iot_client src/iot_client.c)

# 실행 파일에 필요한 라이브러리 링크
target_link_libraries(iot_server PRIVATE ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(iot_client PRIVATE ${CMAKE_THREAD_LIBS_INIT})

# 추가 컴파일러 옵션 설정
target_compile_options(iot_server PRIVATE -Wall -Wextra)
target_compile_options(iot_client PRIVATE -Wall -Wextra)

# 설치 규칙
install(TARGETS iot_server iot_client
    RUNTIME DESTINATION bin
)

# 설정 파일이 있다면 설치
install(FILES config/server_config.conf
    DESTINATION etc/iot_server
    OPTIONAL
)

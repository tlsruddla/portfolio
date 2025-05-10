.PHONY: all clean build debug release run rebuild

# 기본 타겟은 release 빌드
all: release

# Release 빌드 (O2 최적화)
release:
		mkdir -p build/release
		cd build/release && cmake -DCMAKE_BUILD_TYPE=Release ../.. && make

# Debug 빌드 (디버깅 심볼 포함)
debug:
		mkdir -p build/debug
		cd build/debug && cmake -DCMAKE_BUILD_TYPE=Debug ../.. && make

# 빌드 디렉토리 정리
clean:
		rm -rf build/*

# 전체 다시 빌드
rebuild: clean release

# Debug 다시 빌드
rebuild-debug: clean debug

# Release 버전 실행
run:
		./build/release/bin/camera_app

# Debug 버전 실행
run-debug:
		./build/debug/bin/camera_app

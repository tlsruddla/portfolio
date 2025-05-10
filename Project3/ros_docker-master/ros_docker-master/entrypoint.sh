#!/bin/bash
set -e

# 기존 ros_entrypoint.sh 실행
source /ros_entrypoint.sh

# 내 환경 스크립트 source
if [ -f /root/test.bash ]; then
  echo "[custom_entrypoint] sourcing /root/test.bash"
  source /root/test.bash
fi

# 남은 명령 실행
exec "$@"

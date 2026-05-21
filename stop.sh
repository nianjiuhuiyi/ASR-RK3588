#!/usr/bin/env bash

set -e
ps aux | grep asr_rknn_serve | grep -v grep | awk '{print $2}' | xargs kill
echo "服务已停止."

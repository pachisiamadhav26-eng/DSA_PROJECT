#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")/.."
if [ "${QUICK:-0}" = 1 ]; then
  "${PYTHON:-python3}" scripts/run_all.py --quick
else
  "${PYTHON:-python3}" scripts/run_all.py
fi

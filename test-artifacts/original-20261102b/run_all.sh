#!/usr/bin/env bash
# Sequential Linux regression against Statistics Backend 2.5.0 Original (02/11/2026, batch b).
cd /home/dev/DDS-Monitor
B=$(cat .task3-deps/task3-builds/LATEST)
A=test-artifacts/original-20261102b
export LD_LIBRARY_PATH=/home/dev/dds-e2e/backend-original-install/lib:/home/dev/dds-deps/install/lib:/home/dev/Qt/6.7.2/gcc_64/lib
export TASK3_PATCHED_BACKEND_PREFIX=/home/dev/dds-e2e/backend-original-install
step(){ name=$1; shift; start=$(date +%s); "$@" > $A/$name.stdout 2>&1; rc=$?; echo "$name rc=$rc $(( $(date +%s)-start ))s" | tee -a $A/summary.txt; }
step ctest-release ctest --test-dir $B/release --output-on-failure
step local python3 -I task3/tests/local_integration.py --build $B/release --hello-world /home/dev/dds-tests/hello_world/build/hello_world --output $A/local
step continuation python3 -I task3/tests/continuation_integration.py --build $B/release --output $A/continuation --duration 60
step subscriber-restart python3 -I test-artifacts/p0-crash-20261016/subscriber_restart_regression.py --build $B/release --output $A/subscriber-restart
for i in 1 2 3 4 5; do
  step publisher-30-run$i python3 -I test-artifacts/p0-crash-20261016/publisher_stop_restart_reproducer.py --master $B/release/dds_monitor --probe $B/release/task3/task3_probe --output $A/publisher-30-run$i --cycles 30 --dwell 2
done
echo DONE | tee -a $A/summary.txt

#!/usr/bin/env bash
# Render every Task 3 page state with the QML and the WebEngine dashboard, then stack each pair side by side.
set -u
cd /home/dev/DDS-Monitor
OUT=${1:-test-artifacts/webengine-20261102/shots}
mkdir -p $OUT
cmake --build .task3-deps/qt683-builds/release --target task3_dashboard_preview --parallel 6 >/dev/null || exit 1
P=.task3-deps/qt683-builds/release/task3/task3_dashboard_preview
A=test-artifacts/final-fix-20261019/ui-1600/mapped_application_and_flow.json
B=test-artifacts/final-fix-20261019/ui-1600/offline_unverified_alert.json
export LD_LIBRARY_PATH=/home/dev/Qt/6.8.3/gcc_64/lib QT_QPA_PLATFORM=offscreen
shot(){ name=$1; shift; for pg in qml web; do timeout 60 $P --page $pg "$@" --output $OUT/$name-$pg.png >/dev/null 2>&1 || echo "FAIL $name $pg"; done
  python3 -I -c "
from PIL import Image; import sys
a,b=Image.open('$OUT/$name-qml.png'),Image.open('$OUT/$name-web.png')
c=Image.new('RGB',(a.width,a.height+b.height+6),'red'); c.paste(a,(0,0)); c.paste(b,(0,a.height+6)); c.save('$OUT/$name-pair.png')" 2>/dev/null; }
shot overview --fixture $A
shot resources --fixture $A --host linux-agent-a --tab 0
shot network --fixture $A --host linux-agent-a --tab 0 --resource-tab 1
shot apps --fixture $A --host linux-agent-a --app publisher --tab 1
shot flows --fixture $A --host linux-agent-a --app publisher --tab 2 --flow
shot time --fixture $A --host linux-agent-a --tab 3
shot offline --fixture $B --host linux-agent-a --app publisher --tab 1
shot events --fixture $B --events
echo done

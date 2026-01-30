#!/usr/bin/env bash
set -euo pipefail

export DISPLAY=${DISPLAY:-:99}
export BM_API_PORT=${BM_API_PORT:-3001}
export BM_VNC_PORT=${BM_VNC_PORT:-5900}
export BM_NOVNC_PORT=${BM_NOVNC_PORT:-6080}

Xvfb "$DISPLAY" -screen 0 1920x1080x24 -nolisten tcp &

x11vnc -display "$DISPLAY" \
  -forever \
  -shared \
  -nopw \
  -rfbport "$BM_VNC_PORT" \
  -noxrecord -noxfixes -noxdamage &

websockify --web /usr/share/novnc "$BM_NOVNC_PORT" "localhost:${BM_VNC_PORT}" &

if [ -f /crawler/server.js ]; then
node /crawler/server.js
else
node /crawler/dist/server.js
fi

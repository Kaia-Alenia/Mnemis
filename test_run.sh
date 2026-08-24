#!/bin/bash
export QT_DEBUG_PLUGINS=1
export QML_IMPORT_TRACE=1
cd /home/alejandro/Escritorio/Mnemis
./build/bin/mnemis_app > run_log_verbose.txt 2>&1 &
APP_PID=$!
sleep 5
kill $APP_PID
cat run_log_verbose.txt | grep -E "(Layout|layout|Stack|stack|visibility|window|Window)" | head -n 30

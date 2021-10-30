#!/bin/bash






exterior_serial_dev="/build/sim_exterior.pts"
interior_serial_dev="/build/sim_interior.pts"
shed=/build/shed3.dbg.exec
config_location=/workspaces/the-bike-shed/sim-shed-config
serial_log="/build/sim_serial.log"
shed_log="/build/sim_shed.log"

socat -d -d -v PTY,link="$exterior_serial_dev",rawer,echo=0 PTY,link="$interior_serial_dev",rawer,echo=0 >> $serial_log 2>&1  &

SHED_TRACE=1 $shed $config_location &> $shed_log & shed_pid=$


tail -f /build/sim_*.log &









function f() { cat <<EOF
EmailAddress: tmp-from@testtest.test
EmailServer:  smtp://127.0.0.1:8025
EmailUserPass:  user:pass
DestinationEmailAddress: loggidddtmp-test.test
DebugSupervisorArg: /bin/sh
DeasdfasdbugSupervisorArg: -c
DebugSupervisorArg: /usr/bin/ping 127.0.0.1 | ts
EOF
}

f > /tmp/config_validator_1

lldb-server-10 gdbserver :5001 -- /build/config_validator.dbg.exec /tmp/config_validator_1








function f() { cat <<EOF
EmailAddress: tmp-from@testtest.test
EmailServer:  smtp://127.0.0.1:8025
EmailUserPass:  user:pass
DestinationEmailAddress: logging@tmp-test.test
EOF
}


/build/supervisor.dbg.exec <( f )

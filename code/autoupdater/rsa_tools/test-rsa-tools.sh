#!/bin/bash

if [ -f privatekey.bin ]; then
    echo "move your existing keys out of the way."
    exit 1
fi

( ./rsa_make_keys && echo "key making okay") || echo "key making NOT okay"
echo "The quick brown fox jumped over the lazy dog." >testmsg.txt
( ./rsa_sign testmsg.txt && echo "signing okay" ) || echo "signing NOT okay"
( ./rsa_verify testmsg.txt && echo "basic verifying okay" ) || echo "basic verifying NOT okay"
echo "The quick brown fox jumped over the lazy dog!" >testmsg.txt
( ./rsa_verify testmsg.txt 2>/dev/null && echo "tamper test NOT okay" ) || echo "tamper test okay"
echo "The quick brown fox jumped over the lazy dog." >testmsg.txt
( ./rsa_verify testmsg.txt && echo "reverify okay" ) || echo "reverify NOT okay"
rm -f testmsg.txt testmsg.txt.sig publickey.bin privatekey.bin


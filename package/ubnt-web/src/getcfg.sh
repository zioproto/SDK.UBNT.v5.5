#!/bin/sh
echo -e "Content-Type: text/plain;\r\n"
grep $QUERY_STRING /tmp/system.cfg

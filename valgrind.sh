#!/bin/sh
APP=$*
LOG=$1-valgrind.log
valgrind \
    --verbose \
    --log-file=$LOG \
    --leak-check=full \
    --run-libc-freeres=no \
    --suppressions=valgrind-suppressions \
    $APP

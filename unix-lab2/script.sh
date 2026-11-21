#!/bin/sh
set -e
exec 8>>/data/.lock
id=$(od -An -N8 -tx1 /dev/urandom | tr -d '[:space:]')
i=1
count=1
while :; do
    name=$(printf '%03d' "$i")
    flock -x 8
    while [ -e "/data/$name" ]; do
        i=$((i + 1))
        name=$(printf '%03d' "$i")
    done
    echo "$id №$count" > "/data/$name"
    flock -u 8
    count=$((count + 1))
    sleep 1
    rm "/data/$name"
    i=1
    sleep 1
done
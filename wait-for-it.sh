#!/bin/sh

set -e

HOSTPORT="$1"
shift

TIMEOUT=30
START=$(date +%s)

HOST=$(echo $HOSTPORT | cut -d':' -f1)
PORT=$(echo $HOSTPORT | cut -d':' -f2)

echo "Waiting for $HOST:$PORT..."

while ! nc -z "$HOST" "$PORT"; do
  NOW=$(date +%s)
  ELAPSED=$((NOW - START))
  if [ "$ELAPSED" -ge "$TIMEOUT" ]; then
    echo "Timeout reached ($TIMEOUT s), host not available."
    exit 1
  fi
  sleep 1
done

echo "$HOST:$PORT is available!"

exec "$@"

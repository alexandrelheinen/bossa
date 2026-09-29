#!/bin/bash

echo "=== Building bossa-daemon ==="
./scripts/build.sh

echo "=== Setting up local D1 database ==="
cd workers/bossa-worker
npx wrangler d1 execute bossa_d1 --local --file=./schema.sql || echo "Schema might already exist"

echo "=== Starting Cloudflare Worker ==="
npx wrangler dev --port 8787 > ../../worker.log 2>&1 &
WORKER_PID=$!

echo "Worker running on PID $WORKER_PID, waiting for it to start..."
sleep 5

cd ../..

echo "=== Starting bossa-daemon ==="
./build/final/bin/bossa-daemon --foreground --config config/ubuntu-fake.yml &
DAEMON_PID=$!

echo "Daemon running on PID $DAEMON_PID. Process runs in background."
echo "Use 'kill $WORKER_PID $DAEMON_PID' to stop."

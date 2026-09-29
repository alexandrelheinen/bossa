# BOSSA Full Stack Tutorial

This guide demonstrates how to configure and run the full BOSSA stack, including the simulated edge daemon generating fake telemetry and the remote Cloudflare Worker with a D1 database.

## 1. Running the Local Demo

The project includes fake drivers (`fake-bme280`, `fake-mpu6050`, `fake-tsl2561`) that simulate realistic peripheral data streams, which is useful for testing without a real Raspberry Pi.

### Prerequisites

Ensure you have installed the required native dependencies on your Ubuntu/WSL machine:

```bash
sudo apt-get update
sudo apt-get install -y libcurl4-openssl-dev libsystemd-dev libgpiod-dev
```

Additionally, you need Node.js and npm installed for the Cloudflare Worker stack.

### Quick Start

A convenient wrapper script handles the entire orchestration. It will:
1. Compile the edge daemon with the fake drivers.
2. Provision a local Cloudflare D1 database.
3. Start the Cloudflare Worker locally (listening on port 8787).
4. Start the `bossa-daemon` connecting to the local Worker.

Run the script from the repository root:

```bash
./run_all.sh
```

### Dashboard

Once the services are running, the edge node will begin buffering and syncing telemetry in batches to the local Worker.

Open a browser and navigate to:
**http://127.0.0.1:8787/**

You will see a real-time OEE / Dataviz dashboard plotting the incoming streams (temperature, humidity, pressure, accelerometer data, lux) using Chart.js.

To stop the background processes, follow the PID kill instructions printed at the end of the `run_all.sh` output.

## 2. Deploying to Cloudflare (Remote D1 & Worker)

Once you are ready to deploy the ingress Worker to Cloudflare for real devices, follow these steps.

### Step 1: Create a D1 Database

Use the Wrangler CLI to create a remote D1 database:

```bash
cd workers/bossa-worker
npx wrangler d1 create bossa_d1
```

Wrangler will output the `database_id`.

### Step 2: Update `wrangler.toml`

Edit `workers/bossa-worker/wrangler.toml` and replace `database_id` with the ID provided in the previous step:

```toml
name = "bossa-worker"
main = "src/index.ts"
compatibility_date = "2024-02-23"

[[d1_databases]]
binding = "DB"
database_name = "bossa_d1"
database_id = "YOUR-DATABASE-ID"
```

### Step 3: Apply the Schema

Run the migration on the remote D1 instance:

```bash
npx wrangler d1 execute bossa_d1 --remote --file=./schema.sql
```

### Step 4: Deploy the Worker

Deploy the Worker to Cloudflare:

```bash
npx wrangler deploy
```

This will give you a public URL (e.g., `https://bossa-worker.your-subdomain.workers.dev`).

### Step 5: Configure the Edge Device

On your real edge device (e.g., Raspberry Pi 5), update your `config.yaml` to point to the remote worker:

```yaml
server:
  url: https://bossa-worker.your-subdomain.workers.dev
```

Restart the `bossa-daemon` on the Pi, and your physical hardware will now synchronize with your global Cloudflare database! The same dashboard will be accessible at the Worker's root URL.

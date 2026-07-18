# BOSSA Technical Specification

This document is the authoritative reference for what BOSSA must do, how its
components interact, and which third-party libraries are adopted. It complements
the [coding guidelines](guidelines.md) (how to write code) with architectural and
behavioral requirements (what to build).

**Status:** Draft v0.1 — April 2026 baseline, updated July 2026.

---

## 1. Goals and Non-Goals

### Goals

1. Provide a **modular edge runtime** that samples sensors and controls
   actuators on ARM Linux with predictable timing.
2. Make it **easy to add drivers**, especially when a C++ library already exists
   for the hardware.
3. Offer a **declarative sync policy** so each channel independently defines
   what data is stored locally, what is uploaded, and at what frequency.
4. Ship a **server component** that ingests telemetry from edge nodes and
   persists it to a **SQL database** (PostgreSQL), aligned with the companion
   cloud data model.
5. Operate reliably **offline**: buffer on the edge, replay when connectivity
   returns.
6. Maintain **testability** through interface abstractions and GTest coverage.

### Non-Goals (v1)

- Replacing a full cloud platform (dashboards, alerting, user management).
- Supporting non-Linux embedded targets (bare-metal MCUs, FreeRTOS).
- Providing a graphical configuration UI.
- Real-time guarantees below 1 ms (soft real-time at tens of milliseconds is
  sufficient for IoT sensing).

---

## 2. System Context

```
                    ┌──────────────────────────────────┐
                    │  Companion cloud project (SQL)   │
                    │  dashboards, analytics, alerts   │
                    └───────────────┬──────────────────┘
                                    │ reads
                           ┌────────▼────────┐
                           │   PostgreSQL    │
                           │  TimescaleDB    │
                           └────────▲────────┘
                                    │ writes
                           ┌────────┴────────┐
                           │  bossa-server   │
                           │  REST ingress   │
                           └────────▲────────┘
                                    │ HTTPS (batched JSON)
              ┌─────────────────────┼─────────────────────┐
              │                     │                     │
     ┌────────┴────────┐   ┌───────┴────────┐   ┌───────┴────────┐
     │  bossa-daemon   │   │  bossa-daemon  │   │  bossa-daemon  │
     │  (edge node A)  │   │  (edge node B) │   │  (edge node C) │
     └────────┬────────┘   └───────┬────────┘   └───────┬────────┘
              │                    │                     │
         sensors /            sensors /             sensors /
         actuators             actuators              actuators
```

BOSSA owns the edge runtime and the telemetry ingress server. The companion
cloud project (private repository) owns the canonical SQL schema, analytics, and
operator-facing services. BOSSA's PostgreSQL tables are a **subset** that the
cloud project can extend with views, aggregates, and foreign keys.

---

## 3. Deployable Artifacts

| Binary | Runs on | Role |
|--------|---------|------|
| `bossa-daemon` | Edge device (Pi 5) | Driver hosting, sampling, local storage, upload |
| `bossa-server` | Server / cloud VM / same Pi | REST ingress, authentication, DB writes |

Both binaries link against shared static libraries (`bossa_core`, `bossa_io`,
`bossa_telemetry`, etc.) built from the same CMake project.

---

## 4. Library and Dependency Choices

### 4.1 Core (required)

| Concern | Library | Version | Rationale |
|---------|---------|---------|-----------|
| Build system | CMake | ≥ 3.16 | Already in use; cross-compile support |
| Unit testing | Google Test | ≥ 1.14 | Project standard; integrates with CTest |
| Configuration | yaml-cpp | ≥ 0.8 | Readable YAML for device and sync config |
| JSON | nlohmann/json | ≥ 3.11 | Header-only; driver params and API payloads |
| Logging | POSIX `syslog` | — | Embedded convention; no extra dependency |
| Threading | C++20 STL | — | `std::thread`, `std::mutex`, `std::atomic` |

### 4.2 Hardware I/O (edge)

| Bus | Library / API | Rationale |
|-----|---------------|-----------|
| GPIO | **libgpiod v2** (`libgpiod-dev`) | Modern character-device API; sysfs GPIO is deprecated |
| I2C | Linux `i2c-dev` (`/dev/i2c-*`) | Kernel interface; no wrapper library needed |
| SPI | Linux `spidev` (`/dev/spidev*`) | Kernel interface; no wrapper library needed |
| Serial | POSIX `termios` | Standard for UART devices |

All I/O is hidden behind virtual interfaces in `bossa::io` so unit tests use
mocks and hardware tests run on the Pi.

### 4.3 Storage

| Layer | Library | Rationale |
|-------|---------|-----------|
| Edge local buffer | **SQLite 3** (`libsqlite3-dev`) | Single-file, zero-config, survives power loss; ideal offline queue |
| Remote primary store | **Cloudflare D1** (SQLite) via Worker | Same Cloudflare platform as the companion cloud stack; no extra DB service |
| Remote fallback (optional) | PostgreSQL via libpqxx | Only if D1 limits or Freshy schema require it |

SQLite on the edge is a **transient buffer**, not the system of record. The
remote D1 database (behind a Worker) is authoritative for uploaded telemetry.

**Rejected alternatives:**

| Alternative | Reason rejected |
|-------------|-----------------|
| InfluxDB | Not SQL; incompatible with companion cloud schema |
| MongoDB | Document store; user requires relational SQL model |
| Standalone PostgreSQL as default | Multiplies services when Cloudflare D1 already covers SQL needs |
| LevelDB / RocksDB | Key-value; poor fit for time-series SQL queries |

### 4.4 Networking

| Concern | Library | Rationale |
|---------|---------|-----------|
| HTTP client (edge upload) | **libcurl** | Mature, supports TLS, retries, timeouts |
| HTTP server (ingress) | **Cloudflare Worker** | Hosts `POST /api/v1/telemetry`; writes D1 |
| TLS | OpenSSL (via curl); Worker edge TLS | System library on Debian/Raspberry Pi OS |
| MQTT bridge (optional, Phase 5) | **libmosquitto** | Standard IoT pub/sub for integrations that expect MQTT |

**Rejected alternatives:**

| Alternative | Reason rejected |
|-------------|-----------------|
| Boost.Beast | Heavy dependency for embedded targets |
| gRPC | Overkill for batched telemetry; larger binary |
| Custom socket code | Reinvents TLS, retry, and parsing |
| C++ cpp-httplib server as default | Prefer Worker on existing Cloudflare account |

### 4.5 Driver Plugins

| Concern | Mechanism | Rationale |
|---------|-----------|-----------|
| Dynamic loading | POSIX `dlopen` / `dlsym` | Standard on Linux; no plugin framework needed |
| Static registration | CMake object library + macro | Zero runtime cost for built-in drivers |

---

## 5. Namespace and Module Layout

```
bossa::
├── core::          Service, config loader, application lifecycle
├── io::            GpioController, I2cBus, SpiBus (virtual interfaces)
├── drivers::       Driver base class, registry, plugin loader
├── telemetry::     Sample, Channel, RingBuffer, Scheduler
├── storage::       LocalStore (SQLite), schema migrations
└── sync::          UploadPolicy, RetryQueue, HttpUploader (edge → Worker/D1)
```

Namespace mirrors directory structure per [guidelines](guidelines.md).

> **Note:** A C++ `bossa::server` / `bossa-server` binary is an optional fallback.
> The preferred remote path is a Cloudflare Worker writing to D1.

---

## 6. Configuration

### 6.1 Edge configuration (`/etc/bossa/config.yaml`)

```yaml
node:
  id: greenhouse-sensor-01
  api_key_file: /etc/bossa/api.key

server:
  url: https://telemetry.example.com
  upload_timeout_seconds: 30
  max_batch_size: 500

local_storage:
  path: /var/lib/bossa/telemetry.db
  max_size_megabytes: 256
  retention_hours: 72

channels:
  - id: ambient_temperature
    driver: bme280
    parameters:
      bus: /dev/i2c-1
      address: 0x76
    sample_rate_hz: 1.0
    sync:
      destinations: [local, remote]
      remote_interval_seconds: 60
      priority: normal
      mode: batch

  - id: soil_moisture
    driver: ads1115
    parameters:
      bus: /dev/i2c-1
      address: 0x48
      channel: 0
    sample_rate_hz: 0.1
    sync:
      destinations: [remote]
      remote_interval_seconds: 300
      priority: low
      mode: batch

  - id: pump_relay
    driver: gpio_output
    parameters:
      chip: gpiochip4
      line: 17
    sample_rate_hz: 0          # event-driven actuator, no periodic read
    sync:
      destinations: []        # actuators are command-driven, not sampled
```

### 6.2 Server configuration (`/etc/bossa/server.yaml`)

```yaml
listen:
  host: 0.0.0.0
  port: 8443
  tls_cert: /etc/bossa/server.crt
  tls_key: /etc/bossa/server.key

database:
  connection_string: "postgresql://bossa:secret@localhost:5432/bossa"
  pool_size: 10

auth:
  api_keys_table: edge_nodes   # or flat file for v1 simplicity
```

### 6.3 Validation rules

- `channel.id` must be unique within a node.
- `sample_rate_hz` must be ≥ 0; 0 means event-driven (no periodic poll).
- `sync.destinations` is a subset of `[local, remote]`.
- `remote_interval_seconds` must be ≥ `1 / sample_rate_hz` when both are set.
- `priority` is one of `critical`, `normal`, `low`.

---

## 7. Driver Interface

### 7.1 Abstract base class

```cpp
namespace bossa::drivers {

/**
 * @brief Hardware driver interface.
 *
 * Every sensor or actuator adapter implements this class. Drivers must not
 * allocate memory in read() or write() after configure() completes.
 */
class Driver {
 public:
  virtual ~Driver() = default;

  /** @brief Unique driver type name, e.g. "bme280". */
  virtual std::string name() const = 0;

  /**
   * @brief One-time setup from YAML parameters block.
   * @throws std::runtime_error on invalid configuration (not in hot path).
   */
  virtual void configure(const nlohmann::json& parameters) = 0;

  /**
   * @brief Read current samples from hardware.
   * @return One Sample per logical channel this driver exposes.
   */
  virtual std::vector<telemetry::Sample> read() = 0;

  /**
   * @brief Execute an actuator command.
   * @param command JSON payload, driver-specific schema.
   */
  virtual void write(const nlohmann::json& command) = 0;
};

}  // namespace bossa::drivers
```

### 7.2 Registration

**Static (built-in drivers):**

```cpp
// drivers/bme280/bme280_driver.cpp
namespace bossa::drivers {
class Bme280Driver : public Driver { /* ... */ };
BOSSA_REGISTER_DRIVER(Bme280Driver, "bme280");
}
```

**Dynamic (third-party or out-of-tree):**

```yaml
plugins:
  - path: /opt/bossa/drivers/libbossa_driver_sht31.so
    drivers: [sht31]
```

The plugin exports a C-linkage factory:

```cpp
extern "C" bossa::drivers::Driver* bossa_create_driver(const char* name);
extern "C" void bossa_destroy_driver(bossa::drivers::Driver* driver);
```

### 7.3 Wrapping an existing C++ library

Pattern for integrating a vendor C++ driver (e.g. Adafruit BME280 library):

1. Create `drivers/bme280/bme280_driver.cpp` — thin adapter implementing
   `bossa::drivers::Driver`.
2. Map library types to `telemetry::Sample` (value, unit, timestamp).
3. Open the bus (I2C) through `bossa::io::I2cBus`, inject via constructor
   for testability.
4. Add a GTest with a mock `I2cBus` that returns canned register values.
5. Optionally ship as `.so` if license or build isolation requires it.

---

## 8. Telemetry Model

### 8.1 Sample

```cpp
namespace bossa::telemetry {

struct Sample {
  std::string channel_id;
  double value;
  std::chrono::system_clock::time_point timestamp;
  std::string unit;           // physical quantity: "celsius", "percent", "pascal"
  SampleQuality quality;      // good, uncertain, bad
};

enum class SampleQuality { kGood, kUncertain, kBad };

}  // namespace bossa::telemetry
```

Physical variable naming follows the `who_what` convention from
[guidelines](guidelines.md). Units describe the quantity, not the SI symbol.

### 8.2 Ring buffer

- Fixed capacity, allocated at startup from `max_batch_size` config.
- Lock-free single-producer / single-consumer where possible; `std::mutex`
  otherwise.
- **No heap allocation** in `enqueue()` or `dequeue()`.
- Overflow policy: drop `low` priority samples first; log `LOG_WARNING`.

### 8.3 Scheduler

- One `std::jthread` per priority level (`critical`, `normal`, `low`).
- Each channel has a `next_deadline` computed from `sample_rate_hz`.
- Channels are sorted by deadline; the thread sleeps until the nearest deadline
  using `std::condition_variable` with `wait_until`.
- Missed deadlines (overrun): log `LOG_WARNING`, skip to next deadline (no
  catch-up storm).

---

## 9. Sync Engine

### 9.1 Modes

| Mode | Behavior |
|------|----------|
| `batch` | Accumulate samples; flush when `remote_interval_seconds` elapses or buffer is full |
| `realtime` | Upload each sample immediately after enqueue (for `critical` channels) |
| `on_change` | Upload only when `abs(value - last_uploaded) > threshold` |

### 9.2 Upload protocol

Edge → server: `POST /api/v1/telemetry`

```json
{
  "node_id": "greenhouse-sensor-01",
  "sent_at": "2026-07-10T13:45:00Z",
  "samples": [
    {
      "channel_id": "ambient_temperature",
      "timestamp": "2026-07-10T13:44:59Z",
      "value": 23.7,
      "unit": "celsius",
      "quality": "good"
    }
  ]
}
```

Server response:

| Code | Meaning |
|------|---------|
| `202 Accepted` | Rows persisted; edge may trim buffer |
| `400 Bad Request` | Schema violation; edge logs and drops batch |
| `401 Unauthorized` | Invalid API key; edge retries after backoff |
| `503 Service Unavailable` | Server overloaded; edge retains batch in SQLite |

### 9.3 Offline behavior

1. Upload fails → samples written to SQLite `pending_uploads` table.
2. Background retry thread polls connectivity every 30 seconds.
3. On success, delete acknowledged rows from SQLite.
4. If `max_size_megabytes` exceeded, delete oldest `low` priority rows first.

### 9.4 Retry policy

- Exponential backoff: 5 s → 10 s → 30 s → 60 s → 300 s (cap).
- Jitter: ±10 % to avoid thundering herd.
- After 24 h of failure, log `LOG_CRIT` once per hour.

---

## 10. Server Component

### 10.1 REST API

| Method | Path | Description |
|--------|------|-------------|
| `POST` | `/api/v1/telemetry` | Ingest batched samples |
| `GET` | `/api/v1/health` | Liveness (`{"status":"ok"}`) |
| `GET` | `/api/v1/health/ready` | Readiness (DB connection alive) |

Authentication: `Authorization: Bearer <api_key>` header.

### 10.2 PostgreSQL schema (v1)

Designed for compatibility with TimescaleDB hypertables.

```sql
CREATE TABLE edge_nodes (
    node_id       TEXT PRIMARY KEY,
    api_key_hash  TEXT NOT NULL,
    created_at    TIMESTAMPTZ NOT NULL DEFAULT now(),
    last_seen_at  TIMESTAMPTZ
);

CREATE TABLE telemetry_points (
    node_id       TEXT NOT NULL REFERENCES edge_nodes(node_id),
    channel_id    TEXT NOT NULL,
    timestamp     TIMESTAMPTZ NOT NULL,
    value         DOUBLE PRECISION NOT NULL,
    unit          TEXT NOT NULL,
    quality       TEXT NOT NULL DEFAULT 'good',
    received_at   TIMESTAMPTZ NOT NULL DEFAULT now(),
    PRIMARY KEY (node_id, channel_id, timestamp)
);

-- Optional: convert to hypertable when TimescaleDB is available
-- SELECT create_hypertable('telemetry_points', 'timestamp');
```

Indexes:

```sql
CREATE INDEX idx_telemetry_node_time
    ON telemetry_points (node_id, timestamp DESC);
CREATE INDEX idx_telemetry_channel_time
    ON telemetry_points (channel_id, timestamp DESC);
```

The companion cloud project may add tables (alerts, devices, users) that
reference `edge_nodes.node_id` via foreign keys.

### 10.3 Write path

1. Validate JSON schema and API key.
2. Begin transaction.
3. Bulk `INSERT ... ON CONFLICT DO NOTHING` (idempotent replay).
4. Update `edge_nodes.last_seen_at`.
5. Commit.

Batch inserts use libpqxx `pqxx::work` with prepared statements.

---

## 11. Security

| Concern | Approach |
|---------|----------|
| Transport | TLS 1.2+ on all edge-to-server communication |
| API keys | Stored hashed (bcrypt) on server; plain text only in edge file with `0600` permissions |
| Local SQLite | File permissions `0640`, owned by `bossa` user |
| Input validation | JSON schema validation on server; reject oversize payloads (> 1 MB) |
| Privilege separation | `bossa-daemon` runs as non-root `bossa` user; GPIO access via group membership (`gpio`) |

---

## 12. Real-Time and Embedded Constraints

Per [guidelines](guidelines.md):

- **No exceptions** in `read()`, `write()`, scheduler loop, or signal handlers.
- **No heap allocation** in hot paths (pre-allocate at `configure()` time).
- **Signal handlers** use `volatile sig_atomic_t` flags only.
- **Logging** via `syslog()`, not `std::cout`.
- System call return values are always checked.

---

## 13. Testing Requirements

| Layer | Strategy |
|-------|----------|
| Drivers | Mock `bossa::io` interfaces; feed canned bus data |
| Scheduler | Deterministic time injection; verify deadline ordering |
| Ring buffer | Boundary tests (full, empty, overflow policy) |
| Sync engine | Simulated HTTP failures; verify SQLite fallback |
| Server | In-process httplib test client; test PostgreSQL via Docker in CI |
| Integration | Deploy to Pi 5; read real sensor; verify row in PostgreSQL |

Minimum coverage target: **90 %** line coverage on `bossa_core`, `bossa_telemetry`,
`bossa_sync`, and `bossa_server`.

---

## 14. Build and Packaging

### CMake targets (target layout)

```cmake
add_library(bossa_core STATIC ...)
add_library(bossa_io STATIC ...)
add_library(bossa_drivers STATIC ...)
add_library(bossa_telemetry STATIC ...)
add_library(bossa_storage STATIC ...)
add_library(bossa_sync STATIC ...)
add_library(bossa_server_lib STATIC ...)

add_executable(bossa-daemon src/daemon_main.cpp)
add_executable(bossa-server server/main.cpp)

target_link_libraries(bossa-daemon PRIVATE bossa_core bossa_io ...)
target_link_libraries(bossa-server PRIVATE bossa_server_lib ...)
```

### Debian packages (future)

- `bossa-daemon` — edge runtime + systemd unit + example config
- `bossa-server` — server binary + systemd unit + SQL migration scripts
- `libbossa-dev` — headers for out-of-tree driver development

---

## 15. Versioning and Compatibility

- API version in URL path: `/api/v1/...`
- Configuration schema version field: `config_version: 1`
- Breaking config changes increment `config_version` and are documented in
  CHANGELOG.
- Driver plugin ABI: major BOSSA version must match (e.g. `libbossa_driver.so`
  built for BOSSA 0.2 only loads on 0.2.x).

---

## 16. Open Decisions

| Topic | Current lean | Decision needed by |
|-------|-------------|-------------------|
| Remote store platform | **Cloudflare Worker + D1** (not C++ PostgreSQL) | Decided for Phase 4 |
| Config hot-reload (`SIGHUP`) | Yes, reload channels without restart | Phase 3 (implementing) |
| MQTT bridge priority | Phase 5, optional | Phase 5 planning |
| Multi-tenancy on ingress | Single-tenant v1; add `tenant_id` later if needed | Before production deploy |
| D1 vs Durable Object SQLite | D1 for shared telemetry table; DO only if per-node isolation needed | Phase 4 design |

---

## References

- [BOSSA Roadmap](roadmap.md)
- [Coding Guidelines](guidelines.md)
- libgpiod: https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/
- libpqxx: https://github.com/jtv/libpqxx
- TimescaleDB: https://www.timescale.com/
- yaml-cpp: https://github.com/jbeder/yaml-cpp
- cpp-httplib: https://github.com/yhirose/cpp-httplib

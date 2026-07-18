# BOSSA

<img src="docs/images/bossa.svg" alt="BOSSA Logo" width="120" align="left">

**Base Operating System for Sensors and Actuators**

BOSSA is a modular C++20 framework for IoT edge devices that publishes telemetry
to SQLite: a local offline buffer on the device, and a remote Cloudflare D1
database via a BOSSA Worker. It targets ARM Linux (Raspberry Pi 5) and is
designed so that new hardware drivers—especially those already available in
C++—can be added with minimal glue code.

The edge runtime samples sensors and actuators on configurable schedules. A
declarative sync policy selects which channels are stored locally, which are
pushed online, and at what frequency. A Cloudflare Worker ingests telemetry
from one or many edge nodes and persists it to D1 (SQLite).

## Documentation

| Document | Purpose |
|----------|---------|
| [Contributing](CONTRIBUTING.md) | Development constitution: SDD, V-cycle, quality gates, agent policy |
| [Specification](docs/specification.md) | Requirements, APIs, data model, library choices |
| [Roadmap](docs/roadmap.md) | Phased delivery plan and acceptance criteria |
| [Coding guidelines](docs/guidelines.md) | Authoritative C++ and embedded conventions |

## Architecture Overview

BOSSA’s edge binary shares core libraries with the upload path; remote ingress
is a Cloudflare Worker writing D1 (not a second SQL engine):

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           Edge device (Pi 5)                            │
│                                                                         │
│  ┌──────────┐   ┌─────────────┐   ┌──────────────┐   ┌──────────────┐  │
│  │  Config  │──▶│   Driver    │──▶│  Telemetry   │──▶│  Sync engine │  │
│  │  (YAML)  │   │  registry   │   │  scheduler   │   │  + buffer    │  │
│  └──────────┘   └──────┬──────┘   └──────────────┘   └──────┬───────┘  │
│                        │                                      │         │
│               ┌────────┴────────┐                    ┌─────────┴───────┐ │
│               │  bossa::io      │                    │ SQLite (local)  │ │
│               │  GPIO I2C SPI   │                    │ offline cache   │ │
│               └────────┬────────┘                    └─────────┬───────┘ │
│                        │                                      │         │
│               ┌────────┴────────┐                             │ HTTPS    │
│               │ Driver plugins  │                             │ batch    │
│               │ (.so / static)  │                             ▼         │
│               └─────────────────┘                    ┌─────────────────┐  │
│                                                    │ Cloudflare      │  │
│  bossa-daemon (systemd)                            │ Worker + D1     │  │
└────────────────────────────────────────────────────┴────────┬────────┴──┘
                                                              │
                                                     ┌────────▼────────┐
                                                     │  Cloudflare D1  │
                                                     │ (SQLite remote) │
                                                     └─────────────────┘
```

### Edge runtime (`bossa-daemon`)

Long-running systemd service built on `bossa::Service`. Responsibilities:

- Load YAML configuration (devices, channels, sync policies).
- Instantiate drivers through a registry (built-in or dynamically loaded `.so`).
- Run a priority-aware scheduler that calls each driver at its configured
  sample rate without heap allocations in the hot path.
- Buffer samples in a pre-allocated ring buffer and flush to local SQLite when
  the network is unavailable.
- Push batched telemetry to a remote HTTPS endpoint (`server.url`) via
  `POST /api/v1/telemetry`.

### Remote ingress (Cloudflare Worker + D1)

Phase 4 path: a **BOSSA** Cloudflare Worker accepts `POST /api/v1/telemetry` and
writes to a BOSSA-owned **D1** (SQLite) database. The stack is SQLite end to
end: edge local file + remote D1.

BOSSA owns the edge-facing upload contract and the D1 schema subset used for
telemetry; companion projects may read the same D1 data or mirror it.

## Modular Driver Model

Every hardware interface implements `bossa::drivers::Driver`:

```cpp
namespace bossa::drivers {

struct Sample {
  std::string channel_id;
  double value;
  std::chrono::system_clock::time_point timestamp;
  std::string unit;  // physical quantity, e.g. "celsius", not "C"
};

class Driver {
 public:
  virtual ~Driver() = default;
  virtual std::string name() const = 0;
  virtual void configure(const nlohmann::json& parameters) = 0;
  virtual std::vector<Sample> read() = 0;
  virtual void write(const nlohmann::json& command) = 0;
};

}  // namespace bossa::drivers
```

Drivers are registered in one of two ways:

1. **Static** — linked at build time via `BOSSA_REGISTER_DRIVER(MyDriver)`.
2. **Dynamic** — compiled as a shared library (`libbossa_driver_*.so`) and
   loaded with `dlopen` when listed in the configuration file.

Wrapping an existing C++ driver library typically requires only a thin adapter
class that translates its API into `read()` / `write()` and maps its outputs to
`Sample` values.

## Sync Policy

Each channel declares independent sampling and synchronization behavior in
configuration (see [specification](docs/specification.md) for the full schema):

```yaml
channels:
  - id: ambient_temperature
    driver: bme280
    device: /dev/i2c-1
    address: 0x76
    sample_rate_hz: 1.0
    sync:
      destinations: [local, remote]
      remote_interval_seconds: 60
      priority: normal
      mode: batch
```

| Field | Effect |
|-------|--------|
| `sample_rate_hz` | How often the driver is polled on the edge |
| `sync.destinations` | `local` (edge SQLite), `remote` (Worker → D1) |
| `sync.remote_interval_seconds` | Minimum interval between uploads for this channel |
| `sync.priority` | `critical` / `normal` / `low` — scheduler ordering under load |
| `sync.mode` | `batch`, `realtime`, or `on_change` |

## Technology Stack

| Layer | Choice | Notes |
|-------|--------|-------|
| Language | C++20 | RAII, `std::chrono`, `std::optional`, concepts |
| Build | CMake 3.16+ | Native x86_64 + ARM64 cross-compile |
| Edge GPIO | [libgpiod](https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/) v2 | Character-device API, no sysfs |
| Edge I2C / SPI | Linux `i2c-dev` / `spidev` | Kernel interfaces, zero extra dependencies |
| Configuration | [yaml-cpp](https://github.com/jbeder/yaml-cpp) | Human-readable device and sync config |
| JSON | [nlohmann/json](https://github.com/nlohmann/json) | Driver parameters, API payloads |
| Local storage | [SQLite](https://www.sqlite.org/) | Offline buffer on the edge |
| Remote database | [Cloudflare D1](https://developers.cloudflare.com/d1/) (SQLite) | Authoritative remote store |
| HTTP client | [libcurl](https://curl.se/libcurl/) | Edge → Worker telemetry upload |
| HTTP ingress | Cloudflare Worker | `POST /api/v1/telemetry` → D1 |
| MQTT (optional, Phase 5) | [Eclipse Mosquitto](https://mosquitto.org/) (`libmosquitto`) | Pub/sub bridge; parked for now |
| Plugin loading | POSIX `dlfcn` | Dynamic driver `.so` modules |
| Testing | [Google Test](https://github.com/google/googletest) | Unit and integration tests |
| Logging | `syslog` | Daemon logging per embedded conventions |
| Service manager | systemd | `Type=notify` once readiness signaling is implemented |

## Repository Layout (target)

```
bossa/
├── include/bossa/
│   ├── core/          # Service, scheduler, config loader
│   ├── io/            # GPIO, I2C, SPI abstractions
│   ├── drivers/       # Driver interface and registry
│   ├── telemetry/     # Sample types, ring buffer
│   ├── storage/       # Edge SQLite local store
│   ├── sync/          # Upload policy and HTTP uploader
│   └── telemetry/     # Sample, ring buffer, scheduler
├── src/               # Implementations
├── drivers/           # Built-in and example driver adapters
├── workers/           # (Phase 4) Cloudflare Worker + D1 migrations
├── tests/             # GTest suites (mirrors include/ structure)
├── config/            # Example YAML, systemd units
├── scripts/           # Build, format, deploy
└── docs/              # Specification, roadmap, guidelines
```

## Data Flow

```mermaid
sequenceDiagram
    participant HW as Hardware
    participant DR as Driver
    participant SC as Scheduler
    participant BUF as Ring buffer
    participant LS as Edge SQLite
    participant WK as BOSSA Worker
    participant D1 as Cloudflare D1

    loop every sample_rate_hz
        SC->>DR: read()
        DR->>HW: bus transaction
        HW-->>DR: raw data
        DR-->>SC: Sample[]
        SC->>BUF: enqueue
    end

    alt local destination
        BUF->>LS: flush batch
    end

    alt remote destination (interval elapsed)
        BUF->>WK: POST /api/v1/telemetry
        WK->>D1: INSERT rows
        WK-->>BUF: 202 Accepted
        BUF->>BUF: acknowledge / trim
    end

    Note over LS,WK: On network failure, edge retains samples in local SQLite until connectivity returns.
```

## Current Implementation Status

| Component | Status |
|-----------|--------|
| `bossa::core::Service` daemon base | Implemented |
| `bossa::io` GPIO / I2C abstractions | Implemented (libgpiod + Linux i2c-dev) |
| `bossa::drivers` registry + BME280 | Implemented (mock-tested; Pi smoke paused) |
| `bossa-daemon` edge binary | Implemented |
| YAML configuration loader | Implemented (channels + sync) |
| Telemetry scheduler + ring buffer | Implemented (Phase 3) |
| Edge SQLite local store | Implemented (Phase 3) |
| Upload policy + HTTP uploader | Implemented (Phase 3; mockable client) |
| `TelemetryRuntime` + `sim` driver | Implemented (Phase 3 pipeline) |
| Unit tests (GTest) | Implemented — `./scripts/test/unit.sh` |
| BOSSA Worker + D1 ingress | Planned (Phase 4) |
| Dynamic driver plugins | Planned (Phase 5) |
| MQTT bridge | Optional / parked (Phase 5) |

See the [roadmap](docs/roadmap.md) for the delivery sequence.

## Quick Start

```bash
./scripts/setup.sh          # install build dependencies
./scripts/build.sh            # native build (x86_64)
./scripts/build.sh -t toolchain-arm64.cmake   # cross-compile for Pi 5
./scripts/sync.sh -t pi@raspberry.local       # deploy to device
```

Full build, test, format, deploy, and contribution workflow are in
[CONTRIBUTING.md](CONTRIBUTING.md).

## License

MIT License. See [LICENSE](LICENSE).

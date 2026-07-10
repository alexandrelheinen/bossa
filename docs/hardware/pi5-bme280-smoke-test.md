# Raspberry Pi 5 — BME280 hardware smoke test

This guide documents the **Phase 2 acceptance smoke test**: read a real BME280
over I2C on a Raspberry Pi 5 and log temperature (and humidity) approximately
every second via syslog.

Until Phase 3 wires YAML channels into the daemon scheduler, use the
`bossa-bme280-smoke` helper binary (same `Bme280Driver` stack as production).
`bossa-daemon --foreground` remains the deployment sanity check (config load +
service loop).

**Traceability:** [roadmap.md](../roadmap.md) Phase 2 acceptance criteria.

---

## What you need

| Item | Notes |
|------|-------|
| Raspberry Pi 5 | Raspberry Pi OS **Bookworm 64-bit** |
| BME280 breakout | I2C, 3.3 V logic (most modules include a regulator) |
| Jumper wires | Female–female or Qwiic/Stemma QT cable |
| BOSSA build | Cross-compiled on dev host or native on the Pi |

---

## Sensor reference

### Datasheet (authoritative)

- [Bosch Sensortec BME280 datasheet (BST-BME280-DS002)](https://www.bosch-sensortec.com/media/boschsensorblock/docs/datasheets/BST-BME280-DS002.pdf)

### Where to buy (common breakouts)

These boards expose I2C on 3.3 V and work with Pi 5:

| Vendor | Product | Link |
|--------|---------|------|
| Adafruit | BME280 I2C/SPI Temperature, Pressure, Humidity Sensor | [adafruit.com/product/2652](https://www.adafruit.com/product/2652) |
| SparkFun | Atmospheric Sensor Breakout — BME280 (Qwiic) | [sparkfun.com/products/15440](https://www.sparkfun.com/products/15440) |
| Pimoroni | BME280 Breakout | [shop.pimoroni.com/products/bme280](https://shop.pimoroni.com/products/bme280) |
| Amazon / AliExpress | Search “BME280 I2C module” | Verify **3.3 V** I/O and I2C (not SPI-only) |

### Tutorials

| Topic | Link |
|-------|------|
| Adafruit BME280 Arduino guide (register map applies) | [learn.adafruit.com/bme280](https://learn.adafruit.com/adafruit-bme280-humidity-barometric-pressure-temperature-sensor/overview) |
| Raspberry Pi — enable I2C | [raspberrypi.com/documentation/computers/configuration.html#interfacing-options](https://www.raspberrypi.com/documentation/computers/configuration.html#interfacing-options) |
| `i2cdetect` quick scan | [linuxhint.com/raspberry_pi_i2c](https://linuxhint.com/raspberry_pi_i2c_tutorial/) |

---

## Wiring (Pi 5 ↔ BME280)

Pi 5 exposes I2C1 on the 40-pin header as **GPIO2 (SDA)** and **GPIO3 (SCL)**.
`/dev/i2c-1` maps to this bus.

```
Pi 5 pin   Signal     BME280 breakout
--------   ------     ----------------
  3.3 V    3V3   -->  VIN / VCC
 GND (6)   GND   -->  GND
 Pin 3     SDA   -->  SDA / SDIO
 Pin 5     SCL   -->  SCL / SCK
```

Typical I2C addresses:

| ADDR pin | 7-bit address |
|----------|---------------|
| GND | `0x76` (default on many breakouts) |
| VCC | `0x77` |

> Do **not** connect 5 V to a 3.3 V-only breakout.

---

## Enable I2C on the Pi

```bash
sudo raspi-config
# Interface Options → I2C → Enable
sudo reboot
```

After reboot, confirm the bus and sensor:

```bash
sudo apt-get install -y i2c-tools
i2cdetect -y 1
```

You should see `76` or `77` in the grid. If the cell is `--`, check wiring and
address pin.

---

## Deploy BOSSA to the Pi

On your development host:

```bash
./scripts/build.sh -t toolchain-arm64.cmake
./scripts/sync.sh -t pi@raspberry.local
```

On the Pi, binaries land in `/opt/bossa/bin/`:

- `bossa-daemon` — edge runtime (config + heartbeat loop today)
- `bossa-bme280-smoke` — Phase 2 BME280 poll harness (1 Hz syslog)

---

## Run the smoke test

### Option A — launch script (recommended)

From your **development host** (after `./scripts/sync.sh`):

```bash
./scripts/test/pi5_bme280_smoke.sh --target pi@raspberry.local
```

On the **Pi** (repo checkout or copy `scripts/test/pi5_bme280_smoke.sh`):

```bash
./scripts/test/pi5_bme280_smoke.sh --bus /dev/i2c-1 --address 0x76
```

The script checks I2C, runs `bossa-bme280-smoke --foreground` for 10 seconds,
and prints recent syslog lines.

### Option B — manual

```bash
# Poll temperature + humidity every 1 s (foreground, logs to syslog + stderr)
sudo /opt/bossa/bin/bossa-bme280-smoke --foreground \
  --bus /dev/i2c-1 --address 0x76

# In another terminal, watch syslog:
sudo journalctl -t bossa-bme280-smoke -f
```

Expected syslog lines (values vary with environment):

```
bossa-bme280-smoke: ambient_temperature=23.45 celsius
bossa-bme280-smoke: ambient_humidity=48.12 percent
```

Use `--address 0x77` if `i2cdetect` showed `77`.

### Daemon sanity check (optional)

Confirms config load and foreground service loop (heartbeat every 5 s):

```bash
sudo /opt/bossa/bin/bossa-daemon --foreground --config /etc/bossa/config.yaml
```

Channel-driven BME280 polling inside `bossa-daemon` arrives in **Phase 3**.

---

## Troubleshooting

| Symptom | Check |
|---------|-------|
| `i2cdetect` empty | Wiring, 3.3 V power, I2C enabled in `raspi-config` |
| `failed to open I2C bus` | User in `i2c` group or run with `sudo`; device path `/dev/i2c-1` |
| `unexpected chip id` | Wrong device on bus; not a BME280 |
| `failed to read chip id` | Loose SDA/SCL, wrong address (`0x76` vs `0x77`) |
| No syslog output | Use `--foreground`; run `journalctl -t bossa-bme280-smoke -f` |

---

## Related documents

- [Phase 2 design](../phase-2-io-driver.md)
- [Contributing — Pi 5 hardware validation](../../CONTRIBUTING.md#raspberry-pi-5-hardware-validation)
- [Unit test script](../../scripts/test/unit.sh) (GTest on dev host, not CI)

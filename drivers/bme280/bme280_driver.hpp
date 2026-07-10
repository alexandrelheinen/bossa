/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "bossa/drivers/driver.hpp"
#include "bossa/io/i2c_bus.hpp"

namespace bossa::drivers {

/**
 * @brief BME280 temperature and humidity driver over I2C.
 */
class Bme280Driver : public Driver {
  public:
    explicit Bme280Driver(std::unique_ptr<io::I2cBus> bus = nullptr);

    std::string_view name() const override;
    void configure(const nlohmann::json &parameters) override;
    ReadResult read() override;
    void write(const nlohmann::json &command) override;

  private:
    struct Calibration {
        std::uint16_t dig_t1{0};
        std::int16_t dig_t2{0};
        std::int16_t dig_t3{0};
        std::uint8_t dig_h1{0};
        std::int16_t dig_h2{0};
        std::uint8_t dig_h3{0};
        std::int16_t dig_h4{0};
        std::int16_t dig_h5{0};
        std::int8_t dig_h6{0};
    };

    bool load_calibration();
    bool trigger_measurement();
    bool read_raw(std::int32_t *raw_temperature, std::int32_t *raw_humidity);
    float compensate_temperature(std::int32_t raw_temperature,
                                 std::int32_t *fine_temperature) const;
    float compensate_humidity(std::int32_t raw_humidity,
                              std::int32_t fine_temperature) const;

    std::unique_ptr<io::I2cBus> bus_;
    bool owns_bus_{false};
    std::uint8_t address_{0x76};
    Calibration calibration_{};
    bool configured_{false};

    std::string temperature_channel_id_{"ambient_temperature"};
    std::string humidity_channel_id_{"ambient_humidity"};
    static constexpr std::string_view kCelsiusUnit{"celsius"};
    static constexpr std::string_view kPercentUnit{"percent"};
};

} // namespace bossa::drivers

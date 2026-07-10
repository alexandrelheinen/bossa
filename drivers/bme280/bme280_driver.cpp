/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include "bme280_driver.hpp"

#include <chrono>
#include <cstring>
#include <stdexcept>

#include "bossa/drivers/registry.hpp"
#include "bossa/io/linux_i2c_bus.hpp"

namespace bossa::drivers {

namespace {

constexpr std::uint8_t kChipIdRegister = 0xD0;
constexpr std::uint8_t kExpectedChipId = 0x60;
constexpr std::uint8_t kCalibStartRegister = 0x88;
constexpr std::uint8_t kHumidityCalibStartRegister = 0xE1;
constexpr std::uint8_t kCtrlHumidityRegister = 0xF2;
constexpr std::uint8_t kCtrlMeasureRegister = 0xF4;
constexpr std::uint8_t kDataStartRegister = 0xF7;

std::uint16_t read_uint16_le(const std::uint8_t *buffer) {
    return static_cast<std::uint16_t>(buffer[0]) |
           (static_cast<std::uint16_t>(buffer[1]) << 8);
}

std::int16_t read_int16_le(const std::uint8_t *buffer) {
    return static_cast<std::int16_t>(read_uint16_le(buffer));
}

} // namespace

Bme280Driver::Bme280Driver(std::unique_ptr<io::I2cBus> bus)
    : bus_(std::move(bus)) {}

std::string_view Bme280Driver::name() const { return "bme280"; }

void Bme280Driver::configure(const nlohmann::json &parameters) {
    if (!parameters.contains("bus") || !parameters["bus"].is_string()) {
        throw std::runtime_error("bme280: missing string parameter 'bus'");
    }
    if (!parameters.contains("address")) {
        throw std::runtime_error("bme280: missing parameter 'address'");
    }

    const std::string bus_path = parameters["bus"].get<std::string>();
    if (parameters["address"].is_string()) {
        const std::string address_text = parameters["address"].get<std::string>();
        address_ = static_cast<std::uint8_t>(std::stoul(address_text, nullptr, 0));
    } else {
        address_ = static_cast<std::uint8_t>(parameters["address"].get<int>());
    }

    if (parameters.contains("temperature_channel_id")) {
        temperature_channel_id_ =
            parameters["temperature_channel_id"].get<std::string>();
    }
    if (parameters.contains("humidity_channel_id")) {
        humidity_channel_id_ =
            parameters["humidity_channel_id"].get<std::string>();
    }

    if (!bus_) {
        bus_ = std::make_unique<io::LinuxI2cBus>();
        owns_bus_ = true;
    }

    if (!bus_->open(bus_path)) {
        throw std::runtime_error("bme280: failed to open I2C bus: " + bus_path);
    }

    const std::array<std::uint8_t, 1> chip_id_register{kChipIdRegister};
    std::array<std::uint8_t, 1> chip_id{};
    if (!bus_->write_read(address_, chip_id_register, chip_id)) {
        throw std::runtime_error("bme280: failed to read chip id");
    }
    if (chip_id[0] != kExpectedChipId) {
        throw std::runtime_error("bme280: unexpected chip id");
    }

    if (!load_calibration()) {
        throw std::runtime_error("bme280: failed to load calibration data");
    }

    configured_ = true;
}

ReadResult Bme280Driver::read() {
    ReadResult result;
    if (!configured_) {
        return result;
    }

    if (!trigger_measurement()) {
        return result;
    }

    std::int32_t raw_temperature = 0;
    std::int32_t raw_humidity = 0;
    if (!read_raw(&raw_temperature, &raw_humidity)) {
        return result;
    }

    std::int32_t fine_temperature = 0;
    const float temperature_celsius =
        compensate_temperature(raw_temperature, &fine_temperature);
    const float humidity_percent =
        compensate_humidity(raw_humidity, fine_temperature);

    const auto timestamp = std::chrono::system_clock::now();

    result.samples[0].channel_id = temperature_channel_id_;
    result.samples[0].value = static_cast<double>(temperature_celsius);
    result.samples[0].timestamp = timestamp;
    result.samples[0].unit = kCelsiusUnit;
    result.samples[0].quality = telemetry::SampleQuality::kGood;

    result.samples[1].channel_id = humidity_channel_id_;
    result.samples[1].value = static_cast<double>(humidity_percent);
    result.samples[1].timestamp = timestamp;
    result.samples[1].unit = kPercentUnit;
    result.samples[1].quality = telemetry::SampleQuality::kGood;

    result.sample_count = 2;
    return result;
}

void Bme280Driver::write(const nlohmann::json &) {
    throw std::runtime_error("bme280: write not supported");
}

bool Bme280Driver::load_calibration() {
    const std::array<std::uint8_t, 1> register_address{kCalibStartRegister};
    std::array<std::uint8_t, 26> buffer{};
    if (!bus_->write_read(address_, register_address, buffer)) {
        return false;
    }

    calibration_.dig_t1 = read_uint16_le(&buffer[0]);
    calibration_.dig_t2 = read_int16_le(&buffer[2]);
    calibration_.dig_t3 = read_int16_le(&buffer[4]);
    calibration_.dig_h1 = buffer[25];

    const std::array<std::uint8_t, 1> humidity_register{kHumidityCalibStartRegister};
    std::array<std::uint8_t, 7> humidity_buffer{};
    if (!bus_->write_read(address_, humidity_register, humidity_buffer)) {
        return false;
    }

    calibration_.dig_h2 = read_int16_le(&humidity_buffer[0]);
    calibration_.dig_h3 = humidity_buffer[2];
    calibration_.dig_h4 =
        static_cast<std::int16_t>((static_cast<std::int16_t>(humidity_buffer[3]) << 4) |
                                  (humidity_buffer[4] & 0x0F));
    calibration_.dig_h5 =
        static_cast<std::int16_t>((static_cast<std::int16_t>(humidity_buffer[5]) << 4) |
                                  (humidity_buffer[4] >> 4));
    calibration_.dig_h6 = static_cast<std::int8_t>(humidity_buffer[6]);
    return true;
}

bool Bme280Driver::trigger_measurement() {
    const std::array<std::uint8_t, 2> humidity_command{kCtrlHumidityRegister, 0x01};
    if (!bus_->write(address_, humidity_command)) {
        return false;
    }

    const std::array<std::uint8_t, 2> measure_command{
        kCtrlMeasureRegister,
        static_cast<std::uint8_t>(0x25)}; // temp x1, forced mode
    return bus_->write(address_, measure_command);
}

bool Bme280Driver::read_raw(std::int32_t *raw_temperature,
                            std::int32_t *raw_humidity) {
    if (raw_temperature == nullptr || raw_humidity == nullptr) {
        return false;
    }

    const std::array<std::uint8_t, 1> register_address{kDataStartRegister};
    std::array<std::uint8_t, 8> buffer{};
    if (!bus_->write_read(address_, register_address, buffer)) {
        return false;
    }

    *raw_temperature = (static_cast<std::int32_t>(buffer[3]) << 12) |
                       (static_cast<std::int32_t>(buffer[4]) << 4) |
                       (static_cast<std::int32_t>(buffer[5]) >> 4);
    *raw_humidity = (static_cast<std::int32_t>(buffer[6]) << 8) |
                    static_cast<std::int32_t>(buffer[7]);
    return true;
}

float Bme280Driver::compensate_temperature(std::int32_t raw_temperature,
                                           std::int32_t *fine_temperature) const {
    const std::int32_t var1 =
        (((raw_temperature >> 3) - (static_cast<std::int32_t>(calibration_.dig_t1) << 1)) *
         static_cast<std::int32_t>(calibration_.dig_t2)) >>
        11;
    const std::int32_t var2 =
        (((((raw_temperature >> 4) - static_cast<std::int32_t>(calibration_.dig_t1)) *
           ((raw_temperature >> 4) - static_cast<std::int32_t>(calibration_.dig_t1))) >>
          12) *
         static_cast<std::int32_t>(calibration_.dig_t3)) >>
        14;

    const std::int32_t fine = var1 + var2;
    if (fine_temperature != nullptr) {
        *fine_temperature = fine;
    }
    return static_cast<float>(((fine * 5 + 128) >> 8)) / 100.0F;
}

float Bme280Driver::compensate_humidity(std::int32_t raw_humidity,
                                        std::int32_t fine_temperature) const {
    std::int32_t var1 = fine_temperature - static_cast<std::int32_t>(76800);

    const std::int32_t var2 =
        (((raw_humidity << 14) -
          (static_cast<std::int32_t>(calibration_.dig_h4) << 20) -
          (static_cast<std::int32_t>(calibration_.dig_h5) * var1)) +
         static_cast<std::int32_t>(16384)) >>
        15;

    const std::int32_t var3 =
        (((((var1 * static_cast<std::int32_t>(calibration_.dig_h6)) >> 10) *
           (((var1 * static_cast<std::int32_t>(calibration_.dig_h3)) >> 11) +
            static_cast<std::int32_t>(32768))) >>
          10) +
         static_cast<std::int32_t>(2097152)) *
            static_cast<std::int32_t>(calibration_.dig_h2) +
        static_cast<std::int32_t>(8192);

    var1 = (var2 * (var3 >> 14));

    var1 = var1 - (((((var1 >> 15) * (var1 >> 15)) >> 7) *
                    static_cast<std::int32_t>(calibration_.dig_h1)) >>
                   4);

    if (var1 < 0) {
        var1 = 0;
    } else if (var1 > 419430400) {
        var1 = 419430400;
    }

    return static_cast<float>(var1 >> 12) / 100.0F;
}

BOSSA_REGISTER_DRIVER(Bme280Driver, "bme280");

} // namespace bossa::drivers

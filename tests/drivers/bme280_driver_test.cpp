/*
 *   Copyright (c) 2026 Alexandre Loeblein Heinen
 *   SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <memory>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "bme280_driver.hpp"
#include "bossa/drivers/registry.hpp"
#include "mock_i2c_bus.hpp"

namespace {

void copy_bytes(std::span<std::uint8_t> destination,
                std::span<const std::uint8_t> source) {
    ASSERT_GE(destination.size(), source.size());
    std::memcpy(destination.data(), source.data(), source.size());
}

} // namespace

// Phase 2.6 — built-in driver registration.
TEST(RegistryTest, RegistersBme280Driver) {
    EXPECT_TRUE(bossa::drivers::Registry::contains("bme280"));

    const std::unique_ptr<bossa::drivers::Driver> driver =
        bossa::drivers::Registry::create("bme280");
    ASSERT_NE(driver, nullptr);
    EXPECT_EQ(driver->name(), "bme280");
}

// Phase 2.8 — mock I2C returns canned BME280 bytes; driver yields expected
// samples.
TEST(Bme280DriverTest, MockI2cProducesExpectedSamples) {
    auto mock_bus = std::make_unique<bossa::io::MockI2cBus>();
    bossa::io::MockI2cBus *mock = mock_bus.get();

    mock->set_register_handler(0xD0,
                               [](std::uint8_t, std::span<const std::uint8_t>,
                                  std::span<std::uint8_t> read_data) {
                                   read_data[0] = 0x60;
                                   return true;
                               });

    const std::array<std::uint8_t, 26> calibration_block{
        0x30, 0x6F, 0xF7, 0x68, 0x32, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4B};
    mock->set_register_handler(
        0x88, [&calibration_block](std::uint8_t, std::span<const std::uint8_t>,
                                   std::span<std::uint8_t> read_data) {
            copy_bytes(read_data, calibration_block);
            return true;
        });

    const std::array<std::uint8_t, 7> humidity_calibration{
        0x6B, 0x01, 0x00, 0x12, 0x03, 0x1E, 0x00};
    mock->set_register_handler(
        0xE1,
        [&humidity_calibration](std::uint8_t, std::span<const std::uint8_t>,
                                std::span<std::uint8_t> read_data) {
            copy_bytes(read_data, humidity_calibration);
            return true;
        });

    mock->set_register_handler(0xF2,
                               [](std::uint8_t, std::span<const std::uint8_t>,
                                  std::span<std::uint8_t>) { return true; });
    mock->set_register_handler(0xF4,
                               [](std::uint8_t, std::span<const std::uint8_t>,
                                  std::span<std::uint8_t>) { return true; });

    const std::array<std::uint8_t, 8> measurement_block{0x00, 0x80, 0x00, 0x82,
                                                        0x3F, 0x80, 0x7F, 0x45};
    mock->set_register_handler(
        0xF7, [&measurement_block](std::uint8_t, std::span<const std::uint8_t>,
                                   std::span<std::uint8_t> read_data) {
            copy_bytes(read_data, measurement_block);
            return true;
        });

    bossa::drivers::Bme280Driver driver(std::move(mock_bus));
    const nlohmann::json parameters = {{"bus", "/dev/i2c-1"},
                                       {"address", 0x76}};
    driver.configure(parameters);

    const bossa::drivers::ReadResult result = driver.read();
    ASSERT_EQ(result.sample_count, 2U);

    EXPECT_EQ(result.samples[0].channel_id, "ambient_temperature");
    EXPECT_EQ(result.samples[0].unit, "celsius");
    EXPECT_NEAR(result.samples[0].value, 25.0, 0.5);

    EXPECT_EQ(result.samples[1].channel_id, "ambient_humidity");
    EXPECT_EQ(result.samples[1].unit, "percent");
    EXPECT_FALSE(std::isnan(result.samples[1].value));

    EXPECT_EQ(result.samples[0].quality,
              bossa::telemetry::SampleQuality::kGood);
    EXPECT_EQ(result.samples[1].quality,
              bossa::telemetry::SampleQuality::kGood);
}

// Phase 2 acceptance — read() uses fixed-capacity ReadResult (no heap growth).
TEST(Bme280DriverTest, ReadUsesFixedCapacityResult) {
    auto mock_bus = std::make_unique<bossa::io::MockI2cBus>();
    bossa::io::MockI2cBus *mock = mock_bus.get();

    mock->set_register_handler(0xD0,
                               [](std::uint8_t, std::span<const std::uint8_t>,
                                  std::span<std::uint8_t> read_data) {
                                   read_data[0] = 0x60;
                                   return true;
                               });

    const std::array<std::uint8_t, 26> calibration_block{};
    mock->set_register_handler(
        0x88, [&calibration_block](std::uint8_t, std::span<const std::uint8_t>,
                                   std::span<std::uint8_t> read_data) {
            copy_bytes(read_data, calibration_block);
            return true;
        });

    const std::array<std::uint8_t, 7> humidity_calibration{};
    mock->set_register_handler(
        0xE1,
        [&humidity_calibration](std::uint8_t, std::span<const std::uint8_t>,
                                std::span<std::uint8_t> read_data) {
            copy_bytes(read_data, humidity_calibration);
            return true;
        });

    mock->set_register_handler(0xF2,
                               [](std::uint8_t, std::span<const std::uint8_t>,
                                  std::span<std::uint8_t>) { return true; });
    mock->set_register_handler(0xF4,
                               [](std::uint8_t, std::span<const std::uint8_t>,
                                  std::span<std::uint8_t>) { return true; });
    mock->set_register_handler(0xF7, [](std::uint8_t,
                                        std::span<const std::uint8_t>,
                                        std::span<std::uint8_t> read_data) {
        std::fill(read_data.begin(), read_data.end(), 0);
        return true;
    });

    bossa::drivers::Bme280Driver driver(std::move(mock_bus));
    driver.configure({{"bus", "/dev/i2c-1"}, {"address", 0x76}});

    const bossa::drivers::ReadResult result = driver.read();
    EXPECT_LE(result.sample_count, bossa::drivers::ReadResult::kMaxSamples);
}

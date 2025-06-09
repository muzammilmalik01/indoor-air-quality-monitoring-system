/*
 * Copyright (c) 2024 Jan Fäh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SCD4X_H_
#define ZEPHYR_DRIVERS_SENSOR_SCD4X_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>

#define SCD4X_CMD_REINIT                         0
#define SCD4X_CMD_START_PERIODIC_MEASUREMENT     1
#define SCD4X_CMD_STOP_PERIODIC_MEASUREMENT      2
#define SCD4X_CMD_READ_MEASUREMENT               3
#define SCD4X_CMD_SET_TEMPERATURE_OFFSET         4
#define SCD4X_CMD_GET_TEMPERATURE_OFFSET         5
#define SCD4X_CMD_SET_SENSOR_ALTITUDE            6
#define SCD4X_CMD_GET_SENSOR_ALTITUDE            7
#define SCD4X_CMD_SET_AMBIENT_PRESSURE           8
#define SCD4X_CMD_GET_AMBIENT_PRESSURE           9
#define SCD4X_CMD_FORCED_RECALIB                 10
#define SCD4X_CMD_SET_AUTOMATIC_CALIB_ENABLE     11
#define SCD4X_CMD_GET_AUTOMATIC_CALIB_ENABLE     12
#define SCD4X_CMD_LOW_POWER_PERIODIC_MEASUREMENT 13
#define SCD4X_CMD_GET_DATA_READY_STATUS          14
#define SCD4X_CMD_PERSIST_SETTINGS               15
#define SCD4X_CMD_SELF_TEST                      16
#define SCD4X_CMD_FACTORY_RESET                  17
#define SCD4X_CMD_MEASURE_SINGLE_SHOT            18
#define SCD4X_CMD_MEASURE_SINGLE_SHOT_RHT        19
#define SCD4X_CMD_POWER_DOWN                     20
#define SCD4X_CMD_WAKE_UP                        21
#define SCD4X_CMD_SET_SELF_CALIB_INITIAL_PERIOD  22
#define SCD4X_CMD_GET_SELF_CALIB_INITIAL_PERIOD  23
#define SCD4X_CMD_SET_SELF_CALIB_STANDARD_PERIOD 24
#define SCD4X_CMD_GET_SELF_CALIB_STANDARD_PERIOD 25

#define SCD4X_CRC_POLY 0x31
#define SCD4X_CRC_INIT 0xFF

#define SCD4X_STARTUP_TIME_MS 30

#define SCD4X_TEMPERATURE_OFFSET_IDX_MAX 20
#define SCD4X_SENSOR_ALTITUDE_IDX_MAX    3000
#define SCD4X_AMBIENT_PRESSURE_IDX_MAX   1200
#define SCD4X_BOOL_IDX_MAX               1

#define SCD4X_MAX_TEMP 175
#define SCD4X_MIN_TEMP -45

enum scd4x_model_t {
	SCD4X_MODEL_SCD40,
	SCD4X_MODEL_SCD41,
};

enum scd4x_mode_t {
	SCD4X_MODE_NORMAL,
	SCD4X_MODE_LOW_POWER,
	SCD4X_MODE_SINGLE_SHOT,
};

struct scd4x_config {
	struct i2c_dt_spec bus;
	enum scd4x_model_t model;
	enum scd4x_mode_t mode;
};

struct scd4x_data {
	uint16_t temp_sample;
	uint16_t humi_sample;
	uint16_t co2_sample;
};

struct cmds_t {
	uint16_t cmd;
	uint16_t cmd_duration_ms;
};



 enum sensor_attribute_scd4x {
     /* Offset temperature: Toffset_actual = Tscd4x – Treference + Toffset_previous
      * 0 - 20°C
      */
     SENSOR_ATTR_SCD4X_TEMPERATURE_OFFSET = SENSOR_ATTR_PRIV_START,
     /* Altidude of the sensor;
      * 0 - 3000m
      */
     SENSOR_ATTR_SCD4X_SENSOR_ALTITUDE,
     /* Ambient pressure in hPa
      * 700 - 1200hPa
      */
     SENSOR_ATTR_SCD4X_AMBIENT_PRESSURE,
     /* Set the current state (enabled: 1 / disabled: 0).
      * Default: enabled.
      */
     SENSOR_ATTR_SCD4X_AUTOMATIC_CALIB_ENABLE,
     /* Set the initial period for automatic self calibration correction in hours. Allowed values
      * are integer multiples of 4 hours.
      * Default: 44
      */
     SENSOR_ATTR_SCD4X_SELF_CALIB_INITIAL_PERIOD,
     /* Set the standard period for automatic self calibration correction in hours. Allowed
      * values are integer multiples of 4 hours. Default: 156
      */
     SENSOR_ATTR_SCD4X_SELF_CALIB_STANDARD_PERIOD,
 };
 
 /**
  * @brief Performs a forced recalibration.
  *
  * Operate the SCD4x in the operation mode for at least 3 minutes in an environment with a
  * homogeneous and constant CO2 concentration. Otherwise the recalibratioin will fail. The sensor
  * must be operated at the voltage desired for the application when performing the FRC sequence.
  *
  * @param dev Pointer to the sensor device
  * @param target_concentration Reference CO2 concentration.
  * @param frc_correction Previous differences from the target concentration
  *
  * @return 0 if successful, negative errno code if failure.
  */
 int scd4x_forced_recalibration(const struct device *dev, uint16_t target_concentration,
                    uint16_t *frc_correction);
 
 /**
  * @brief Performs a self test.
  *
  * The self_test command can be used as an end-of-line test to check the sensor functionality
  *
  * @param dev Pointer to the sensor device
  *
  * @return 0 if successful, negative errno code if failure.
  */
 int scd4x_self_test(const struct device *dev);
 
 /**
  * @brief Performs a self test.
  *
  * The persist_settings command can be used to save the actual configuration. This command
  * should only be sent when persistence is required and if actual changes to the configuration have
  * been made. The EEPROM is guaranteed to withstand at least 2000 write cycles
  *
  * @param dev Pointer to the sensor device
  *
  * @return 0 if successful, negative errno code if failure.
  */
 int scd4x_persist_settings(const struct device *dev);
 
 /**
  * @brief Performs a factory reset.
  *
  * The perform_factory_reset command resets all configuration settings stored in the EEPROM and
  * erases the FRC and ASC algorithm history.
  *
  * @param dev Pointer to the sensor device
  *
  * @return 0 if successful, negative errno code if failure.
  */
 int scd4x_factory_reset(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_SCD4X_H_ */

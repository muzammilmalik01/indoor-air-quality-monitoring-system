#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor/ccs811.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include "sensor/scd4x/scd4x.h"

//TODO: Add CoAP Data Transmission.
//TODO: Add data validation for both sensors. (DONE)
//TODO: Modularize code for better readability and maintainability.
//TODO: Retry mechanism for sensor data fetching in case of failure.
//TODO: Add Sensor Status (bool scd41_ok) 

#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_scd41)
#error "No sensirion,scd4x compatible node found in the device tree"
#endif

const struct device *scd41 = DEVICE_DT_GET_ANY(sensirion_scd41);
const struct device *ccs81 = DEVICE_DT_GET_ANY(ams_ccs811);

int main(void)
{
	bool scd41_ok = true;
	bool ccs81_ok = true;
	if (!device_is_ready(scd41)) {
		printk("SCD41 device is not ready\n");
		scd41_ok = false;
		return -1;
	}
	if (!device_is_ready(ccs81)) {
		printk("CCS811 device is not ready\n");
		ccs81_ok = false;
		return -1;
	}
	
	struct sensor_value co2_41, co2_811, temo, humi, tvoc, voltage, current;
	while (true)
	{
		if (sensor_sample_fetch(scd41) < 0)
		{
			printk("Failed to fetch sample from SCD41\n");
			continue;
			// message to Server for SCD41 not working
		}
		if (sensor_sample_fetch(ccs81) < 0)
		{
			printk("Failed to fetch sample from CCS811\n");
			continue;
			// message to Server for CCS811 not working
		}
		else{
			printk("Fetched samples successfully from both sensors.\n");
			// message to Server for sensors working.

			// ***** Error hangling for SCD41 *****
			if(sensor_channel_get(scd41, SENSOR_CHAN_CO2, &co2_41) < 0){
				printk("Failed to get CO2 channel from SCD41\n");
				// message to server: Cannot get CO2 Data from SCD41
				continue;
			}
			if(sensor_channel_get(scd41, SENSOR_CHAN_AMBIENT_TEMP, &temo) < 0){
				printk("Failed to get temperature from SCD41\n");
				// message to server: Cannot get Temperature Data from SCD41
				continue;
			}
			if(sensor_channel_get(scd41, SENSOR_CHAN_HUMIDITY, &humi) < 0){
				printk("Failed to get Humidity from SCD41\n");
				// message to server: Cannot get Humidity from SCD41
				continue;
			}

			// ***** Error hangling for SCD41 ENDS *****

			// ***** Error hangling for CCS81 *****
			
			if(sensor_channel_get(ccs81, SENSOR_CHAN_CO2, &co2_811) < 0){
				printk("Failed to get eCO2 from CCS811\n");
				continue;
				// message to server: Cannot get eCO2 from CCS811
			}

			if(sensor_channel_get(ccs81, SENSOR_CHAN_VOC, &tvoc) < 0){
				printk("Failed to get TVOC from CCS81\n");
				// message to server: Cannot get TVOC from CCS811
				continue;
			}
			if(sensor_channel_get(ccs81, SENSOR_CHAN_VOLTAGE, &voltage) < 0.0){
				printk("Failed to get Voltage from CCS811\n");
				// message to server: Cannot get Voltage from CCS811
				continue;
			}
			if(sensor_channel_get(ccs81, SENSOR_CHAN_CURRENT, &current) < 0){
				printk("Failed to get Current from CCS811\n");
				// message to server: Cannot get Current from CCS811
				continue;
			}
			// ***** Error hangling for CCS81 ENDS *****
			else{
				// Data validation for SCD41 and CCS811
				if (temo.val1 < -40 || temo.val1 > 85){
					printk("Invalid Temperature value from SCD41: %d.%06d C\n", temo.val1, temo.val2);
					// continue;
					//message to server: Invalid Temperature value from SCD41
				}
				if (humi.val1 < 0 || humi.val1 > 100){
					printk("Invalid Humidity value from SCD41: %d.%06d %%\n", humi.val1, humi.val2);
					// continue;
					//message to server: Invalid Humidity value from SCD41
				}
				if (co2_41.val1 < 0 && co2_41.val1 > 5000){
					printk("Invalid CO2 value from SCD41: %d\n", co2_41.val1);
					// continue;
					//message to server: Invalid CO2 value from SCD41
					
				}
				if (co2_811.val1 < 400 && co2_811.val1 > 8192){
					printk("Invalid eCO2 value from CCS811: %u\n", co2_811.val1);
					continue;
					//message to server: Invalid eCO2 value from CCS811
				}
				if (tvoc.val1 < 0 && tvoc.val1 > 1187){
					printk("Invalid TVOC value from CCS811: %u\n", tvoc.val1);
					continue;
					//message to server: Invalid TVOC value from CCS811
				}
				printk("\nSCD41:\n");
				printk("CO2: %d.%06d ppm, Temperature: %d.%06d C, Humidity: %d.%06d %%\n",
					   co2_41.val1, co2_41.val2, temo.val1, temo.val2, humi.val1, humi.val2);
				printk("\nCCS811:\n");
				printk("%u ppm eCO2; %u ppb eTVOC\n",co2_811.val1, tvoc.val1);
				printk("Voltage: %d.%06dV; Current: %d.%06dA\n", voltage.val1,
					voltage.val2, current.val1, current.val2);
				// CoAP message to server with the data
			}

		}
		k_sleep(K_SECONDS(5));
	}
	return 0;
}
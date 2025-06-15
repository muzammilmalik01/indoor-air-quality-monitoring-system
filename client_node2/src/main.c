#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include "sensor/sps30/sps30.h"

#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sps30)
#error "No sensirion,sps30 compatible node found in the device tree"
#endif

const struct device *sps30 = DEVICE_DT_GET_ANY(sensirion_sps30);

bool is_sps30_data_valid(struct sensor_value pm_1p0, struct sensor_value pm_2p5, struct sensor_value pm_10p0) {
	return (pm_1p0.val1 > 0 && pm_1p0.val1 < 1000) &&
           (pm_2p5.val1 > 0 && pm_2p5.val1 < 1000) &&
           (pm_10p0.val1 > 0 && pm_10p0.val1 < 1000);
}
int main(void)
{
	bool SPS30_OK = false;
	if (!device_is_ready(sps30)) {
		printk("SPS30 device not ready\n");
		return 1;
	}
	printk("SPS30 device is ready\n");
	struct sensor_value pm_1p0, pm_2p5, pm_10p0;

	while(true) {
		if (sensor_sample_fetch(sps30) < 0){
			printk("Failed to fetch sample from SPS30 sensor\n");
			SPS30_OK = false;
		} else {
			sensor_channel_get(sps30, SENSOR_CHAN_PM_1_0, &pm_1p0);
			sensor_channel_get(sps30, SENSOR_CHAN_PM_2_5, &pm_2p5);
			sensor_channel_get(sps30, SENSOR_CHAN_PM_10, &pm_10p0);
		}

		bool sps30_valid = is_sps30_data_valid(pm_1p0, pm_2p5, pm_10p0);
		if (sps30_valid) {
			SPS30_OK = true;
			printk("Sending SPS30 Data...\n");
			//send sps30 data function.
		} else {
			SPS30_OK = false;
			printk("No valid data to send (Sensor Data out of bound).\n");
		}

		// sensor_sample_fetch(sps30);
		
		// printk("SPS30 Sensor Data:\n");
		// printk("PM 1.0: %d.%06d mg/m^3\n", pm_1p0.val1, pm_1p0.val2);
		// printk("PM 2.5: %d.%06d mg/m^3\n", pm_2p5.val1, pm_2p5.val2);
		// printk("PM 10.0: %d.%06d mg/m^3\n", pm_10p0.val1, pm_10p0.val2);

		k_sleep(K_SECONDS(20));
	}
	return 0;
}
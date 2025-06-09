#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor/ccs811.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include "sensor/scd4x/scd4x.h"
#include <openthread/coap.h>
#include <openthread/thread.h> // Added for otMeshLocalPrefix definition

// TODO: Add error message to server sending for sensor.
// Improve sending data (messages) to server.
// Complete SCD41  + CCS811 Data Sending to Server.
// TODO: Modularize code for better readability and maintainability.
// TODO: Retry mechanism for sensor data fetching in case of failure.
// TODO: Add Sensor Status (bool scd41_ok)

// THREAD NETWORK CONFIGURATION //
void coap_init(void)
{
	otInstance *p_instance = openthread_get_default_instance();
	otError error = otCoapStart(p_instance, OT_DEFAULT_COAP_PORT);
	if (error != OT_ERROR_NONE)
		printk("Failed to start Coap: %d\n", error);
	else
	{
		printk("Coap started successfully.\n");
	}
}
static void coap_send_data_response_cb(void *p_context, otMessage *p_message,
									   const otMessageInfo *p_message_info, otError result)
{
	if (result == OT_ERROR_NONE)
	{
		printk("Delivery confirmed.\n");
	}
	else
	{
		printk("Delivery not confirmed: %d\n", result);
	}
}
static void send_coap_message(const char *uri_path, const char *payload)
{
	otError error = OT_ERROR_NONE;
	otMessage *message;
	otMessageInfo message_info;
	otInstance *instance = openthread_get_default_instance();
	const otMeshLocalPrefix *mesh_prefix = otThreadGetMeshLocalPrefix(instance);
	uint8_t server_interface_id[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

	do
	{
		// Allocate a new CoAP message
		message = otCoapNewMessage(instance, NULL);
		if (message == NULL)
		{
			printk("Failed to allocate CoAP message\n");
			return;
		}

		// Initialize the CoAP message
		otCoapMessageInit(message, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_PUT);

		// Append URI path
		error = otCoapMessageAppendUriPathOptions(message, uri_path);
		if (error != OT_ERROR_NONE)
		{
			printk("Failed to append URI path: %d\n", error);
			otMessageFree(message);
			return;
		}

		// Append content format option (JSON)
		error = otCoapMessageAppendContentFormatOption(message, OT_COAP_OPTION_CONTENT_FORMAT_JSON);
		if (error != OT_ERROR_NONE)
		{
			printk("Failed to append content format option: %d\n", error);
			otMessageFree(message);
			return;
		}

		// Set payload marker
		error = otCoapMessageSetPayloadMarker(message);
		if (error != OT_ERROR_NONE)
		{
			printk("Failed to set payload marker: %d\n", error);
			otMessageFree(message);
			return;
		}

		// Append payload
		error = otMessageAppend(message, payload, strlen(payload));
		if (error != OT_ERROR_NONE)
		{
			printk("Failed to append payload: %d\n", error);
			otMessageFree(message);
			return;
		}

		// Set up message info
		memset(&message_info, 0, sizeof(message_info));
		memcpy(&message_info.mPeerAddr.mFields.m8[0], mesh_prefix, 8);
		memcpy(&message_info.mPeerAddr.mFields.m8[8], server_interface_id, 8);
		message_info.mPeerPort = OT_DEFAULT_COAP_PORT;

		// Send the CoAP request
		error = otCoapSendRequest(instance, message, &message_info, coap_send_data_response_cb, NULL);
	} while (false);

	if (error != OT_ERROR_NONE)
	{
		printk("Failed to send CoAP request: %d\n", error);
		otMessageFree(message);
	}
	else
	{
		printk("CoAP message sent successfully.\n");
	}
}

// THREAD NETWORK CONFIGURATION ENDS //
#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_scd41)
#error "No sensirion,scd4x compatible node found in the device tree"
#endif

const struct device *scd41 = DEVICE_DT_GET_ANY(sensirion_scd41);
const struct device *ccs81 = DEVICE_DT_GET_ANY(ams_ccs811);

// Function to send SCD41 data in JSON format
void send_scd41_data(struct sensor_value co2_41, struct sensor_value temo, struct sensor_value humi)
{
	char payload[256]; // Buffer to hold the JSON payload

	// Construct the JSON payload
	snprintf(payload, sizeof(payload),
			 "{\"sensor\":\"scd41\",\"data\":{\"CO2\":%d.%02d,\"Temperature\":%d.%02d,\"Humidity\":%d.%02d}}",
			 co2_41.val1, co2_41.val2, temo.val1, temo.val2, humi.val1, humi.val2);


	// Send the CoAP message
	send_coap_message("sensor_data", payload);
}

int main(void)
{
	coap_init();
	bool scd41_ok = true;
	bool ccs81_ok = true;
	if (!device_is_ready(scd41))
	{
		printk("SCD41 device is not ready\n");
		scd41_ok = false;
		return -1;
	}
	if (!device_is_ready(ccs81))
	{
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
		else
		{
			printk("Fetched samples successfully from both sensors.\n");
			// message to Server for sensors working.

			// ***** Error hangling for SCD41 *****
			if (sensor_channel_get(scd41, SENSOR_CHAN_CO2, &co2_41) < 0)
			{
				printk("Failed to get CO2 channel from SCD41\n");
				// message to server: Cannot get CO2 Data from SCD41
				continue;
			}
			if (sensor_channel_get(scd41, SENSOR_CHAN_AMBIENT_TEMP, &temo) < 0)
			{
				printk("Failed to get temperature from SCD41\n");
				// message to server: Cannot get Temperature Data from SCD41
				continue;
			}
			if (sensor_channel_get(scd41, SENSOR_CHAN_HUMIDITY, &humi) < 0)
			{
				printk("Failed to get Humidity from SCD41\n");
				// message to server: Cannot get Humidity from SCD41
				continue;
			}

			// ***** Error hangling for SCD41 ENDS *****

			// ***** Error hangling for CCS81 *****

			if (sensor_channel_get(ccs81, SENSOR_CHAN_CO2, &co2_811) < 0)
			{
				printk("Failed to get eCO2 from CCS811\n");
				continue;
				// message to server: Cannot get eCO2 from CCS811
			}
			if (sensor_channel_get(ccs81, SENSOR_CHAN_VOC, &tvoc) < 0)
			{
				printk("Failed to get TVOC from CCS81\n");
				// message to server: Cannot get TVOC from CCS811
				continue;
			}
			if (sensor_channel_get(ccs81, SENSOR_CHAN_VOLTAGE, &voltage) < 0.0)
			{
				printk("Failed to get Voltage from CCS811\n");
				// message to server: Cannot get Voltage from CCS811
				continue;
			}
			if (sensor_channel_get(ccs81, SENSOR_CHAN_CURRENT, &current) < 0)
			{
				printk("Failed to get Current from CCS811\n");
				// message to server: Cannot get Current from CCS811
				continue;
			}
			// ***** Error hangling for CCS81 ENDS *****
			else
			{
				// Data validation for SCD41 and CCS811
				if (temo.val1 < -40 || temo.val1 > 85)
				{
					printk("Invalid Temperature value from SCD41: %d.%06d C\n", temo.val1, temo.val2);
					// continue;
					// message to server: Invalid Temperature value from SCD41
				}
				if (humi.val1 < 0 || humi.val1 > 100)
				{
					printk("Invalid Humidity value from SCD41: %d.%06d %%\n", humi.val1, humi.val2);
					// continue;
					// message to server: Invalid Humidity value from SCD41
				}
				if (co2_41.val1 < 0 && co2_41.val1 > 5000)
				{
					printk("Invalid CO2 value from SCD41: %d\n", co2_41.val1);
					// continue;
					// message to server: Invalid CO2 value from SCD41
				}
				if (co2_811.val1 < 400 && co2_811.val1 > 8192)
				{
					printk("Invalid eCO2 value from CCS811: %u\n", co2_811.val1);
					continue;
					// message to server: Invalid eCO2 value from CCS811
				}
				if (tvoc.val1 < 0 && tvoc.val1 > 1187)
				{
					printk("Invalid TVOC value from CCS811: %u\n", tvoc.val1);
					continue;
					// message to server: Invalid TVOC value from CCS811
				}

				printk("\nSCD41:\n");
				printk("CO2: %d.%06d ppm, Temperature: %d.%06d C, Humidity: %d.%06d %%\n",
					   co2_41.val1, co2_41.val2, temo.val1, temo.val2, humi.val1, humi.val2);
				const char *test_payload = "{\"sensor\":\"test\",\"data\":{\"value\":123}}";
				send_scd41_data(co2_41, temo, humi); // Send SCD41 data in JSON format
				// printk("\nCCS811:\n");
				// printk("%u ppm eCO2; %u ppb eTVOC\n", co2_811.val1, tvoc.val1);
				// printk("Voltage: %d.%06dV; Current: %d.%06dA\n", voltage.val1,
				// 	   voltage.val2, current.val1, current.val2);
				// CoAP message to server with the data
			}
		}
		k_sleep(K_SECONDS(10));
	}
	return 0;
}
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor/ccs811.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
#include "sensor/scd4x/scd4x.h"
#include <openthread/coap.h>
#include <openthread/thread.h>
#include <zephyr/net/openthread.h> 

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

#if !DT_HAS_COMPAT_STATUS_OKAY(ams_ccs811)
#error "No sensirion,scd4x compatible node found in the device tree"
#endif

const struct device *scd41 = DEVICE_DT_GET_ANY(sensirion_scd41);
const struct device *ccs811 = DEVICE_DT_GET_ANY(ams_ccs811);

// Function to send data in JSON format to CoAP Server
void send_error_message(const char *message, bool *scd41_ok, bool *ccs811_ok)
{
	char payload[256]; // Buffer to hold the JSON payload

	// Construct the JSON payload
	snprintf(payload, sizeof(payload), "{\"error\":\"%s\",\"SCD41_OK\":%s,\"CCS811_OK\":%s}\n", message, (*scd41_ok) ? "true" : "false", (*ccs811_ok) ? "true" : "false");

	// Send the CoAP message
	send_coap_message("sensor_data", payload);
}

void send_scd41_data(struct sensor_value co2_41, struct sensor_value temo, struct sensor_value humi, bool *scd41_ok)
{
	char payload[256]; // Buffer to hold the JSON payload

	// Construct the JSON payload
	snprintf(payload, sizeof(payload),
			 "{\"sensor\":\"scd41\",\"data\":{\"CO2\":%d.%02d,\"Temperature\":%d.%02d,\"Humidity\":%d.%02d, \"SCD41_OK\":%s}}\n",
			 co2_41.val1, co2_41.val2, temo.val1, temo.val2, humi.val1, humi.val2,
			 (*scd41_ok) ? "true" : "false");

	// Send the CoAP message
	send_coap_message("sensor_data", payload);
}

void send_ccs811_data(struct sensor_value co2_881, struct sensor_value tvoc, bool *ccs881_ok)
{
	char payload[256]; // Buffer to hold the JSON payload

	// Construct the JSON payload
	snprintf(payload, sizeof(payload),
			 "{\"sensor\":\"ccs811\",\"data\":{\"eCO2\":%d.%02d,\"TVOC\":%d.%02d, \"CCS811_OK\":%s}}\n",
			 co2_881.val1, co2_881.val2, tvoc.val1, tvoc.val2, (*ccs881_ok) ? "true" : "false");

	// Send the CoAP message
	send_coap_message("sensor_data", payload);
}

bool is_scd41_data_valid(struct sensor_value co2, struct sensor_value temp, struct sensor_value hum) {
	return (co2.val1 > 0 && co2.val1 < 5000) &&
           (temp.val1 > -40 && temp.val1 < 85) &&
           (hum.val1 >= 0 && hum.val1 <= 100);
}

bool is_ccs811_data_valid(struct sensor_value co2, struct sensor_value voc) {
    return (co2.val1 > 400 && co2.val1 < 8192) &&
           (voc.val1 >= 0 && voc.val1 < 1187); 
}

int main(void)
{
	bool SCD41_OK = false;
	bool CCS811_OK = false;
	coap_init();
	if (!device_is_ready(scd41) && !device_is_ready(ccs811))
	{
		// Driver Issue or Sensor not physically connected. 
		// Sensor is not powered, Sensor is not connected, I2C Pin mis-configured.
		printk("SCD41 and CCS811 device is not ready\n");
		send_error_message("SCD41 and CCS881 not ready - Sensors not connected or Sensor's PINs mis-configured.", &SCD41_OK, &CCS811_OK);
		return -1;
	}
	if (!device_is_ready(scd41) && device_is_ready(ccs811))
	{
		CCS811_OK = true;
		printk("SCD41 device is not ready\n");
		send_error_message("SCD41 not ready - Sensor not connected or Sensor's PINs mis-configured.", &SCD41_OK, &CCS811_OK);
		return -1;
	}
	if (!device_is_ready(ccs811) && device_is_ready(scd41))
	{
		SCD41_OK = true;
		printk("CCS811 device is not ready\n");
		send_error_message("CCS811 not ready - Sensor not connected or Sensor's PINs mis-configured.", &SCD41_OK, &CCS811_OK);
		return -1;
	}

	SCD41_OK = true;
	CCS811_OK = true;
	printk("SCD41 and CCS811 devices are ready\n");
	struct sensor_value co2_41, co2_811, temp, humi, tvoc;
	while (true) {
        // Fetch data from SCD41
        if (sensor_sample_fetch(scd41) < 0) {
            printk("Failed to fetch sample from SCD41\n");
			SCD41_OK = false;
			send_error_message("Unable to read data from SCD41 - Sensor Warming Up or Unresponsive.", &SCD41_OK, &CCS811_OK);
        } else {
			SCD41_OK = true;
            sensor_channel_get(scd41, SENSOR_CHAN_CO2, &co2_41);
            sensor_channel_get(scd41, SENSOR_CHAN_AMBIENT_TEMP, &temp);
            sensor_channel_get(scd41, SENSOR_CHAN_HUMIDITY, &humi);
        }

        // Fetch data from CCS811
        if (sensor_sample_fetch(ccs811) < 0) {
            printk("Failed to fetch sample from CCS811\n");			
			CCS811_OK = false;
			send_error_message("Unable to read data from CCS811 - Sensor Warming Up or Unresponsive.", &SCD41_OK, &CCS811_OK);
        } else {
			CCS811_OK = true;
            sensor_channel_get(ccs811, SENSOR_CHAN_CO2, &co2_811);
            sensor_channel_get(ccs811, SENSOR_CHAN_VOC, &tvoc);
        }

        // Validate data
        bool scd41_valid = is_scd41_data_valid(co2_41, temp, humi);
        bool ccs811_valid = is_ccs811_data_valid(co2_811, tvoc);

        // Send data conditionally
        if (scd41_valid && ccs811_valid) {
			// if both sensors get valid data, send both
			SCD41_OK = true;
			CCS811_OK = true;
            printk("Sending data from both sensors...\n");
            send_scd41_data(co2_41, temp, humi, &SCD41_OK);
            send_ccs811_data(co2_811, tvoc, &CCS811_OK); 
        } else if (scd41_valid) {
			// if only SCD41 is valid, send SC41 data only
			SCD41_OK = true;
			CCS811_OK = false;
            printk("Sending data from SCD41 only...\n");
            send_scd41_data(co2_41, temp, humi, &SCD41_OK);
			send_error_message("CCS811 data invalid, sending SCD41 data only.", &SCD41_OK, &CCS811_OK);
        } else if (ccs811_valid) {
			// if only CCS811 is valid, send CCS811 data only
			CCS811_OK = true;
			SCD41_OK = false;
            printk("Sending data from CCS811 only...\n");
            send_ccs811_data(co2_811, tvoc, &CCS811_OK);
			send_error_message("SCD41 data invalid, sending CCS811 data only.", &SCD41_OK, &CCS811_OK);
        } else {
			// no data valid to send
			SCD41_OK = false;
			CCS811_OK = false;
            printk("No valid data to send (Sensor Data out of bound).\n");
			send_error_message("INVALID DATA SENT FROM SCD41 and CCS811",&SCD41_OK, &CCS811_OK);
        }

        k_sleep(K_SECONDS(60)); // Sleep before the next iteration
    }
	return 0;
}
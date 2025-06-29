#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor.h>
#include "sensor/sps30/sps30.h"
#include <openthread/coap.h>
#include <zephyr/net/openthread.h> 
#include <openthread/thread.h>
#include <zephyr/logging/log.h>

#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sps30)
#error "No sensirion,sps30 compatible node found in the device tree"
#endif

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

const struct device *sps30 = DEVICE_DT_GET_ANY(sensirion_sps30);

bool is_sps30_data_valid(struct sensor_value pm_1p0, struct sensor_value pm_2p5, struct sensor_value pm_10p0) {
	return (pm_1p0.val1 > 0 && pm_1p0.val1 < 1000) &&
           (pm_2p5.val1 > 0 && pm_2p5.val1 < 1000) &&
           (pm_10p0.val1 > 0 && pm_10p0.val1 < 1000);
}

void send_sps30_data(struct sensor_value pm_1p0, struct sensor_value pm_2p5, struct sensor_value pm_10p0, bool *sps30_ok) {
	char payload[256]; // Buffer to hold the JSON payload

	// Construct the JSON payload
	snprintf(payload, sizeof(payload),
			 "{\"sensor\":\"sps30\",\"data\":{\"PM1.0\":%d.%02d,\"PM2.5\":%d.%02d,\"PM10.0\":%d.%02d, \"SPS30_OK\":%s}}\n",
			 pm_1p0.val1, pm_1p0.val2, pm_2p5.val1, pm_2p5.val2, pm_10p0.val1, pm_10p0.val2,
			 (*sps30_ok) ? "true" : "false");

	// Send the CoAP message
	send_coap_message("sensor_data", payload);
}

void send_error_message(const char *message, bool *sps30_ok)
{
	char payload[256]; // Buffer to hold the JSON payload

	// Construct the JSON payload
	snprintf(payload, sizeof(payload), "{\"error\":\"%s\",\"SPS30_OK\":%s}\n", message, (*sps30_ok) ? "true" : "false");

	// Send the CoAP message
	send_coap_message("sensor_data", payload);
}
int main(void)
{
    bool SPS30_OK = false;
    coap_init();

    if (!device_is_ready(sps30)) {
        printk("SPS30 device not ready\n");
        send_error_message("SPS30 not ready - Sensor not connected or Sensor's PINs mis-configured.", &SPS30_OK);
        return -1;
    }
    printk("SPS30 device is ready\n");
    SPS30_OK = true;

    struct sensor_value pm_1p0, pm_2p5, pm_10p0;
    struct sensor_value pm_1p0_sum = {0}, pm_2p5_sum = {0}, pm_10p0_sum = {0};
    int valid_readings = 0;

    while (true) {
        // Reset sums and counters for averaging
        pm_1p0_sum = (struct sensor_value){0};
        pm_2p5_sum = (struct sensor_value){0};
        pm_10p0_sum = (struct sensor_value){0};
        valid_readings = 0;

        // Collect 3 readings over 15 seconds
        for (int i = 0; i < 3; i++) {
            if (sensor_sample_fetch(sps30) < 0) {
                printk("Failed to fetch sample from SPS30 sensor\n");
                SPS30_OK = false;
            } else {
                SPS30_OK = true;
                sensor_channel_get(sps30, SENSOR_CHAN_PM_1_0, &pm_1p0);
                sensor_channel_get(sps30, SENSOR_CHAN_PM_2_5, &pm_2p5);
                sensor_channel_get(sps30, SENSOR_CHAN_PM_10, &pm_10p0);

                if (is_sps30_data_valid(pm_1p0, pm_2p5, pm_10p0)) {
                    pm_1p0_sum.val1 += pm_1p0.val1;
                    pm_1p0_sum.val2 += pm_1p0.val2;
                    pm_2p5_sum.val1 += pm_2p5.val1;
                    pm_2p5_sum.val2 += pm_2p5.val2;
                    pm_10p0_sum.val1 += pm_10p0.val1;
                    pm_10p0_sum.val2 += pm_10p0.val2;
                    valid_readings++;
                }
            }

            k_sleep(K_SECONDS(15)); // Wait 15 seconds before the next reading
        }

        // Calculate averages
        if (valid_readings > 0) {
            pm_1p0_sum.val1 /= valid_readings;
            pm_1p0_sum.val2 /= valid_readings;
            pm_2p5_sum.val1 /= valid_readings;
            pm_2p5_sum.val2 /= valid_readings;
            pm_10p0_sum.val1 /= valid_readings;
            pm_10p0_sum.val2 /= valid_readings;

            printk("Sending averaged SPS30 data...\n");
            send_sps30_data(pm_1p0_sum, pm_2p5_sum, pm_10p0_sum, &SPS30_OK);
        } else {
            printk("No valid data to send (Sensor Data out of bound).\n");
            send_error_message("SPS30 - No valid data to send (Sensor Data out of bound)", &SPS30_OK);
        }

        k_sleep(K_SECONDS(15)); // Sleep for the remaining time to complete 60 seconds
    }

    return 0;
}
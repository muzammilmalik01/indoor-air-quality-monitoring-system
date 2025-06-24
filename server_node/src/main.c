#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <openthread/coap.h>
#include <openthread/thread.h>
#include <zephyr/net/openthread.h>
#include <stdio.h>


// UART FT232 Configuration
#define UART1_NODE DT_NODELABEL(uart1)
static const struct device *uart_dev = DEVICE_DT_GET(UART1_NODE);
static uint8_t tx_buf[256];
static int tx_buf_length;


// COAP Server Implenentation

#define TEXTBUFFER_SIZE 256
char myText[TEXTBUFFER_SIZE];
uint16_t myText_length = 0;

static void storedata_request_cb(void *p_context, otMessage *p_message, 
    const otMessageInfo *p_message_info);
static void storedata_response_send(otMessage *p_request_message, 
    const otMessageInfo *p_message_info);

static otCoapResource m_storedata_resource = {
    .mUriPath = "sensor_data",
    .mHandler = storedata_request_cb,
    .mContext = NULL,
    .mNext = NULL
};


// Handles incoming PUT requests to the "storedata" resource.
static void storedata_request_cb(void *p_context, otMessage *p_message, 
    const otMessageInfo *p_message_info) {
    otCoapCode messageCode = otCoapMessageGetCode(p_message);
    otCoapType messageType = otCoapMessageGetType(p_message);

    do {
        if (messageType != OT_COAP_TYPE_CONFIRMABLE && 
            messageType != OT_COAP_TYPE_NON_CONFIRMABLE) {
            break;
        }
        if (messageCode != OT_COAP_CODE_PUT) {
            break;
        }

        myText_length = otMessageRead(p_message, otMessageGetOffset(p_message),
            myText, TEXTBUFFER_SIZE - 1);
        myText[myText_length] = '\0';
        printk("\nReceived: %s\n", myText);
        tx_buf_length = sprintf(tx_buf, myText);
        uart_tx(uart_dev, tx_buf, tx_buf_length, SYS_FOREVER_US);
        if (messageType == OT_COAP_TYPE_CONFIRMABLE) {
            storedata_response_send(p_message, p_message_info);
        }
    } while (false);
}

// Sends acknowledgment for confirmable messages.
static void storedata_response_send(otMessage *p_request_message, 
    const otMessageInfo *p_message_info) {
    otError error = OT_ERROR_NO_BUFS;
    otMessage *p_response;
    otInstance *p_instance = openthread_get_default_instance();

    p_response = otCoapNewMessage(p_instance, NULL);
    if (p_response == NULL) {
        printk("Failed to allocate message for CoAP response\n");
        return;
    }

    do {
        error = otCoapMessageInitResponse(p_response, p_request_message,
            OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_CHANGED);
        if (error != OT_ERROR_NONE) { break; }

        error = otCoapSendResponse(p_instance, p_response, p_message_info);
    } while (false);

    if (error != OT_ERROR_NONE) {
        printk("Failed to send store data response: %d\n", error);
        otMessageFree(p_response);
    }
}

// Assigns a fixed IPv6 address to the server (fdde:ad00:beef:0::1).
void addIPv6Address(void) {
    otInstance *myInstance = openthread_get_default_instance();
    otNetifAddress aAddress;
    const otMeshLocalPrefix *ml_prefix = otThreadGetMeshLocalPrefix(myInstance);
    uint8_t interfaceID[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

    memcpy(&aAddress.mAddress.mFields.m8[0], ml_prefix, 8);
    memcpy(&aAddress.mAddress.mFields.m8[8], interfaceID, 8);

    otError error = otIp6AddUnicastAddress(myInstance, &aAddress);
    if (error != OT_ERROR_NONE)
        printk("addIPAdress Error: %d\n", error);
}
// Initializes the CoAP server and registers the resource.
void coap_init(void) {
    otError error;
    otInstance *p_instance = openthread_get_default_instance();
    m_storedata_resource.mContext = p_instance;

    do {
        error = otCoapStart(p_instance, OT_DEFAULT_COAP_PORT);
        if (error != OT_ERROR_NONE) { break; }

        otCoapAddResource(p_instance, &m_storedata_resource);
    } while(false);

    if (error == OT_ERROR_NONE) {
        printk("CoAP server started successfully.\n");
    } else {
        printk("Failed to start CoAP server: %d\n", error);
    }
}

int main(void) {
    addIPv6Address();
    coap_init();
    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return -1;
    }
    printk("UART device is ready\n");
    
    while (1) {
       
    }
}
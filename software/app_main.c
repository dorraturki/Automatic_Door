/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"

// Configuration constants
static const char *TAG = "mqtt5_dorra";
static const char *MQTT_BROKER_URI = "mqtt://test.mosquitto.org";
static const char *TOPIC_STATUS = "/dorra/status";
static const char *TOPIC_CONTROL = "/dorra/control";

// LED Configuration
#define LED_GPIO_PIN    GPIO_NUM_2  // Built-in LED on most ESP32 boards
#define LED_ON_LEVEL    1           // 1 for active high, 0 for active low

// Message constants
static const char *MSG_CONNECTED = "ESP Connected";
static const char *MSG_DISCONNECTED = "ESP Disconnected";
static const char *MSG_OPEN_RESPONSE = "it's open";
static const char *MSG_CLOSE_RESPONSE = "it's closed";
static const char *CMD_OPEN = "open";
static const char *CMD_CLOSE = "close";

// Function prototypes
static void log_error_if_nonzero(const char *message, int error_code);
static void led_init(void);
static void led_set_state(bool state);
static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void handle_mqtt_connected(esp_mqtt_client_handle_t client);
static void handle_mqtt_data(esp_mqtt_event_handle_t event, esp_mqtt_client_handle_t client);
static void process_control_message(const char *data, int data_len, esp_mqtt_client_handle_t client);
static void mqtt5_app_start(void);

/**
 * @brief Log error if error code is non-zero
 */
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/**
 * @brief Initialize LED GPIO
 */
static void led_init(void)
{
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << LED_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    ESP_ERROR_CHECK(gpio_config(&led_config));
    
    // Initialize LED to OFF state
    led_set_state(false);
    ESP_LOGI(TAG, "LED initialized on GPIO %d", LED_GPIO_PIN);
}

/**
 * @brief Set LED state
 * @param state true to turn LED on, false to turn LED off
 */
static void led_set_state(bool state)
{
    gpio_set_level(LED_GPIO_PIN, state ? LED_ON_LEVEL : !LED_ON_LEVEL);
    ESP_LOGI(TAG, "LED turned %s", state ? "ON" : "OFF");
}

/**
 * @brief Handle MQTT connected event
 */
static void handle_mqtt_connected(esp_mqtt_client_handle_t client)
{
    int msg_id;
    
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    
    // Send connection status message
    msg_id = esp_mqtt_client_publish(client, TOPIC_STATUS, MSG_CONNECTED, 0, 1, 0);
    ESP_LOGI(TAG, "Published connection message to %s, msg_id=%d", TOPIC_STATUS, msg_id);
    
    // Subscribe to control topic
    msg_id = esp_mqtt_client_subscribe(client, TOPIC_CONTROL, 1);
    ESP_LOGI(TAG, "Subscribed to %s, msg_id=%d", TOPIC_CONTROL, msg_id);
}

/**
 * @brief Process control messages and send appropriate responses
 */
static void process_control_message(const char *data, int data_len, esp_mqtt_client_handle_t client)
{
    int msg_id;
    
    ESP_LOGI(TAG, "Processing control message: %.*s", data_len, data);
    
    if (strncmp(data, CMD_OPEN, data_len) == 0) {
        ESP_LOGI(TAG, "Command: OPEN received");
        
        // Turn LED ON
        led_set_state(true);
        
        // Send response
        msg_id = esp_mqtt_client_publish(client, TOPIC_STATUS, MSG_OPEN_RESPONSE, 0, 1, 0);
        ESP_LOGI(TAG, "Sent OPEN response: '%s', msg_id=%d", MSG_OPEN_RESPONSE, msg_id);
    }
    else if (strncmp(data, CMD_CLOSE, data_len) == 0) {
        ESP_LOGI(TAG, "Command: CLOSE received");
        
        // Turn LED OFF
        led_set_state(false);
        
        // Send response
        msg_id = esp_mqtt_client_publish(client, TOPIC_STATUS, MSG_CLOSE_RESPONSE, 0, 1, 0);
        ESP_LOGI(TAG, "Sent CLOSE response: '%s', msg_id=%d", MSG_CLOSE_RESPONSE, msg_id);
    }
    else {
        ESP_LOGW(TAG, "Unknown command received: %.*s", data_len, data);
    }
}

/**
 * @brief Handle MQTT data received event
 */
static void handle_mqtt_data(esp_mqtt_event_handle_t event, esp_mqtt_client_handle_t client)
{
    ESP_LOGI(TAG, "MQTT_EVENT_DATA - Message received!");
    ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
    ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
    
    // Process messages from control topic
    if (strncmp(event->topic, TOPIC_CONTROL, event->topic_len) == 0) {
        process_control_message(event->data, event->data_len, client);
    }
}

/**
 * @brief Event handler registered to receive MQTT events
 */
static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        handle_mqtt_connected(client);
        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
        
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_DATA:
        handle_mqtt_data(event, client);
        break;
        
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        ESP_LOGI(TAG, "MQTT5 return code is %d", event->error_handle->connect_return_code);
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
        
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

/**
 * @brief Initialize and start MQTT5 client
 */
static void mqtt5_app_start(void)
{
    esp_mqtt_client_config_t mqtt5_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .network.disable_auto_reconnect = false,
        .session.last_will.topic = TOPIC_STATUS,
        .session.last_will.msg = MSG_DISCONNECTED,
        .session.last_will.msg_len = strlen(MSG_DISCONNECTED),
        .session.last_will.qos = 1,
        .session.last_will.retain = true,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt5_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL);
    esp_mqtt_client_start(client);
}

/**
 * @brief Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    // Set log levels
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);

    // Initialize system components
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize LED
    led_init();

    // Connect to WiFi
    ESP_ERROR_CHECK(example_connect());

    // Start MQTT client
    mqtt5_app_start();
}
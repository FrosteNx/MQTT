#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "driver/gpio.h"

#define TAG "mqtt_sensors"
esp_mqtt_client_handle_t client;
char mac_address[18]; // MAC w formacie tekstowym

// Pobranie adresu MAC
void get_mac_address(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(mac_address, sizeof(mac_address), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "Device MAC Address: %s", mac_address);
}

// Zadanie dla BH1750
void bh1750_task(void *pvParameters)
{
    while (1)
    {
        int value = rand() % 65536; // Zakres 1–65535 lx
        char message[16];
        snprintf(message, sizeof(message), "%d", value);

        char topic[50];
        snprintf(topic, sizeof(topic), "%s/bh1750/light", mac_address);
        esp_mqtt_client_publish(client, topic, message, 0, 1, 0);
        ESP_LOGI(TAG, "Published to %s: %s", topic, message);

        vTaskDelay(10000 / portTICK_PERIOD_MS); // Co 10 sekund
    }
}

// Zadanie dla KY037 (analogowe)
void ky037_analog_task(void *pvParameters)
{
    while (1)
    {
        int value = rand() % 1024; // Zakres 0–1023
        char message[16];
        snprintf(message, sizeof(message), "%d", value);

        char topic[50];
        snprintf(topic, sizeof(topic), "%s/ky037/analog", mac_address);
        esp_mqtt_client_publish(client, topic, message, 0, 1, 0);
        ESP_LOGI(TAG, "Published to %s: %s", topic, message);

        vTaskDelay(10000 / portTICK_PERIOD_MS); // Co 10 sekund
    }
}

// Zadanie dla KY037 (cyfrowe)
void ky037_digital_task(void *pvParameters)
{
    while (1)
    {
        int value = rand() % 2; // Cyfrowo: 0 lub 1
        char message[16];
        snprintf(message, sizeof(message), "%d", value);

        char topic[50];
        snprintf(topic, sizeof(topic), "%s/ky037/digital", mac_address);
        esp_mqtt_client_publish(client, topic, message, 0, 1, 0);
        ESP_LOGI(TAG, "Published to %s: %s", topic, message);

        vTaskDelay(10000 / portTICK_PERIOD_MS); // Co 10 sekund
    }
}

// Zadanie dla ambilight
void ambilight_task(void *pvParameters)
{
    while (1)
    {
        int value = rand() % 2; // Ambilight: 0 lub 1
        char message[16];
        snprintf(message, sizeof(message), "%d", value);

        char topic[50];
        snprintf(topic, sizeof(topic), "%s/ambilight/state", mac_address);
        esp_mqtt_client_publish(client, topic, message, 0, 1, 0);
        ESP_LOGI(TAG, "Published to %s: %s", topic, message);

        vTaskDelay(10000 / portTICK_PERIOD_MS); // Co 10 sekund
    }
}

// Zadanie dla RGB
void rgb_task(void *pvParameters)
{
    while (1)
    {
        int r = rand() % 256; // Zakres 0–255
        int g = rand() % 256;
        int b = rand() % 256;

        char message[50];
        snprintf(message, sizeof(message), "%d,%d,%d", r, g, b);

        char topic[50];
        snprintf(topic, sizeof(topic), "%s/rgb/values", mac_address);
        esp_mqtt_client_publish(client, topic, message, 0, 1, 0);
        ESP_LOGI(TAG, "Published to %s: %s", topic, message);

        vTaskDelay(10000 / portTICK_PERIOD_MS); // Co 10 sekund
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(client, "/sensor/command", 1);
        ESP_LOGI(TAG, "Subscribed to /sensor/command");
        break;

    case MQTT_EVENT_DATA:
        if (strncmp(event->topic, "/sensor/command", event->topic_len) == 0)
        {
            if (strncmp(event->data, "readData", event->data_len) == 0)
            {
                ESP_LOGI(TAG, "Starting sensor tasks...");
                xTaskCreate(bh1750_task, "bh1750_task", 4096, NULL, 5, NULL);
                xTaskCreate(ky037_analog_task, "ky037_analog_task", 4096, NULL, 5, NULL);
                xTaskCreate(ky037_digital_task, "ky037_digital_task", 4096, NULL, 5, NULL);
                xTaskCreate(ambilight_task, "ambilight_task", 4096, NULL, 5, NULL); 
                xTaskCreate(rgb_task, "rgb_task", 4096, NULL, 5, NULL);            
            }
        }
        break;

    default:
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://192.168.43.6:1883",
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    example_connect();
    get_mac_address();
    mqtt_app_start();
}

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "pico_webhook.h"
#include "secrets.h" 

#define TEMP_THRESHOLD_C 35.0f
#define ALERT_COOLDOWN_MS 60000 // Wait 60 seconds before sending another alert

float get_onboard_temperature() {
    adc_select_input(4); 
    uint16_t adc_raw = adc_read();
    float conversion_factor = 3.3f / (1 << 12);
    float voltage = adc_raw * conversion_factor;
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

void my_webhook_cb(webhook_status_t status, int http_status, void *user_arg) {
    if (status == WEBHOOK_SUCCESS) {
        printf("Threshold webhook delivered! HTTP Status: %d\n", http_status);
    } else {
        printf("Threshold webhook failed. Error Code: %d\n", status);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000); 

    adc_init();
    adc_set_temp_sensor_enabled(true);

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_init() != 0) return 1;
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_MIXED_PSK, 30000) != 0) {
        printf("Wi-Fi Connection failed!\n");
        return 1;
    }
    printf("Wi-Fi Connected!\n");

    webhook_init();
    
    uint32_t last_alert_time = 0;
    char json_payload[128];

    printf("Monitoring CPU temperature. Alert threshold: %.1f °C\n", TEMP_THRESHOLD_C);

    while (true) {
        float temp = get_onboard_temperature();
        
        if (temp > TEMP_THRESHOLD_C) {
            uint32_t current_time = to_ms_since_boot(get_absolute_time());
            
            // Check if this is the first alert, or if the cooldown has passed
            if (last_alert_time == 0 || (current_time - last_alert_time) > ALERT_COOLDOWN_MS) {
                last_alert_time = current_time; 
                
                printf("Threshold exceeded (%.1f C)! Sending webhook...\n", temp);
                
                snprintf(json_payload, sizeof(json_payload), 
                         "{\"content\": \"🔥 **High Temp Alert!** CPU is at **%.1f °C**\"}", temp);
                
                webhook_request_t req = {
                    .host = WEBHOOK_HOST,
                    .path = WEBHOOK_PATH,
                    .json_payload = json_payload,
                    .callback = my_webhook_cb,
                    .user_arg = NULL
                };
                
                webhook_send(&req);
            }
        }
        
        // Yield for 2 seconds before checking temperature again
        sleep_ms(2000); 
    }

    return 0;
}
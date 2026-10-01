#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico_webhook.h"
#include "secrets.h" // Ensure WIFI_SSID, WIFI_PASSWORD, WEBHOOK_HOST, WEBHOOK_PATH are defined

#define BUTTON_PIN 15

void my_webhook_cb(webhook_status_t status, int http_status, void *user_arg) {
    if (status == WEBHOOK_SUCCESS) {
        printf("Button webhook delivered! HTTP Status: %d\n", http_status);
    } else {
        printf("Button webhook failed. Error Code: %d\n", status);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000); 

    // Initialize the GPIO button pin with an internal pull-up
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_init() != 0) return 1;
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_MIXED_PSK, 30000) != 0) {
        printf("Wi-Fi Connection failed!\n");
        return 1;
    }
    printf("Wi-Fi Connected!\n");

    webhook_init();
    
    // Since the pin is pulled up, the unpressed state is 'true'
    bool last_button_state = true; 
    char json_payload[128];

    printf("Ready! Press the button on GPIO %d to send an alert...\n", BUTTON_PIN);

    while (true) {
        bool current_state = gpio_get(BUTTON_PIN);
        
        // Trigger only on the press transition (HIGH to LOW)
        if (last_button_state && !current_state) {
            printf("Button Pressed! Sending webhook...\n");
            
            snprintf(json_payload, sizeof(json_payload), 
                     "{\"content\": \"🔘 **Alert:** Physical button was pressed on GPIO %d!\"}", BUTTON_PIN);
            
            webhook_request_t req = {
                .host = WEBHOOK_HOST,
                .path = WEBHOOK_PATH,
                .json_payload = json_payload,
                .callback = my_webhook_cb,
                .user_arg = NULL
            };
            
            if (!webhook_send(&req)) {
                printf("Failed to queue webhook (is one already sending?).\n");
            }
        }
        
        last_button_state = current_state;
        sleep_ms(50); // Debounce delay
    }

    return 0;
}
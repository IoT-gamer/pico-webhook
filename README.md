# pico_webhook

A lightweight, non-blocking C library for the Raspberry Pi Pico W and Pico 2 W that sends secure HTTPS webhooks (such as Discord, Slack, or generic REST APIs).

Built directly on top of the Pico SDK's native `lwIP` and `mbedTLS` stacks, this library handles DNS resolution, TLS handshaking, and asynchronous HTTP POST requests without stalling your main application or sensor loops.

## Features
* **Fully Asynchronous:** Uses lwIP's callback-driven TCP API (`altcp`). Your main loop never blocks waiting for network replies.
* **TLS / HTTPS Support:** Securely communicates with modern web APIs using mbedTLS.
* **Dynamic DNS:** Resolves hostnames dynamically per request, preventing failures when cloud services (like Discord) rotate their IP addresses.
* **Drop-in CMake Integration:** Designed to be easily included via FetchContent.

## Prerequisites
* Raspberry Pi Pico SDK (Tested on v2.3.1)
* A board with the CYW43 Wi-Fi chip (Pico W or Pico 2 W) 

## Installation
Include the library in your project's `CMakeLists.txt` using `FetchContent`.

```cmake

include(FetchContent)

FetchContent_Declare(
    pico_webhook
    GIT_REPOSITORY https://github.com/IoT-gamer/pico-webhook.git
    GIT_TAG        main 
)
FetchContent_MakeAvailable(pico_webhook)

# CRITICAL: Inject your application's configuration directory into the library
# so it can find your custom lwipopts.h and mbedtls_config.h
target_include_directories(pico_webhook PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Link the library to your main executable
target_link_libraries(your_executable_name
    pico_stdlib
    pico_webhook
)
```
## Required Networking Configuration
This library relies on your application's `lwipopts.h` and `mbedtls_config.h`. You must enable Application Layer TCP (ALTCP) and TLS in your `lwipopts.h`:

```c
// Add these to your project's lwipopts.h
#define LWIP_ALTCP                  1
#define LWIP_ALTCP_TLS              1
#define LWIP_ALTCP_TLS_MBEDTLS      1
#define LWIP_DNS                    1
```

## Usage Example
Ensure your Wi-Fi is connected before initializing the webhook client.

```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico_webhook.h"

// Optional callback to handle the response
void on_webhook_complete(webhook_status_t status, int http_status, void *user_arg) {
    if (status == WEBHOOK_SUCCESS) {
        printf("Success! HTTP Code: %d\n", http_status);
    } else {
        printf("Failed to send webhook. Error: %d\n", status);
    }
}

int main() {
    stdio_init_all();
    
    // Initialize Wi-Fi
    cyw43_arch_init();
    cyw43_arch_enable_sta_mode();
    cyw43_arch_wifi_connect_timeout_ms("YOUR_SSID", "YOUR_PASSWORD", CYW43_AUTH_WPA2_MIXED_PSK, 30000);

    // Initialize the Webhook TLS context (Call exactly once)
    webhook_init();

    // Prepare the request
    webhook_request_t req = {
        .host = "discord.com",
        .path = "/api/webhooks/YOUR_WEBHOOK_URL",
        .json_payload = "{\"content\": \"Hello from the Pico W!\"}",
        .callback = on_webhook_complete,
        .user_arg = NULL
    };

    // Send the request asynchronously
    if (!webhook_send(&req)) {
        printf("Client is busy or missing configuration.\n");
    }

    // Main loop
    while (true) {
        // Your application logic (sensor reading, etc.) goes here
        sleep_ms(100); 
    }

    return 0;
}
```

## Included Examples
This repository includes practical examples demonstrating different trigger conditions.

* `gpio_button`: Triggers a webhook when a physical button on GPIO 15 is pressed.
* `temp_threshold`: Reads the RP2040/RP2350 internal ADC temperature sensor and fires an alert if it exceeds a specified limit, utilizing a cooldown timer to prevent spam.

(Note: The examples require a `secrets.h` file in the project root defining WIFI_SSID, WIFI_PASSWORD, WEBHOOK_HOST, and WEBHOOK_PATH and `lwipopts.h` and `mbedtls_config.h`. See `examples/common`).

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgements
- [pico-examples http_client](https://github.com/raspberrypi/pico-examples/tree/master/pico_w/wifi/http_client)
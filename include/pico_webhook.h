#pragma once

#include <stdbool.h>
#include <stdint.h>

// Status codes to pass back to the application
typedef enum {
    WEBHOOK_SUCCESS = 0,
    WEBHOOK_ERR_DNS_FAILED,
    WEBHOOK_ERR_CONNECTION,
    WEBHOOK_ERR_TIMEOUT,
    WEBHOOK_ERR_OOM
} webhook_status_t;

// Callback signature triggered when the webhook finishes (or fails)
typedef void (*webhook_cb_t)(webhook_status_t status, int http_status_code, void *user_arg);

// Configuration struct for the request
typedef struct {
    const char *host;          // e.g., "discord.com"
    const char *path;          // e.g., "/api/webhooks/..."
    const char *json_payload;  // The actual message body
    webhook_cb_t callback;     // Function to call on completion
    void *user_arg;            // Optional context (can be NULL)
} webhook_request_t;

// Initialize the TLS context (call once at boot)
void webhook_init(void);

// Dispatch a webhook asynchronously
bool webhook_send(const webhook_request_t *request);
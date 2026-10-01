#include "pico_webhook.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"

// --- Internal State ---
static struct altcp_tls_config *tls_config = NULL;
static bool is_busy = false;
static webhook_request_t current_req;
static ip_addr_t server_ip;
static struct altcp_pcb *tls_pcb = NULL;
static int parsed_http_status = 0;

// --- Forward Declarations ---
static err_t tls_connected_cb(void *arg, struct altcp_pcb *pcb, err_t err);
static void dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg);

// --- Helpers ---
static void cleanup_and_callback(webhook_status_t status) {
    if (tls_pcb) {
        altcp_arg(tls_pcb, NULL);
        altcp_recv(tls_pcb, NULL);
        altcp_err(tls_pcb, NULL);
        altcp_close(tls_pcb);
        tls_pcb = NULL;
    }
    
    is_busy = false;
    
    if (current_req.callback) {
        current_req.callback(status, parsed_http_status, current_req.user_arg);
    }
}

// --- Initialization ---
void webhook_init(void) {
    if (!tls_config) {
        // Initialize TLS config exactly ONCE for the lifetime of the program[cite: 1]
        tls_config = altcp_tls_create_config_client(NULL, 0);
    }
}

// --- Callbacks ---
static void tls_err_cb(void *arg, err_t err) {
    printf("[Webhook] Connection error: %d\n", err);
    cleanup_and_callback(WEBHOOK_ERR_CONNECTION);
}

static err_t tls_recv_cb(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        // Connection closed by the remote host
        cleanup_and_callback(WEBHOOK_SUCCESS);
        return ERR_OK;
    }

    // Acknowledge the received data so lwIP can slide the TCP window[cite: 1]
    altcp_recved(pcb, p->tot_len);

    // Naive HTTP status code parsing (e.g., looking for "HTTP/1.1 204")
    if (parsed_http_status == 0 && p->tot_len >= 12) {
        char buffer[16] = {0};
        uint16_t len = p->tot_len < 15 ? p->tot_len : 15;
        pbuf_copy_partial(p, buffer, len, 0);
        
        if (strncmp(buffer, "HTTP/1.", 7) == 0) {
            parsed_http_status = atoi(&buffer[9]);
        }
    }

    pbuf_free(p); // Free the buffer memory[cite: 1]
    return ERR_OK;
}

static err_t tls_connected_cb(void *arg, struct altcp_pcb *pcb, err_t err) {
    if (err != ERR_OK) {
        cleanup_and_callback(WEBHOOK_ERR_CONNECTION);
        return err;
    }

    char request[1024];
    int req_len = snprintf(request, sizeof(request),
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        current_req.path, current_req.host, 
        strlen(current_req.json_payload), current_req.json_payload
    );

    if (req_len >= sizeof(request)) {
        // Payload too large for our buffer
        cleanup_and_callback(WEBHOOK_ERR_OOM);
        return ERR_OK;
    }

    // Write and flush the data to the TCP stack[cite: 1]
    err_t write_err = altcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    if (write_err == ERR_OK) {
        altcp_output(pcb);
    } else {
        cleanup_and_callback(WEBHOOK_ERR_CONNECTION);
    }

    return ERR_OK;
}

static void connect_to_server() {
    tls_pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY);
    if (tls_pcb == NULL) {
        cleanup_and_callback(WEBHOOK_ERR_OOM);
        return;
    }

    // Set Server Name Indication (SNI) which is required by most HTTPS endpoints
    mbedtls_ssl_set_hostname((mbedtls_ssl_context*)altcp_tls_context(tls_pcb), current_req.host);

    altcp_arg(tls_pcb, NULL);
    altcp_recv(tls_pcb, tls_recv_cb);
    altcp_err(tls_pcb, tls_err_cb);
    
    err_t conn_err = altcp_connect(tls_pcb, &server_ip, 443, tls_connected_cb);
    if (conn_err != ERR_OK) {
        cleanup_and_callback(WEBHOOK_ERR_CONNECTION);
    }
}

static void dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (ipaddr) {
        server_ip = *ipaddr;
        connect_to_server();
    } else {
        cleanup_and_callback(WEBHOOK_ERR_DNS_FAILED);
    }
}

// --- Public API ---
bool webhook_send(const webhook_request_t *request) {
    if (tls_config == NULL || request == NULL || is_busy) {
        return false;
    }

    is_busy = true;
    current_req = *request;
    parsed_http_status = 0;

    cyw43_arch_lwip_begin();

    // Dynamically resolve the IP rather than relying on a static boot-time cache
    err_t dns_err = dns_gethostbyname(current_req.host, &server_ip, dns_found_cb, NULL);
    
    if (dns_err == ERR_OK) {
        // DNS was already cached, proceed directly to connection
        connect_to_server();
    } else if (dns_err != ERR_INPROGRESS) {
        // Immediate DNS failure
        cyw43_arch_lwip_end();
        cleanup_and_callback(WEBHOOK_ERR_DNS_FAILED);
        return false;
    }

    cyw43_arch_lwip_end();
    return true;
}
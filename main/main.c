// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 GMIC AI Inc.
// Portions Copyright LiveKit, Inc. See NOTICE for what changed.

#include <string.h>
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "livekit.h"
#include "livekit_example_utils.h"

#include "board.h"
#include "example.h"
#include "media.h"

// Ships with no credentials baked in. Print what has to be filled in and where,
// so whoever powers the board up first knows the next step without reading code.
static void print_setup_banner(void)
{
    bool wifi_set = strlen(CONFIG_LK_EXAMPLE_WIFI_SSID) > 0;
#if CONFIG_LK_EXAMPLE_USE_SANDBOX
    bool lk_set = strlen(CONFIG_LK_EXAMPLE_SANDBOX_ID) > 0;
    const char *lk_what = "LiveKit sandbox ID";
#else
    bool lk_set = strlen(CONFIG_LK_EXAMPLE_TOKEN) > 0;
    const char *lk_what = "LiveKit server URL + token";
#endif
    if (wifi_set && lk_set) {
        return;
    }
    printf("\n"
           "=========================================================\n"
           " GMIC HA-TOYMD  -  LiveKit reference firmware\n"
           " Shipped unconfigured. Two values are still needed:\n"
           "   %s Wi-Fi SSID and password   (2.4 GHz, WPA2 only)\n"
           "   %s %s\n"
           " Set both with:  idf.py menuconfig  ->  Example Configuration\n"
           " then:           idf.py build flash monitor\n"
           " Hardware (mic, speaker, codec) is already verified below.\n"
           "=========================================================\n\n",
           wifi_set ? "[ok]" : "[--]", lk_set ? "[ok]" : "[--]", lk_what);
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    print_setup_banner();

    livekit_system_init();
    board_init();
    media_init();

    esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(
        2, ESP_SNTP_SERVER_LIST("time.google.com", "pool.ntp.org"));
    esp_netif_sntp_init(&sntp_config);

    if (lk_example_network_connect()) {
        join_room();
    }
}

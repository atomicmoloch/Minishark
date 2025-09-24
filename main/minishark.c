#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"

#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "display_handler.h"
#include "mac_database.h"

static const char *TAG = "scan_test";

uint64_t registered_MACs[64] = {0};

/***********************************************************************
 *
 * FUNCTION:     identify_mac
 *
 * DESCRIPTION:  Conducts a binary source on oid database
 *
 * PARAMETERS:   MAC address
 *
 * RETURNED:     Index to corresponding manufacturer string
 *
 ***********************************************************************/
uint16_t identify_mac(uint64_t address) {
    uint8_t offset;
    uint64_t address_prefix;
    oui_entry_t entry;
    size_t l, r, med;

    l = 0;
    r = oui_table_length - 1;

    while (l <= r) {
        med = (l + r) / 2;
        entry = oui_table[med];

        offset = 8 * (6 - entry.length);

        address_prefix = address >>offset;

        if (address_prefix == entry.prefix) {
            ESP_LOGI(TAG, "MAC address identified as %s", manufacturers[i]);
            return entry.man_id;
        }
        else if (address_prefix > entry.prefix) {
            l = med + 1;
        }
        else {
            r = med - 1;
        }
    }
    return 0;
}

/***********************************************************************
 *
 * FUNCTION:     already_seen
 *
 * DESCRIPTION:  Checks of MAC address is in a small cache
 *               (to prevent too much repetition
 *
 * PARAMETERS:   MAC address
 *
 * RETURNED:     true if address in cache, otherwise false. Adds address to cache.
 *
 ***********************************************************************/
bool already_seen(uint64_t address) {
    uint8_t i;

    for (i = 0; i < 64; i++) {
        if (registered_MACs[i] == address) return true;
        if (registered_MACs[i] == 0) break;
    }

    if (i == 64) {
        memset(registered_MACs, 0, sizeof(registered_MACs));
        registered_MACs[0] = address;
        ESP_LOGI(TAG, "MAC cache full, resetting");
        return false;
    }
    else {
        registered_MACs[i] = address;
        return false;
    }
}

/***********************************************************************
 *
 * FUNCTION:     packet_handler
 *
 * DESCRIPTION:  Extracts source and destination MAC addresses from packet
 *
 * PARAMETERS:   pointer to wifi promiscuous packet, and type.
 *               Packet could theoretically be void, but will not be because of upstream filtering
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
void packet_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
    uint64_t src_mac;
    uint64_t dst_mac;
    uint16_t man_id;
    const wifi_promiscuous_pkt_t *pkt = (const wifi_promiscuous_pkt_t*)buf;
    const uint8_t *frame = pkt->payload;

    if (pkt->rx_ctrl.sig_len < 24) return; // exits if too short to contain MAC header

    const uint8_t *addr1 = frame + 4;
    const uint8_t *addr2 = frame + 10;

    src_mac =
        ((uint64_t)addr1[0] << 40) |
        ((uint64_t)addr1[1] << 32) |
        ((uint64_t)addr1[2] << 24) |
        ((uint64_t)addr1[3] << 16) |
        ((uint64_t)addr1[4] <<  8) |
        ((uint64_t)addr1[5]);
    dst_mac =
        ((uint64_t)addr2[0] << 40) |
        ((uint64_t)addr2[1] << 32) |
        ((uint64_t)addr2[2] << 24) |
        ((uint64_t)addr2[3] << 16) |
        ((uint64_t)addr2[4] <<  8) |
        ((uint64_t)addr2[5]);

    if (!already_seen(src_mac)) {
        ESP_LOGI(TAG, "SRC MAC Address Detected: %02X:%02X:%02X:%02X:%02X:%02X",
                 addr1[0],
                 addr1[1],
                 addr1[2],
                 addr1[3],
                 addr1[4],
                 addr1[5]);
        man_id = identify_mac(src_mac);
        display_write_16pt(manufacturers[man_id]);
    }

    if (!already_seen(dst_mac)) {
        ESP_LOGI(TAG, "DST MAC Address Detected: %02X:%02X:%02X:%02X:%02X:%02X",
                 addr2[0],
                 addr2[1],
                 addr2[2],
                 addr2[3],
                 addr2[4],
                 addr2[5]);
        man_id = identify_mac(dst_mac);
        display_write_16pt(manufacturers[man_id]);
    }
}

/***********************************************************************
 *
 * FUNCTION:     wifi
 *
 * DESCRIPTION:  Initializes esp32 wifi in promiscuous mode
 *
 * PARAMETERS:   Channel to initialize scanning on
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
void wifi(uint8_t channel) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init( &cfg ));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous( true ));

    const wifi_promiscuous_filter_t filter = { .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA  };
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter)); //Filters out control and management packets
    esp_wifi_set_promiscuous_rx_cb(packet_handler); //packet handler called every time a packet is received

    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

/***********************************************************************
 *
 * FUNCTION:     mountSPIFFS
 *
 * DESCRIPTION:  Boilerplate SPIFFS mounting code
 *
 * PARAMETERS:   path to mount on, partition label, maximum number of simultaneously-opened files
 *
 * RETURNED:     esp_err_t
 *
 ***********************************************************************/
esp_err_t mountSPIFFS(char * path, char * label, int max_files) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = path,
        .partition_label = label,
        .max_files = max_files,
        .format_if_mount_failed = false, //true for corrupted filesystem recovery
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret ==ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret== ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
        }
        return ret;
    }

    ESP_LOGI(TAG, "Performing SPIFFS_check().");
    ret = esp_spiffs_check(conf.partition_label);
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return ret;
    } else {
            ESP_LOGI(TAG, "SPIFFS_check() successful");
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if ( ret != ESP_OK ) {
        ESP_LOGE(TAG,"Failed to get SPIFFS partition information (%s)",esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG,"Mount %s to %s success", path, label);
        ESP_LOGI(TAG,"Partition size: total: %d, used: %d", total, used);
    }

    return ret;
}


void app_main(void) {
    // Mounts SPIFFS font partition and initializes display handler
    mountSPIFFS( "/fonts", "storage1" , 7 );
    display_init();

    // Initialize NVS boilerplate code
    esp_err_t ret = nvs_flash_init();
    if ( ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND ) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_LOGI(TAG, "\nOpening Non-Volatile Storage (NVS) handle...");
    nvs_handle_t st_handle;
    ret = nvs_open("scan_test", NVS_READWRITE, &st_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(ret));
        return;
    }

    //Increments channel at each startup
    uint8_t channel = 0;
    ESP_LOGI(TAG, "\nReading stored channel from NVS...");
    ret = nvs_get_u8(st_handle, "channel", &channel);
    switch (ret) {
        case ESP_OK:
            ESP_LOGI(TAG, "Read channel = %u, incrementing", channel);
            channel++;
            if (channel > CONFIG_MAX_CHANNEL) {
                channel = CONFIG_MIN_CHANNEL;
            }
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGW(TAG, "Channel not initialized, setting to MIN_CHANNEL value %u", CONFIG_MIN_CHANNEL);
            channel = CONFIG_MIN_CHANNEL;
            break;
        default:
            ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "\nWriting to NVS...");
    ret = nvs_set_u8(st_handle, "channel", channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write counter");
    }

    ret = nvs_commit(st_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS changes");
    }

    nvs_close(st_handle);
    ESP_LOGI(TAG, "NVS handle closed.");

    char channel_notice[13];
    snprintf(channel_notice, 13, "Channel: %u", channel);
    display_write_16pt(channel_notice);

    wifi(channel);
}

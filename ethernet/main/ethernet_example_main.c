/* ethernet Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_err.h"
#include "esp_event_loop.h"
#include "esp_event.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_eth.h"
#include <lwip/sockets.h>

#include "rom/ets_sys.h"
#include "rom/gpio.h"

#include "soc/dport_reg.h"
#include "soc/io_mux_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"

#include "tcpip_adapter.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "soc/emac_ex_reg.h"
#include "driver/periph_ctrl.h"


#ifdef CONFIG_PHY_LAN8720
#include "eth_phy/phy_lan8720.h"
#define DEFAULT_ETHERNET_PHY_CONFIG phy_lan8720_default_ethernet_config
#endif
#ifdef CONFIG_PHY_TLK110
#include "eth_phy/phy_tlk110.h"
#define DEFAULT_ETHERNET_PHY_CONFIG phy_tlk110_default_ethernet_config
#endif
#ifdef CONFIG_PHY_KSZ8081
#include "eth_phy/phy_ksz8081.h"
#define DEFAULT_ETHERNET_PHY_CONFIG phy_ksz8081_default_ethernet_config
#endif

static const char *TAG = "eth_example";

#define PIN_PHY_POWER CONFIG_PHY_POWER_PIN
#define PIN_SMI_MDC   CONFIG_PHY_SMI_MDC_PIN
#define PIN_SMI_MDIO  CONFIG_PHY_SMI_MDIO_PIN

#ifdef CONFIG_PHY_USE_POWER_PIN
/* This replaces the default PHY power on/off function with one that
   also uses a GPIO for power on/off.

   If this GPIO is not connected on your device (and PHY is always powered), you can use the default PHY-specific power
   on/off function rather than overriding with this one.
*/
static void phy_device_power_enable_via_gpio(bool enable)
{
    assert(DEFAULT_ETHERNET_PHY_CONFIG.phy_power_enable);

    if (!enable) {
        /* Do the PHY-specific power_enable(false) function before powering down */
        DEFAULT_ETHERNET_PHY_CONFIG.phy_power_enable(false);
    }

    gpio_pad_select_gpio(PIN_PHY_POWER);
    gpio_set_direction(PIN_PHY_POWER,GPIO_MODE_OUTPUT);
    if(enable == true) {
        gpio_set_level(PIN_PHY_POWER, 1);
        ESP_LOGD(TAG, "phy_device_power_enable(TRUE)");
    } else {
        gpio_set_level(PIN_PHY_POWER, 0);
        ESP_LOGD(TAG, "power_enable(FALSE)");
    }

    // Allow the power up/down to take effect, min 300us
    vTaskDelay(1);

    if (enable) {
        /* Run the PHY-specific power on operations now the PHY has power */
        DEFAULT_ETHERNET_PHY_CONFIG.phy_power_enable(true);
    }
}
#endif

static void eth_gpio_config_rmii(void)
{
    // RMII data pins are fixed:
    // TXD0 = GPIO19
    // TXD1 = GPIO22
    // TX_EN = GPIO21
    // RXD0 = GPIO25
    // RXD1 = GPIO26
    // CLK == GPIO0
    phy_rmii_configure_data_interface_pins();
    // MDC is GPIO 23, MDIO is GPIO 18
    phy_rmii_smi_configure_pins(PIN_SMI_MDC, PIN_SMI_MDIO);
}



extern int _ETH_RX;
extern int _ETH_TX;

void eth_task(void *pvParameter)
{
    tcpip_adapter_ip_info_t ip;
    memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
    vTaskDelay(2000 / portTICK_PERIOD_MS);

/*

		IP4_ADDR(&ip.ip, 192,168,11,200);
		IP4_ADDR(&ip.gw, 192,168,11,1);
		IP4_ADDR(&ip.netmask, 255,255,255,0);

	esp_err_t ret = ESP_OK;
    tcpip_adapter_ip_info_t ipInfo;
#define inet_ntop4(af,src,dst,size) \
    (((af) == AF_INET) ? ip4addr_ntoa_r((const ip4_addr_t*)(src),(dst),(size)) : NULL)
#define inet_pton4(af,src,dst) \
    (((af) == AF_INET) ? ip4addr_aton((src),(ip4_addr_t*)(dst)) : 0)



    // myIp -> structure that save your static ip settings
    inet_pton4(AF_INET, &ip.ip,      &ipInfo.ip);
    inet_pton4(AF_INET, &ip.gw,       &ipInfo.gw);
    inet_pton4(AF_INET, &ip.netmask, &ipInfo.netmask);
*/
/*
    ESP_LOGI(TAG, "dhcp client stop RESULT: %d", ret);
    tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_ETH, &ipInfo);
*/


//	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_ETH);
	int iter = 0;
    while (1) {

        vTaskDelay(2000 / portTICK_PERIOD_MS);

        if (tcpip_adapter_get_ip_info(ESP_IF_ETH, &ip) == 0) {
            ESP_LOGI(TAG, "~~~~~~~~~~~");
            ESP_LOGI(TAG, "ETHIP:"IPSTR, IP2STR(&ip.ip));
            ESP_LOGI(TAG, "ETHPMASK:"IPSTR, IP2STR(&ip.netmask));
            ESP_LOGI(TAG, "ETHPGW:"IPSTR, IP2STR(&ip.gw));
            ESP_LOGI(TAG, "~~~~~~~~~~~");
			int j;
#if 1
			for( j = 0; j < 7; j++ )
			{
				ESP_LOGI(TAG, "%d: %04x", j, esp_eth_smi_read(j) );
			}
				ESP_LOGI(TAG, "%d: %04x", 17, esp_eth_smi_read(17) );
				ESP_LOGI(TAG, "%d: %04x", 18, esp_eth_smi_read(18) );
				ESP_LOGI(TAG, "%d: %04x", 26, esp_eth_smi_read(26) );
				ESP_LOGI(TAG, "%d: %04x", 27, esp_eth_smi_read(27) );
				ESP_LOGI(TAG, "%d: %04x", 30, esp_eth_smi_read(30) );
				ESP_LOGI(TAG, "%d: %04x", 31, esp_eth_smi_read(31) );
#else
		    phy_ksz8081_dump_registers();
#endif
			ESP_LOGI( TAG, "%d %d\n", _ETH_RX, _ETH_TX );
        }
		iter++;
		if( iter == 4 )
		{
	//		ESP_LOGI(TAG, "STARTING\n" );
	 //   tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_ETH); // ret=0x5000 -> tcpip_adapter_invalid_params, very old esp-idf didn't implementated this yet.
		}
    }
}

void app_main()
{
	//Reset with GPIO2
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1<<2)| (1<<25)| (1<<26)|(1<<27)|(1<<13);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
	gpio_set_level(13, 0); //SET PHYAD0 "RXER" ... or on the KSZ, 0 indicates no factory reset.
	gpio_set_level(25, 1);	//SET MODE 0, 1, 2
	gpio_set_level(27, 1); //SET TO 1 FOR LAN8720
	gpio_set_level(26, 1);
	gpio_set_level(2, 0);

	ESP_LOGI( TAG, "Start up" );

    esp_err_t ret = ESP_OK;
    tcpip_adapter_init();
    esp_event_loop_init(NULL, NULL);

	gpio_set_level(2, 1);

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1<<25)| (1<<26)|(1<<27)|(1<<13);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);


    eth_config_t config = DEFAULT_ETHERNET_PHY_CONFIG;
    /* Set the PHY address in the example configuration */
    config.phy_addr = CONFIG_PHY_ADDRESS;
    config.gpio_config = eth_gpio_config_rmii;
    config.tcpip_input = tcpip_adapter_eth_input;
    config.clock_mode = CONFIG_PHY_CLOCK_MODE;

#ifdef CONFIG_PHY_USE_POWER_PIN
    /* Replace the default 'power enable' function with an example-specific
       one that toggles a power GPIO. */
    config.phy_power_enable = phy_device_power_enable_via_gpio;
#endif

    ret = esp_eth_init(&config);

    if(ret == ESP_OK) {
        esp_eth_enable();
        xTaskCreate(eth_task, "eth_task", 2048, NULL, (tskIDLE_PRIORITY + 2), NULL);
    }

}

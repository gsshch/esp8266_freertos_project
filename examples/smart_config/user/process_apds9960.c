/*
 * process_apds9960.c
 *
 *  Created on: 2018年5月12日
 *      Author: guo
 */

#include "process_apds9960.h"
#include "c_types.h"
#include "i2c_master.h"
#include "esp_libc.h"

#define ESP_DBG os_printf

void ICACHE_FLASH_ATTR handleGesture(void) {
	if (APDS9960_isGestureAvailable()) {
		switch (APDS9960_readGesture()) {
		case DIR_UP:
			ESP_DBG("UP\n")
			;
			break;
		case DIR_DOWN:
			ESP_DBG("DOWN\n")
			;
			break;
		case DIR_LEFT:
			ESP_DBG("LEFT\n")
			;
			break;
		case DIR_RIGHT:
			ESP_DBG("RIGHT\n")
			;
			break;
		case DIR_NEAR:
			ESP_DBG("NEAR\n")
			;
			break;
		case DIR_FAR:
			ESP_DBG("FAR\n")
			;
			break;

		default:
			ESP_DBG("NONE\n")
			;
		}
	}
}

int INT_Flag;
void ICACHE_FLASH_ATTR apds9960_task(void *pvParameters) {
	while (1) {
		if (INT_Flag == true) {
			handleGesture();
			INT_Flag = false;
		}
	}
}

void ICACHE_FLASH_ATTR apds_process_init() {
	i2c.i2c_master_gpio_init();
	APDS9960_Init();
	APDS9960_enableGestureSensor(true);

}


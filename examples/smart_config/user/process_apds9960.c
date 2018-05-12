/*
 * process_apds9960.c
 *
 *  Created on: 2018年5月12日
 *      Author: guo
 */

#include "process_apds9960.h"
#include "c_types.h"
#include "i2c_master.h"

void

void ICACHE_FLASH_ATTR apds_process_init(){
	i2c.i2c_master_gpio_init();
	APDS9960_Init();
	APDS9960_enableGestureSensor(true);


}

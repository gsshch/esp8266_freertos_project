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
#include "task.h"
#include "gpio_register.h"

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

#define apds_intr_pin 13

void intr_cb_func() {
	//可控硅中断触发;
	void siliconControlIsrHandler() {
		u32 pin_status = 0;
		/** 读取GPIO中断状态可以判断是那个端口的中断 */
		pin_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
		ETS_GPIO_INTR_DISABLE();/** 关闭GPIO中断 */
		if (pin_status & BIT(apds_intr_pin)) /*GPIO中断*/
		{
			INT_Flag = true;
		}

		GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, pin_status); /** 清除GPIO中断标志 */
		ETS_GPIO_INTR_ENABLE();
	}
}

void ICACHE_FLASH_ATTR intr_event_init() {
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);    //定义管教功能  作GPIO使用
	GPIO_DIS_OUTPUT(FUNC_GPIO13);  //使能输出功能
	//GPIO13中断配置
	ETS_GPIO_INTR_DISABLE(); 										//中断失能
	ETS_GPIO_INTR_ATTACH(&intr_cb_func, NULL);    		//注册中断函数
	gpio_pin_intr_state_set(GPIO_ID_PIN(apds_intr_pin), GPIO_PIN_INTR_ANYEDGE); //设置中断方式
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(apds_intr_pin)); //清除该引脚的GPIO中断标志
	ETS_GPIO_INTR_ENABLE(); 										//中断使能
}

void ICACHE_FLASH_ATTR apds_process_init() {

	i2c.i2c_master_gpio_init();
	APDS9960_Init();
	APDS9960_enableGestureSensor(true);
	intr_event_init();
	xTaskCreate(apds9960_task, "apds9960_task", 256, NULL, 2, NULL);
}


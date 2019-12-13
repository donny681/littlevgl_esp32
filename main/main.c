/* SPI Master example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "lvgl/lvgl.h"
#include "lv_examples/lv_apps/demo/demo.h"
#include "esp_freertos_hooks.h"
#include "esp_spiffs.h"
#include "esp_err.h"
#include "esp_log.h"
#include "disp_spi.h"
#include "ili9341.h"
#include "lv_lodepng.h"
#include "tp_spi.h"
#include "xpt2046.h"

#include "nvs_flash.h"
static const char *TAG = "example";
static void IRAM_ATTR lv_tick_task(void);
lv_obj_t * CURRENT_img1=NULL;
lv_obj_t * NEXT_day_weather_img=NULL;
lv_obj_t *  Current_weather_label=NULL;
void spiffs_init()
{
    ESP_LOGI(TAG, "Initializing SPIFFS");
    
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    // ESP_LOGI(TAG, "Reading time.png");
}

void my_demo_create()
{
	lv_lodepng_init();
    /*current weather*/
    CURRENT_img1 = lv_img_create(lv_scr_act(), NULL);
    lv_obj_set_pos(CURRENT_img1, 10, 65);
    lv_img_set_src(CURRENT_img1, "/spiffs/wet/100.png");
    lv_obj_set_drag(CURRENT_img1, true);

	    /**Next day weather */
    NEXT_day_weather_img = lv_img_create(lv_scr_act(), NULL);
    lv_obj_set_pos(NEXT_day_weather_img, 0, 80);
    lv_img_set_src(NEXT_day_weather_img, "/spiffs/wet/101.png");
    lv_obj_set_drag(NEXT_day_weather_img, true);
	        /**weather tip current label */
    static lv_style_t current_tip_style;
    char *current_tip_str="Search finished, found 9 page(s) matching the search query.";
    lv_style_copy(&current_tip_style, &lv_style_plain);
    current_tip_style.text.font = &lv_font_roboto_16; /* 设置自定义字体 */
    Current_weather_label = lv_label_create(lv_scr_act(), NULL);
    lv_obj_set_pos(Current_weather_label, 10, 160);
    lv_label_set_align(Current_weather_label, LV_LABEL_ALIGN_CENTER);
    lv_label_set_style(Current_weather_label, LV_LABEL_STYLE_MAIN,&current_tip_style);
    lv_label_set_anim_speed(Current_weather_label,25);
    lv_label_set_long_mode(Current_weather_label,LV_LABEL_LONG_SROLL);
    lv_obj_set_size(Current_weather_label,200,30);
    lv_label_set_text(Current_weather_label, current_tip_str );
}

void app_main()
{
	esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
	
	lv_init();

	disp_spi_init();
	ili9341_init();
	spiffs_init();
#if ENABLE_TOUCH_INPUT
	tp_spi_init();
	xpt2046_init();
#endif

    static lv_color_t buf1[DISP_BUF_SIZE];
    static lv_color_t buf2[DISP_BUF_SIZE];
    static lv_disp_buf_t disp_buf;
    lv_disp_buf_init(&disp_buf, buf1, buf2, DISP_BUF_SIZE);

	lv_disp_drv_t disp_drv;
	disp_drv.ver_res=320;
	disp_drv.hor_res=240;
//	disp_drv.rotated=0;
	lv_disp_drv_init(&disp_drv);
	disp_drv.flush_cb = ili9341_flush;
	disp_drv.buffer = &disp_buf;
	lv_disp_drv_register(&disp_drv);

#if ENABLE_TOUCH_INPUT
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.read_cb = xpt2046_read;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev_drv);
#endif

	esp_register_freertos_tick_hook(lv_tick_task);
	
	my_demo_create();
	
	/*****/
	// wifi_init_sta();
	while(1) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
		lv_task_handler();
		printf("%s buffer get: %d\r\n",__func__,  esp_get_free_heap_size()); 
	}
}

static void IRAM_ATTR lv_tick_task(void)
{
	lv_tick_inc(portTICK_RATE_MS);
}

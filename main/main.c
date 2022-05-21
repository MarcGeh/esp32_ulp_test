#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/rtc_io.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "driver/adc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc_periph.h"
#include "esp32/ulp.h"
#include "ulp_main.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

static void init_ulp_program(void)
{
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start,
            (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    /* Configure ADC channel */
    /* Note: when changing channel here, also change 'adc_channel' constant
       in adc.S */
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_ulp_enable();

    ulp_wake_up_count = 1000;

    /* Set ULP wake up period to 20ms */
    ulp_set_wakeup_period(0, 4000);
    esp_deep_sleep_disable_rom_logging(); // suppress boot messages
}

static void start_ulp_program(void)
{
    /* Reset ULP counter */
    ulp_counter = 0;

    /* Start the program */
    esp_err_t err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
}

// APP_MAIN for deep sleep test
/* void app_main(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        printf("Not ULP wakeup\n");
        init_ulp_program();
    } else {
        printf("ULP ADC measurement %i\n", ulp_last_result & UINT16_MAX);
    }
    printf("Entering deep sleep\n\n");
    start_ulp_program();
    ESP_ERROR_CHECK( esp_sleep_enable_ulp_wakeup() );
    esp_deep_sleep_start();
} */

// APP_MAIN for light sleep test
void app_main(void)
{
    init_ulp_program();
    vTaskDelay(1000 /portTICK_PERIOD_MS);
    ESP_ERROR_CHECK( esp_sleep_enable_ulp_wakeup() );
    while(1)
    {
        start_ulp_program();
        ESP_ERROR_CHECK( esp_light_sleep_start() );
        printf("ULP ADC measurement %i\n", ulp_last_result & UINT16_MAX);
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

#include "led_control.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_PIN GPIO_NUM_48

// 静态变量：保存当前 LED 状态（false = off, true = on）
static bool s_led_state = false;

void led_init(void)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0); // 初始熄灭
    s_led_state = false;        // 同步软件状态
    ESP_LOGI("LED", "Initialized on GPIO %d", LED_PIN);
}

// 反转 LED 状态
void led_toggle(void)
{
    s_led_state = !s_led_state;           // 取反
    gpio_set_level(LED_PIN, s_led_state); // 应用到硬件
    ESP_LOGI("LED", "Toggled to %s", s_led_state ? "ON" : "OFF");
}
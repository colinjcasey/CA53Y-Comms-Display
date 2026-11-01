// Power off and restart.
// Power off = ESP32 deep sleep: radio off, screen asleep, backlight, LED and speaker amp
// held off. Press BOOT (or RESET) to turn it back on. The board's regulator and charger
// still draw a little, so a battery left connected drains in a few days.
#pragma once
#include <WiFi.h>
#include <esp_now.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>

static const gpio_num_t PWR_HELD[] = {
  (gpio_num_t)21,                      // backlight
  (gpio_num_t)PIN_AMP_EN,
  (gpio_num_t)PIN_LED_R, (gpio_num_t)PIN_LED_G, (gpio_num_t)PIN_LED_B
};

// first thing in setup(): let go of the pins held off during power off
void powerBegin() {
  gpio_deep_sleep_hold_dis();
  for (gpio_num_t p : PWR_HELD) gpio_hold_dis(p);
}

static void radioOff() {
  esp_now_deinit();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(50);
}

void restartNow() {
  radioOff();
  ESP.restart();
}

void powerOffNow() {
  radioOff();
  for (gpio_num_t p : PWR_HELD) {
    pinMode(p, OUTPUT);
    digitalWrite(p, p == (gpio_num_t)21 ? LOW : HIGH);   // backlight off; LED and amp are LOW = on
    gpio_hold_en(p);
  }
  gpio_deep_sleep_hold_en();

  pinMode(PIN_BOOT, INPUT_PULLUP);                      // don't sleep while BOOT is down,
  while (digitalRead(PIN_BOOT) == LOW) delay(10);       // or it would wake straight away
  delay(100);

  rtc_gpio_pullup_en((gpio_num_t)PIN_BOOT);
  rtc_gpio_pulldown_dis((gpio_num_t)PIN_BOOT);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BOOT, 0);
  Serial.println("Powering off. Press BOOT to turn on.");
  Serial.flush();
  esp_deep_sleep_start();
}

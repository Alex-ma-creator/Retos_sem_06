#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LED_PIN 2
LiquidCrystal_I2C lcd(0x27, 16, 2);

void tareaLED(void *pv) {
  pinMode(LED_PIN, OUTPUT);
  for (;;) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void tareaLCD(void *pv) {
  int contador = 0;
  for (;;) {
    lcd.setCursor(0, 0);
    lcd.print("Contador:");
    lcd.setCursor(0, 1);
    lcd.print(contador++);
    lcd.print("     ");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  xTaskCreate(tareaLED, "LED", 2048, NULL, 1, NULL);
  xTaskCreate(tareaLCD, "LCD", 4096, NULL, 1, NULL);
}

void loop() { vTaskDelete(NULL); }
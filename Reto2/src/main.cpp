#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BTN_AVANCE 18

LiquidCrystal_I2C lcd(0x27, 16, 2);
SemaphoreHandle_t mutexCiclo;
TimerHandle_t timerAviso;

const char *ETAPAS[] = {"LAVADO", "ENJUAGUE", "CENTRIFUGADO"};
int etapa = 0;

void mostrarEtapa() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(ETAPAS[etapa]);
  Serial.printf("Etapa actual: %s\n", ETAPAS[etapa]);
}

// Se ejecuta cada 5 s sin cambio de etapa
void avisoCb(TimerHandle_t t) {
  if (xSemaphoreTake(mutexCiclo, 0) == pdTRUE) {
    lcd.setCursor(0, 1);
    lcd.print("Desea avanzar? ");
    Serial.println("¿Desea avanzar?");
    xSemaphoreGive(mutexCiclo);
  }
}

void tareaLavado(void *pv) {
  bool previo = HIGH;
  for (;;) {
    bool actual = digitalRead(BTN_AVANCE);
    if (previo == HIGH && actual == LOW) {
      vTaskDelay(pdMS_TO_TICKS(30));  // antirrebote
      if (digitalRead(BTN_AVANCE) == LOW) {
        xSemaphoreTake(mutexCiclo, portMAX_DELAY);
        etapa = (etapa + 1) % 3;
        mostrarEtapa();
        xTimerReset(timerAviso, 0);
        xSemaphoreGive(mutexCiclo);
      }
    }
    previo = actual;
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_AVANCE, INPUT_PULLUP);
  lcd.init();
  lcd.backlight();

  mutexCiclo = xSemaphoreCreateMutex();
  timerAviso = xTimerCreate("aviso", pdMS_TO_TICKS(5000), pdTRUE, NULL, avisoCb);

  mostrarEtapa();
  xTimerStart(timerAviso, 0);
  xTaskCreate(tareaLavado, "Lavado", 4096, NULL, 2, NULL);
}

void loop() { vTaskDelete(NULL); }
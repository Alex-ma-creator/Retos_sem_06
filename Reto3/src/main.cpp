#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BTN_AVANCE 18
#define PIN_TAPA   19   // LOW = tapa abierta

LiquidCrystal_I2C lcd(0x27, 16, 2);
SemaphoreHandle_t mutexCiclo;
TimerHandle_t timerAviso;
TaskHandle_t hLavado, hEmergencia;

const char *ETAPAS[] = {"LAVADO", "ENJUAGUE", "CENTRIFUGADO"};
int etapa = 0;

void mostrarEtapa() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(ETAPAS[etapa]);
  Serial.printf("Etapa actual: %s\n", ETAPAS[etapa]);
}

void avisoCb(TimerHandle_t t) {
  if (xSemaphoreTake(mutexCiclo, 0) == pdTRUE) {
    lcd.setCursor(0, 1);
    lcd.print("Desea avanzar? ");
    Serial.println("¿Desea avanzar?");
    xSemaphoreGive(mutexCiclo);
  }
}

// ISR: solo avisa a la tarea de emergencia
void IRAM_ATTR isrTapa() {
  BaseType_t despertar = pdFALSE;
  vTaskNotifyGiveFromISR(hEmergencia, &despertar);
  portYIELD_FROM_ISR(despertar);
}

void tareaLavado(void *pv) {
  bool previo = HIGH;
  for (;;) {
    bool actual = digitalRead(BTN_AVANCE);
    if (previo == HIGH && actual == LOW) {
      vTaskDelay(pdMS_TO_TICKS(30));
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

void tareaEmergencia(void *pv) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);       // espera la interrupción

    xSemaphoreTake(mutexCiclo, portMAX_DELAY);     // evita suspender con el mutex tomado
    vTaskSuspend(hLavado);
    xTimerStop(timerAviso, 0);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Proceso detenido");
    lcd.setCursor(0, 1);
    lcd.print("cierre la tapa");
    Serial.println("EMERGENCIA: Proceso detenido, cierre la tapa");
    xSemaphoreGive(mutexCiclo);

    while (digitalRead(PIN_TAPA) == LOW) vTaskDelay(pdMS_TO_TICKS(50));
    vTaskDelay(pdMS_TO_TICKS(100));
    ulTaskNotifyTake(pdTRUE, 0);                   // descarta rebotes

    xSemaphoreTake(mutexCiclo, portMAX_DELAY);
    mostrarEtapa();
    xTimerReset(timerAviso, 0);
    xSemaphoreGive(mutexCiclo);
    vTaskResume(hLavado);
    Serial.println("Tapa cerrada, ciclo reanudado");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_AVANCE, INPUT_PULLUP);
  pinMode(PIN_TAPA, INPUT_PULLUP);
  lcd.init();
  lcd.backlight();

  mutexCiclo = xSemaphoreCreateMutex();
  timerAviso = xTimerCreate("aviso", pdMS_TO_TICKS(5000), pdTRUE, NULL, avisoCb);

  mostrarEtapa();
  xTimerStart(timerAviso, 0);

  xTaskCreate(tareaLavado, "Lavado", 4096, NULL, 2, &hLavado);
  xTaskCreate(tareaEmergencia, "Emergencia", 4096, NULL, 3, &hEmergencia);

  attachInterrupt(digitalPinToInterrupt(PIN_TAPA), isrTapa, FALLING);
}

void loop() { vTaskDelete(NULL); }
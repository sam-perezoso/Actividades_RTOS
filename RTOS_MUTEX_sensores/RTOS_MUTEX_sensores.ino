#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

Adafruit_MPU6050 mpu;
Adafruit_BMP280 bmp;

// ============================================================
// HANDLES DE TAREAS
// ============================================================
static TaskHandle_t hMPU = NULL;
static TaskHandle_t hBMP = NULL;
static TaskHandle_t hAvg = NULL;

hw_timer_t *timer = NULL;

// ============================================================
// RECURSOS COMPARTIDOS
// ============================================================
static SemaphoreHandle_t mutex;      // protege las temperaturas compartidas
static SemaphoreHandle_t semListas;  // cuenta cuántas lecturas del ciclo están listas

static float tempMPU = 0.0f;
static float tempBMP = 0.0f;
static uint8_t lecturasListas = 0;   // 0, 1 o 2

// ============================================================
// ISR DEL TIMER: solo notifica a las 3 tareas
// ============================================================
void ARDUINO_ISR_ATTR timerISR() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (hMPU) vTaskNotifyGiveFromISR(hMPU, &xHigherPriorityTaskWoken);
  if (hBMP) vTaskNotifyGiveFromISR(hBMP, &xHigherPriorityTaskWoken);
  if (hAvg) vTaskNotifyGiveFromISR(hAvg, &xHigherPriorityTaskWoken);

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// ============================================================
// TAREA MPU6050  -> lee temperatura y publica en tempMPU
// ============================================================
void MPU6050(void *parameter) {
  sensors_event_t a, g, temp;

  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    mpu.getEvent(&a, &g, &temp);

    // Sección crítica: publicar la lectura
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
      tempMPU = temp.temperature;
      lecturasListas++;
      xSemaphoreGive(mutex);

      // Avisa al promedio que hay una lectura más
      xSemaphoreGive(semListas);
    }
  }
}

// ============================================================
// TAREA BMP280  -> lee temperatura y publica en tempBMP
// ============================================================
void BMP280(void *parameter) {
  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    float lectura = bmp.readTemperature();

    // Sección crítica: publicar la lectura
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
      tempBMP = lectura;
      lecturasListas++;
      xSemaphoreGive(mutex);

      xSemaphoreGive(semListas);
    }
  }
}

// ============================================================
// TAREA PROMEDIO  -> espera ambas lecturas y promedia
// ============================================================
void promedio(void *parameter) {
  while (1) {
    // 1) Esperar a que las DOS lecturas del ciclo estén listas
    xSemaphoreTake(semListas, portMAX_DELAY);
    xSemaphoreTake(semListas, portMAX_DELAY);

    // 2) Tomar el mutex y leer de forma consistente
    float t1 = 0.0f, t2 = 0.0f;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
      t1 = tempMPU;
      t2 = tempBMP;
      lecturasListas = 0;      // reiniciar contador para el próximo ciclo
      xSemaphoreGive(mutex);
    }

    // 3) Calcular e imprimir el promedio
    float prom = (t1 + t2) / 2.0f;

    Serial.print("MPU: ");
    Serial.print(t1, 2);
    Serial.print(" °C | BMP: ");
    Serial.print(t2, 2);
    Serial.print(" °C | Promedio: ");
    Serial.print(prom, 2);
    Serial.println(" °C");
  }
}

// ============================================================
// CONFIGURACIÓN MPU6050
// ============================================================
void setMPU() {
  if (!mpu.begin(0x68)) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("MPU6050 configurado");
}

// ============================================================
// CONFIGURACIÓN BMP280
// ============================================================
void setBMP() {
  unsigned status = bmp.begin(0x76);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1) delay(10);
  }
  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X2,
    Adafruit_BMP280::SAMPLING_X16,
    Adafruit_BMP280::FILTER_X16,
    Adafruit_BMP280::STANDBY_MS_500
  );
  Serial.println("BMP280 configurado");
}

// ============================================================
// CONFIGURACIÓN TIMER
// ============================================================
void setTimer() {
  timer = timerBegin(1000000);          // 1 tick = 1 us
  if (timer == NULL) {
    Serial.println("Error al crear timer");
    return;
  }
  timerAttachInterrupt(timer, &timerISR);
  timerAlarm(timer, 1000000, true, 0);  // 1 s, autoreload
  Serial.println("Timer configurado: 1 Hz");
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(300);

  setMPU();
  setBMP();
  setTimer();

  // Recursos de sincronización
  mutex     = xSemaphoreCreateMutex();
  semListas = xSemaphoreCreateCounting(2, 0);   // máx 2, inicial 0

  if (mutex == NULL || semListas == NULL) {
    Serial.println("Error creando primitivas de sincronización");
    while (1) delay(10);
  }

  // Tareas: MPU y BMP con mayor prioridad que la del promedio,
  // para que publiquen antes de que el promedio intente leer.
  xTaskCreatePinnedToCore(MPU6050, "MPU6050", 4096, NULL, 2, &hMPU, app_cpu);
  xTaskCreatePinnedToCore(BMP280,  "BMP280",  4096, NULL, 2, &hBMP, app_cpu);
  xTaskCreatePinnedToCore(promedio,"promedio",4096, NULL, 1, &hAvg, app_cpu);

  Serial.println("===================");

  vTaskDelete(NULL);
}

// ============================================================
// LOOP
// ============================================================
void loop() {
}
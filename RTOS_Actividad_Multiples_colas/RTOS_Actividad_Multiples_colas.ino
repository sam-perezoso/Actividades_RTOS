
#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

Adafruit_MPU6050 mpu;
Adafruit_BMP280 bmp;

static const uint8_t queue_len = 5;

static QueueHandle_t q_s11;
static QueueHandle_t q_s12;
static QueueHandle_t q_s21;
static QueueHandle_t q_s22;

static SemaphoreHandle_t mutexEtapas;

// Etapa 1.1
void etapa11(void *parameter) {
  float lectura = 0;
  float lecturaMPU = 0;

  while (1) {
    // Sección crítica: envío, acceso a sensor y mensajes
    if (xSemaphoreTake(mutexEtapas, portMAX_DELAY) == pdTRUE) {

      if (xQueueSend(q_s11, (void*)&lectura, 0) == pdTRUE) {
        lectura = bmp.readTemperature();
        Serial.println("Enviando lectura BMP: ");
      } else {
        Serial.println("Cola llena, dato descartado");
      }

      xSemaphoreGive(mutexEtapas);
    }

    // La espera se realiza fuera del mutex
    if (xQueueReceive(q_s12, (void*)&lecturaMPU,portMAX_DELAY) == pdTRUE) {

      if (xSemaphoreTake(mutexEtapas, portMAX_DELAY) == pdTRUE) {
        Serial.print(">>Temperatura de MPU: ");
        Serial.println(lecturaMPU);
        xSemaphoreGive(mutexEtapas);
      }
    } else {
      Serial.println("cola vacía");
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

// Etapa 1.2
void etapa12(void *parameter) {
  float lecturabmp = 0;
  float lecturaMPU = 0;
  float alpha = 0.6;

  sensors_event_t a, g, temp;

  while (1) {
    if (xSemaphoreTake(mutexEtapas, portMAX_DELAY) == pdTRUE) {

      mpu.getEvent(&a, &g, &temp);

      if (xQueueSend(q_s12, (void*)&lecturaMPU, 0) == pdTRUE) {
        float lectura = temp.temperature;
        lecturaMPU = alpha * lectura + (1 - alpha) * lecturaMPU;
        Serial.println("Enviando lectura MPU: ");
      } else {
        Serial.println("Cola llena, dato descartado");
      }

      xSemaphoreGive(mutexEtapas);
    }

    if (xQueueReceive(q_s11, (void*)&lecturabmp,portMAX_DELAY) == pdTRUE) {

      if (xSemaphoreTake(mutexEtapas, portMAX_DELAY) == pdTRUE) {
        Serial.print(">>Temperatura de BMP: ");
        Serial.println(lecturabmp);
        xSemaphoreGive(mutexEtapas);
      }
    } else {
      Serial.println("cola vacía");
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

// Etapa 2.1
void etapa21(void *parameter) {
  float lectura = 0;
  float lecturaMPU = 0;

  while (1) {
    if (xSemaphoreTake(mutexEtapas, portMAX_DELAY) == pdTRUE) {

      if (xQueueSend(q_s21, (void*)&lectura, 0) == pdTRUE) {
        lectura = bmp.readAltitude(1013.25);
        Serial.println("Enviando altitud");
      } else {
        Serial.println("Cola llena, dato descartado");
      }

      xSemaphoreGive(mutexEtapas);
    }

    if (xQueueReceive(q_s22, (void*)&lecturaMPU,portMAX_DELAY) == pdTRUE) {

      if (xSemaphoreTake(mutexEtapas, portMAX_DELAY) == pdTRUE) {
        Serial.print(">>Aceleración de gravedad: ");
        Serial.println(lecturaMPU);
        xSemaphoreGive(mutexEtapas);
      }
    } else {
      Serial.println("cola vacía");
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

// Etapa 2.2
void etapa22(void *parameter) {
  float lecturabmp = 0;
  float lecturaMPU = 0;
  float alpha = 0.6;

  sensors_event_t a, g, temp;

  while (1) {
    if (xSemaphoreTake(mutexEtapas, portMAX_DELAY) == pdTRUE) {

      mpu.getEvent(&a, &g, &temp);

      if (xQueueSend(q_s22, (void*)&lecturaMPU, 0) == pdTRUE) {
        float lectura = a.acceleration.z;
        lecturaMPU = alpha * lectura + (1 - alpha) * lecturaMPU;
        Serial.println("Enviando aceleración de la gravedad");
      } else {
        Serial.println("Cola llena, dato descartado");
      }

      xSemaphoreGive(mutexEtapas);
    }

    if (xQueueReceive(q_s21, (void*)&lecturabmp,
                      portMAX_DELAY) == pdTRUE) {

      if (xSemaphoreTake(mutexEtapas, portMAX_DELAY) == pdTRUE) {
        Serial.print(">>Altitud: ");
        Serial.println(lecturabmp);
        xSemaphoreGive(mutexEtapas);
      }
    } else {
      Serial.println("cola vacía");
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

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

void setBMP() {
  unsigned status = bmp.begin(0x76);

  if (!status) {
    Serial.println(
      F("Could not find a valid BMP280 sensor, check wiring!")
    );
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

void setup() {
  Serial.begin(115200);

  setMPU();
  setBMP();

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // Crear las colas con el tamaño correcto para float
  q_s11 = xQueueCreate(queue_len, sizeof(float));
  q_s12 = xQueueCreate(queue_len, sizeof(float));
  q_s21 = xQueueCreate(queue_len, sizeof(float));
  q_s22 = xQueueCreate(queue_len, sizeof(float));

  // Crear el mutex compartido
  mutexEtapas = xSemaphoreCreateMutex();

  // Verificar que todos los recursos se hayan creado
  if (q_s11 == NULL || q_s12 == NULL ||
      q_s21 == NULL || q_s22 == NULL ||
      mutexEtapas == NULL) {
    Serial.println("Error al crear colas o mutex");
    while (1) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }

  xTaskCreatePinnedToCore(
    etapa11, "etapa1.1", 4096, NULL, 1, NULL, app_cpu
  );

  xTaskCreatePinnedToCore(
    etapa12, "etapa1.2", 4096, NULL, 1, NULL, app_cpu
  );

  xTaskCreatePinnedToCore(
    etapa21, "etapa2.1", 4096, NULL, 1, NULL, app_cpu
  );

  xTaskCreatePinnedToCore(
    etapa22, "etapa2.2", 4096, NULL, 1, NULL, app_cpu
  );

  vTaskDelete(NULL);
}

void loop() {
}
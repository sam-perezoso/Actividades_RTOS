#include <Arduino.h>

// En ESP32 de doble núcleo, las tareas de aplicación se fijan al núcleo 1.
// En ESP32 unicore, se fijan al núcleo 0.
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Manejadores (handles) de las tareas que recibirán notificaciones.
TaskHandle_t tareaProcesamientoHandle;
TaskHandle_t tareaSerialHandle;

// Tarea 1: recepción por puerto serial.
// Lee caracteres, arma un String hasta recibir '\n', reserva memoria dinámica
// y envía el puntero a la tarea de procesamiento mediante una notificación.
void tareaRecepcion(void *parameter){
  String mensaje = "";  // Acumula los caracteres recibidos.

  while(1){
    if(Serial.available()){
      char caracter = Serial.read();

      if(caracter == '\n'){
        // +1 para el carácter nulo de fin de cadena.
        size_t memoriaNecesaria = mensaje.length() + 1;

        // Verifica que haya heap suficiente antes de reservar.
        if(xPortGetFreeHeapSize() >= memoriaNecesaria){
          char *dato = (char *)pvPortMalloc(memoriaNecesaria);

          if(dato != NULL){
            strcpy(dato, mensaje.c_str());

            // Envía la dirección de memoria como valor de notificación.
            // eSetValueWithOverwrite reemplaza una notificación anterior
            // si aún no se procesó.
            xTaskNotify(
              tareaProcesamientoHandle,
              (uint32_t)(uintptr_t)dato,
              eSetValueWithOverwrite
            );
          }
          else{
            Serial.println("Error: No se pudo reservar memoria.");
          }
        }
        else{
          Serial.println("Error: Memoria Heap insuficiente.");
        }

        mensaje = ""; // Reinicia el buffer para el siguiente mensaje.
      }
      else{
        mensaje += caracter; // Sigue acumulando caracteres.
      }
    }

    // Pequeña pausa para no bloquear el planificador.
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// Tarea 2: procesamiento.
// Espera una notificación con la dirección de una cadena, invierte la cadena
// in situ y envía el mismo puntero a la tarea serial.
void tareaProcesamiento(void *parameter){
  while(1){
    uint32_t direccion;

    // Espera hasta 5 segundos una notificación con el puntero.
    // Los dos primeros 0 indican que no se limpian bits de notificación.
    if(xTaskNotifyWait(0, 0, &direccion, 5000 / portTICK_PERIOD_MS) == pdTRUE){
      char *dato = (char *)(uintptr_t)direccion; // Recupera el puntero.
      int longitud = strlen(dato);

      // Invierte la cadena intercambiando extremos.
      for(int i = 0; i < longitud / 2; i++){
        char temporal = dato[i];
        dato[i] = dato[longitud - 1 - i];
        dato[longitud - 1 - i] = temporal;
      }

      // Envía el puntero ya procesado a la tarea serial.
      xTaskNotify(
        tareaSerialHandle,
        (uint32_t)(uintptr_t)dato,
        eSetValueWithOverwrite
      );
    }
    else{
      // Mensaje periódico si no hay actividad durante el tiempo de espera.
      Serial.println("Estado: Sistema inactivo, esperando comandos...");
    }
  }
}

// Tarea 3: transmisión por puerto serial.
// Espera una notificación con la dirección de la cadena procesada, la imprime,
// muestra el heap libre y libera la memoria reservada.
void tareaSerial(void *parameter){
  while(1){
    uint32_t direccion;

    // Espera indefinidamente hasta recibir una notificación.
    xTaskNotifyWait(0, 0, &direccion, portMAX_DELAY);
    char *dato = (char *)(uintptr_t)direccion; // Recupera el puntero.

    Serial.print("Mensaje invertido: ");
    Serial.println(dato);
    Serial.print("Heap disponible: ");
    Serial.print(xPortGetFreeHeapSize());
    Serial.println(" bytes");

    vPortFree(dato); // Libera la memoria reservada en tareaRecepcion.
  }
}

// Configuración inicial.
void setup(){
  Serial.begin(115200);

  // Crea la tarea de recepción serial.
  xTaskCreatePinnedToCore(
    tareaRecepcion,
    "Recepcion",
    2048,
    NULL,
    1,
    NULL,
    app_cpu
  );

  // Crea la tarea de procesamiento y guarda su handle para notificarla.
  xTaskCreatePinnedToCore(
    tareaProcesamiento,
    "Procesamiento",
    2048,
    NULL,
    1,
    &tareaProcesamientoHandle,
    app_cpu
  );

  // Crea la tarea de salida serial y guarda su handle para notificarla.
  xTaskCreatePinnedToCore(
    tareaSerial,
    "Serial",
    2048,
    NULL,
    1,
    &tareaSerialHandle,
    app_cpu
  );
}

// loop vacío: todo el trabajo se realiza en las tareas de FreeRTOS.
void loop(){

}
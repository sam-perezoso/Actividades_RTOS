#if CONFIG_FREERTOS_UNICORE

static const BaseType_t app_cpu = 0;

#else

static const BaseType_t app_cpu = 1;

#endif

#define led01 16
#define led02 17


//====================================================
// TAREA PARA GPIO 16 - 1 Hz
//====================================================

void toggle_led01(void *parameter){

  while(1){

    digitalWrite(led01, HIGH);

    vTaskDelay(500 / portTICK_PERIOD_MS);

    digitalWrite(led01, LOW);

    vTaskDelay(500 / portTICK_PERIOD_MS);

  }

}


//====================================================
// TAREA PARA GPIO 17 - 2 Hz
//====================================================

void toggle_led02(void *parameter){

  while(1){

    digitalWrite(led02, HIGH);

    vTaskDelay(250 / portTICK_PERIOD_MS);

    digitalWrite(led02, LOW);

    vTaskDelay(250 / portTICK_PERIOD_MS);

  }

}


void setup() {

  // Configuración de pines

  pinMode(led01, OUTPUT);
  pinMode(led02, OUTPUT);


  //==================================================
  // CREAR TAREA GPIO 16
  //==================================================

  xTaskCreatePinnedToCore(

    toggle_led01,       // función a llamar
    "toggle_led01",    // nombre de tarea
    1024,              // tamaño de pila
    NULL,              // parámetro
    1,                 // prioridad
    NULL,              // identificador de tarea
    app_cpu            // núcleo
  );


  //==================================================
  // CREAR TAREA GPIO 17
  //==================================================

  xTaskCreatePinnedToCore(

    toggle_led02,       // función a llamar
    "toggle_led02",    // nombre de tarea
    1024,              // tamaño de pila
    NULL,              // parámetro
    1,                 // prioridad
    NULL,              // identificador de tarea
    app_cpu             // núcleo
  );

}


void loop() {

  // El trabajo se realiza mediante las tareas de FreeRTOS.

}
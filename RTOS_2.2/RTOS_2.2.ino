#if CONFIG_FREERTOS_UNICORE

static const BaseType_t app_cpu = 0;

#else

static const BaseType_t app_cpu = 1;

#endif

#define led01 16
#define led02 17
#define btn 21

//====================================================
// TAREA PARA GPIO 16 - 1 Hz
//====================================================

void toggle_led01(void *parameter){

  while(1){

    digitalWrite(led01, HIGH);

    vTaskDelay(10 / portTICK_PERIOD_MS);

    digitalWrite(led01, LOW);

    vTaskDelay(10 / portTICK_PERIOD_MS);

  }

}


//====================================================
// TAREA PARA GPIO 17 - 2 Hz
//====================================================

void toggle_led02(void *parameter){

  while(1){

    digitalWrite(led02, HIGH);

    vTaskDelay(5 / portTICK_PERIOD_MS);

    digitalWrite(led02, LOW);

    vTaskDelay(5 / portTICK_PERIOD_MS);

  }

}

void boton(void *parameter){

  while(1){

   if (!digitalRead(btn)) Serial.println("ON");
   //Serial.println("OFF");
  //vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  
}


void setup() {

  // Configuración de pines

  pinMode(led01, OUTPUT);
  pinMode(led02, OUTPUT);
  pinMode(btn,INPUT_PULLUP);

  Serial.begin(115200);
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

  //==================================================
  // CREAR TAREA BOTON
  //==================================================

  xTaskCreatePinnedToCore(

    boton,       // función a llamar
    "boton",    // nombre de tarea
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
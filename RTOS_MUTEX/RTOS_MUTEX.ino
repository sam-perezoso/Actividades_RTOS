#include <Arduino.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// variable compartida
static int global = 0;
static SemaphoreHandle_t mutex;

// tarea 1
void inctarea(void *parameter){
  int local_var;
  while(1){
    if(xSemaphoreTake(mutex,0)==pdTRUE){
      global++;
      Serial.print("T1: ")
      Serial.println(global);
      xSemaphoreGive(mutex);
      taskYIELD();   
    }else{
      
    }
  }
}



// setup
void setup(){

  Serial.begin(115200);


  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println("===================");
  mutex = xSemaphoreCreateMutex();
  // crear tarea 1
  xTaskCreatePinnedToCore(inctarea,"inctarea1",4096,NULL,1,NULL,app_cpu);

  // crear tarea 2
  xTaskCreatePinnedToCore(inctarea,"inctarea2",4096,NULL,1,NULL,app_cpu);

  vTaskDelete(NULL);
}

// loop
void loop(){

}
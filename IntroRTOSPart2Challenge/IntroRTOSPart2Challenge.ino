// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif


//Pins 
static const int led_pin = LED_BUILTIN;

//our task
void toggleLED1(void *parameter)
{
  while(1)
  {
  digitalWrite(led_pin,HIGH);
  vTaskDelay(700 / portTICK_PERIOD_MS);
  digitalWrite(led_pin,LOW);
  vTaskDelay(700 / portTICK_PERIOD_MS);
  }
}

void toggleLED2(void *parameter)
{
  while(1)
  {
   digitalWrite(led_pin,HIGH);
  vTaskDelay(333 / portTICK_PERIOD_MS);
  digitalWrite(led_pin,LOW);
  vTaskDelay(333 / portTICK_PERIOD_MS);
  }
}

//setup() and loop() run as their own tasks w/ priority 1 in core 1. 

void setup() {
  
  pinMode(led_pin,OUTPUT);

  xTaskCreatePinnedToCore(    //use xTaskCreate() in vanilla FREERTOS
    toggleLED1,              //our task function
    "Toggle LED 1",           // name of task
    1024,                   //Stack size (bytes in ESP32,words in FREERTOS
    NULL,                   //Parameter to pass to function
    1,                      //priority (higher=greater priority, 0 to configMAX_PRIORITIES-1)
    NULL,                   //Task Handle
    app_cpu);               //Run on one core for demo purposes

    xTaskCreatePinnedToCore(    //use xTaskCreate() in vanilla FREERTOS
    toggleLED2,              //our task function
    "Toggle LED 2",           // name of task
    1024,                   //Stack size (bytes in ESP32,words in FREERTOS
    NULL,                   //Parameter to pass to function
    1,                      //priority (higher=greater priority, 0 to configMAX_PRIORITIES-1)
    NULL,                   //Task Handle
    app_cpu);               //Run on one core for demo purposes
}

//NOTE - seems like priotity 1 is the highest priority? I can't get priority 0 to work. 

//IF this were vanilla FREERTOS, you'd need to call the scheduler via vTaskStartScheduler() in main after setting up tasks.
//Its already called for us via arduino. 

void loop() {
  // put your main code here, to run repeatedly:

}

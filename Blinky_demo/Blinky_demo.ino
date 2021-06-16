// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif


//Pins 
static const int led_pin = LED_BUILTIN;

//our task
void toggleLED(void *parameter)
{
  digitalWrite(led_pin,HIGH);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  digitalWrite(led_pin,LOW);
  vTaskDelay(200 / portTICK_PERIOD_MS);
}

//setup() and loop() run as their own tasks w/ priority 1 in core 1. 

void setup() {
  
  pinMode(led_pin,OUTPUT);

  xTaskCreatePinnedToCore(    //use xTaskCreate() in vanilla FREERTOS
    toggleLED,              //our task function
    "Toggle LED",           // name of task
    1024,                   //Stack size (bytes in ESP32,words in FREERTOS
    NULL,                   //Parameter to pass to function
    1,                      //priority (higher=greater priority, 0 to configMAX_PRIORITIES-1)
    NULL,                   //Task Handle
    app_cpu);               //Run on one core for demo purposes
}
//IF this were vanilla FREERTOS, you'd need to call the scheduler via vTaskStartScheduler() in main after setting up tasks.
//Its already called for us via arduino. 

void loop() {
  // put your main code here, to run repeatedly:

}

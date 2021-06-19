/**
 * Digikey Intro to FreeRTOS w/ Shawn Hymel
 * Challenge 3: Accepts user input to control blinking led. 
 * Rob Garrone
 */

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif


//Pins 
static const int led_pin = LED_BUILTIN;

//variables
int ledRate = 500;


//our task
void getLEDRate(void *parameter)
{
  while(1){
    if(Serial.available() > 0) //if we have something
    {
      int incoming = Serial.parseInt(); // assign read value to ledRate
      if(incoming != 0) //why is this needed w/ parseInt? why does parseInt even get to return 0?
      {
        ledRate = incoming;
        Serial.print("LED rate updated to ");
        Serial.println(ledRate);
      }
    }
  }
}

void toggleLED(void *parameter)
{
  while(1)
  {
   digitalWrite(led_pin,HIGH);
  vTaskDelay(ledRate / portTICK_PERIOD_MS);
  digitalWrite(led_pin,LOW);
  vTaskDelay(ledRate / portTICK_PERIOD_MS);
  }
}

//setup() and loop() run as their own tasks w/ priority 1 in core 1. 

void setup() {

  //config pins
  pinMode(led_pin,OUTPUT);

  //start serial
  Serial.begin(115200);
  //wait a second for config
  vTaskDelay(1000/portTICK_PERIOD_MS);
  //show time!
  Serial.println("Enter a number in ms to specify LED on/off blink rate");

  xTaskCreatePinnedToCore(    //use xTaskCreate() in vanilla FREERTOS
    getLEDRate,              //our task function
    "Get LED Rate",           // name of task
    1024,                   //Stack size (bytes in ESP32,words in FREERTOS
    NULL,                   //Parameter to pass to function
    1,                      //priority (higher=greater priority, 0 to configMAX_PRIORITIES-1)
    NULL,                   //Task Handle
    app_cpu);               //Run on one core for demo purposes

    xTaskCreatePinnedToCore(    //use xTaskCreate() in vanilla FREERTOS
    toggleLED,              //our task function
    "Toggle LED",           // name of task
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

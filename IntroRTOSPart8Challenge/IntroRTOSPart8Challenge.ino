/**
 * FreeRTOS SW Timer Challenge
 * 
 * Challenge: Create a task that echoes characters back to the serial terminal (as we’ve done in previous challenges). 
 * When the first character is entered, the onboard LED should turn on. It should stay on so long as characters are being entered.
 * se a timer to determine how long it’s been since the last character was entered 
 * (hint: you can use xTimerStart() to restart a timer’s count, even if it’s already running). 
 * When there has been 5 seconds of inactivity, your timer’s callback function should turn off the LED.
 * 
 * 
 * NOTES:
 * One of the easier challenges I thought. Useful to know!
 * 
 * 
 * Date: June 28, 2021
 * Author: Rob Garrone
 */

// You'll likely need this on vanilla FreeRTOS
//#include semphr.h

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Pins (change this if your Arduino board does not have LED_BUILTIN defined)
static const int led_pin = LED_BUILTIN;

//Global Variables
static TimerHandle_t TimerLED = NULL;

//*****************************************************************************

void TimerLEDCallback(TimerHandle_t xTimer)
{
  Serial.println("CLI input timer expired!");
  //turn off LED if we expire
  digitalWrite(LED_BUILTIN, LOW);
}

// Tasks
void CLIHandler(void *parameters)
{
  char c;
  //xTimerStart(TimerLED,portMAX_DELAY); //im just here to refresh RG's memory
  
  while(1)
  {
    if (Serial.available()>0)
    {
      //if we have serial, turn LED on
      digitalWrite(LED_BUILTIN,HIGH);
      xTimerStart(TimerLED,portMAX_DELAY); //Restart our one-shot!
      c = Serial.read();
      Serial.print(c);
    }
  }
}


//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() {
  
  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS SW Timer Challenge---");

  //initialize LED pin to LOW (off)
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN,LOW);
  
  TimerLED = xTimerCreate(
                          "LED Toggle Timer",         //Name
                          5000/portTICK_PERIOD_MS,    //Period (in ticks)
                          pdFALSE,                    //Auto-reload 
                          (void *)0,                  //Timer ID #
                          TimerLEDCallback);          //Callback function

  if(TimerLED == NULL)
  {
      Serial.println("Couldn't Create a Timer!");
  }

  else
  {
    Serial.println("Timer Created!");
  }
                          
  // Start task 1
  xTaskCreatePinnedToCore(
                          CLIHandler,
                          "CLI Handler",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);

  Serial.println("All Tasks Created!");

  vTaskDelete(NULL);
}

void loop() {
  // Do nothing
}

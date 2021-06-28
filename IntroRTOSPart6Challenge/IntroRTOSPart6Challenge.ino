/**
 * FreeRTOS Mutex Challenge
 * 
 * Pass a parameter to a task using a mutex.
 * 
 * Initial code copied from Digikey/Shawn Hymel: 
 * https://www.digikey.com/en/maker/projects/introduction-to-rtos-solution-to-part-6-freertos-mutex-example/c6e3581aa2204f1380e83a9b4c3807a6
 * 
 * After some attempting I peaked at the solution - the critical thing is to give back the mutex IN the blink task
 * (in other words, flagging to blocked setup that we've read in the delay variable).
 * AFTER copying the delay value from setup. This forces the delay value from setup to be copied from setup's stack memory 
 * into the local delay variable in the stack memory of the blink task. 
 * 
 * This is kinda atypical use. the typical thing is 
 * 
 * xSemaphoreTake()
 * //do stuff w/ common resources
 * xSemaphoreGive()
 * 
 * within some task
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
static SemaphoreHandle_t mutex = NULL;

//*****************************************************************************
// Tasks

// Blink LED based on rate passed by parameter
void blinkLED(void *parameters) {
  int num = 0;
  if(mutex != NULL) //if we have a created mutex
  {
    // Copy the parameter into a local variable
    //knowledge check: this is dereferencing the void pointer parameters after it gets casted to an int pointer.
    num = *(int *)parameters;  
    xSemaphoreGive(mutex); //give the mutex back - its just a flag saying "Hey we set this varible"
  }
    

  // Print the parameter
  Serial.print("Received: ");
  Serial.println(num);

  // Configure the LED pin
  pinMode(led_pin, OUTPUT);

  // Blink forever and ever
  while (1) {
    digitalWrite(led_pin, HIGH);
    vTaskDelay(num / portTICK_PERIOD_MS);
    digitalWrite(led_pin, LOW);
    vTaskDelay(num / portTICK_PERIOD_MS);
  }
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() {

  long int delay_arg; //this is a local variable of setup and resides in the STACK of setup.

  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Mutex Challenge---");
  Serial.println("Enter a number for delay (milliseconds)");

  // Wait for input from Serial
  while (Serial.available() <= 0);

  // Read integer value
  delay_arg = Serial.parseInt();
  Serial.print("Sending: ");
  Serial.println(delay_arg);

  mutex = xSemaphoreCreateMutex(); //
  xSemaphoreTake(mutex,portMAX_DELAY); // take mutex

  // Start task 1
  xTaskCreatePinnedToCore(blinkLED,
                          "Blink LED",
                          1024,
                          (void *)&delay_arg,
                          1,
                          NULL,
                          app_cpu);

  //need to stop setup task from completing before blinkLED reads in delay_arg!
  //mutex is given back within blinkLED task
  xSemaphoreTake(mutex,portMAX_DELAY);//from the docs, portMAX_DELAY suspends indefinitely
  

  // Show that we accomplished our task of passing the stack-based argument
  Serial.println("Done!");
}

void loop() {
  
  // Do nothing but allow yielding to lower-priority tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

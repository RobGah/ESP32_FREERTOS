/**
 * Digikey Intro to FreeRTOS w/ Shawn Hymel
 * Challenge 4: Using Queues
 * Rob Garrone
 */

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

//Settings
static const uint8_t queue_len = 8;
static const int led_pin = LED_BUILTIN; 

//global variables
static QueueHandle_t AtoB_queue;
static QueueHandle_t BtoA_queue;
static int blinkval =0;
static uint8_t blinkcount =0;
static volatile bool printFlag = 0;
void * msgptr = NULL;

//Listen Task
void taskA(void *parameter)
{
  char bstring[10];
  char c;
  const uint8_t arrsize = 255;
  char userinput[arrsize];
  uint8_t idx = 0;

  while(1)
  {
    //If B gives us something, print it!
    if(xQueueReceive(BtoA_queue,(void *)&bstring,0) == pdTRUE)
    {
      Serial.println(bstring);
    }
    
    //read user input
    if(Serial.available()>0)
    {
      c = Serial.read();
      if(idx<arrsize-1)
      {
        userinput[idx] = c;        
        idx++;
        
        if(c=='\n')
        {
          userinput[idx-1] == '\0';
          
          
          int delayval;
          char command [20];
          //parse user input for a command and a delay value
          sscanf(userinput,"%s %d", command, &delayval);

          if(command =="delay")
          {            
            //send our msg, wait 10 ticks
            //I don't think we'd do anything differently if it WASN'T successful sending
            xQueueSend(AtoB_queue,(void *)&delayval,10);
 
            idx=0;
          }
        }
      }
    }
  }
}

void taskB(void *parameter)
{
  char astring[] = "Blinked!";
  
  while(1)
  {
    //update blinkval based on A's sent data
    xQueueReceive(AtoB_queue,(void *)&blinkval,0);

    Serial.print("Current blink rate is ");
    Serial.println(blinkval);
    //blink!
    digitalWrite(led_pin,HIGH);
    vTaskDelay(blinkval / portTICK_PERIOD_MS);
    digitalWrite(led_pin,LOW);
    vTaskDelay(blinkval / portTICK_PERIOD_MS);
    //every blink increments the count
    blinkcount++;
  
    if(blinkcount == 100)
    {
      xQueueSend(BtoA_queue,(void*)astring,10);
    }
  }
}

void printMessage(void *parameter)
{
  while(1)
  {
    if(printFlag==1)
    {

      //FIX THIS! WE need to always have msgptr be a char pointer!
      Serial.println((char*)msgptr); //print message

      // Give amount of free heap memory (uncomment if you'd like to see it)
      //Serial.print("Free heap (bytes): ");
      //Serial.println(xPortGetFreeHeapSize());
      
      vPortFree(msgptr);
      msgptr = NULL;
      printFlag = 0; //reset printFlag
      }
  }
}


//setup() and loop() run as their own tasks w/ priority 1 in core 1. 

void setup() 
{
  //start serial
  Serial.begin(115200);
  //wait a second for config
  vTaskDelay(1000/portTICK_PERIOD_MS);
  //show time!
  Serial.println();
  Serial.println("---------------------------------");
  Serial.println("Program will echo what is given:");

  xTaskCreatePinnedToCore(    //use xTaskCreate() in vanilla FREERTOS
    taskA,              //our task function
    "Task A",           // name of task
    1024,                   //Stack size (bytes in ESP32,words in FREERTOS)
    NULL,                   //Parameter to pass to function
    1,                      //priority (higher=greater priority, 0 to configMAX_PRIORITIES-1)
    NULL,                   //Task Handle
    app_cpu);               //Run on one core for demo purposes

    xTaskCreatePinnedToCore(    //use xTaskCreate() in vanilla FREERTOS
    taskB,              //our task function
    "Task B",           // name of task
    1024,                   //Stack size (bytes in ESP32,words in FREERTOS
    NULL,                   //Parameter to pass to function
    1,                      //priority (higher=greater priority, 0 to configMAX_PRIORITIES-1)
    NULL,                   //Task Handle
    app_cpu);               //Run on one core for demo purposes

    xTaskCreatePinnedToCore(    //use xTaskCreate() in vanilla FREERTOS
    printMessage,              //our task function
    "print Message",           // name of task
    1024,                   //Stack size (bytes in ESP32,words in FREERTOS
    NULL,                   //Parameter to pass to function
    1,                      //priority (higher=greater priority, 0 to configMAX_PRIORITIES-1)
    NULL,                   //Task Handle
    app_cpu);               //Run on one core for demo purposes

    vTaskDelete(NULL); //get rid of "setup and loop" task
}

void loop() {
  // put your main code here, to run repeatedly:

}

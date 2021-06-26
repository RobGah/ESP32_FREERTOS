/**
 * Digikey Intro to FreeRTOS w/ Shawn Hymel
 * Challenge 5: Using Queues
 * Rob Garrone
 * 
 * NOTES: 
 * My solution is slightly different but equivalent to Shawn's. He uses a struct
 * to capture messages, whereas I just go w/ plain old strings. I did peak at the
 * solution Shawn came up with after I got mine to a "workable" state, and cleaned
 * some stuff up based on his solution.
 * 
 * Description:
 * One task performs basic echo on Serial. If it sees "delay" followed by a
 * number, it sends the number (in a queue) to the second task. If it receives
 * a message in a second queue, it prints it to the console. The second task
 * blinks an LED. When it gets a message from the first queue (number), it
 * updates the blink delay to that number. Whenever the LED blinks 100 times,
 * the second task sends a message to the first task to be printed.
 */

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

//Settings
static const uint8_t queue_len = 5;
static const int led_pin = LED_BUILTIN; 
static const char command[] = "delay";
static const uint8_t commandlen = strlen(command);
static const uint8_t userbuffersize = 255;
static const uint8_t stringbuffersize =25;


//global variables
static QueueHandle_t AtoB_queue;
static QueueHandle_t BtoA_queue;

//Listen Task
void taskA(void *parameter)
{
  char fromBstring[stringbuffersize];
  char toBstring[stringbuffersize];
  char c;
  char clicommand[stringbuffersize];
  char userinput[userbuffersize];
  uint8_t idx = 0;

  //clear all buffers
  memset(userinput,0,userbuffersize);
  memset(fromBstring,0,stringbuffersize);
  memset(toBstring,0,stringbuffersize);

  while(1)
  {
    //If B gives us something, print it!
    if(xQueueReceive(BtoA_queue,(void *)&fromBstring,0) == pdTRUE)
    {
      Serial.print("Received from Task B: ");
      Serial.println(fromBstring);
    }
    
    //read user input
    if(Serial.available()>0)
    {
      c = Serial.read();
      if(idx<userbuffersize-1)
      {
        userinput[idx] = c;        
        idx++;
      } 
      if((c=='\n')|| (c=='\r'))
      {       
        Serial.println();         
        //parse user input for a command and a delay value
        sscanf(userinput,"%s %s", &clicommand, &toBstring);
        //Serial.print("Command is: ");
        //Serial.println(clicommand);
        //Serial.print("Delay val is: ");
        //Serial.println(toBstring);
        //Serial.print("strcmp returns ");
        //Serial.println(strcmp(clicommand,command));
        if(strcmp(clicommand,command) == 0)
        
        //could also do this:
        //if(memcmp(clicommand,command,commandlen)==0)
        {
          //check if the delay value is a number
          if(isDigit(toBstring[0]))
            {            
            //send our msg, wait 10 ticks
            //I don't think we'd do anything differently if it WASN'T successful sending
            xQueueSend(AtoB_queue,(void *)&toBstring,10);
          }
          memset(userinput,0,userbuffersize);
          idx=0;
        }
        else
        {
          //clear userinput buffer to accept another read
          memset(userinput,0,userbuffersize);
          idx=0;
        }
      }
      else
      {
        Serial.print(c);
      }
    }
  }
}

void taskB(void *parameter)
{
  //setup
  char toAstring[stringbuffersize];
  char fromAstring[stringbuffersize];
  int blinkcount = 0;  
  int blinkint = 500; 

  memset(toAstring,0,stringbuffersize);
  memset(fromAstring,0,stringbuffersize);


  
  while(1)
  {
    //update blinkval based on A's sent data
    if(xQueueReceive(AtoB_queue,(void *)&fromAstring,0)==pdTRUE)
    {
      blinkint = atoi(fromAstring);
      blinkint = abs(blinkint);
      strcpy(toAstring,fromAstring);
      strcat(toAstring," ms delay applied!");
      xQueueSend(BtoA_queue,(void *)toAstring,10);
    }
  
    //blink!
    digitalWrite(led_pin,HIGH);
    vTaskDelay(blinkint / portTICK_PERIOD_MS);
    digitalWrite(led_pin,LOW);
    vTaskDelay(blinkint / portTICK_PERIOD_MS);
    //every blink increments the count
    blinkcount++;
  
    if(blinkcount == 100)
    {
      strcpy(toAstring,"Blinked 100 Times!");
      xQueueSend(BtoA_queue,(void*)&toAstring,10);
      memset(toAstring,0,stringbuffersize);
      blinkcount = 0;
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
  Serial.println("Enter delay followed by a time in ms e.g. \"delay 500\":");

  pinMode(LED_BUILTIN, OUTPUT);

  AtoB_queue = xQueueCreate(queue_len,sizeof(int));
  BtoA_queue = xQueueCreate(queue_len,25*sizeof(char));
  

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

    vTaskDelete(NULL); //get rid of "setup and loop" task
}

void loop() {
  // put your main code here, to run repeatedly:

}

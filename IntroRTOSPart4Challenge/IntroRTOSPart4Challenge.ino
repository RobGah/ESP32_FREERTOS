/**
 * Digikey Intro to FreeRTOS w/ Shawn Hymel
 * Challenge 4: Character Echo w/ Heap memory
 * Rob Garrone
 */

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

//global variables
bool printFlag = 0; //flag for telling print task we have something to echo
char message[80];

//Listen Task
void getIncomingSerial(void *parameter)
{
  static const int arrsize = 80; //goes to stack - n.c. in heap
  int idx = 0; //4 bytes
  char * incoming =  (char *)pvPortMalloc(arrsize*sizeof(char)); //80 char (byte) piece of heap 

  while(1){
    if(Serial.available() > 0) //if we have something
    {
      incoming[idx] = Serial.read(); // assign read value to 

      if((*)incoming[idx] == "\n")
      {
        strcpy(message,incoming);
        printFlag = 1; //notify print task that we have something
        idx = 0; //reset index
        pvPortFree(incoming); //free our memory
        return; //job done
      }
      else idx++; //if its not \n, increment index and press on
    }
    else 
    {
      // if we don't have anything over serial, free heap.
      pvPortFree(incoming);
      return;
    } 
  }
}

void printMessage(void *parameter)
{
  while(1)
  {
    if(printFlag==1)
    {
      Serial.print(message); //print message
      printFlag == 0; //reset printFlag
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
  Serial.println("Program will echo what is given");

  xTaskCreatePinnedToCore(    //use xTaskCreate() in vanilla FREERTOS
    getIncomingSerial,              //our task function
    "Get Incoming Serial",           // name of task
    1024,                   //Stack size (bytes in ESP32,words in FREERTOS)
    NULL,                   //Parameter to pass to function
    1,                      //priority (higher=greater priority, 0 to configMAX_PRIORITIES-1)
    NULL,                   //Task Handle
    app_cpu);               //Run on one core for demo purposes

    xTaskCreatePinnedToCore(    //use xTaskCreate() in vanilla FREERTOS
    printMessage,              //our task function
    "Print Message",           // name of task
    1024,                   //Stack size (bytes in ESP32,words in FREERTOS
    NULL,                   //Parameter to pass to function
    1,                      //priority (higher=greater priority, 0 to configMAX_PRIORITIES-1)
    NULL,                   //Task Handle
    app_cpu);               //Run on one core for demo purposes

    vTaskDelete(NULL); //get rid of "setup and loop" task
}

//NOTE - seems like priotity 1 is the highest priority? I can't get priority 0 to work. 

//IF this were vanilla FREERTOS, you'd need to call the scheduler via vTaskStartScheduler() in main after setting up tasks.
//Its already called for us via arduino. 

void loop() {
  // put your main code here, to run repeatedly:

}

/**
 * 
 * FreeRTOS Hardware Timer Challenge
 * 
 * Challenge: create a sampling, processing, and interface system using hardware interrupts and 
 * whatever kernel objects (e.g. queues, mutexes, and semaphores) you might need.
 * 
 * You should implement a hardware timer in the ESP32 (here is a good article showing how to do that) that samples from an ADC pin once every 100 ms. 
 * This sampled data should be copied to a double buffer (you could also use a circular buffer). 
 * Whenever one of the buffers is full, the ISR should notify Task A.
 * Task A, when it receives notification from the ISR, should wake up and compute the average of the previously collected 10 samples. 
 * Note that during this calculation time, the ISR may trigger again. This is where a double (or circular) buffer will help: 
 * you can process one buffer while the other is filling up.
 * 
 * When Task A is finished, it should update a global floating point variable that contains the newly computed average. 
 * Do not assume that writing to this floating point variable will take a single instruction cycle! 
 * You will need to protect that action as we saw in the queue episode.
 * 
 * Task B should echo any characters received over the serial port back to the same serial port. 
 * If the command “avg” is entered, it should display whatever is in the global average variable.
 * This is like a “final project” in that you will need to use many of the concepts we covered in previous lectures and challenges.
 * 
 * Notes: 
 * Circular Buffer info is rampant on youtube and was given in a previous example. 
 * I wanted to play w/ double buffers because its not a structure that I've used before. 
 * Best explanation of double buffering I've found: 
 * https://www.brainkart.com/article/Double-buffering_7738/
 * 
 * Got it to compile and do stuff, but always was getting avg = 0.00. Peaking at solution, it seems the way to go is
 * to have a semaphore in CalcAvg for done_reading vs having a semaphore buf_ready going from interrupt -> CalcAvg.
 * 
 * That and having an overrun flag. 
 * 
 * Date: 07/04/2021
 * Author: Rob Garrone (starter code and challenge by Shawn Hymel)
 * 
 */

// You'll likely need this on vanilla FreeRTOS
//#include semphr.h

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
enum {BUF_SIZE = 10};                       // Size of buffer array
//enum {QUEUE_SIZE = 10};                     // Size of intertask comms Queues
static const uint16_t timer_divider = 8;  // 80Mhz divded by this
static const uint64_t timer_max = 1000000;  // max # of ticks, divide again
static const int adc_pin = A0;              // Analog input pin 
static const char command[] = "avg";        // CLI command to recognize
static const uint8_t userbuffersize = 255;  // User's input buffer
static const uint8_t stringbuffersize = 25;
static const int cli_delay = 20;


// Globals
static volatile int bufA[BUF_SIZE];                  // A buffer
static volatile int bufB[BUF_SIZE];                  // B buffer
static volatile int * write_buf_ptr = bufA;          // init write buffer to A
static volatile int * read_buf_ptr = bufB;           // init read buffer to B
static volatile int write_idx = 0;                   // Write buffer index
static float adc_avg = 0; //initialize to 0;         // I guess I want it global?
static volatile bool overrun = false;                // Peaked at solution

// Semaphores, Mutexes, Spinlocks
static SemaphoreHandle_t done_reading = NULL;        // Flag buffer as used up by average task
static SemaphoreHandle_t buf_ready = NULL;           // Flag buffer as ready to be read
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;          // cool spinlock broh

// Queues
//static QueueHandle_t CalculatetoCLI;        // Queue from average to CLIHandler task

// Timers
hw_timer_t * timer = NULL;                  // HW timer

//*****************************************************************************

// Functions
void buf_ptr_swap(void)
{
  // Create a temporary ptr to hold 
  volatile int * temp_ptr = NULL;
  //Assign read -> write to give to avg calc task
  //temp takes read, read takes write, write takes temp (old read)
  temp_ptr = write_buf_ptr;
  write_buf_ptr = read_buf_ptr;
  read_buf_ptr = temp_ptr;
  
}

// Timers
void IRAM_ATTR onTimer()
{
  
  static BaseType_t isWoken = pdFALSE;
  
  if((write_idx < BUF_SIZE) && (overrun == false))
  {
    //read analog, write to write-only buffer
    write_buf_ptr[write_idx] = analogRead(adc_pin);

    //update counter, not in critical section as we're in the ISR
    write_idx++;
  }
    
  if(write_idx >= BUF_SIZE)
  {
    if(xSemaphoreTakeFromISR(done_reading,&isWoken)==pdFALSE)
    {
      overrun = true;
    }
    //if no overrun, swap buffers and reset write_idx
    if(overrun == false)
    { 
      buf_ptr_swap();
      write_idx = 0;
      xSemaphoreGiveFromISR(buf_ready,&isWoken);
    }

  if(isWoken == pdTRUE)
  {
    portYIELD_FROM_ISR();
  }
  
  }
}

// Tasks
void CLIHandler (void *parameter)
{
  char c;
  char fromAvgString[stringbuffersize];
  char userinput[userbuffersize];
  uint8_t idx = 0;

  //clear all buffers
  memset(fromAvgString,0,stringbuffersize);
  memset(userinput,0,userbuffersize);
  
  while(1)
  {
//    //check Queue for incoming messages from average function
//    if(xQueueReceive(CalculatetoCLI,(void *)&fromAvgString,0) == pdTRUE)
//    {
//      Serial.print("Current average is: ");
//      Serial.println(fromAvgString);
//      memset(fromAvgString,0,stringbuffersize);
//    }
    // if there's something in serial, get it
    if(Serial.available() > 0)
    {
      c = Serial.read();
      if(idx<userbuffersize-1)
      {
       userinput[idx] = c;
       idx++; 
      }
      //if we're at the end of a string
      if(c=='\n'||c=='\r')
      {
        char clicommand[stringbuffersize];
        Serial.println();         
        //parse user input for a command and a delay value
        sscanf(userinput,"%s", &clicommand);
        
        //if the cli command is our command
        if(strcmp(clicommand,command) == 0)
        {
          Serial.println("avg is");
          Serial.println(adc_avg);

          //clear everything
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
    // This is needed to stop this task from hogging up processor time from CalculateAvg.
    vTaskDelay(cli_delay / portTICK_PERIOD_MS); 
  }
}

void CalculateAvg(void * parameter)
{
  int sum = 0;
  float avg = 0;
  while(1)
  {
    xSemaphoreTake(buf_ready,portMAX_DELAY); //blocked until we get a go signal
    //Calc Avg
    sum = 0;
    for(int i = 0; i<10;i++)
    {
      sum += read_buf_ptr[i];
    }

    avg = (float)sum/BUF_SIZE;
    
    portENTER_CRITICAL(&spinlock);
    adc_avg = avg;
    overrun = 0;
    xSemaphoreGive(done_reading);
    portEXIT_CRITICAL(&spinlock);
  }
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() 
{
  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS HW Timer Solution---");

  //Create semaphores
  done_reading = xSemaphoreCreateBinary();
  buf_ready = xSemaphoreCreateBinary();

  xSemaphoreGive(done_reading); // prime done_reading, don't prime buf_ready
  //ensure semaphore actually gets made
  
  if((done_reading == NULL) || buf_ready == NULL)
  {
    ESP.restart(); // cool trick fromt the solution!
  }
  
  // Timer setup
  timer = timerBegin(0, timer_divider, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, timer_max, true);
  timerAlarmEnable(timer);

  // CLI Handler
  xTaskCreatePinnedToCore(CLIHandler,
                            "CLI Handler",
                            1024,
                            NULL,
                            2, // Give higher priority to handling CLI
                            NULL,
                            app_cpu);

  // Calc Avg
    xTaskCreatePinnedToCore(CalculateAvg,
                            "Calculate Average",
                            1024,
                            NULL,
                            1,
                            NULL,
                            app_cpu);

 
  // Notify that all tasks have been created
  Serial.println("All tasks created");

  vTaskDelete(NULL);
}

void loop() {
  
  // Do nothing but allow yielding to lower-priority tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

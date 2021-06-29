/**
 * 
 * FreeRTOS Counting Semaphore Challenge
 * 
 * Challenge: use a mutex and counting semaphores to protect the shared buffer 
 * so that each number (0 throguh 4) is printed exactly 3 times to the Serial 
 * monitor (in any order). Do not use queues to do this!
 * 
 * Hint: you will need 2 counting semaphores in addition to the mutex, one for 
 * remembering number of filled slots in the buffer and another for 
 * remembering the number of empty slots in the buffer.
 * 
 * Date: 06/29/2021
 * Author: Rob Garrone (starter code and challenge by Shawn Hymel)
 * License: 0BSD
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
enum {BUF_SIZE = 5};                  // Size of buffer array
static const int num_prod_tasks = 5;  // Number of producer tasks
static const int num_cons_tasks = 2;  // Number of consumer tasks
static const int num_writes = 3;      // Num times each producer writes to buf

// Globals
static int buf[BUF_SIZE];             // Shared buffer
static int head = 0;                  // Writing index to buffer
static int tail = 0;                  // Reading index to buffer
static SemaphoreHandle_t bin_sem;     // Waits for parameter to be read
static SemaphoreHandle_t buf_mutex;   // Protects buf and serial
static SemaphoreHandle_t filled_slots;// Counts # of filled slots
static SemaphoreHandle_t empty_slots; // Counts # of empty slots

//*****************************************************************************
// Tasks

// Producer: write a given number of times to shared buffer
void producer(void *parameters) {

  // Copy the parameters into a local variable
  int num = *(int *)parameters;

  // Release the binary semaphore
  xSemaphoreGive(bin_sem);

  // Fill shared buffer with task number
  for (int i = 0; i < num_writes; i++) {
    
    // writing? take an empty slot in the buffer 
    // wait for an empty slot if we don't have one avail
    xSemaphoreTake(empty_slots,portMAX_DELAY);
    
    // Critical section (accessing shared buffer)
    // take mutex to protect buffer
    xSemaphoreTake(buf_mutex,portMAX_DELAY);
    buf[head] = num;
    head = (head + 1) % BUF_SIZE; // increments head after write. If head+1=BUF_SIZE, goes back to 0.
    xSemaphoreGive(buf_mutex); // release mutex 

    //after a write, give a filled slot
    xSemaphoreGive(filled_slots);
    
  }

  // Delete self task
  vTaskDelete(NULL);
}

// Consumer: continuously read from shared buffer
void consumer(void *parameters) {

  int val;

  // Read from buffer
  while (1) {

    // reading? take a filled slot
    // wait for a filled slot prior to a read
    xSemaphoreTake(filled_slots,portMAX_DELAY);
    
    // Critical section (accessing shared buffer and Serial)
    xSemaphoreTake(buf_mutex, portMAX_DELAY);
    val = buf[tail];
    tail = (tail + 1) % BUF_SIZE; //increments tail after read, if tail+1 = BUF_SIZE, goes back to 0
    Serial.println(val);
    xSemaphoreGive(buf_mutex);

    // after a read, empty a slot
    xSemaphoreGive(empty_slots);

    // eventually we'll get to a point where we have 5 empty slots and 0 filled slots
    // blocking consumers as there's no filled_slot semaphores to take!
  }
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() {

  char task_name[12];
  
  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Semaphore Alternate Solution---");

  // Create mutexes and semaphores before starting tasks
  bin_sem = xSemaphoreCreateBinary();
  buf_mutex = xSemaphoreCreateMutex();
  filled_slots = xSemaphoreCreateCounting(5,0);
  empty_slots = xSemaphoreCreateCounting(BUF_SIZE,BUF_SIZE); // start off totally empty

  // Start producer tasks (wait for each to read argument)
  for (int i = 0; i < num_prod_tasks; i++) {
    sprintf(task_name, "Producer %i", i);
    xTaskCreatePinnedToCore(producer,
                            task_name,
                            1024,
                            (void *)&i,
                            1,
                            NULL,
                            app_cpu);
    xSemaphoreTake(bin_sem, portMAX_DELAY);
  }

  // Start consumer tasks
  for (int i = 0; i < num_cons_tasks; i++) {
    sprintf(task_name, "Consumer %i", i);
    xTaskCreatePinnedToCore(consumer,
                            task_name,
                            1024,
                            NULL,
                            1,
                            NULL,
                            app_cpu);
  }

  // Works w/o this but the given solution has this protected w/ the mutex.
  // consistent and likely good practice!
  xSemaphoreTake(buf_mutex,portMAX_DELAY);
  // Notify that all tasks have been created
  Serial.println("All tasks created");
  xSemaphoreGive(buf_mutex);
}

void loop() {
  
  // Do nothing but allow yielding to lower-priority tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

/*
# Family Name: Walia

# Given Name:  Amanpreet

#
*/

#include "sch-helpers.h"
#include <stdio.h>
#include <stdlib.h>




process processes[MAX_PROCESSES+1];   // a large structure array to hold all processes read from data file
int numberOfProcesses;              // total number of processes
int next;                         // variable to decide which processes to enter ready queue
int waitingTime;                 /* total waiting time */
int contextSwitchCount;             /* total context switch */
long int ticks ;                   //CPU clock
int pTime;                  /* Number of steps for processor */


/************* Simulating MultiProcessor************************/
process *Processors[NUMBER_OF_PROCESSORS];

/**
* Ready process queue and waiting process queue
**/
process_queue ready;
process_queue wait;

/**
* Temporary Queue to do intermediate tasks
**/
process *temp[MAX_PROCESSES];
int tempSize = 0;


/********************** Functions Related to task scheduling ***********************/

/* Add the process arriving at any given time to Ready Queue */
void addProcess(){
  while ((next < numberOfProcesses) && (processes[next].arrivalTime <= ticks)){
    temp[tempSize] = &processes[next];
    tempSize++;
    next++;
  }
}

/** Obtaine process at the head of ready queue
*   remove it for further processing
*/
process *nextReadyProcess(){
  if (ready.size == 0){
    return NULL;
  }
  process *proc = ready.front->data;
  dequeueProcess(&ready);
  return proc;
}

/*
* If currently running Process has finished its CPU burst then move it to waiting queue
* if it has bursts left otherwise terminate the process.
* If the process is finished , signal termination by setting clock time to the end time of Process.
*/

void ProcessorToWait(){
  int i = 0;
  while(i < NUMBER_OF_PROCESSORS){
    if (Processors[i] != NULL){   // Processor Holds a valid Process
      if (Processors[i]->bursts[Processors[i]->currentBurst].step == Processors[i]->bursts[Processors[i]->currentBurst].length){ // Current Burst has reached its end

        Processors[i]->currentBurst++; // Moving to next Burst

        if (Processors[i]->currentBurst < Processors[i]->numberOfBursts){ // Bursts for process haven't Finished
        enqueueProcess(&wait, Processors[i]);                            // Move to waiting Queue
      }
      else{
        Processors[i]->endTime = ticks;                                 // Last Burst for process, Process ends,set ending time to current time
      }

      Processors[i] = NULL;                                              //Free the Processor
    }

  }

  i++;                                                                    // Move to next processor
}
}
/*Add process at the head of ready queue to the processors */
void addToProcessors(){
  int i;

  qsort(temp, tempSize, sizeof(process*), compareByPID); //sort temporary queue by process ID

  for (i=0;i<tempSize;i++) {
    enqueueProcess(&ready, temp[i]);                     //Enqueue each process to Ready queue
  }

  tempSize = 0;                                          // set Temporary queue work again from beginning

  for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
    if (Processors[i] == NULL){                          // If a processor is free , assign it a new process.
      Processors[i] = nextReadyProcess();
    }
  }
}


/**
* If a process in Waiting Queue has finished its I/O burst , pull it out to enqueue it temporary queue
* which after sorting by PID will be put on Ready Queue.
*/
void waitToReady(void){

  int i;
  int Wsize = wait.size;

  for(i = 0; i < Wsize; i++){
    process *proc = wait.front->data;
    dequeueProcess(&wait);                      //obtain the process at the head of waiting queue

    if(proc->bursts[proc->currentBurst].step == proc->bursts[proc->currentBurst].length){  // Current I/O burst has been finished
      proc->currentBurst++;                     //Increment Burst Counter
      temp[tempSize++] = proc;                  // Enqueue the process to temporary Queue
    }
    else{
      enqueueProcess(&wait, proc);
    }
  }
}
/**
* Iterates over all CPU's
* return the total number of
* currently running processes
*/
int runningProcesses(void){
  int runningProcesses = 0;
  int i;
  for (i = 0; i < NUMBER_OF_PROCESSORS; i++){
    if (Processors[i] != NULL){
      runningProcesses++;
    }
  }
  return runningProcesses;
}
/**
* Function to update waiting processes
*/
void updateWaitingQueue()
{
  int i = 0;
  int Wsize = wait.size;

  while(i<Wsize) {
    // Obtain the frontal element of Waiting Queue
    process *start = wait.front->data;
    dequeueProcess(&wait);

    //Update its Burst Time
    start->bursts[start->currentBurst].step++;
    enqueueProcess(&wait, start);

    i++; //Move to next process in waiting Queue
  }

}
/**
* Function to update ready processes
*/
void updateReadyQueue()
{
  int i = 0;
  int Rsize = ready.size;       // Obtain size of Ready Queue

  while(i<Rsize) {

    // Obtain the frontal element of Ready Queue
    process *start = ready.front->data;
    dequeueProcess(&ready);

    // Increment Waiting time for each process
    start->waitingTime++;
    enqueueProcess(&ready, start);

    i++; // Move to next process in ready Queue
  }

}
/**
* Function to update running processes
*/
void updateProcessors()
{
  int i;
  for (i=0;i<NUMBER_OF_PROCESSORS;i++) {
    if (Processors[i] != NULL) {

      // Increase Burst step for each process on Processor
      Processors[i]->bursts[Processors[i]->currentBurst].step++;
    }
  }

}
int main()
{
  int status = 0;      // read status of each process read
  int pr = 0;          // Loop variable for processors
  int i;               // General Loop Variable
  int totalTurnAround = 0; // Variable to to store total turn around time

  /*Resetting every program variable*/

  numberOfProcesses = 0;
  next = 0;
  pTime = 0;
  ticks = 0;
  waitingTime = 0;
  contextSwitchCount = 0;

  /************************************** Setting each processor to point to NULL process*************************************/
  for(;pr < NUMBER_OF_PROCESSORS;pr++)
  {
    Processors[pr] = NULL;
  }
  /************************************** Initialize ready and waiting queues*************************************************/

  initializeProcessQueue(&ready);
  initializeProcessQueue(&wait);

  /************************************** Reading and organising processes from Input********************************************/
  while (status=readProcess(&processes[numberOfProcesses]))  {
    if(status==1)  numberOfProcesses ++;                    // it reads pid, arrival_time, bursts to one element of the above struct array
    if (numberOfProcesses > MAX_PROCESSES || numberOfProcesses == 0){
      error_process_limit(numberOfProcesses);// Number of Processes not a legal number
      return -1;
    }
  }

  qsort(processes, numberOfProcesses, sizeof(process), compareByArrival); //Sorting processes by their arrival time.

  /************************************** Process Iterations ******************************************************************/
  while(1)
  {

    /***************************** Processes shuffled between Ready Queue,Processors and Waiting Queue************************/

    addProcess();             /* add newly arriving processes to ready queue*/
    ProcessorToWait();        /* organize processes who finished their CPU burst */
    waitToReady();            /* pick processes for which waiting time is finished and put them in ready queue */
    addToProcessors();        /* add processes in ready queue to processors free for execution */

    /***************************** Updating states for CPU,Processes and Both Queues *****************************************/

    updateWaitingQueue();
    updateReadyQueue();
    updateProcessors();

    pTime += runningProcesses();
    //printf("Processor time Now :%d\n",pTime);

    /****************************Loop Exit************************************************************************************/
    if (runningProcesses() == 0 && (numberOfProcesses == next) && wait.size == 0) // No running Processes ,No process in waiting Queue and the process pointer reach process Count
    // then terminate Loop
    break;

    /**************************** Clock ticks next ***************************************************************************/
    ticks++;
  }

  for (i=0;i<numberOfProcesses;i++) {
    totalTurnAround += processes[i].endTime - processes[i].arrivalTime;
    //printf("waiting time for process %d is :%d\n",i,processes[i].waitingTime);
    waitingTime += processes[i].waitingTime;
  }

  double avgCPUut = (100.0*pTime)/ticks;


  printf("Average waiting time                 : %.2f units\n",waitingTime / (double)numberOfProcesses);
  printf("Average turnaround time              : %.2f units\n",totalTurnAround/(double)numberOfProcesses);
  printf("Time all processes finished          : %ld\n",ticks);
  printf("Average CPU utilization              : %.1lf%%\n",avgCPUut);
  printf("Number of context switches           : %d\n",contextSwitchCount);
  printf("PID(s) of last process(es) to finish : ");

  for (i=0;i<numberOfProcesses;i++) {
    if (processes[i].endTime == ticks) {
      printf("%d ",processes[i].pid);
    }
  }

  printf("\n");
  return 0;
}

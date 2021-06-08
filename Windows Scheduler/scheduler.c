/*
   CS302 
   Assignment 3
   alowther@pnw.edu

*/
#include <windows.h>
#include <stdio.h>
#include <math.h>


typedef struct processor_data {
   int affinityMask;                /* affinity mask of this processor (just one bit set) */
   PROCESS_INFORMATION processInfo; /* process currently running on this processor */
   int running;                     /* 1 when this processor is running a task, 0 otherwise */
} ProcessorData;


/* compare functions for LJF and SJF
 * so we can sort out times using qsort
 */
   int cmpfunc(void const*a, void const *b)
   {
         return (*(int*)a - *(int*)b);
   }
   int cmpfunc2(void const*a, void const *b)
   {
         return -(*(int*)a - *(int*)b);
   }


   void printError(char* functionName);

   int main(int argc, char *argv[])
   {
      int processorCount = 0;       /* the number of allocated processors */
      ProcessorData *processorPool; /* an array of ProcessorData structures */
      HANDLE *processHandles;       /* an array of handles to processes */
      int *handleInPool;       /* where each handle comes from in the pool */
      int *times [argc-2];                   /* array to hold job duration times */
      DWORD handleCount = 0;
      int sType;
      ULONG_PTR processAffinityMask;
      ULONG_PTR systemAffinityMask;                   
      int timeCount = argc - 2 ;  //keeping an index to use for when we launch more processes than there are processors
      int ran=0;    // the jobs that have been ran will always be equal to the index of next job in the array 
      if (argc < 3)
      {
         fprintf(stderr, "usage, %s  SCHEDULE_TYPE  SECONDS...\n", argv[0]);
         fprintf(stderr, "Where: SCHEDULE_TYPE = 0 means \"first come first serve\"\n");
         fprintf(stderr, "       SCHEDULE_TYPE = 1 means \"shortest job first\"\n");
         fprintf(stderr, "       SCHEDULE_TYPE = 2 means \"longest job first\"\n");
         return 0;
      }


      /* Read the schedule type off the command-line. */
      sType = atoi(argv[1]);

      /* Read the job duration times off the command-line. */
      for(int i = 2; i < argc; i++)
      {
         times[i-2] = atoi(argv[i]);
         //fprintf(stdout, "%i \n", times[i-2]);
         
      }

      /* Get the processor affinity mask for this process. */
      
      if(GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask) == 0)
      {
         printError("Get Affinity Mask");
      }

      /* Count the number of processors set in the affinity mask. */
      //simple loop to convert affinity mask to more readable and count processors at the same time
      while(processAffinityMask != 0)
      {
         if((processAffinityMask & 1) == 1)
         {
            processorCount++;
         }
         processAffinityMask = processAffinityMask >> 1;
      }

      /* Create, and then initialize, the processor pool array of data structures. */
      
      processorPool = malloc(processorCount * sizeof(ProcessorData));
      
      if(GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask) == 0)
      {
         printError("Get Affinity Mask");
      }
      /*while the current affinitymask is not zero, set the aff mask of this specific
        process in our array to be the actual processor it should be ran on
        this is printed on my machine as a decimal value where cpu 0 = 1, 1 = 2, 2 = 4, 3 = 8
        */
      int x = 1;
      for(int i = 0; i < processorCount; i++)
      {
         while(processAffinityMask != 0)
         {
            if((processAffinityMask & 1) == 1)
            {
               processorPool[i].affinityMask = pow(2, x-1);
               processorPool[i].running = 0;
               processAffinityMask = processAffinityMask >> 1;
               x++;
               break;
            }
            else
            {
               processAffinityMask = processAffinityMask >> 1;
               x++;
            }
         }
      }

      /* Start the first group of processes. 
      if our type is 1 or 2 we can use qsort along with out 
      previously declared compare fucntions to determine LJF or SJF
      */
      
      if(sType == 1)
      {
         qsort(times, argc-2,argc-2, cmpfunc);
      }
      if(sType == 2)
      {
         qsort(times, argc-2,argc-2, cmpfunc2);

      }

      for(int i = 0; i < processorCount; i++)
      {
        /* if(i >= (argc-2))
         {
            break;
         }*/
         /*using sprintf and create process we can launch as many processes without having to improvise
           until there are more processes than avaliable processors */
           
         
         char temp[100];
         sprintf(temp, "computeProgram_64.exe %d", times[i]);
         LPSTR lpCommandLine = temp;
         STARTUPINFO startInfo;
         ZeroMemory(&startInfo, sizeof(startInfo));
         startInfo.cb = sizeof(startInfo);
         if(!CreateProcess(
                           NULL,
                           lpCommandLine,
                           NULL,
                           NULL,
                           FALSE,
                           NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_SUSPENDED,
                           NULL,
                           NULL,
                           &startInfo,
                           &processorPool[i].processInfo))
         {
            printError("Create Process");
         }
         else
         {
            ran++;
            fprintf(stdout, "Running, will take about %i seconds! \n", times[i]);
            if(SetProcessAffinityMask(processorPool[i].processInfo.hProcess, processorPool[i].affinityMask) == 0)
            {
               printError("Setting Affinity Mask");
            }
            else
            {
               if(ResumeThread(processorPool[i].processInfo.hThread) == (DWORD)-1)
               {
                  printError("Resuming thread");
               }
               else
               {
                  processorPool[i].running = 1;
                  handleCount++;
               }
               
            }
         }
      }

      /* Repeatedly wait for a process to finish and then,
         if there are more jobs to run, run a new job on
         the processor that just became free. */
       processHandles = malloc(handleCount * sizeof(HANDLE));
       handleInPool = malloc(handleCount * sizeof(int));

      int runningCount = 0;
      while (runningCount == 0)
      {
         /* Get, from the processor pool, handles to the currently running processes. */
         /* Put those handles in an array. */
         /* Use a parallel array to keep track of where in the processor pool each handle came from. 
            if processorpool[n] is still running, increment count and add this information to the array
         */
         for(int i = 0; i < processorCount; i++)
         {
            for(int n = 0; n < processorCount; n++)
            {
               if(processorPool[n].running == 1)
               {
                  processHandles[i] = processorPool[n].processInfo.hProcess;
                  handleInPool[i] = n;
                  runningCount++;
               }
            }
         }
         DWORD result;
         if (WAIT_FAILED == (result = WaitForMultipleObjects(handleCount, processHandles, FALSE, INFINITY)))
         {
            printError("WaitForMultipleObjects");
         }
           //now that weve checked if processes are running, we can close the handles of the first group and decrement
           //runningcount is incremented to assure we dont end up in the loop again without reason
            processorPool[handleInPool[result]].running = 0;
            CloseHandle(processorPool[handleInPool[result]].processInfo.hThread);
            CloseHandle(processorPool[handleInPool[result]].processInfo.hProcess);
            runningCount++;
            handleCount--;
        //if we've entered more times than jobs that have ran then we launch the next group
        //this time we use our parrallell array otherwise we wont have a valid handle for setprocess
    
         if(timeCount > ran)
         {
            char temp[100];
            sprintf(temp, "computeProgram_64.exe %d", times[timeCount]);
            LPSTR lpCommandLine = temp;
            STARTUPINFO startInfo;
            ZeroMemory(&startInfo, sizeof(startInfo));
            startInfo.cb = sizeof(startInfo);
            if(!CreateProcess(
                              NULL,
                              lpCommandLine,
                              NULL,
                              NULL,
                              FALSE,
                              NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_SUSPENDED,
                              NULL,
                              NULL,
                              &startInfo,
                              &processorPool[handleInPool[result]].processInfo))
            {
               printError("Create Process");
            }
            else
            {
               fprintf(stdout, "running, will take about %i seconds! \n", times[ran]);
               ran++;
               if(SetProcessAffinityMask(processorPool[handleInPool[result]].processInfo.hProcess, processorPool[handleInPool[result]].affinityMask) == 0)
               {
               
                  printError("Setting Affinity Mask");
               }
               else
               {
               
                  if(ResumeThread(processorPool[handleInPool[result]].processInfo.hThread) == (DWORD)-1)
                  {
                     printError("Resuming thread");
                  }
                           DWORD newresult;
                         if (WAIT_FAILED == (newresult = WaitForMultipleObjects(handleCount, processHandles, FALSE, INFINITY)))
                           {
                              printError("WaitForMultipleObjects");
                           }
                              processorPool[handleInPool[newresult]].running = 0;
                              CloseHandle(processorPool[handleInPool[newresult]].processInfo.hThread);
                              CloseHandle(processorPool[handleInPool[newresult]].processInfo.hProcess);
                              runningCount++;
                              handleCount--;
             
               }
         
            }
     
         } return 0;
      }
      
      fprintf(stdout, "All processes finished successfully!");

      return 0;
   }









   /*******************************************************************
      This function prints out "meaningful" error messages. If you call
      a Windows function and it returns with an error condition, then
      call this function right away and pass it a string containing the
      name of the Windows function that failed. This function will print
      out a reasonable text message explaining the error.
   */
   void printError(char* functionName)
   {
      LPSTR lpMsgBuf = NULL;
      int error_no;
      error_no = GetLastError();
      FormatMessage(
         FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
         NULL,
         error_no,
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
         (LPTSTR)&lpMsgBuf,
         0,
         NULL
      );
      /* Display the string. */
      fprintf(stderr, "\n%s failed on error %d: %s", functionName, error_no, lpMsgBuf);
      //MessageBox(NULL, lpMsgBuf, functionName, MB_OK);
      /* Free the buffer. */
      LocalFree( lpMsgBuf );
   }

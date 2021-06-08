/*
    This program takes one command-line parameter which is an
    integer number of seconds that this program should run for.

    The given parameter specifies CPU time (not wall-clock-time).
    So this program measures how much CPU time it has accumulated
    and compares that to the given parameter. The time when this
    program is in the ready state (when it loses the CPU) is not
    being counted.
*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define COMPUTE_COUNTER 1000000

int main(int argc, char **argv)
{
   HANDLE hProcess = GetCurrentProcess();
   DWORD processId = GetCurrentProcessId();
   DWORD_PTR processAffinityMask;  /* this is a 64 bit data type */
   DWORD_PTR  systemAffinityMask;
   double x = 0;

   union  /* structure used for file time arithmetic */
   {      /* See "Windows System Programming" 4th Ed., J.M. Hart, page 203 */
      FILETIME ft;
      ULONGLONG li;
   } kernelTime,  userTime, junkTime;

   if (argc != 2)
   {
      fprintf(stderr, "usage: %s <seconds>\n", argv[0]);
      return 0;
   }

   int seconds = atoi(argv[1]);

   ULONGLONG timeSoFar = 0;
   ULONGLONG totalTime = 10000000 * seconds; /* each tick is 100 nanoseconds long */

   while (timeSoFar < totalTime)
   {
      /* do a longish calculation on the cpu */
      for (int i = 0; i < COMPUTE_COUNTER; ++i)
      {
         x = cos(i);
      }

      GetProcessAffinityMask(hProcess, &processAffinityMask, &systemAffinityMask);
      int processorNumber = GetCurrentProcessorNumber(); /* this is a 64 bit function */
      /* we only care about the kernel and user times */
      GetProcessTimes(hProcess, &(junkTime.ft), &(junkTime.ft), &(kernelTime.ft), &(userTime.ft));
      timeSoFar = kernelTime.li + userTime.li;
      fprintf(stdout, "pid=%d, AffinityMask=%#4.2x, processor=%d, cpuTimeSoFar=%ld\n",
                      (int)processId, (int)processAffinityMask, processorNumber, timeSoFar);
   }

   return 0;
}

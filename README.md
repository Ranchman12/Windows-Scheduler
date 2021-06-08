# Windows-Scheduler
A CMD program written in C that launches processes for x amount of seconds and displays which processor the processes is on
 this is done by using Windows API's

compile and use scheduler.exe in a CMD window (in the same directory as the other files)

  where the first cmd argument is the schedule type
  0 = first come first serve
  1 = shortest job first
  2 = longest job first
  
  and where the rest of the arguments is the amount of seconds we run a program (laucnhes an instance of compute_Program_64.exe
  
  scheduler 0 5 10 15
  
  should launch a process for 5 seconds, 10 seconds, and 15 seconds

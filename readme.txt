This is the assignment2 for cmput379. 

Usage:
   Compile: make
	    (Will compile the wbs379.c and wbc379.c)
            clean
	    (Will delete the compiled files.) 

   Start server: ./wbs379 8888 -n number.
           (First run, will generate a whiteboard.all after is has been stopped.)
	   or
	   ./wbs379 8888 -f whiteboard.all
           (Use -f whiteboard.all if the whiteboard.all file already exist.)

   Stop server: ps -fel
                (List the processes running, find the wbs379 process and its PID)
		kill -SIGTERM PID
		(stop the process with the PID we found in the process list. 
                 After the wbs has been stopped, a whiteboard.all file will be generated.)
   
   Start client: ./wbc379 127.0.0.1 8888 key.dat
		 (Start the client and ask for user input.)
		 Enter “Get”, “Update” or “Quit”
# Kernel Module Code :
   ## How to run :
        1. ~$ make
        2. ~$ sudo insmod mp1.ko
   ## To check for process times :
        ~$ cat /proc/mp1/status
   ## To remove the module
        ~$ sudo rmmod mp1

# Kernel Description
   ## Initialization:
        1.  Initialize the kernel linked list  *test_head* used to maintain the file data
        2.  We created the **mp1** directory and **status** file for processess to register
        3.  Initialize the workqueue *my_wq* to handle bottom halve of interrupt handler
        4.  Setup a timer and the associated intterupt handler, and set the timer to a given interval
   ## Callbacks:
        We use call backs to define the read and write to the */proc/mp1/status* file, we handle them as follows:
   ###### mp1_read:
            To handle the read from the file, when the request comes, we verify correct parameters. Following this we acquire a lock and read all contents of the linked list to a buffer. Finally we write the buffer to userspace, allowing procs to read the data stored.
   ###### mp1_write:
            This function handles the write request (registration) of a new process to the file system. 
            We read the pid supplied from the user, and convert the pid to int using *kstrtoint_from_user* function. Following this we check if the pid values are correct and the we append it to the linked list : *test_head*, initializing the cpu_time as 0.
   ## Timer Interrupt
        

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
        2.  I created the **mp1** directory and **status** file for processess to register
        3.  Initialize the workqueue *my_wq* to handle bottom halve of interrupt handler
        4.  Setup a timer and the associated intterupt handler, and set the timer to a given interval
   ## Callbacks:
        Callbacks are used to define the read and write to the */proc/mp1/status* file.
   ###### mp1_read:
            To handle the read from the file, when the request comes, we verify correct parameters. Following this we acquire a lock and read all contents of the linked list to a buffer. Finally we write the buffer to userspace, allowing procs to read the data stored.
   ###### mp1_write:
            This function handles the write request (registration) of a new process to the file system. 
            I read the pid supplied from the user, and convert the pid to int using *kstrtoint_from_user* function. Following this I check if the pid values are correct and the we append it to the linked list : *test_head*, initializing the cpu_time as 0.
   ## Timer Interrupt
            I initialized the timer using *setup_timer* and attached it to the interrupt handler function : *my_timer_callback*. And then modified the timer to run after 5 second.
            Inside the interrupt handler, **my_timer_callback** : *setup_work* function, initializes a work of updating the list and adds it to the workqueue. And then modify the timer for another 5 second.
   ##  Updating the Linked List (The queued work)
            The queued work, calls the **upDateFunction**, which iterates over the linked list *test_head*, and either updates or deletes the entry based on the output of the **get_cpu_use**, function. The updation is done under a spin lock, to prevent any inconsistency in the list, if other functions are called.

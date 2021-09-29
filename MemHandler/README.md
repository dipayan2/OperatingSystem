# Kernel Module Code :
   ## How to run :
        1. ~$ cd mp3/
        2. ~$ make
        3. ~$ sudo insmod mp3.ko
   ## To check for process :
        ~$ cat /proc/mp3/status
   ## To remove the module
        ~$ sudo rmmod mp3

# Kernel Description
   ## Initialization:
        1.  Initialize the kernel linked list  *test_head* used to maintain the file data
        2.  I created the **mp3** directory and **status** file for processess to register
        3.  Initialize the workqueue *my_wq* to handle bottom halve of interrupt handler
        4.  Setup a timer and the associated intterupt handler, and set the timer to a given interval
        5.  Allocate memory for virtulaization *vmalloc_area* and sets it PG_reserved bit which prevents system from swapping them out
        6. And finally initializing the character device  
   ## Callbacks:
        Callbacks are used to define the read and write to the */proc/mp1/status* file.
   ###### device_mmap:
            This is the implemetation of the mmap function inside character devices, in this is we get the pfn of the virtual memory abd then map to a physical page
   ###### Reg/DeReg:
            This function handles the (registration/deregistration) of a new process to the file system. 
            In registration, if the pid is valid and existing, then we allocate the process struct with its fault and utilization value. And the first registration starts the workqueue.
            In deregistration, we delete them from list_head and if the list becomes empty the workqueue is flushed, to avoid system resource wastage.
   ## Timer Interrupt
            I initialized the timer using *setup_timer* and attached it to the interrupt handler function : *my_timer_callback*. And then modified the timer to run after 50 milisecond.
            Inside the interrupt handler, **my_timer_callback** : *setup_work* function, initializes a work of updating the list and adds it to the workqueue. 
   ## Bottom Half (Fault Data Collection) (The queued work)
            The queued work, calls the **memFunction**, which iterates over the linked list *test_head*, and either updates or deletes the entry based on the output of the **get_cpu_use**, function. The updation is done under a mutex lock, to prevent any inconsistency in the list, if other functions are called.

        


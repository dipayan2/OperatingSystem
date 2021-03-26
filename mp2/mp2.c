#define LINUX

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h> //simple_from works here
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/slab.h> 
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/kthread.h>  // for thread
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/mutex.h>

#include "mp2_given.h"
#define MAX_PERIOD 1000000
#define READY 1
#define RUNNING 1
#define SLEEPING 0

MODULE_LICENSE("GPL");
MODULE_AUTHOR("dipayan2");
MODULE_DESCRIPTION("CS-423 MP2");

/* LOCKING VARIABLE*/
static DEFINE_SPINLOCK(my_spin);
//static DEFINE_MUTEX(my_lock);




//enum task_state {READY = 0, RUNNING = 1, SLEEPING = 2}; 


struct list_head test_head;

struct kmem_cache* mp2_cache;

long long Cp;

struct timeval time_val ;


struct mp2_task_struct {
  struct task_struct *linux_task;
  struct timer_list wakeup_timer;
  struct list_head list;
  pid_t pid;
  unsigned long period_ms;
  unsigned long runtime_ms;
  unsigned long deadline_jiff;
  unsigned long long initial_time; 
  int state;
};

void mp2_task_constructor(void *buf, int size)
{
    struct mp2_task_struct *mp2_task_struct = buf;
}


// To maintiain current running application
struct mp2_task_struct* crt_task; // current pinter
struct task_struct* kernel_task;

int accept_proc(unsigned long period, unsigned long runtime){
      long long procCost;
      procCost = ((long long)runtime*1000000)/(long long)period;
      spin_lock(&my_spin);
      if (Cp+procCost >= 693000){
         spin_unlock(&my_spin);
         return 0; // Not admitted
      }
      else{
         Cp += procCost;
         spin_unlock(&my_spin);
         return 1;
      }
      spin_unlock(&my_spin);
      return 1;

}

void removeLeadSpace(char** ptr){
   while(**ptr != '\0'){
      if (**ptr == ' '){
         (*ptr)++;
      }
      else{
         return;
      }
   }
   return;
}

/**
 * 
 * Timer handler function
 * 
 * **/
void my_timer_callback(unsigned long data) {
   printk(KERN_ALERT "This line is timer _ handler %lu \n", data );
   struct list_head *pos, *q;
   struct mp2_task_struct *tmp;
   int flag = 0;
   unsigned long state_save;
   // Make the current PID , READY
   printk(KERN_ALERT "Entering lock of timer %d\n", data);
   // spin_lock_irqsave(&my_lock, state_save);
   //mutex_lock(&my_lock);
  spin_lock(&my_spin);
   if(!list_empty(&test_head)){
      list_for_each_safe(pos, q, &test_head){
         tmp= list_entry(pos, struct mp2_task_struct, list);
         printk(KERN_ALERT "Timer looping through list %d \n",tmp->pid);
         if (tmp->pid == data){
            tmp->state = READY;

            printk(KERN_ALERT "Timer  Found list pid :%d and state : %d\n",tmp->pid,tmp->state);

            flag = 1;
         }
      }
   }
 spin_unlock(&my_spin);
 //  mutex_unlock(&my_lock);
//   spin_unlock_irqrestore(&my_lock, state_save);
   printk(KERN_ALERT "Finished looping %d\n", data);

   if(flag == 0){
      // No process here
      return;
   }
   printk(KERN_ALERT "Timer %lu Calling dispatcher it is state\n",data);
   struct sched_param sparam; 
  //set_current_state(TASK_UNINTERRUPTIBLE); // Allow the kernel thread to sleep
   wake_up_process(kernel_task);
  // schedule();  // can't add that
 
   // sparam.sched_priority=99;
   // sched_setscheduler(kernel_task, SCHED_FIFO, &sparam);
   // schedule();
}

/**
 * 
 * Kernel Dispatch Handler
 * 
 * **/

int my_dispatch(void* data){
   struct list_head *pos, *q;
   struct mp2_task_struct *tmp, *next_task, *init_tsk;
   unsigned long period;
   struct sched_param sparam; 
   long myflag = 0;
   long cswitch = 1;

   

   while(!kthread_should_stop()){

       period = MAX_PERIOD;
       //init_tsk = crt_task;
      next_task = NULL;
      if(crt_task != NULL && crt_task->state == SLEEPING){
         sparam.sched_priority=0;
         sched_setscheduler(crt_task->linux_task, SCHED_NORMAL, &sparam);
         printk(KERN_ALERT "[Disp] Starting task %d and state : %d\n",crt_task->pid, crt_task->state);
         set_task_state(crt_task->linux_task, TASK_UNINTERRUPTIBLE);
         crt_task = NULL;
      }

      //printk(KERN_ALERT "[Disp] before the lock");
      // mutex_lock(&my_lock);
      spin_lock(&my_spin);
      // Find the task to run
          if(!list_empty(&test_head)){
            list_for_each_safe(pos, q, &test_head){

               tmp= list_entry(pos, struct mp2_task_struct, list);
                printk(KERN_INFO "[Disp]Looping List PID : %d and state %d\n",tmp->pid, tmp->state);
               if (tmp->state == READY && tmp->period_ms < period){
                  next_task = tmp;
                  period = tmp->period_ms;
                  printk(KERN_INFO "Selected next task is %d and state %d\n", next_task->pid, next_task->state);
                  myflag = 1;
               }
            }
          }
          spin_unlock(&my_spin);
      // mutex_unlock(&my_lock);
      if (next_task != NULL){
         printk(KERN_ALERT "[Disp]Found the next task to run : %d\n",next_task->pid);
      }
      else{
         printk(KERN_ALERT "[Disp]No next task found, keep runing the hol dtask");
      }

      if(myflag == 1){
         if(crt_task != NULL){
            if (crt_task->period_ms <= next_task->period_ms && crt_task->state == RUNNING){
               // Non pre-emption
               printk(KERN_ALERT "[Disp] Non pre-emption : %d \n", next_task->pid);
               next_task->state = READY;
               cswitch = 0;
              
            }
            else{
               // pre-emption
               printk(KERN_ALERT "[Disp]  pre-emption : %d  waking %d\n", crt_task->pid, next_task->pid);
               next_task->state = RUNNING;
               wake_up_process(next_task->linux_task); 
               sparam.sched_priority=99;
               sched_setscheduler(next_task->linux_task, SCHED_FIFO, &sparam);
              // wakes up the next process
               //printk(KERN_ALERT "[Disp] pre-emption  Waking up the %d from %d\n",next_task->pid, crt_task->pid);
              // if(crt_task != NULL && crt_task->linux_task != NULL){
                  crt_task->state = READY;
                  sparam.sched_priority = 0;
                  sched_setscheduler(crt_task->linux_task, SCHED_NORMAL, &sparam); // Add the pre-empted task to the list
                  set_task_state(crt_task->linux_task, TASK_UNINTERRUPTIBLE);
               //}

              // printk(KERN_ALERT "[Disp]atcher Scheduled %d and removed %d\n ", crt_task->pid, next_task->pid);
               crt_task = next_task;
               


            }
         }
         else if (next_task != NULL){
            // No executing task
            printk(KERN_ALERT "[Disp] No executing task setting  %d\n", next_task->pid);
            next_task->state = RUNNING;
            wake_up_process(next_task->linux_task); // wakes up the next process
            sparam.sched_priority=99;
            sched_setscheduler(next_task->linux_task, SCHED_FIFO, &sparam);
            crt_task = next_task;
            // printk(KERN_ALERT "Dispatcher Scheduled %d\n", crt_task->pid);

         }
      }
      else{
         // wakeup the curre{}nt task
         if(crt_task != NULL){
            crt_task->state = RUNNING;
            wake_up_process(crt_task->linux_task);
            sparam.sched_priority=99;
            sched_setscheduler(crt_task->linux_task, SCHED_FIFO, &sparam);

         }
      }
      printk(KERN_ALERT "[Disp] All task dine");
     // if (crt_task != NULL){
      //   if (init_tsk != NULL && crt_task!= NULL && init_tsk->pid == crt_task->pid && cswitch==1){
         if (crt_task != NULL){
            printk(KERN_ALERT "[Disp] Exiting Dispatcher with task : %d\n", crt_task->pid);
         }
         else{
            printk(KERN_ALERT "[Disp] Exiting Dispatcher with task NULL\n");
         }
      //   }
         set_current_state(TASK_UNINTERRUPTIBLE); // Allow the kernel thread to sleep
         schedule(); // Schedule the added task 
       
    
         
      
      //}


   }// end of while, it will exit properly

   return 0;
}

/***
 * 
 * Registration Handler
*/
void handleRegistration(char *kbuf){

   // Data for reading the input
   //printk(KERN_ALERT "Handle Registration \n");
   int idx = 0;
   char* token;
   long readVal[5];
   char action;
   char *endptr;
   long value;
   long tperiod, comp_time;
   int pid_inp;

   // Data for creating and storing the process block
   struct mp2_task_struct *task_inp;


   while( (token = strsep(&kbuf,",")) != NULL ){
         if (idx == 0){
            action = *token;
            // printk(KERN_ALERT "This value is %c \n",action);
         }
         else if (idx <= 3){
            removeLeadSpace(&token);
            //printk(KERN_ALERT "The token : %s\n",token);
            value = simple_strtol(token, &endptr, 10);
            if (value == 0 && endptr == token){
               // printk(KERN_ALERT "Error in long conversion");
            } 
            else{
               // printk(KERN_ALERT "The value is %ld\n", value);
               readVal[idx-1] = value;
            }
            
         }
      idx += 1;
   } // read all the data

   pid_inp = (int)readVal[0];
   tperiod = readVal[1];
   comp_time = readVal[2];
   printk(KERN_ALERT "Register %d, %lu , %lu \n",pid_inp,tperiod,comp_time);
   if (accept_proc(tperiod,comp_time) == 0){

      return;
   }
   // Need to use kmem. for now using kmallloo
   task_inp = kmem_cache_alloc(mp2_cache,GFP_KERNEL);
   if (!task_inp){
      printk(KERN_INFO "Unable to allocate pid_inp memory\n");
      return ;
   }

   task_inp->pid = pid_inp;
   task_inp->period_ms = tperiod;
   task_inp->runtime_ms = comp_time;
   task_inp->linux_task = find_task_by_pid(pid_inp);
   task_inp->state = SLEEPING;
    do_gettimeofday(&time_val);
    task_inp->initial_time = time_val.tv_sec * 1000 * 1000 + time_val.tv_usec ;
   setup_timer( &task_inp->wakeup_timer, my_timer_callback, task_inp->pid );


   // Add to list should be within lock
   spin_lock(&my_spin);
   list_add(&(task_inp->list),&test_head);
   spin_unlock(&my_spin);

   return;
   
}

/**
 * 
 * Yield Hanlder
 * 
 * **/

void handleYield(char *kbuf){
   printk(KERN_ALERT "Handle Yield\n");
   int idx = 0;
   char* token;
   long t_pid = -1;
   char action;
   char *endptr;
   long value;
   unsigned long time_to_wakeup ;
  unsigned long long curr_time ; 

// List data structure 
   struct list_head *pos, *q;
   struct mp2_task_struct *temp, *sleep_task;
   struct sched_param sparam; 
   int flag = 0;
   unsigned long long deadline;


   while( (token = strsep(&kbuf,",")) != NULL ){
      if (idx == 0){
         action = *token;
         // printk(KERN_ALERT "This value is %c \n",action);
      }
      else if (idx <= 1){
         removeLeadSpace(&token);
         //printk(KERN_ALERT "The token : %s\n",token);
         value = simple_strtol(token, &endptr, 10);
         if (value == 0 && endptr == token){
            // printk(KERN_ALERT "Error in long conversion");
         } 
         else{
            // printk(KERN_ALERT "The value is %ld\n", value);
            t_pid = value;
         }
         
      }
      idx += 1;
   } // read all the data

   spin_lock(&my_spin);
    if(!list_empty(&test_head)){
   list_for_each_safe(pos, q, &test_head){
      temp= list_entry(pos, struct mp2_task_struct, list);
      if (temp->pid == t_pid){
         temp->state = SLEEPING;
         sleep_task = temp;
         flag = 1;
      }
   }
    }
   spin_unlock(&my_spin);
   // Do work of Yield
   if (flag == 0){
      return;
   }
   printk(KERN_ALERT "Yield task %d to state : %d\n",sleep_task->pid, sleep_task->state);
   //printk(KERN_ALERT "Yield Current task now is %d\n",crt_task->pid);
   sleep_task->state = SLEEPING;
   do_gettimeofday(&time_val);
   curr_time = time_val.tv_sec * 1000 * 1000 + time_val.tv_usec ; // in msecs
   //task->period_ms - (((curr_time - task->initial_time )/1000)%(task->period_ms));
   deadline = sleep_task->period_ms -(((curr_time - sleep_task->initial_time)/1000)% sleep_task->period_ms);
   printk(KERN_ALERT" The deadline for %d is  %lu\n", sleep_task->pid, deadline);
   
   mod_timer(&sleep_task->wakeup_timer, jiffies + msecs_to_jiffies(deadline));
   set_task_state(sleep_task->linux_task, TASK_UNINTERRUPTIBLE);
  // set_current_state(TASK_UNINTERRUPTIBLE);

   wake_up_process(kernel_task); // wakes up the next process

   // sparam.sched_priority=99;
   // sched_setscheduler(kernel_task, SCHED_FIFO, &sparam);
    schedule();
   return;
}


/**
 * 
 * De registration Handler
 * 
 * **/

void handleDeReg(char *kbuf){
   printk(KERN_ALERT "Handle Dereg\n");
   int idx = 0;
   char* token;
   int t_pid = -1;
   char action;
   char *endptr;
   long value;

   // Data for dereg the task
   struct list_head *posv, *qv;
   struct mp2_task_struct *temp;
   int flag = 0;

   while( (token = strsep(&kbuf,",")) != NULL ){
      if (idx == 0){
         action = *token;
         // printk(KERN_ALERT "This value is %c \n",action);
      }
      else if (idx <= 1){
         removeLeadSpace(&token);
         //printk(KERN_ALERT "The token : %s\n",token);
         value = simple_strtol(token, &endptr, 10);
         if (value == 0 && endptr == token){
            // printk(KERN_ALERT "Error in long conversion");
         } 
         else{
            // printk(KERN_ALERT "The value is %ld\n", value);
            t_pid = (int)value;
            break;
         }
         
      }
      idx += 1;
   } // read all the data

   // Do work DeReg
   // need to stop the timer, but let's not do that yet!!
   // Just remove from the list

   spin_lock(&my_spin);
    if(!list_empty(&test_head)){
      list_for_each_safe(posv, qv, &test_head){

         temp= list_entry(posv, struct mp2_task_struct, list);
         if (temp->pid == t_pid){
            // found a task to remove
            if(crt_task == temp){
               crt_task = NULL;
            }
            //set_task_state(temp->linux_task, TASK_UNINTERRUPTIBLE);
            Cp -= ((long long)temp->runtime_ms*1000000)/(long long) temp->period_ms;
            list_del(posv);
            del_timer(&temp->wakeup_timer);
            // need to delete the timer too
            printk(KERN_ALERT "\nDeleted Pid : %d and cpu_time\n", temp->pid);
            kmem_cache_free(mp2_cache,temp);
            flag = 1;
         }
      }
    }
   spin_unlock(&my_spin);

   if (flag == 0){
      return;
   }
   //set_current_state(TASK_UNINTERRUPTIBLE);
   wake_up_process(kernel_task);
   schedule();

}

 /**
  * 
  * READ and WRITE API for this proc/stat
  * 
  * **/
 // frin user to kernel
static ssize_t mp2_write (struct file *file, const char __user *buffer, size_t count, loff_t *data){
    long ret;
    int pidx;
    char *kbuf; // buffer where we will read the data
    
    //long readVal[5];
    //char action;


    if (*data > 0){
        printk(KERN_INFO "\nError in offset of the file\n");
        return -EFAULT;
    }
    // Define a buffer for the string
    kbuf = (char*) kmalloc(count,GFP_KERNEL); 
    ret = strncpy_from_user(kbuf, buffer, count);
    if (ret == -EFAULT){
        printk(KERN_INFO "\nerror in reading the PID\n");
        return -EFAULT;
    }
    printk(KERN_ALERT "%s\n",kbuf);
    if (kbuf[0]=='R'){
       handleRegistration(kbuf);
    } 
    else if(kbuf[0]=='Y'){
       handleYield(kbuf);
    }
    else if(kbuf[0]=='D'){
       handleDeReg(kbuf);
    }

    
    kfree(kbuf);
    return ret;
}

// kernel to proc
static ssize_t mp2_read (struct file *file, char __user *buffer, size_t count, loff_t *data){

     // Data declaration
   int copied;                // Return Value
   int len=0;                 // How much data is read
   char *buf;                 // Kernel buffer to read data
   struct list_head *ptr;     // Kernel Linked List 
   struct mp2_task_struct *entry; // Kernel Linked List
   // Checking if the offset is correct
   if (*data > 0){
      printk(KERN_INFO "\nRead offset issue\n");
      return 0;
   }
   printk(KERN_INFO "\nRead function called\n");
  
   buf = (char *) kmalloc(count,GFP_KERNEL);    // Allocating kernel memory and checking if properly allocated
   if (!buf){
      printk(KERN_INFO "Unable to allocate buffer!!\n");
       return -ENOMEM;
   }
      
   copied = 0;
   // Checking id list is empty otherwise, acquiring the lock and iterationg over and writing list values into the buffer.
   spin_lock(&my_spin);
   if (!list_empty(&test_head)){

      list_for_each(ptr,&test_head){
         entry=list_entry(ptr,struct mp2_task_struct,list);
         //printk(KERN_INFO "\n PID %d:Time %lu  \n ", entry->my_id,entry->cpu_time);
         // Add this entry into the buffer
         len += scnprintf(buf+len,count-len,"%d: %lu, %lu\n",entry->pid,entry->period_ms, entry->runtime_ms);
      }
      
   }
   spin_unlock(&my_spin);


   // Sending data to user buffer from kernel buffer
   copied = simple_read_from_buffer(buffer,count,data,buf,len);
   if (copied < 0){
      return -EFAULT;
   }
   kfree(buf);
   return copied ;
}
/**
 * Proc Fs data structure 
 **/
static struct proc_dir_entry* mp2_dir;    // pointer to a new dir
static struct proc_dir_entry *mp2_file;
// File ops for the newly minted files in mp1 dir
static const struct file_operations mp2_fops = {
     .owner	= THIS_MODULE,
     .read = mp2_read,
     .write	= mp2_write,
 };

// Init and exit code for mp2
int __init mp2_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP2 MODULE LOADING\n");
   #endif
   printk(KERN_ALERT "Hello World\n");
   // Initilizing the linked list
   INIT_LIST_HEAD(&test_head);
//    // Created the directory
   mp2_dir = proc_mkdir("mp2", NULL);
   // Adding the files
   mp2_file = proc_create("status", 0666, mp2_dir, &mp2_fops);

   mp2_cache = kmem_cache_create("mp2_cache", sizeof(struct mp2_task_struct),0,0, (void*)mp2_task_constructor);
//.    // Checkpoint 1 done
//    // Setup Work Queue
//    my_wq = create_workqueue("mp1q");
//    printk(KERN_ALERT "Initializing a module with timer.\n");
//    // Set up the timer
//    setup_timer(&my_timer, my_timer_callback, 0);
//    mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));
   Cp = 0;
   crt_task = NULL;
   kernel_task = kthread_create(&my_dispatch,NULL,"my_dispatch");

   printk(KERN_ALERT "MP2 MODUL LOADED\n");
   return 0;   
}

void __exit mp2_exit(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP2 MODULE UNLOADING\n");
   #endif
   printk(KERN_ALERT "Goodbye\n");
   // Removing the timer
//    del_timer(&my_timer);
//    //Removing the workqueue
//    flush_workqueue( my_wq );
//    destroy_workqueue( my_wq );
//    // Remove the list
    struct list_head *pos, *q;
    struct mp2_task_struct *tmp;

   kthread_stop(kernel_task);
   spin_lock(&my_spin);
   if(!list_empty(&test_head)){
         list_for_each_safe(pos, q, &test_head){

            tmp= list_entry(pos, struct mp2_task_struct, list);
            // printk(KERN_INFO "Freeing List\n");
            list_del(pos);
            del_timer(&tmp->wakeup_timer);
            kmem_cache_free(mp2_cache,tmp);
         }
    }
   spin_unlock(&my_spin);
   printk(KERN_ALERT "List Freed\n");

//     // Removing the timer 
//    printk(KERN_ALERT "removing timer\n");
 
//    // Removing the directory and files
  
   kmem_cache_destroy(mp2_cache);
   remove_proc_entry("status", mp2_dir);
   remove_proc_entry("mp2", NULL);

   printk(KERN_ALERT "MP2 MODULE UNLOADED\n");
}

// Loading bnew and unloading the kernel modules
module_init(mp2_init);
module_exit(mp2_exit);



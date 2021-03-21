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

#include "mp2_given.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("dipayan2");
MODULE_DESCRIPTION("CS-423 MP2");

/* LOCKING VARIABLE*/
static DEFINE_SPINLOCK(my_lock);

enum task_state {READY = 0, RUNNING = 1, SLEEPING = 2}; 


struct list_head test_head;

struct mp2_task_struct {
  struct task_struct *linux_task;
  struct timer_list wakeup_timer;
  struct list_head list;
  pid_t pid;
  unsigned long period_ms;
  unsigned long runtime_ms;
  unsigned long deadline_jiff;
  enum task_state state;
};

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

void handleRegistration(char *kbuf){

   // Data for reading the input
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
            printk(KERN_ALERT "This value is %c \n",action);
         }
         else if (idx <= 3){
            removeLeadSpace(&token);
            printk(KERN_ALERT "The token : %s\n",token);
            value = simple_strtol(token, &endptr, 10);
            if (value == 0 && endptr == token){
               printk(KERN_ALERT "Error in long conversion");
            } 
            else{
               printk(KERN_ALERT "The value is %ld\n", value);
               readVal[idx-1] = value;
            }
            
         }
      idx += 1;
   } // read all the data

   pid_inp = (int)readVal[0];
   tperiod = readVal[1];
   comp_time = readVal[2];

   // Need to use kmem. for now using kmallloo
   task_inp = (struct mp2_task_struct *) kmalloc(sizeof(struct mp2_task_struct),GFP_KERNEL);
   if (!task_inp){
      printk(KERN_INFO "Unable to allocate pid_inp memory\n");
      return -EFAULT;
   }

   task_inp->pid = pid_inp;
   task_inp->period_ms = tperiod;
   task_inp->runtime_ms = comp_time;
   task_inp->linux_task = find_task_by_pid(pid_inp);
   task_inp->state = SLEEPING;


   // Add to list should be within lock
   spin_lock(&my_lock);
   list_add(&(task_inp->list),&test_head);
   spin_unlock(&my_lock);


   // Do the work of registration
   
}

void handleYield(char *kbuf){
   int idx = 0;
   char* token;
   long t_pid;
   char action;
   char *endptr;
   long value;
   while( (token = strsep(&kbuf,",")) != NULL ){
      if (idx == 0){
         action = *token;
         printk(KERN_ALERT "This value is %c \n",action);
      }
      else if (idx <= 1){
         removeLeadSpace(&token);
         printk(KERN_ALERT "The token : %s\n",token);
         value = simple_strtol(token, &endptr, 10);
         if (value == 0 && endptr == token){
            printk(KERN_ALERT "Error in long conversion");
         } 
         else{
            printk(KERN_ALERT "The value is %ld\n", value);
            t_pid = value;
         }
         
      }
      idx += 1;
   } // read all the data

   // Do work of Yield
}

void handleDeReg(char *kbuf){
   int idx = 0;
   char* token;
   int t_pid;
   char action;
   char *endptr;
   long value;

   // Data for dereg the task
   struct list_head *posv, *qv;
   struct mp2_task_struct *temp;

   while( (token = strsep(&kbuf,",")) != NULL ){
      if (idx == 0){
         action = *token;
         printk(KERN_ALERT "This value is %c \n",action);
      }
      else if (idx <= 1){
         removeLeadSpace(&token);
         printk(KERN_ALERT "The token : %s\n",token);
         value = simple_strtol(token, &endptr, 10);
         if (value == 0 && endptr == token){
            printk(KERN_ALERT "Error in long conversion");
         } 
         else{
            printk(KERN_ALERT "The value is %ld\n", value);
            t_pid = (int)value;
            break;
         }
         
      }
      idx += 1;
   } // read all the data

   // Do work DeReg
   // need to stop the timer, but let's not do that yet!!
   // Just remove from the list
   spin_lock(&my_lock);
   list_for_each_safe(posv, qv, &test_head){

		 temp= list_entry(posv, struct mp2_task_struct, list);
       if (temp->pid == t_pid){
          list_del(posv);
          printk(KERN_INFO "\nDeleted Pid : %d and cpu_time\n", temp->my_id);
          kfree(temp);
       }
	}
   spin_unlock(&my_lock);

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
   if (!list_empty(&test_head)){
      spin_lock(&my_lock);
      list_for_each(ptr,&test_head){
         entry=list_entry(ptr,struct mp2_task_struct,list);
         //printk(KERN_INFO "\n PID %d:Time %lu  \n ", entry->my_id,entry->cpu_time);
         // Add this entry into the buffer
         len += scnprintf(buf+len,count-len,"%d: %lu, %lu\n",entry->pid,entry->period_ms, entry->runtime_ms);
      }
      spin_unlock(&my_lock);
   }


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
//    // Checkpoint 1 done
//    // Setup Work Queue
//    my_wq = create_workqueue("mp1q");
//    printk(KERN_ALERT "Initializing a module with timer.\n");
//    // Set up the timer
//    setup_timer(&my_timer, my_timer_callback, 0);
//    mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));

   printk(KERN_ALERT "MP2 MODULE LOADED\n");
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

   spin_lock(&my_lock);
   if(!list_empty(&test_head)){
         list_for_each_safe(pos, q, &test_head){

            tmp= list_entry(pos, struct mp2_task_struct, list);
            // printk(KERN_INFO "Freeing List\n");
            list_del(pos);
            kfree(tmp);
         }
    }
   spin_unlock(&my_lock);
   printk(KERN_ALERT "List Freed\n");

//     // Removing the timer 
//    printk(KERN_ALERT "removing timer\n");
 
//    // Removing the directory and files
   remove_proc_entry("status", mp2_dir);
   remove_proc_entry("mp2", NULL);

   printk(KERN_ALERT "MP2 MODULE UNLOADED\n");
}

// Loading and unloading the kernel modules
module_init(mp2_init);
module_exit(mp2_exit);



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


enum task_state {READY = 0, RUNNING = 1, SLEEPING = 2}; 

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

 /**
  * 
  * READ and WRITE API for this proc/stat
  * 
  * **/
 // frin user to kernel
static ssize_t mp2_write (struct file *file, const char __user *buffer, size_t count, loff_t *data){
    long ret;
    int pidx, idx;
    char *kbuf; // buffer where we will read the data
    char* token;
    long readVal[5];
    char action;
    char *endptr;
    long value;

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
    idx = 0;
    while( (token = strsep(&kbuf,",")) != NULL ){
      if (idx == 0){
            action = *token;
            printk(KERN_ALERT "This value is %c \n",action);
         }
         else{
            if (action == 'R'){
               if (idx <= 3){
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
               else{
                  break;
               }
            }
            else if (action == 'Y' || action == 'D'){
        
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
                
               break;
            }
         }
         idx += 1;
         //printk(KERN_ALERT "%s\n",token);
    }
        

    // use strsep
    
    kfree(kbuf);
    return ret;
}

// kernel to proc
static ssize_t mp2_read (struct file *file, char __user *buffer, size_t count, loff_t *data){
    return 0;
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
//    INIT_LIST_HEAD(&test_head);
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
//    struct list_head *pos, *q;
//    struct my_pid_data *tmp;

//    spin_lock(&my_lock);
//    if(!list_empty(&test_head)){
//          list_for_each_safe(pos, q, &test_head){

//             tmp= list_entry(pos, struct my_pid_data, list);
//             // printk(KERN_INFO "Freeing List\n");
//             list_del(pos);
//             kfree(tmp);
//          }
//     }
//    spin_unlock(&my_lock);
//    printk(KERN_INFO "List Freed\n");

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



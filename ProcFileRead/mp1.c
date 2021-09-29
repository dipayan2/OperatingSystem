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

#include "mp1_given.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("dipayan2");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1
/* LOCKING VARIABLE*/
static DEFINE_SPINLOCK(my_lock);

/**
 * LINKED LIST DATA STRUCTURE DEFINED
**/
struct list_head test_head;
struct my_pid_data{
   struct list_head list;
   int my_id;
   unsigned long cpu_time;
};

/**
 * CALLBACKS FOR THE PROCFS FILE SYSTEM
 **/

/**
* READ : To send data back to the process
**/
static ssize_t mp1_read (struct file *file, char __user *buffer, size_t count, loff_t *data){
   // Data declaration
   int copied;                // Return Value
   int len=0;                 // How much data is read
   char *buf;                 // Kernel buffer to read data
   struct list_head *ptr;     // Kernel Linked List 
   struct my_pid_data *entry; // Kernel Linked List
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
         entry=list_entry(ptr,struct my_pid_data,list);
         //printk(KERN_INFO "\n PID %d:Time %lu  \n ", entry->my_id,entry->cpu_time);
         // Add this entry into the buffer
         len += scnprintf(buf+len,count-len,"%d: %lu\n",entry->my_id,entry->cpu_time);
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
* WRITE : To send data(PID) from process to kernel
**/
static ssize_t mp1_write (struct file *file, const char __user *buffer, size_t count, loff_t *data){

   int ret,pidx;
   struct my_pid_data *pid_inp;
   // Checking for input offset
   if (*data > 0){
      printk(KERN_INFO "\nError in offset of the file\n");
      return -EFAULT;
   }
   // Reading data from user space and converting it to int
   ret = kstrtoint_from_user(buffer, count, 10, &pidx);
   if (ret){
      printk(KERN_INFO "\nerror in reading the PID\n");
      return -EFAULT;
   }
   if (pidx < 0){
      return -EFAULT;
   }

   // Initializing a node for PID and its cpu_time
   pid_inp = (struct my_pid_data *) kmalloc(sizeof(struct my_pid_data),GFP_KERNEL); // need to free this memory
   if (!pid_inp){
      printk(KERN_INFO "Unable to allocate pid_inp memory\n");
      return -EFAULT;
   }
   pid_inp->my_id = pidx;
   pid_inp->cpu_time = 0;
   // Adding the enrty to kernel linked list
   spin_lock(&my_lock);
   list_add(&(pid_inp->list),&test_head);
   spin_unlock(&my_lock);

   printk(KERN_INFO "\nRegistered %d in the list\n", pidx);
   // Finished entry, might have added this in lock
   return count;
}

/**
 * Proc Fs data structure 
 **/
static struct proc_dir_entry* mp1_dir;    // pointer to a new dir
static struct proc_dir_entry *mp1_file;
// File ops for the newly minted files in mp1 dir
static const struct file_operations mp1_fops = {
     .owner	= THIS_MODULE,
     .read = mp1_read,
     .write	= mp1_write,
 };


/**
 *  
 * WORK QUEUE data structure and relevant functions
 * 
 * **/

 static struct workqueue_struct *my_wq;
 struct work_struct my_work;

 /**
  * Function called from the Work Queue to modify the kernel linked list
  * **/
 static void upDateFunction(struct work_struct *work){ 
   
   struct list_head *posv, *qv;
   struct my_pid_data *temp;
   unsigned long run_tm;
   int to_del;
   //Acquire a lock and then update or delete nodes from the linked list
   spin_lock(&my_lock);
   list_for_each_safe(posv, qv, &test_head){

		 temp= list_entry(posv, struct my_pid_data, list);
		 to_del = get_cpu_use(temp->my_id, &run_tm);
       if (to_del == -1){
          list_del(posv);
          printk(KERN_INFO "\nDeleted Pid : %d and cpu_time\n", temp->my_id);
          kfree(temp);
       }else
       {
          temp->cpu_time = run_tm;
       }	 
	}
   spin_unlock(&my_lock);
   // Removed lock
   //printk(KERN_ALERT "Updated entries after 5 seconds\n"); 

}

/**
 * Sets up the work with a function, and adds the work to the workqueue
 * **/
 static void setup_work(void){
   
   // my_work = (work_struct *) kmalloc(sizeof(work_struct), GFP_KERNEL);
    INIT_WORK(&my_work, upDateFunction); // Attached function to the work
    queue_work(my_wq, &my_work); // Added to queue

    // Add it to the queue to execute later
 }


/**
 * 
 * KERNEL TIMER 
 * 
 * **/
static struct timer_list my_timer;
/***
 * Interrupt Handler
*/
void my_timer_callback(unsigned long data) {
  //printk(KERN_ALERT "This line is printed after 5 seconds.\n");
  setup_work();
  mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000)); // This function ensures that timer is called again in 5 second
}

/**
 * Module Load and Unloading codes
 * **/
// mp1_init - Called when module is loaded
int __init mp1_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE LOADING\n");
   #endif
   printk(KERN_ALERT "Hello World\n");
   // Initilizing the linked list
   INIT_LIST_HEAD(&test_head);
   // Created the directory
   mp1_dir = proc_mkdir("mp1", NULL);
   // Adding the files
   mp1_file = proc_create("status", 0666, mp1_dir, & mp1_fops);
   // Checkpoint 1 done
   // Setup Work Queue
   my_wq = create_workqueue("mp1q");
   printk(KERN_ALERT "Initializing a module with timer.\n");
   // Set up the timer
   setup_timer(&my_timer, my_timer_callback, 0);
   mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));

   printk(KERN_ALERT "MP1 MODULE LOADED\n");
   return 0;   
}

// mp1_exit - Called when module is unloaded
void __exit mp1_exit(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE UNLOADING\n");
   #endif
   printk(KERN_ALERT "Goodbye\n");
   // Removing the timer
   del_timer(&my_timer);
   //Removing the workqueue
   flush_workqueue( my_wq );
   destroy_workqueue( my_wq );
   // Remove the list
   struct list_head *pos, *q;
   struct my_pid_data *tmp;

   spin_lock(&my_lock);
   if(!list_empty(&test_head)){
         list_for_each_safe(pos, q, &test_head){

            tmp= list_entry(pos, struct my_pid_data, list);
            // printk(KERN_INFO "Freeing List\n");
            list_del(pos);
            kfree(tmp);
         }
    }
   spin_unlock(&my_lock);
   printk(KERN_INFO "List Freed\n");

    // Removing the timer 
   printk(KERN_ALERT "removing timer\n");
 
   // Removing the directory and files
   remove_proc_entry("status", mp1_dir);
   remove_proc_entry("mp1", NULL);

   printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);

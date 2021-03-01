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

#include "mp1_given.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("dipayan2");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1

// defining the linux lined list -- data type
struct list_head test_head;
struct my_pid_data{
   struct list_head list;
   int my_id;
   unsigned long cpu_time;
};

// callback
static ssize_t mp1_read (struct file *file, char __user *buffer, size_t count, loff_t *data){
   int copied;
   int len=0;
   char *buf;
   // Read the whole list?
   struct list_head *ptr;
   struct my_pid_data *entry;

   if (*data > 0){
      printk(KERN_INFO "Read offset issue\n");
      return 0;
   }

  
   buf = (char *) kmalloc(count,GFP_KERNEL); 
   if (!buf){
      printk(KERN_INFO "Unable to allocate buffer!!\n");
       return -ENOMEM;
   }
      
   copied = 0;
   // Reading the list
   list_for_each(ptr,&test_head){
      entry=list_entry(ptr,struct my_pid_data,list);
      printk(KERN_INFO "\n PID %d:Time %lu  \n ", entry->my_id,entry->cpu_time);
      // Add this entry into the buffer
      len += scnprintf(buf+len,count-len,"%d: %lu\n",entry->my_id,entry->cpu_time);
   }


   copied = simple_read_from_buffer(buffer,count,data,buf,len);
   if (copied < 0){
      return -EFAULT;
   }
   kfree(buf);
   return copied ;
}
static ssize_t mp1_write (struct file *file, const char __user *buffer, size_t count, loff_t *data){
// implementation goes here... 
   int ret,pidx;
   struct my_pid_data *pid_inp;
   if (*data > 0){
      printk(KERN_INFO "Error in offset of the file\n");
      return -EFAULT;
   }
  
   ret = kstrtoint_from_user(buffer, count, 10, &pidx);
   if (ret){
      printk(KERN_INFO " error in reading the PID\n");
      return -EFAULT;
   }
   if (pidx < 0){
      return -EFAULT;
   }

   // Add to list here
   
   pid_inp = kmalloc(sizeof(struct my_pid_data *),GFP_KERNEL);
   pid_inp->my_id = pidx;
   pid_inp->cpu_time = 0;
   // Adding the enrty
   list_add(&pid_inp->list,&test_head);
   // Finished entry, might have added this in lock

   return count;
}

// Directory Creation
static struct proc_dir_entry* mp1_dir; // pointer to a new dir
static struct proc_dir_entry *mp1_file;
// File ops for the newly minted files in mp1 dir
static const struct file_operations mp1_fops = {
     .owner	= THIS_MODULE,
     .read = mp1_read,
     .write	= mp1_write,
 };


//Update code for the work function
 static struct workqueue_struct *my_wq;
 static void upDateFunction(struct work_struct *work){
   // Do stuff here
   printk(KERN_ALERT "This line is printed after 5 seconds.\n");
   // Ending 

}
 static void setup_work(void){
    // Create new work
    struct work_struct my_work;
   // my_work = (work_struct *) kmalloc(sizeof(work_struct), GFP_KERNEL);
    INIT_WORK(&my_work, upDateFunction); // Attached function to the work
    queue_work(my_wq, &my_work); // Added to queue
    return ;
    // Add it to the queue to execute later
 }


 // Timer functions
static struct timer_list my_timer;
void my_timer_callback(unsigned long data) {
  printk(KERN_ALERT "This line is printed after 5 seconds.\n");
  setup_work();
  //mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));
}


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
   // Creating the timer
   my_wq = create_workqueue("mp1q");
   printk(KERN_ALERT "Initializing a module with timer.\n");
  
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
   // Insert your code here ...
   del_timer(&my_timer);
   //Removing the workqueue
   flush_workqueue( my_wq );
   destroy_workqueue( my_wq );
   // Remove the list
   struct list_head *pos, *q;
   struct my_pid_data *tmp;
   list_for_each_safe(pos, q, &test_head){

		 tmp= list_entry(pos, struct my_pid_data, list);
		 printk(KERN_INFO "Freeing List\n");
		 list_del(pos);
		 kfree(tmp);
	}
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

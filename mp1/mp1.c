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

   char * buf;
   buf = (char *) kmalloc(count,GFP_KERNEL); 
   if (!buf){
       return -ENOMEM;
   }
      
   copied = 0;
   // Read the whole list?
   struct list_head *ptr;
   struct my_pid_data *entry;
   list_for_each(ptr,&test_head){
      entry=list_entry(ptr,struct my_pid_data,list);
      printk(KERN_INFO "\n PID %d:Time %lu  \n ", entry->my_id,entry->cpu_time);
      // Add this entry into the buffer
      len += scnprintf(buf+len,count-len,"%d: %lu\n",entry->my_id,entry->cpu_time);
   }
   //... put something into the buf, updated copied 
   //copy_to_user(buffer, buf, copied); 

   copied = simple_read_from_buffer(buffer,count,data,buf,len);
   kfree(buf);
   return copied ;
}
static ssize_t mp1_write (struct file *file, const char __user *buffer, size_t count, loff_t *data){
// implementation goes here... 
   return 0;
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
   // Init the kernel linked list for registration

// I am modifyingh Last change
   
   
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
   // Removing the files

   // Removing the directory
   remove_proc_entry("status", mp1_dir);
   remove_proc_entry("mp1", NULL);

   printk(KERN_ALERT "MP1 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp1_init);
module_exit(mp1_exit);
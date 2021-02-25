#define LINUX

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include<linux/fs.h>
#include <linux/seq_file.h>
#include "mp1_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("dipayan2");
MODULE_DESCRIPTION("CS-423 MP1");

#define DEBUG 1


// callsback
static ssize_t mp1_read (struct file *file, char __user *buffer, size_t count, loff_t *data){
   return 1;
}
static ssize_t mp1_write (struct file *file, const char __user *buffer, size_t count, loff_t *data){
// implementation goes here... 
   return 1;
}

// Directory Creation
static struct proc_dir_entry* mp1_dir; // pointer to a new dir
// File ops for the newly minted files in mp1 dir
static const struct file_operations mp1_fops = {
     .owner	= THIS_MODULE,
     .read = mp1_read
     .write	= mp1_write,
 };


// mp1_init - Called when module is loaded
int __init mp1_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP1 MODULE LOADING\n");
   #endif
   printk(KERN_ALERT "Hello World\n");
   // Created the directory
   mp1_dir = proc_mkdir("mp1", NULL);
   // Adding the files
   mp1_file = proc_create("status", 0666, mp1_dir, & mp1_fops);

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

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
#include <linux/vmalloc.h>

#include "mp3_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("dipayan2");
MODULE_DESCRIPTION("CS-423 MP3");
#define DEBUG 1
#define BUFFER_BLOCK (128*4*1024)


/* LOCKING VARIABLE*/
static DEFINE_SPINLOCK(my_spin);

//Linked list header
static struct workqueue_struct *my_wq;
struct work_struct my_work;
struct list_head test_head;
void* my_buffer; // for vmalloc usage
int init_wq = 0;
struct mp3_task_struct {
  struct task_struct *linux_task;
  struct list_head list;
  unsigned long long process_utilization; 
  pid_t pid;
  unsigned long min_flt;
  unsigned long maj_flt;
  unsigned long utime;
  unsigned long stime;
};




void memFunction(){
   return;
}

void create_workqueue(){
   init_wq = 1;
   my_wq = create_workqueue("mp3q");
   INIT_WORK(&my_work, memFunction); // Attached function to the work
   queue_work(my_wq, &my_work); 
}

void delete_workqueue(){
   flush_workqueue( my_wq );
   destroy_workqueue( my_wq );
   init_wq = 0;
   return;
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
   long readVal[2];
   char action;
   char *endptr;
   long value;
   int pid_inp;
   unsigned long min_flt;
   unsigned long maj_flt;
   unsigned long utime;
   unsigned long stime;
  // 

   // Data for creating and storing the process block
   struct mp3_task_struct *task_inp;

   // Read the pid from the buffer input data
   while( (token = strsep(&kbuf," ")) != NULL ){
         if (idx == 0){
            action = *token;
            // printk(KERN_ALERT "This value is %c \n",action);
         }
         else{
            removeLeadSpace(&token);
            //printk(KERN_ALERT "The token : %s\n",token);
            value = simple_strtol(token, &endptr, 10);
            if (value == 0 && endptr == token){
                printk(KERN_ALERT "Error in long conversion");
            } 
            else{
               // printk(KERN_ALERT "The value is %ld\n", value);
               readVal[idx-1] = value;
            }
            
         }
      idx += 1;
   } 

   // Allocate proc util for taks by PID and add them to the list
   pid_inp = (int)readVal[0];
   printk(KERN_ALERT "Register %d, %lu , %lu \n",pid_inp,tperiod,comp_time);
  
   // Need to use kmem. for now using kmallloo
   task_inp = (struct mp3_task_struct *) kmalloc(sizeof(struct mp3_task_struct),GFP_KERNEL); // need to free this memory
   if (!task_inp){
      printk(KERN_INFO "Unable to allocate pid_inp memory\n");
      return ;
   }
   // Allocate things to the  list value
  
   if (get_cpu_use(pid_inp, &min_flt, &maj_flt, &utime, &stime) == -1){
      printk(KERN_ALERT "Unable to get page details for the task")
      kfree(task_inp);
      return;
   }

   // Adding data to the node
   task_inp->linux_task = find_task_by_pid(pid_inp);
   task_inp->maj_flt = maj_flt;
   task_inp->min_flt = min_flt;
   task_inp->utime = utime;
   task_inp->stime = stime;



   // Add to list should be within lock
   // Should check if the list is 
   if (init_wq == 0){
      create_workqueue();
   }
   //create_workqueue();
   spin_lock(&my_spin);
   list_add(&(task_inp->list),&test_head);
   spin_unlock(&my_spin);

   return;
   
}


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
   struct mp3_task_struct *temp;
   int flag = 0;

   while( (token = strsep(&kbuf," ")) != NULL ){
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
   // Need to remove the workqueue

   spin_lock(&my_spin);
    if(!list_empty(&test_head)){
      list_for_each_safe(posv, qv, &test_head){

         temp= list_entry(posv, struct mp2_task_struct, list);
         if (temp->pid == t_pid){
            list_del(posv);
            
            // need to delete the timer too
            printk(KERN_ALERT "\nDeleted Pid : %d and cpu_time\n", temp->pid);
            kfree(temp);
            
         }
      }
    }
   spin_unlock(&my_spin);
   spin_lock(&my_spin);
   if(list_empty(&test_head)){
      delete_workqueue();
   }
   spin_unlock(&my_spin);


   //delete_workqueue();
   return;
}

/**
  * 
  * READ and WRITE API for this proc/stat
  * 
  * **/
 // from user to kernel
static ssize_t mp3_write (struct file *file, const char __user *buffer, size_t count, loff_t *data){
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
    else if(kbuf[0]=='U'){
       handleDeReg(kbuf);
    }

    
    kfree(kbuf);
    return ret;
}

// kernel to proc
static ssize_t mp3_read (struct file *file, char __user *buffer, size_t count, loff_t *data){

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
         len += scnprintf(buf+len,count-len,"%dn",entry->pid);
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
static struct proc_dir_entry* mp3_dir;    // pointer to a new dir
static struct proc_dir_entry *mp3_file;
// File ops for the newly minted files in mp1 dir
static const struct file_operations mp3_fops = {
     .owner	= THIS_MODULE,
     .read = mp3_read,
     .write	= mp3_write,
 };

int __init mp3_init(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP3 MODULE LOADING\n");
   #endif
   printk(KERN_ALERT "Hello World, this is MP3\n");
   // Initialize a block of memory using vmalloc
   my_buffer = vmalloc(BUFFER_BLOCK);
   if(!my_buffer){
      printk(KERN_ALERT "Unable to allocate buffer memory");
   }
   // Initilizing the linked list
   INIT_LIST_HEAD(&test_head);
   // Created the directory
   mp3_dir = proc_mkdir("mp3", NULL);
   // Adding the files
   mp3_file = proc_create("status", 0666, mp3_dir, & mp3_fops);
   // Checkpoint 1 done
   // Setup Work Queue
   //my_wq = create_workqueue("mp1q");
   printk(KERN_ALERT "Initializing a module with timer.\n");
   // Set up the timer
  // setup_timer(&my_timer, my_timer_callback, 0);
   //mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));

   printk(KERN_ALERT "MP3MODULE LOADED\n");
   return 0;   
}

// mp1_exit - Called when module is unloaded
void __exit mp3_exit(void)
{
   #ifdef DEBUG
   printk(KERN_ALERT "MP3 MODULE UNLOADING\n");
   #endif
   printk(KERN_ALERT "Goodbye\n");
   // Removing the timer
  // del_timer(&my_timer);
   //Removing the workqueue
  // flush_workqueue( my_wq );
   //destroy_workqueue( my_wq );
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
   remove_proc_entry("status", mp3_dir);
   remove_proc_entry("mp3", NULL);

   printk(KERN_ALERT "MP3 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp3_init);
module_exit(mp3_exit);

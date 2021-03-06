#define LINUX
/**
 * HEADERS  FOR  THE MP3
 * **/
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
#include <linux/kdev_t.h> 
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/mutex.h>

#include "mp3_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("dipayan2");
MODULE_DESCRIPTION("CS-423 MP3");

#define DEBUG 1
#define NPAGES 128
//#define PAGE_SIZE 4096
#define BUFFER_BLOCK (128*4*1024)
#define DEVICENAME "mp3buf" // Name of the device



/* LOCKING VARIABLE*/
static DEFINE_SPINLOCK(my_spin);
static DEFINE_MUTEX(my_mutex);

/*
* FILLER FUINCTION : To remove the white space in the buffer
*/
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
 * Data structure to store the data in vmalloc buffer , get cpu_util and jiffies and faults
 * **/

struct fault_stats{

    unsigned long time_jiffies;
    unsigned long total_minor_fault_count;   
    unsigned long total_major_fault_count;
    unsigned long cpu_utilization;
};


static struct workqueue_struct *my_wq; // Work Queue
struct work_struct my_work; // Work variable
struct list_head test_head; // Linked list to store registered process
static struct timer_list my_timer; // Timer Interrupt
struct fault_stats* vmalloc_area; // vmalloc pointer variable
unsigned long buffer_index = 0;
unsigned long init_jiffy; // Variable to store when new work queue was created
int init_wq = 0;
/**
 * Enhanced PCB
 * **/
struct mp3_task_struct {
  struct task_struct *linux_task;
  struct list_head list;
  unsigned long long process_utilization; 
  pid_t pid;
  unsigned long min_flt;
  unsigned long maj_flt;
};


/**
 * Bottom Handler
 * WorkQueue Task : Triggered by timer , this function updates the fault count and utilization to the vmalloc area
 * **/

static void memFunction(struct work_struct *work){
    int ret;
    struct list_head *pos, *q;
    struct mp3_task_struct *tmp;
    long minor_fault_count;
    long major_fault_count;
    long utime;
    long stime;

    struct fault_stats* data_ptr;
    //Update index
    data_ptr= vmalloc_area + (buffer_index++);

    if (buffer_index == (BUFFER_BLOCK/sizeof(struct fault_stats)) - 2 )
    {
      printk(KERN_INFO "WARNING! BUFFER FULL \n");
    }
    if (buffer_index  > (BUFFER_BLOCK/sizeof(struct fault_stats)) - 2 )
    {
      printk(KERN_INFO "ERROR - BUFFER FULL!!! \n");
      return;
    }

    data_ptr->time_jiffies = jiffies;
    data_ptr->total_minor_fault_count = 0;
    data_ptr->total_major_fault_count = 0;
    data_ptr->cpu_utilization = 0;
    
    (data_ptr + 1)->time_jiffies = -1; // for the last element
  
   

    // should put a lock here, because the registration can cause inconsistent state.
    //printk(KERN_INFO "This is memfunction going into lock\n");
    //spin_lock(&my_spin);
    mutex_lock(&my_mutex);
    if(!list_empty(&test_head)){
    list_for_each_safe(pos, q, &test_head){
      tmp= list_entry(pos, struct mp3_task_struct, list);
      //printk(KERN_ALERT "[MemFunc] The pid of the task : %d\n", tmp->pid );
      ret = get_cpu_use(tmp->pid, &minor_fault_count, &major_fault_count, &utime, &stime);
      if ( ret ){
        printk( KERN_ALERT "Process %ld does not exist anymore, will be removed\n", (long int)tmp->pid);
        list_del(pos);
        if(tmp != NULL){
           kfree(tmp);
        }
        
        buffer_index--;
      }
      else{

        //Save state in each process
        tmp->min_flt   += minor_fault_count;
        tmp->maj_flt   += major_fault_count;
        utime = cputime_to_jiffies(utime);
        stime = cputime_to_jiffies(stime);
        tmp->process_utilization = ((utime + stime) * 10000) / (jiffies - init_jiffy);

        //Update stats on the buffer
        data_ptr->total_minor_fault_count += tmp->min_flt;
        data_ptr->total_major_fault_count += tmp->maj_flt;
        data_ptr->cpu_utilization         += tmp->process_utilization;
      }
    }
    }
    //spin_unlock(&my_spin);
    mutex_unlock(&my_mutex);

   return;
}


/**
 * Timer Handler : Top Half
 * 
 * **/

void my_timer_callback(unsigned long data) {
  //printk(KERN_ALERT "This line is printed after 5 seconds.\n");
  // Add job to the queue
 // spin_lock(&my_spin);
  if (!list_empty(&test_head)){
     if (my_wq != NULL){
       queue_work(my_wq, &my_work); 
     }
     mod_timer(&my_timer, jiffies + msecs_to_jiffies(50)); // This function ensures that timer is called again in 5 second
  }
  // spin_unlock(&my_spin);
   return;
}
int mycreate_workqueue(void){
   init_wq = 1;
   init_jiffy = jiffies;
   my_wq = create_workqueue("mp3q");
   INIT_WORK(&my_work, memFunction); // Attached function to the work
   queue_work(my_wq, &my_work); 
   mod_timer(&my_timer, jiffies + msecs_to_jiffies(50)); 
   return 0;
}

int mydelete_workqueue(void){
   if (my_wq != NULL){
   flush_workqueue( my_wq );
   destroy_workqueue( my_wq );
   }
   init_wq = 0;
   my_wq = NULL;
   printk(KERN_ALERT "in myDelete WQ\n");
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
   long readVal[2];
   char action;
   char *endptr;
   long value;
   int pid_inp = -1;
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
                printk(KERN_ALERT "[Reg]Error in long conversion");
                readVal[idx-1] = -1;
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
   printk(KERN_ALERT "Register %d\n",pid_inp);
  
   if (pid_inp == -1){
      return;
   }
  
   task_inp = (struct mp3_task_struct *) kmalloc(sizeof(struct mp3_task_struct),GFP_KERNEL); // need to free this memory
   if (!task_inp){
      printk(KERN_INFO "Unable to allocate pid_inp memory\n");
      return ;
   }
   // Allocate things to the  list value
  
   // if (get_cpu_use(pid_inp, &min_flt, &maj_flt, &utime, &stime) == -1){
   //    printk(KERN_ALERT "Unable to get page details for the task");
   //    kfree(task_inp);
   //    return;
   // }

   // Adding data to the node
   task_inp->linux_task = find_task_by_pid(pid_inp);
   task_inp->maj_flt = 0;
   task_inp->min_flt = 0;
   task_inp->process_utilization = 0;
   task_inp->pid = pid_inp;



   // Add to list should be within lock
   // Should check if the list is 
   if (init_wq == 0){
      printk(KERN_ALERT "[Reg]Creating the workqueue\n");
      mycreate_workqueue();
   }
   //create_workqueue();
   //spin_lock(&my_spin);
   mutex_lock(&my_mutex);
   list_add(&(task_inp->list),&test_head);
   //spin_unlock(&my_spin);
   mutex_unlock(&my_mutex);

   // // Check if the list if addedd
   // if(list_empty(&test_head)){
   //    printk(KERN_ALERT "[Reg]Unable to add the element after Registration\n");
   // }
   return;
   
}

/**
 * DeRegistration of Process Handler
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
   struct mp3_task_struct *temp;
   int flag = 0;

   while( (token = strsep(&kbuf," ")) != NULL ){
      if (idx == 0){
         action = *token;
          printk(KERN_ALERT "[De Reg]: This value is %c \n",action);
      }
      else if (idx <= 1){
         removeLeadSpace(&token);
         //printk(KERN_ALERT "The token : %s\n",token);
         value = simple_strtol(token, &endptr, 10);
         if (value == 0 && endptr == token){
            printk(KERN_ALERT "[DeReg]Error in long conversion\n");
         } 
         else{
            printk(KERN_ALERT "[DeReg]The pid value is %ld\n", value);
            t_pid = (int)value;
            break;
         }
         
      }
      idx += 1;
   } // read all the data

   printk(KERN_ALERT "[DeReg] Pid deregistered is : %d\n", t_pid);
   // Do work DeReg
   // need to stop the timer, but let's not do that yet!!
   // Just remove from the list
   // Need to remove the workqueue
   //spin_lock(&my_spin);
   mutex_lock(&my_mutex);
    if(!list_empty(&test_head)){
      list_for_each_safe(posv, qv, &test_head){

         temp= list_entry(posv, struct mp3_task_struct, list);
         printk(KERN_ALERT "[DeReg] Looping the list pid : %d\n",(int)temp->pid);
         if ((int)temp->pid == t_pid){
            flag = 1;

          
            list_del(posv);
            
            // need to delete the timer too
            printk(KERN_ALERT "\n[DeReg]Deleted Pid : %d\n", temp->pid);
            if (temp != NULL){
                  kfree(temp);
            }
            
            
            
         }
      }
    }
    //spin_unlock(&my_spin);
    mutex_unlock(&my_mutex);
   

   if (flag == 0){
      printk(KERN_ALERT "[DerReg] No Deregistered Pid : %d\n", t_pid);
   }

   //spin_lock(&my_spin);
 //  mutex_lock(&my_mutex);
   if(list_empty(&test_head)){
      printk(KERN_ALERT "[DeReg]Entering thr del workqueue");
      mydelete_workqueue();
      printk(KERN_ALERT "Deleted the workqueue\n");
      buffer_index = 0;
      init_jiffy = jiffies;
   }
 // spin_unlock(&my_spin);
// mutex_unlock(&my_mutex);


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
   if (!kbuf){
      printk(KERN_INFO "Unable to allocate buffer!!\n");
       return -ENOMEM;
   }
    ret = strncpy_from_user(kbuf, buffer, count);
    if (ret == -EFAULT){
        printk(KERN_INFO "\nerror in reading the PID\n");
        return -EFAULT;
    }
    //printk(KERN_ALERT "%s\n",kbuf);
    if (kbuf[0]=='R'){
       handleRegistration(kbuf);
    } 
    else if(kbuf[0]=='U'){
       handleDeReg(kbuf);
    }

    if (kbuf != NULL){
         kfree(kbuf);
    }
  
    return ret;
}

// kernel to proc
static ssize_t mp3_read (struct file *file, char __user *buffer, size_t count, loff_t *data){

     // Data declaration
   int copied;                // Return Value
   int len=0;                 // How much data is read
   char *buf;                 // Kernel buffer to read data
   struct list_head *ptr;     // Kernel Linked List 
   struct mp3_task_struct *entry; // Kernel Linked List
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
   //spin_lock(&my_spin);
   mutex_lock(&my_mutex);
   if (!list_empty(&test_head)){

      list_for_each(ptr,&test_head){
         entry=list_entry(ptr,struct mp3_task_struct,list);
         //printk(KERN_INFO "\n PID %d:Time %lu  \n ", entry->my_id,entry->cpu_time);
         // Add this entry into the buffer
         len += scnprintf(buf+len,count-len,"%d\n",entry->pid);
      }
      
   }
   else{
      printk(KERN_ALERT "[Read] Empty list\n");
   }
   //spin_unlock(&my_spin);
   mutex_unlock(&my_mutex);


   // Sending data to user buffer from kernel buffer
   copied = simple_read_from_buffer(buffer,count,data,buf,len);
   if (copied < 0){
      return -EFAULT;
   }
   if (buf != NULL){
         kfree(buf);
   }

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

/**
 * 
 * Character Device Handling
 * 
 * **/

static int device_open(struct inode *inode, struct file *flip) {
    return 0;
}

static int device_close(struct inode *inode, struct file *flip) {
    return 0;
}

/* Provides mmap function to char devices*/
static int device_mmap(struct file *filp, struct vm_area_struct *vma){
 printk(KERN_ALERT "[mmap]Inside the mmap function\n");
  int ret;
  unsigned long pfn, size;
  unsigned long start = vma->vm_start;
  unsigned long length = vma->vm_end - start;
  char *ptr = (char*) vmalloc_area;
  printk(KERN_ALERT "[mmap] Value done\n");
  if (vma->vm_pgoff > 0 || length > BUFFER_BLOCK)
    return -EIO;
  while (length > 0) {
    pfn = vmalloc_to_pfn(ptr);
    //printk(KERN_ALERT "The pfn number is %lu \n", pfn);
    size = length < PAGE_SIZE ? length : PAGE_SIZE;
    ret = remap_pfn_range(vma, start, pfn, size, PAGE_SHARED);
    if (ret < 0)
      return ret;
    start += PAGE_SIZE;
    ptr += PAGE_SIZE;
    length -= PAGE_SIZE;
  }
  printk(KERN_ALERT "[mmap] Done\n");
  return 0;

}

struct cdev *mp3_dev; /*this is the name of my char driver that i will be registering*/
int major_number; /* will store the major number extracted by dev_t*/
dev_t dev_num; /*will hold the major number that the kernel gives*/


struct file_operations mp3_cops = { /* these are the file operations provided by our driver */
    .owner = THIS_MODULE, /*prevents unloading when operations are in use*/
    .open = device_open,  /*to open the device*/
    .mmap = device_mmap,
    .release = device_close /*to close the device*/
    
};



int __init mp3_init(void)
{
   int i;
   int ret;
   #ifdef DEBUG
   printk(KERN_ALERT "MP3 MODULE LOADING\n");
   #endif
   printk(KERN_ALERT "Hello World, this is MP3\n");
      // Initilizing the linked list
   INIT_LIST_HEAD(&test_head);
   // Initialize a block of memory using vmalloc
   vmalloc_area = vmalloc(NPAGES*PAGE_SIZE);
   if(!vmalloc_area){
      printk(KERN_ALERT "Unable to allocate buffer memory");
      return -1;
   }
   /*Mark pages reserved*/
   for(i=0;i<NPAGES*PAGE_SIZE;i+= PAGE_SIZE){
       SetPageReserved(vmalloc_to_page((void *)(((unsigned long)vmalloc_area) + i)));
   }
   /* INIT CHARACTER DEVICE*/
   ret = alloc_chrdev_region(&dev_num,0,1,DEVICENAME);
   if (ret<0){
         printk(KERN_INFO "Cannot allocate major number for device 1\n");
         return -1;
   }
   major_number = MAJOR(dev_num);
   printk(KERN_INFO "Character device of major : %u initialized\n",major_number);
   // Allocate cdev structure
   mp3_dev = cdev_alloc();
   mp3_dev->ops = &mp3_cops;
   ret = cdev_add(mp3_dev,dev_num,1);
   if( ret< 0){
         printk(KERN_INFO "Device : device adding to the kerknel failed\n");
         return ret;
   }



   // Created the directory
   mp3_dir = proc_mkdir("mp3", NULL);
   // Adding the files
   mp3_file = proc_create("status", 0666, mp3_dir, & mp3_fops);
   // Checkpoint 1 done
   // Setup Work Queue
   my_wq = NULL;
   printk(KERN_ALERT "Initializing a module with timer.\n");
   // Set up the timer
   setup_timer(&my_timer, my_timer_callback, 0);
   //mod_timer(&my_timer, jiffies + msecs_to_jiffies(5000));
   init_jiffy = jiffies;

   printk(KERN_ALERT "MP3MODULE LOADED\n");
   return 0;   
}

// mp1_exit - Called when module is unloaded
void __exit mp3_exit(void)
{
   int i;
   #ifdef DEBUG
   printk(KERN_ALERT "MP3 MODULE UNLOADING\n");
   #endif
   printk(KERN_ALERT "Goodbye\n");
   // Removing the timer
   del_timer(&my_timer);
   //Removing the workqueue
   if (my_wq!= NULL){
      flush_workqueue( my_wq );
      destroy_workqueue( my_wq );
   }

   // Remove the list
   struct list_head *pos, *q;
   struct mp3_task_struct *tmp;

  // spin_lock(&my_spin);
  mutex_lock(&my_mutex);
   if(!list_empty(&test_head)){
         list_for_each_safe(pos, q, &test_head){

            tmp= list_entry(pos, struct mp3_task_struct, list);
            // printk(KERN_INFO "Freeing List\n");
            list_del(pos);
            if (tmp != NULL){
               kfree(tmp);
            }
            
         }
    }
   //spin_unlock(&my_spin);
   mutex_unlock(&my_mutex);
   printk(KERN_INFO "List Freed\n");

    // Removing the timer 
   
   /*Unreserving thr memory*/
   for(i=0;i<NPAGES*PAGE_SIZE;i+= PAGE_SIZE){
       ClearPageReserved(vmalloc_to_page((void *)(((unsigned long)vmalloc_area) + i)));
   }
   // Freeing virtual memory
   printk(KERN_ALERT "Freeing memory\n");
   vfree(vmalloc_area);
   // Character device deletion
   cdev_del(mp3_dev);
   unregister_chrdev_region(dev_num,1);
   // Removing the directory and files
   remove_proc_entry("status", mp3_dir);
   remove_proc_entry("mp3", NULL);

   printk(KERN_ALERT "MP3 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp3_init);
module_exit(mp3_exit);

#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h> /* Necessary because we use the proc fs */
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/delay.h>
#include "mp2_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_30");
MODULE_DESCRIPTION("CS-423 mp2");

#define DEBUG 1

#define procfs_name "status"

/* Allocated buffer for pid reception, which is an int32 */
#define INCOMMING_MAX_SIZE  sizeof(struct info_send)

#define MAX_UTILIZATION 693
#define MAX_PERIOD 100000
#define STATE_RUNNING  0
#define STATE_READY    1
#define STATE_SLEEPING 2


/* Lock data structure */

DEFINE_SPINLOCK( mr_lock );

/* proc filesystem and linked list data structures */

LIST_HEAD(mp2_linked_list) ;

struct kmem_cache* mp2_task_struct_cache;

struct mp2_task_struct {
     unsigned long pid;
     unsigned long user_time;
     unsigned int duration_ms;
     unsigned int period_ms;
     unsigned int state;
     unsigned long long initial_time; 
     struct task_struct* kernel_pcb;
     struct timer_list wakeup_timer;
     struct list_head mp2_list ;
} ;

void mp2_task_struct_constructor(void *buf, int size)
{
    struct mp2_task_struct *mp2_task_struct = buf;
}

/* Running task struct */
struct mp2_task_struct* running_task;
struct task_struct* dispatching_task;

struct timeval time_val ;

unsigned int utilization;

//--------------

void timer_handler( unsigned long pid )
{

    struct list_head *pos, *q;
    struct mp2_task_struct * tmp, *task;  
    char flag = 0x0;
    unsigned long flags;
    // We only trigger the work when the list is not empty
    printk("Timer called: %ld \n", pid);

    spin_lock_irqsave(&mr_lock, flags);
    //spin_lock(&mr_lock);
    list_for_each_safe(pos, q, & mp2_linked_list){
      tmp= list_entry(pos, struct mp2_task_struct, mp2_list);

      if (tmp->pid == pid){
        task = tmp;
        task->state = STATE_READY;
        flag = 0x1;
      }
    }
    //spin_unlock(&mr_lock);
    spin_unlock_irqrestore(&mr_lock, flags);

    if ( flag == 0x0 ){
      printk( "ERROR - Process %ld does not exist in the interrupt! \n", pid);
      return;
    }

    wake_up_process(dispatching_task);
}

static int status_proc_show(struct seq_file *m, void *v) {

  struct mp2_task_struct* tmp; 

  #ifdef DEBUG
  printk("calling status_proc_show \n");
  #endif

  // should put a lock here, because the registration can cause inconsisten state.
  spin_lock(&mr_lock);

  //We used seq_file to print because it is a tool that allow us no to
  //worry about paging. 
  list_for_each_entry ( tmp , & mp2_linked_list, mp2_list ) 
  { 
    seq_printf(m, "%ld (%d, %d) \n" , tmp->pid, tmp->duration_ms, tmp->period_ms ); 
  }

  spin_unlock(&mr_lock);

  return 0;
}

static int status_proc_open(struct inode *inode, struct  file *file) {

   printk("calling status_proc_open \n" );
   return single_open(file, status_proc_show, NULL);
}

int admission_control(struct info_send* receive){

  unsigned int temp_utilization; 

  temp_utilization = (receive->duration_ms * 1000) / receive->period_ms;

  /* We keep track of a global utilizacion, 
      so this call is O(1) */

  if (utilization + temp_utilization > MAX_UTILIZATION){
    printk( "Not enough time. Actual: %d, requested: %d \n", utilization, temp_utilization);
    return -1;
  }
  else{
    utilization += temp_utilization;
    printk("Utilizacion: %d \n",utilization );
  }

  return 0;
}

int dispatching_thread(void *data)
{
  struct list_head *pos, *q;
  struct mp2_task_struct * tmp, *task;  
  char flag = 0x0;
  unsigned int period;
  struct sched_param sparam; 

  while(!kthread_should_stop())
  {
    printk( "Dispatcher waked up \n");
    period = MAX_PERIOD;
    flag = 0x0;

    if (running_task != NULL && running_task->state == STATE_SLEEPING){
      // Because of the YIELD
      sparam.sched_priority=0; 
      sched_setscheduler(running_task->kernel_pcb, SCHED_NORMAL, &sparam);
      running_task = NULL;
    }

    spin_lock(&mr_lock);
    list_for_each_safe(pos, q, & mp2_linked_list){
      tmp= list_entry(pos, struct mp2_task_struct, mp2_list);

      if (tmp->state == STATE_READY && tmp->period_ms < period){
        task = tmp;
        period = tmp->period_ms;
        flag = 0x1;
      }
      
    }
    spin_unlock(&mr_lock);

    if ( flag == 0x0 )
    {
      printk( "No process in STATE_READY! \n");
      
      if (running_task != NULL)
      { /*
        if(running_task->state == STATE_RUNNING){
          running_task->state = STATE_READY;
        }
        sparam.sched_priority=0; 
        sched_setscheduler(running_task->kernel_pcb, SCHED_NORMAL, &sparam);
        printk("Preemting runnig task %ld \n", running_task->pid);
        running_task = NULL;
        */
      }

    }
    else 
    {
      printk( "Dispatcher ready pid: %ld \n", task->pid);

      if (running_task != NULL)
      {
        if (running_task->period_ms <= task->period_ms && running_task->state == STATE_RUNNING)
        {
          printk("Not Preemting %ld with %ld \n", running_task->pid, task->pid);
          task->state = STATE_READY;
        } 
        else
        { // Preamtion case
          task->state = STATE_RUNNING; 
          
          sparam.sched_priority=99;
          sched_setscheduler(task->kernel_pcb, SCHED_FIFO, &sparam);
          wake_up_process(task->kernel_pcb);

          running_task->state = STATE_READY;
          sparam.sched_priority=0; 
          sched_setscheduler(running_task->kernel_pcb, SCHED_NORMAL, &sparam);
    
          printk("Preemting %ld with %ld \n", running_task->pid, task->pid);
          running_task = task;
        }
      }
      else
      {
          task->state = STATE_RUNNING; 
          wake_up_process(task->kernel_pcb);
          sparam.sched_priority=99;
          sched_setscheduler(task->kernel_pcb, SCHED_FIFO, &sparam);
    
          running_task = task;
          printk( "Dispatching %ld ! \n", running_task->pid);
      }

      printk( "Dispatching %ld done! \n", running_task->pid);
    }

    set_task_state(dispatching_task, TASK_UNINTERRUPTIBLE);
    printk( "Sleeping dispatcher ! \n");
    schedule();

  } // end while



  return 0;
}

int registration(struct info_send* receive){

  struct mp2_task_struct* new_process;
  struct task_struct* new_task;
  
  new_task = find_task_by_pid(receive->pid);

  if ( new_task == NULL ){
    printk( "Process received %d does not exist \n", receive->pid);
    return -1;
  }

  if ( admission_control(receive) ){
    printk( "Not admission control for %d \n", receive->pid);
    return -1;
  }
  
  {

    //new_process = kmalloc(sizeof(struct mp2_task_struct), GFP_KERNEL); 
    new_process = kmem_cache_alloc(mp2_task_struct_cache, GFP_KERNEL);

    new_process->pid = receive->pid;
    new_process->duration_ms = receive->duration_ms;
    new_process->period_ms   = receive->period_ms;
    new_process->kernel_pcb = new_task;
    new_process->state = STATE_SLEEPING;
    do_gettimeofday(&time_val);
    new_process->initial_time = time_val.tv_sec * 1000 * 1000 + time_val.tv_usec ;
    setup_timer( &new_process->wakeup_timer, timer_handler, new_process->pid );

    printk("receive: %ld, %d, %d \n", new_process->pid, new_process->duration_ms, 
                                      new_process->period_ms);

    // should put a lock here, conflict with other methods using the list.
    spin_lock(&mr_lock);
    list_add ( &new_process->mp2_list , &mp2_linked_list ) ;
    spin_unlock(&mr_lock);
  }

  return 0;

}

int deregistration(unsigned int pid){

  struct list_head *pos, *q;
  struct mp2_task_struct *tmp;  
  char flag = 0x0;

  spin_lock(&mr_lock);
  list_for_each_safe(pos, q, & mp2_linked_list){
    tmp= list_entry(pos, struct mp2_task_struct, mp2_list);

    if (tmp->pid == pid){

      if (running_task == tmp){
        running_task = NULL;
      }

      /* We need to decrement the utilizacion */
      utilization -= (tmp->duration_ms * 1000) / tmp->period_ms;
      list_del(pos);
      del_timer(&tmp->wakeup_timer);
      kmem_cache_free(mp2_task_struct_cache, tmp);

      //kfree(tmp);
      flag = 0x1;
    }
    
  }
  spin_unlock(&mr_lock);

  if ( flag == 0x0 ){
    printk( "Process %d does not exist - NO deregistration done! \n", pid);
    return -1;
  }

  wake_up_process(dispatching_task);

  return 0;
}

int Yield(unsigned int pid){

  struct list_head *pos, *q;
  struct mp2_task_struct * tmp, *task;  
  char flag = 0x0;
  struct sched_param sparam; 
  unsigned long time_to_wakeup ;
  unsigned long long curr_time ; 

  spin_lock(&mr_lock);
  list_for_each_safe(pos, q, & mp2_linked_list){
    tmp= list_entry(pos, struct mp2_task_struct, mp2_list);

    if (tmp->pid == pid){
      task = tmp;
      flag = 0x1;
    }
    
  }
  spin_unlock(&mr_lock);

  if ( flag == 0x0 ){
    printk( "Process %d does not exist - NO yield done! \n", pid);
    return -1;
  }

  task->state = STATE_SLEEPING;

  set_task_state( task->kernel_pcb , TASK_UNINTERRUPTIBLE); 

  do_gettimeofday(&time_val);
  curr_time = time_val.tv_sec * 1000 * 1000 + time_val.tv_usec ; // in msecs
  time_to_wakeup = task->period_ms - (((curr_time - task->initial_time )/1000)%(task->period_ms));
  mod_timer( &task->wakeup_timer, jiffies + msecs_to_jiffies(task->period_ms - task->duration_ms) );

  sparam.sched_priority=0; 
  sched_setscheduler(task->kernel_pcb, SCHED_NORMAL, &sparam);
  wake_up_process(dispatching_task);

  printk( "Process %d Yield done! \n", pid);
  return 0;
}

int procfile_write(struct file *file, const char *buffer, 
                      unsigned long count, void *data)
{
    int buffer_size = 0;
    struct info_send receive;

    /* get buffer size */
    buffer_size = count;
    if (buffer_size > INCOMMING_MAX_SIZE ) {
      buffer_size = INCOMMING_MAX_SIZE;
    }

    /* write data to the buffer */
    if ( copy_from_user( (void *)&(receive), buffer, buffer_size) ) {
      return -EFAULT;
    }

    switch (receive.opcode){

        case 'R': 
                printk( "Registration (%d,%d,%d) \n", receive.pid, receive.duration_ms, receive.period_ms);
                if ( registration(&receive) ){
                  printk( "Error registering %d \n", receive.pid);
                  buffer_size = -1;
                };
                break;
        case 'Y': 
                printk( "Yield (%d) \n", receive.pid);
                if ( Yield(receive.pid) ){
                  printk( "Error Yield %d \n", receive.pid);
                };
                break;
        case 'D': 
                printk( "Deregister (%d) \n", receive.pid);
                if ( deregistration(receive.pid) ){
                  printk( "Error deregistering %d \n", receive.pid);
                };
                break;
        default:
                printk ("ERROR OPCODE!!: %c \n", receive.opcode);
                break;
    }

    return buffer_size;
}

static const struct file_operations status_proc_fops = {
  .owner = THIS_MODULE,
  .open = status_proc_open,
  .read = seq_read, // This simplifies the reading process
  .write = procfile_write, // Implement our own writing method
  .llseek = seq_lseek,
  .release = single_release,
};



// mp2_init - Called when module is loaded
int __init mp2_init(void)
{
    struct proc_dir_entry *proc_parent; 
    struct proc_dir_entry *proc_write_entry;

    #ifdef DEBUG
    printk(KERN_ALERT "MP2 MODULE LOADING\n");
    #endif
    // Insert your code here ...

    /* Creation proc filesytems dir and file */
    proc_parent = proc_mkdir("mp2",NULL);
    proc_write_entry =proc_create("status", 0666, proc_parent, &status_proc_fops);

    mp2_task_struct_cache = kmem_cache_create("mp2_task_struct_cache", 
                              sizeof (struct mp2_task_struct), 0, 0,
                              (void*)mp2_task_struct_constructor);

    utilization = 0;

    running_task = NULL;

    dispatching_task = kthread_create(&dispatching_thread,(void *)&utilization,"dispatching_thread");

    printk(KERN_ALERT "MP2 MODULE LOADED\n");
    return 0;   
}

// mp2_exit - Called when module is unloaded
void __exit mp2_exit(void)
{
    struct list_head *pos, *q;
    struct mp2_task_struct *tmp;

    #ifdef DEBUG
    printk(KERN_ALERT "MP2 MODULE UNLOADING\n");
    #endif
    // Insert your code here ...

    /* Removing entrys in proc filesystem */

    remove_proc_entry("mp2/status",NULL);
    remove_proc_entry("mp2",NULL);

    kthread_stop(dispatching_task);

    /* Deleting list */

    /* let'sfree the mp2_task_struct items. since we will be removing items
    * off the list using list_del() we need to use a safer version of the list_for_each() 
    * macro aptly named list_for_each_safe(). Note that you MUST use this macro if the loop 
    * involves deletions of items (or moving items from one list to another).
    */
    //printk("deleting the list using list_for_each_safe()\n");
    spin_lock(&mr_lock);
    list_for_each_safe(pos, q, & mp2_linked_list){
      tmp= list_entry(pos, struct mp2_task_struct, mp2_list);

      #ifdef DEBUG
      printk("freeing item pid= %ld \n", tmp->pid);
      #endif
      //del_timer(&tmp->wakeup_timer);
      list_del(pos);
      del_timer(&tmp->wakeup_timer);
      kmem_cache_free(mp2_task_struct_cache, tmp);
    }
    spin_unlock(&mr_lock);

    kmem_cache_destroy(mp2_task_struct_cache);

    printk(KERN_ALERT "MP2 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp2_init);
module_exit(mp2_exit);

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/atomic.h>
#include <linux/sched.h>
#include <linux/fs_struct.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <linux/time.h>
#include <asm/io.h>
#include <linux/proc_fs.h>
#include <linux/ktime.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

#define IRQ_NUM 1
#define DIR_NAME "key_buf"

char* ascii[84] =  { " ", "Esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "+", "Backspace", 
                      "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Ctrl",
                      "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "\"", "'", "Shift (left)", "|", 
                      "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "Shift (right)", 
                      "*", "Alt", "Space", "CapsLock", 
                      "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
                      "NumLock", "ScrollLock", "Home", "Up", "Page-Up", "-", "Left",
                      " ", "Right", "+", "End", "Down", "Page-Down", "Insert", "Delete"};

static struct tasklet_struct *tasklet = NULL;
static struct proc_dir_entry *proc_entry = NULL;

char last_key_name[20];
int last_key_code = -1;

static ktime_t irq_start_time;

void my_tasklet_fun(unsigned long data) 
{
    int code = inb(0x60);
    int is_release = (code & 0x80) ? 1 : 0;
    int press_code = code & 0x7F;
    if (is_release && press_code < 84 && press_code != 28)
    {
        last_key_code = code;
        strncpy(last_key_name, ascii[press_code], sizeof(last_key_name) - 1);
        last_key_name[sizeof(last_key_name) - 1] = '\0';

        ktime_t end_time = ktime_get();
        ktime_t diff;
        s64 nsecs;
        diff = ktime_sub(end_time, irq_start_time);
        nsecs = ktime_to_ns(diff);

        printk(KERN_INFO "+ Key: %s Code: %d Execution time: %lld\n", last_key_name, last_key_code, nsecs);
    }
}

static irqreturn_t my_irq_handler(int irq, void *dev_id) 
{
    if (irq == IRQ_NUM) 
    {
        irq_start_time = ktime_get();
        tasklet_schedule(tasklet);
        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}

static int key_buf_show(struct seq_file *m, void *v) 
{
    if (last_key_code != -1)
        seq_printf(m, "Key: %s Code: %d\n",
                   last_key_name, last_key_code);
    return 0;
}

static int key_buf_open(struct inode *inode, struct file *file) 
{
    return single_open(file, key_buf_show, NULL);
}

static const struct proc_ops key_buf_fops = {
    .proc_open = key_buf_open,
    .proc_read = seq_read,
    .proc_release = single_release,
};

static int __init my_init(void) 
{
    int ret = request_irq(IRQ_NUM, my_irq_handler, IRQF_SHARED, 
                         "my_irq_handler", (void *)(my_irq_handler));
    if (ret) 
    {
        printk(KERN_ERR "Error request_irq: %d\n", ret);
        return ret;
    }

    tasklet = kmalloc(sizeof(struct tasklet_struct), GFP_KERNEL);
    if (!tasklet) 
    {
        printk(KERN_ERR "Error kmalloc tasklet\n");
        free_irq(IRQ_NUM, (void *)(my_irq_handler));
        return -ENOMEM;
    }

    tasklet_init(tasklet, my_tasklet_fun, 0);

    proc_entry = proc_create(DIR_NAME, 0444, NULL, &key_buf_fops);
    if (!proc_entry) 
    {
        printk(KERN_ERR "Failed to create\n");
        tasklet_kill(tasklet);
        kfree(tasklet);
        free_irq(IRQ_NUM, (void *)(my_irq_handler));
        return -ENOMEM;
    }

    printk(KERN_INFO "+ module loaded.\n");
    return 0;
}

static void __exit my_exit(void) 
{
    tasklet_kill(tasklet);
    kfree(tasklet);
    free_irq(IRQ_NUM, (void *)(my_irq_handler));
    
    if (proc_entry)
        proc_remove(proc_entry);
        
    printk(KERN_INFO "+ module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);
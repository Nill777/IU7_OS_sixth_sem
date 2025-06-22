#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/stddef.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/ktime.h>

MODULE_LICENSE("GPL");
#define IRQ_NUM 1
#define DIR_NAME "key_buf"

char *ascii[84] = {" ", "Esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "+", "Backspace",
                   "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Ctrl",
                   "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "\"", "'", "Shift (left)", "|",
                   "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "Shift (right)",
                   "*", "Alt", "Space", "CapsLock",
                   "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
                   "NumLock", "ScrollLock", "Home", "Up", "Page-Up", "-", "Left",
                   " ", "Right", "+", "End", "Down", "Page-Down", "Insert", "Delete"};

static struct workqueue_struct *my_wq1;
static struct workqueue_struct *my_wq2;

struct my_work_struct
{
    struct work_struct work;
    ktime_t start_time;
    int code;
};
static struct my_work_struct *work1;
static struct my_work_struct *work2;

char last_key_name[20];
int last_key_code = -1;

static struct proc_dir_entry *proc_entry = NULL;

static int key_buf_show(struct seq_file *m, void *v)
{
    if (last_key_code != -1)
        seq_printf(m, "Key: %s Code: %d\n", last_key_name, last_key_code);
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

void work1_func(struct work_struct *work)
{
    printk(KERN_INFO "+ work1 begin");
    struct my_work_struct *mw = container_of(work, struct my_work_struct, work);
    ktime_t end_time;
    s64 diff_ns;

    last_key_code = mw->code;
    strncpy(last_key_name, ascii[mw->code & 0x7F], sizeof(last_key_name) - 1);
    last_key_name[sizeof(last_key_name) - 1] = '\0';

    end_time = ktime_get();
    diff_ns = ktime_to_ns(ktime_sub(end_time, mw->start_time));

    printk(KERN_INFO "+ Key: %s Code: %d Execution time: %lld\n", last_key_name, last_key_code, diff_ns);
    printk(KERN_INFO "+ work1 end");
}

void work2_func(struct work_struct *work)
{
    printk(KERN_INFO "+ work2 begin");
    msleep(10);
    struct my_work_struct *mw = container_of(work, struct my_work_struct, work);

    last_key_code = mw->code;
    strncpy(last_key_name, ascii[mw->code & 0x7F], sizeof(last_key_name) - 1);
    last_key_name[sizeof(last_key_name) - 1] = '\0';

    printk(KERN_INFO "+ Key: %s Code: %d\n", last_key_name, last_key_code);
    printk(KERN_INFO "+ work2 end");
}

irqreturn_t my_irq_handler(int irq, void *dev)
{
    int code = inb(0x60);
    int is_release = (code & 0x80) ? 1 : 0;
    int press_code = code & 0x7F;
    if (irq == IRQ_NUM)
        if (is_release && press_code < 84 && press_code != 28)
        {
            work1->start_time = ktime_get();
            work1->code = code;
            work2->start_time = ktime_get();
            work2->code = code;

            queue_work(my_wq1, (struct work_struct *)work1);
            queue_work(my_wq2, (struct work_struct *)work2);
            return IRQ_HANDLED;
        }
    return IRQ_NONE;
}

static int __init my_workqueue_init(void)
{
    int ret = request_irq(IRQ_NUM, my_irq_handler, IRQF_SHARED,
                          "my_irq_handler", (void *)my_irq_handler);

    printk(KERN_INFO "+ init");
    if (ret)
    {
        printk(KERN_ERR "Error request_irq: %d\n", ret);
        return ret;
    }
    else
    {
        my_wq1 = alloc_workqueue("my_wq1", WQ_MEM_RECLAIM, 1);
        my_wq2 = alloc_workqueue("my_wq2", WQ_MEM_RECLAIM, 1);

        if (!my_wq1 || !my_wq2)
        {
            printk(KERN_ERR "+ create queue error");
            ret = GFP_NOIO;
            return ret;
        }

        work1 = kmalloc(sizeof(struct my_work_struct), GFP_KERNEL);
        if (work1 == NULL)
        {
            printk(KERN_ERR "+ work1 alloc error");
            destroy_workqueue(my_wq1);
            destroy_workqueue(my_wq2);
            ret = GFP_NOIO;
            return ret;
        }

        work2 = kmalloc(sizeof(struct work_struct), GFP_KERNEL);
        if (work2 == NULL)
        {
            printk(KERN_ERR "+ work2 alloc error");
            destroy_workqueue(my_wq1);
            destroy_workqueue(my_wq2);
            kfree(work1);
            ret = GFP_NOIO;
            return ret;
        }

        INIT_WORK((struct work_struct *)work1, work1_func);
        INIT_WORK((struct work_struct *)work2, work2_func);

        proc_entry = proc_create(DIR_NAME, 0444, NULL, &key_buf_fops);
        if (!proc_entry)
        {
            printk(KERN_ERR "+ Failed to create proc entry\n");
            destroy_workqueue(my_wq1);
            destroy_workqueue(my_wq2);
            kfree(work1);
            kfree(work2);
            free_irq(IRQ_NUM, (void *)my_irq_handler);
            return -ENOMEM;
        }
    }
    return 0;
}

static void __exit my_workqueue_exit(void)
{
    printk(KERN_INFO "+ exit");

    synchronize_irq(IRQ_NUM); 
    free_irq(IRQ_NUM, my_irq_handler);

    flush_workqueue(my_wq1);
    flush_workqueue(my_wq2);

    destroy_workqueue(my_wq1);
    destroy_workqueue(my_wq2);

    kfree(work1);
    kfree(work2);

    printk(KERN_INFO "+ unloaded");
}

module_init(my_workqueue_init);
module_exit(my_workqueue_exit);
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/rcupdate.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

#define DIRNAME "seqfiles"
#define FILENAME "seqfile"
#define SYMLINK "seqfile_link"
#define FILEPATH DIRNAME "/" FILENAME

static struct proc_dir_entry *dir;
static struct proc_dir_entry *afile;
static struct proc_dir_entry *link;

static int *pids = NULL;
static int pids_count = 0;
static int pids_capacity = 10;

static int init_pids_buffer(void)
{
    pids = kmalloc_array(pids_capacity, sizeof(int), GFP_KERNEL);
    return pids ? 0 : -ENOMEM;
}

static int add_pid(int pid)
{
    if (pids_count >= pids_capacity)
    {
        int new_capacity = pids_capacity * 2;
        int *new_pids = krealloc(pids, new_capacity * sizeof(int), GFP_KERNEL);
        if (!new_pids)
            return -ENOMEM;
        pids = new_pids;
        pids_capacity = new_capacity;
    }
    pids[pids_count++] = pid;
    return 0;
}

int seqfile_show(struct seq_file *m, void *v)
{
    printk(KERN_INFO "+ show, m = %p, v = %p.\n", m, v);
    struct task_struct *task;

    if (pids_count == 0)
    {
        seq_puts(m, "No pids stored\n");
        return 0;
    }

    for (int i = 0; i < pids_count; i++)
    {
        rcu_read_lock();
        task = pid_task(find_vpid(pids[i]), PIDTYPE_PID);
        if (task)
        {
            seq_printf(m, "pid %d, policy %d, prio %d\n",
                       task->pid, task->policy, task->prio);
        }
        else
        {
            seq_printf(m, "pid %d: not found\n", pids[i]);
        }
        rcu_read_unlock();
    }

    return 0;
}

ssize_t my_write(struct file *file, const char __user *buf, size_t len, loff_t *offp)
{
    char input[16];
    int new_pid;

    printk("+ write\n");

    if (len > sizeof(input) - 1)
    {
        printk(KERN_ERR "+ input too long\n");
        return -EINVAL;
    }

    if (copy_from_user(input, buf, len))
    {
        printk(KERN_ERR "+ copy_from_user failed\n");
        return -EFAULT;
    }
    input[len] = '\0';

    if (kstrtoint(input, 10, &new_pid))
    {
        printk(KERN_ERR "+ invalid PID format\n");
        return -EINVAL;
    }

    if (add_pid(new_pid))
    {
        printk(KERN_ERR "+ Failed to store PID\n");
        return -ENOMEM;
    }

    return len;
}

int my_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "+ open\n");
    return single_open(file, seqfile_show, NULL);
}

int my_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "+ release\n");
    return single_release(inode, file);
}

ssize_t my_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    int len = seq_read(file, buf, size, ppos);
    if (len > 0)
        printk("+ read before\n");
    else printk("+ read after\n");
    return len;
}

static struct proc_ops fops = {
    .proc_read = my_read,
    .proc_write = my_write,
    .proc_open = my_open,
    .proc_release = my_release};

static void freemem(void)
{
    if (link)
        remove_proc_entry(SYMLINK, dir);
    if (afile)
        remove_proc_entry(FILENAME, dir);
    if (dir)
        remove_proc_entry(DIRNAME, NULL);
    kfree(pids);
    pids = NULL;
    pids_count = 0;
    pids_capacity = 0;
}

static int __init mod_init(void)
{
    if (init_pids_buffer())
    {
        printk(KERN_ERR "+ Failed to init pids buffer\n");
        return -ENOMEM;
    }

    dir = proc_mkdir(DIRNAME, NULL);
    afile = proc_create(FILENAME, 0666, dir, &fops);
    link = proc_symlink(SYMLINK, dir, FILEPATH);

    printk(KERN_INFO "+ module loaded!\n");
    return 0;
}

static void __exit mod_exit(void)
{
    freemem();
    printk(KERN_INFO "+ module unloaded!\n");
}

module_init(mod_init);
module_exit(mod_exit);
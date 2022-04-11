#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>


#define MAJOR_NUMBER 51

/*forward delcalration*/
int onebyte_open(struct inode *inode, struct file *filep);
int onebyte_release(struct inode *inode, struct file *filep);
ssize_t onebyte_read(struct file *filep, char *buf, size_t count, loff_t *f_pos);
ssize_t onebyte_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos);
static void onebyte_exit(void);

static void tasklet_fn(unsigned long);

DECLARE_TASKLET(tasklet, tasklet_fn, 10);

static void tasklet_fn(unsigned long arg) {
	tasklet_disable(&tasklet);
	printk(KERN_INFO "Executing tasklet function with arg: %ld\n", arg);
	tasklet_enable(&tasklet);
}


/*definition of the file operation structure*/
struct file_operations onebyte_fops = {
	read:	onebyte_read,
	write:	onebyte_write,
	open:	onebyte_open,
	release:onebyte_release
};

char *onebyte_data = NULL;

int onebyte_open(struct inode *inode, struct file *filep) {
	return 0; // always successful
}

int onebyte_release(struct inode *inode, struct file *filep) {
	return 0; // always successful
}

ssize_t onebyte_read(struct file *filep, char *buf, size_t count, loff_t *f_pos) {
	
	if (!(*onebyte_data) || *f_pos >= 1) return 0;
	
	if (copy_to_user(buf, onebyte_data, 1)) {
		return -EFAULT;
	}
	
	*f_pos += 1;
	return 1;

}

ssize_t onebyte_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos) {
	
	if (count <= 0) return 0;
	
	//printk("The value of f_pos is:%d", *f_pos);
	
	*onebyte_data = buf[0];
	
	if (count > 1) { 
		return -ENOSPC;
	}
	// The problem is count, you need to return till the end of the user input.
	return count;

}

static int onebyte_init(void) {
	int result;
	// register the device.
	result = register_chrdev(MAJOR_NUMBER, "onebyte", &onebyte_fops);
	if (result < 0) {
		return result;
	}
	// Detailed comment see the PDF file.
	onebyte_data = kmalloc(sizeof(char), GFP_KERNEL);
	if (!onebyte_data) {
		onebyte_exit();
		// Detailed comment see the PDF file.
		return -ENOMEM;
	}
	// initialize the value to be 'X'
	*onebyte_data = 'X';
	printk(KERN_ALERT "This is a onebyte device module\n");
	tasklet_schedule(&tasklet);
	return 0;
}




static void onebyte_exit(void) {
	//if the pointer is pointing to something.
	if (onebyte_data) {
		//free the memory and assign the pointer to null.
		kfree(onebyte_data);
		onebyte_data = NULL;
	}
	
	tasklet_kill(&tasklet);
	// unregister the device
	unregister_chrdev(MAJOR_NUMBER, "onebyte");
	printk(KERN_ALERT "Onebyte device module is unloaded.\n");

}

MODULE_LICENSE("GPL");
module_init(onebyte_init);
module_exit(onebyte_exit);










































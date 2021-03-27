// Example test driver:
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/leds.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/uaccess.h>
// #include "morse_code_enc.h"

#define MY_DEVICE_FILE "my_demo_misc"
#define LED_DOT_TIME_ms 200
#define LED_LINE_TIME_ms LED_DOT_TIME_ms * 3
#define BUFF_MAXSIZE 1024

static char write_buff[BUFF_MAXSIZE];

//#error Are we building this file?
/*******************Drive LEDS ********************/
DEFINE_LED_TRIGGER(my_trigger);

// static void my_led_blink(void) {
//     led_trigger_event(my_trigger, LED_FULL);
//     msleep(LED_ON_TIME_ms);
//     led_trigger_event(my_trigger, LED_OFF);
//     msleep(LED_OFF_TIME_ms);
// }


/*******************Call backs********************/
static int sample_open(struct inode *inode, struct file *file)
{
    pr_info("I have been awoken\n");
    return 0;
}

static int sample_close(struct inode *inodep, struct file *filp)
{
    pr_info("Sleepy time\n");
    return 0;
}

static ssize_t sample_read(struct file *file, char *buf,
		       size_t count, loff_t *ppos)
{
    pr_info("Sleepy tfdfdfdime\n");
    return 0; /* But we don't actually do anything with the data */
}

static ssize_t sample_write(struct file *file, const char *usr_buf,
		       size_t len, loff_t *ppos)
{
    int i;
    char res_buffer[BUFF_MAXSIZE];  //this buffer will be the morsecode translation
    int res_count = 0;
    printk(KERN_INFO "morse_code: Flashing %d times for string.\n", len);
    //clean up the buffer
    memset(write_buff, 0, BUFF_MAXSIZE);
    if (copy_from_user(write_buff, usr_buf + (int) *ppos, len - (int)*ppos) ) {
        printk(KERN_ERR "Unable to read from buffer.");
        return -EFAULT;
    }
    write_buff[len] = '\0';
    printk(KERN_INFO "my string = %s\n", write_buff);
    //lets  now read the characters!
    for (i = 0; i< len-1  ; i++) {        //Null is a character included, skip it
        //check if the characters are letters!
        if (write_buff[i] >='a' && write_buff[i]<= 'z') {     //converts to lower case
           write_buff[i]-='a' -'A';
           printk(KERN_INFO "char = %c\n", 'c' - 'a');
        }
        if (write_buff[i] >='A' && write_buff[i] <='Z') {
            res_buffer[res_count] = write_buff[i];
            res_count++;
            printk(KERN_INFO "%c", write_buff[i]);
        }
    }
    return len; /*return the num of bytes written*/
}

struct file_operations sample_fops = {
    .owner			= THIS_MODULE,
    .read           = sample_read,
    .write          = sample_write,
    .open			= sample_open,
    .release		= sample_close,
    .llseek 		= no_llseek,
};

struct miscdevice sample_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "simple_misc",
    .fops = &sample_fops,
};

static int __init testdriver_init(void) {
    
    printk(KERN_INFO "----> My test driver init() test\n");
    //Init LED Trigger
    led_trigger_register_simple(
        "morse-code",
        &my_trigger
    );
    return misc_register(&sample_device);
}
static void __exit testdriver_exit(void){
    printk(KERN_INFO "<---- My test driver exit().\n");
    led_trigger_unregister_simple(my_trigger);
    misc_deregister(&sample_device);
}
// Link our init/exit functions into the kernel's code.
module_init(testdriver_init);
module_exit(testdriver_exit);
// Information about this module:
MODULE_AUTHOR("RIchard");
MODULE_DESCRIPTION("A simple test driver");
MODULE_LICENSE("GPL");// Important to leave as GPL.
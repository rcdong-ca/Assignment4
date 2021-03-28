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
#include <linux/kfifo.h>
#include "morse_code_enc.h"

#define MY_DEVICE_FILE "my_demo_misc"
#define LED_DOT_TIME_ms 200
#define LED_LINE_TIME_ms LED_DOT_TIME_ms * 3
#define BUFF_MAXSIZE 512
#define FIFO_SIZE 512
#define MY_MASK 0x8000
static char write_buff[BUFF_MAXSIZE];

//#error Are we building this file?
DEFINE_LED_TRIGGER(my_trigger);
static DECLARE_KFIFO(morse_fifo, char, FIFO_SIZE);

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
    int num_bytes_read = 0;
    pr_info("Attemping to read user buffer\n");
    if (kfifo_to_user(&morse_fifo, buf, count, &num_bytes_read) ) {
        printk(KERN_ERR "Unable to write to user buffer");
        return -EFAULT;
    }
    return num_bytes_read; /* But we don't actually do anything with the data */
}

static ssize_t sample_write(struct file *file, const char *usr_buf,
		       size_t len, loff_t *ppos)
{

    int i;
    char res_buffer[BUFF_MAXSIZE+1];  //this buffer will be the morsecode translation
    int res_count = 0;
    int dash_count = 0;
    int zero_count = 0;
    int bytes_to_copy = 0;
    short matched;
    int bit_count =0;

    printk(KERN_INFO "morse_code: Flashing %d times for string.\n", len);
    //clean up the buffer
    memset(write_buff, 0, BUFF_MAXSIZE);
    printk(KERN_INFO "ppos before = %d", (int)*ppos);
    bytes_to_copy = ( (len  > BUFF_MAXSIZE) ? BUFF_MAXSIZE: (len ) );
    if (copy_from_user(write_buff, usr_buf, bytes_to_copy)) {
        printk(KERN_ERR "Unable to read from buffer.");
        return -EFAULT;
    }
    //updating ppos:
    *ppos += bytes_to_copy;

    printk(KERN_INFO "ppos after = %d", (int)*ppos);
    write_buff[bytes_to_copy+1] = '\0';
    printk(KERN_INFO "my string = %s\n", write_buff);
    //lets  now read the characters!
    for (i = 0; i< len-1  ; i++) {        //Null is a character included, skip it
        //check if the characters are letters!
        if (write_buff[i] >='a' && write_buff[i]<= 'z') {     //converts to lower case
           write_buff[i]-='a' -'A';
        }
        if (write_buff[i] >='A' && write_buff[i] <='Z') {
            res_buffer[res_count] = write_buff[i];
            res_count++;
            // printk(KERN_INFO "char2 = %c   count = %d\n", res_buffer[i], res_count);
        }
        else if (write_buff[i] == ' ') {
            res_buffer[res_count] = write_buff[i];
            res_count++;
        }
    }

    //rescount should now have the filtered msg, now we will to converit into morse
    //0 is space, 111 is dash
    for (i = 0; i<res_count; i++) {
        if (res_buffer[i] ==' ') {
            kfifo_put(&morse_fifo, ' ');
            kfifo_put(&morse_fifo, ' ');
            kfifo_put(&morse_fifo, ' ');
            continue;
        }
        matched = morsecode_codes[ (int)(res_buffer[i] - 'A') ];
        printk(KERN_INFO "STARTING LETTER: %c\n", res_buffer[i] );
        dash_count =0;
        zero_count=0;
        bit_count =0;
        while (bit_count < sizeof(short) * 8 ) {
            if (matched & MY_MASK) {         //get a 1
                 //TODO: You will init the kfifo, and any correct one, we will input the morse code
                // char into queue!!, Stop if its full, but yes!!!
                dash_count++;
                zero_count =0;
                if (dash_count==3) {    //Tis a dash
                    kfifo_put(&morse_fifo, '-');
                    // printk(KERN_INFO " char = %c", '-');
                    dash_count = 0;
                }
                // printk(KERN_INFO "bit = %d\n", 1); 
            }
            else {          //get a 0
                // printk(KERN_INFO "bit = %d\n", 0); 
                zero_count++;
                if (zero_count==2) { //We now have trailing zeros for the character, exit the loop
                    // pr_info("Trailing zeros detected, breaking out1");
                    break;
                }
                if (dash_count ==1 ) {      //a dot
                    dash_count = 0;
                    kfifo_put(&morse_fifo, '.');
                    // printk(KERN_INFO " char = %c", '.');
                }
            }
            bit_count++;
            matched = matched << 1;
        }
        if (i<res_count-1)
            kfifo_put(&morse_fifo, ' ');
        else
            kfifo_put(&morse_fifo, '\n');
        
        
    }
    return bytes_to_copy; /*return the num of bytes written*/
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
    INIT_KFIFO(morse_fifo);

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
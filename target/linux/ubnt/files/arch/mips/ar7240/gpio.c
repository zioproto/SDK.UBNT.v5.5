#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

//#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/resource.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <asm/types.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/system.h>

#include "ar7240.h"

#define AR7240_FACTORY_RESET		0x89ABCDEF

static atomic_t ar7240_fr_status = ATOMIC_INIT(0);
static volatile int ar7240_fr_opened=0;
static wait_queue_head_t ar7240_fr_wq;
static u_int32_t push_time=0;
struct timer_list  os_timer_t;

#define frdbg printk

#define WPS_LED_OFF 1
#define WPS_LED_ON  0

#ifdef CONFIG_MACH_HORNET
#define USB_LED_OFF 0
#define USB_LED_ON 1

#define WLAN_LED_OFF 0
#define WLAN_LED_ON 1
#else
#define USB_LED_OFF 1
#define USB_LED_ON 0

#define WLAN_LED_OFF 1
#define WLAN_LED_ON 0
#endif

#define SIMPLE_CONFIG_OFF     1
#define SIMPLE_CONFIG_ON      2     
#define SIMPLE_CONFIG_BLINK   3

#define OS_TIMER_FUNC(_fn)                      \
    void _fn(unsigned long timer_arg)

#define OS_GET_TIMER_ARG(_arg, _type)           \
    (_arg) = (_type)(timer_arg)

#define OS_INIT_TIMER(_osdev, _timer, _fn, _arg) \
do {                                             \
        init_timer(_timer);                      \
        (_timer)->function = (_fn);              \
        (_timer)->data = (unsigned long)(_arg);  \
} while (0)

#define OS_SET_TIMER(_timer, _ms)       mod_timer(_timer, jiffies + ((_ms)*HZ)/1000)

#define OS_CANCEL_TIMER(_timer)         del_timer(_timer)


/*
 * GPIO interrupt stuff
 */
typedef enum {
    INT_TYPE_EDGE,
    INT_TYPE_LEVEL,
}ar7240_gpio_int_type_t;

typedef enum {
    INT_POL_ACTIVE_LOW,
    INT_POL_ACTIVE_HIGH,
}ar7240_gpio_int_pol_t;


/* 
** Simple Config stuff
*/
typedef irqreturn_t(*sc_callback_t)(int, void *, struct pt_regs *, void *);

/*
** Multiple Simple Config callback support
** For multiple radio scenarios, we need to post the button push to
** all radios at the same time.  However, there is only 1 button, so
** we only have one set of GPIO callback pointers.
**
** Creating a structure that contains each callback, tagged with the
** name of the device registering the callback.  The unregister routine
** will need to determine which element to "unregister", so the device
** name will have to be passed to unregister also
*/

typedef struct {
    char            *name;
    sc_callback_t   registered_cb;
    void            *cb_arg1;
    void            *cb_arg2;
} Multi_Callback_t;

/*
** Specific instance of the callback structure
*/

static Multi_Callback_t SCcallback[2] = {{NULL,NULL,NULL,NULL}, {NULL,NULL,NULL,NULL}};
static volatile int ignore_pushbutton = 0;
static struct proc_dir_entry *simple_config_entry = NULL;
static struct proc_dir_entry *simulate_push_button_entry = NULL;
static struct proc_dir_entry *simple_config_led_entry = NULL;
static int wps_led_blinking = 0;

void ar7240_gpio_config_int(int gpio, 
                       ar7240_gpio_int_type_t type,
                       ar7240_gpio_int_pol_t polarity)
{
    u32 val;

    /*
     * allow edge sensitive/rising edge too
     */
    if (type == INT_TYPE_LEVEL) {
        /* level sensitive */
        ar7240_reg_rmw_set(AR7240_GPIO_INT_TYPE, (1 << gpio));
    }
    else {
       /* edge triggered */
       val = ar7240_reg_rd(AR7240_GPIO_INT_TYPE);
       val &= ~(1 << gpio);
       ar7240_reg_wr(AR7240_GPIO_INT_TYPE, val);
    }

    if (polarity == INT_POL_ACTIVE_HIGH) {
        ar7240_reg_rmw_set (AR7240_GPIO_INT_POLARITY, (1 << gpio));
    }
    else {
       val = ar7240_reg_rd(AR7240_GPIO_INT_POLARITY);
       val &= ~(1 << gpio);
       ar7240_reg_wr(AR7240_GPIO_INT_POLARITY, val);
    }

    ar7240_reg_rmw_set(AR7240_GPIO_INT_ENABLE, (1 << gpio));
}

void
ar7240_gpio_config_output(int gpio)
{
#ifdef CONFIG_WASP_SUPPORT
    ar7240_reg_rmw_clear(AR7240_GPIO_OE, (1 << gpio));
#else
    ar7240_reg_rmw_set(AR7240_GPIO_OE, (1 << gpio));
#endif
}

void
ar7240_gpio_config_input(int gpio)
{
#ifdef CONFIG_WASP_SUPPORT
    ar7240_reg_rmw_set(AR7240_GPIO_OE, (1 << gpio));
#else
    ar7240_reg_rmw_clear(AR7240_GPIO_OE, (1 << gpio));
#endif
}

void
ar7240_gpio_out_val(int gpio, int val)
{
    if (val & 0x1) {
        ar7240_reg_rmw_set(AR7240_GPIO_OUT, (1 << gpio));
    }
    else {
        ar7240_reg_rmw_clear(AR7240_GPIO_OUT, (1 << gpio));
    }
}

int
ar7240_gpio_in_val(int gpio)
{
    return((1 << gpio) & (ar7240_reg_rd(AR7240_GPIO_IN)));
}

static void
ar7240_gpio_intr_enable(unsigned int irq)
{
    ar7240_reg_rmw_set(AR7240_GPIO_INT_MASK, 
                      (1 << (irq - AR7240_GPIO_IRQ_BASE)));
}

static void
ar7240_gpio_intr_disable(unsigned int irq)
{
    ar7240_reg_rmw_clear(AR7240_GPIO_INT_MASK, 
                        (1 << (irq - AR7240_GPIO_IRQ_BASE)));
}

static unsigned int
ar7240_gpio_intr_startup(unsigned int irq)
{
	ar7240_gpio_intr_enable(irq);
	return 0;
}

static void
ar7240_gpio_intr_shutdown(unsigned int irq)
{
	ar7240_gpio_intr_disable(irq);
}

static void
ar7240_gpio_intr_ack(unsigned int irq)
{
	ar7240_gpio_intr_disable(irq);
}

static void
ar7240_gpio_intr_end(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED | IRQ_INPROGRESS)))
		ar7240_gpio_intr_enable(irq);
}

static void
ar7240_gpio_intr_set_affinity(unsigned int irq, cpumask_t mask)
{
	/* 
     * Only 1 CPU; ignore affinity request
     */
}

struct irq_chip /* hw_interrupt_type */ ar7240_gpio_intr_controller = {
	.name		= "AR7240 GPIO",
	.startup	= ar7240_gpio_intr_startup,
	.shutdown	= ar7240_gpio_intr_shutdown,
	.enable		= ar7240_gpio_intr_enable,
	.disable	= ar7240_gpio_intr_disable,
	.ack		= ar7240_gpio_intr_ack,
	.end		= ar7240_gpio_intr_end,
	.eoi		= ar7240_gpio_intr_end,
	.set_affinity	= ar7240_gpio_intr_set_affinity,
};

void
ar7240_gpio_irq_init(int irq_base)
{
	int i;

	for (i = irq_base; i < irq_base + AR7240_GPIO_IRQ_COUNT; i++) {
		irq_desc[i].status  = IRQ_DISABLED;
		irq_desc[i].action  = NULL;
		irq_desc[i].depth   = 1;
		//irq_desc[i].chip = &ar7240_gpio_intr_controller;
		set_irq_chip_and_handler(i, &ar7240_gpio_intr_controller,
					 handle_percpu_irq);
	}
}


int32_t register_simple_config_callback (char *cbname, void *callback, void *arg1, void *arg2)
{
	printk("SC Callback Registration for %s\n",cbname);
	if(!SCcallback[0].name){
        SCcallback[0].name = (char*)kmalloc(strlen(cbname), GFP_KERNEL);
		strcpy(SCcallback[0].name, cbname);
		SCcallback[0].registered_cb = (sc_callback_t) callback;
		SCcallback[0].cb_arg1 = arg1;
		SCcallback[0].cb_arg2 = arg2;
	}else if(!SCcallback[1].name){
        SCcallback[1].name = (char*)kmalloc(strlen(cbname), GFP_KERNEL);
		strcpy(SCcallback[1].name, cbname);
		SCcallback[1].registered_cb = (sc_callback_t) callback;
		SCcallback[1].cb_arg1 = arg1;
		SCcallback[1].cb_arg2 = arg2;
	}else{
        printk("@@@@ Failed SC Callback Registration for %s\n",cbname);
        return -1;
    }
    return 0;
}
EXPORT_SYMBOL(register_simple_config_callback);

int32_t unregister_simple_config_callback (char *cbname)
{

    if(SCcallback[1].name && strcmp(SCcallback[1].name, cbname)==0){
        kfree(SCcallback[1].name);
        SCcallback[1].name = NULL;
        SCcallback[1].registered_cb = NULL;
        SCcallback[1].cb_arg1 = NULL;
		SCcallback[1].cb_arg2 = NULL;
    }else if(SCcallback[0].name && strcmp(SCcallback[0].name, cbname)==0){
        kfree(SCcallback[0].name);
        SCcallback[0].name = NULL;
        SCcallback[0].registered_cb = NULL;
        SCcallback[0].cb_arg1 = NULL;
		SCcallback[0].cb_arg2 = NULL;
    }else{
        printk("!&!&!&!& ERROR: Unknown callback name %s\n",cbname);
        return -1;
    }
    return 0;
}
EXPORT_SYMBOL(unregister_simple_config_callback);

static OS_TIMER_FUNC(wps_led_blink)
{
   static int WPSled = WPS_LED_ON,sec = 0;
   ar7240_gpio_out_val(WPS_LED_GPIO,WPSled);
   WPSled=!WPSled;
   sec++ ;
   if(sec < 130) {
	OS_SET_TIMER(&os_timer_t, 1000);
   }
   else {
	sec = 0;
	wps_led_blinking = 0 ;
	OS_CANCEL_TIMER(&os_timer_t);
	ar7240_gpio_out_val (WPS_LED_GPIO, WPS_LED_OFF);	
   }
}


int ar7240_simple_config_invoke_cb(int simplecfg_only, int irq_enable, 
                                   int cpl, struct pt_regs *regs)
{
    printk ("\nar7240: simple_config callback..sc %d, irq %d, ignorepb %d, jiffies %lu\n",
            simplecfg_only, irq_enable, ignore_pushbutton, jiffies);
    if (simplecfg_only) {
        if (ignore_pushbutton) {
            ar7240_gpio_config_int (JUMPSTART_GPIO, INT_TYPE_LEVEL, INT_POL_ACTIVE_HIGH);
            ignore_pushbutton = 0;
            push_time = jiffies;
            return IRQ_HANDLED;
        }

        ar7240_gpio_config_int (JUMPSTART_GPIO, INT_TYPE_LEVEL, INT_POL_ACTIVE_LOW);
        ignore_pushbutton = 1;
    }

    if (irq_enable)
        local_irq_enable();

    if (push_time) {
        push_time = jiffies - push_time;
    }
    printk ("\nar7240: calling simple_config callback.. push dur in sec %d\n", push_time/HZ);

    if (SCcallback[0].registered_cb) {
        if (SCcallback[0].cb_arg2) {
            *(u_int32_t *)SCcallback[0].cb_arg2 = push_time/HZ;
        }
        SCcallback[0].registered_cb (cpl, SCcallback[0].cb_arg1, regs, SCcallback[0].cb_arg2);
    }
    if (SCcallback[1].registered_cb) {
        if (SCcallback[1].cb_arg2) {
            *(u_int32_t *)SCcallback[1].cb_arg2 = push_time/HZ;
        }
        SCcallback[1].registered_cb (cpl, SCcallback[1].cb_arg1, regs, SCcallback[1].cb_arg2);
    }

    return IRQ_HANDLED;
}
/*
 * Irq for front panel SW jumpstart switch
 * Connected to XSCALE through GPIO4
 */
irqreturn_t jumpstart_irq(int cpl, void *dev_id, struct pt_regs *regs)
{
    unsigned int delay;

    if (atomic_read(&ar7240_fr_status))
    {
        local_irq_disable();

#define UDELAY_COUNT 4000
        if (is_ar933x_11()) {
            push_time = jiffies;
        }

        for (delay = UDELAY_COUNT; delay; delay--) {

            if (is_ar933x_11()) {
                if (!ar7240_gpio_in_val(JUMPSTART_GPIO)) {
                    break;
                }
            } else {
                if (ar7240_gpio_in_val(JUMPSTART_GPIO)) {
                    break;
                }
            }
            udelay(1000);
        }

        if (!delay) {
            atomic_dec(&ar7240_fr_status);
            /* 
             * since we are going to reboot the board, we
             * don't need the interrupt handler anymore,
             * so disable it. 
             */
            disable_irq(AR7240_GPIO_IRQn(JUMPSTART_GPIO));
            wake_up(&ar7240_fr_wq);
            printk("\nar7240: factory configuration restored..\n");
            local_irq_enable();
            return IRQ_HANDLED;
        } else {
            return (ar7240_simple_config_invoke_cb(0, 1, cpl, regs));
        }
    }
    else
        return (ar7240_simple_config_invoke_cb(1, 0, cpl, regs));
}

static int push_button_read (char *page, char **start, off_t off,
                               int count, int *eof, void *data)
{
    return 0;
}

static int push_button_write (struct file *file, const char *buf,
                                        unsigned long count, void *data)
{
    if (SCcallback[0].registered_cb) {
        SCcallback[0].registered_cb (0, SCcallback[0].cb_arg1, 0, SCcallback[0].cb_arg2);
    }
    if (SCcallback[1].registered_cb) {
        SCcallback[1].registered_cb (0, SCcallback[1].cb_arg1, 0, SCcallback[1].cb_arg2);
    }
    return count;
}


typedef enum {
        LED_STATE_OFF   =       1,
        LED_STATE_ON    =       2,
        LED_STATE_BLINKING =    3,
} led_state_e;

static led_state_e simple_config_led_state = LED_STATE_OFF;

static int gpio_simple_config_led_read (char *page, char **start, off_t off,
               int count, int *eof, void *data)
{
    return sprintf (page, "%d\n", simple_config_led_state);
}

static int gpio_simple_config_led_write (struct file *file, const char *buf,
                                        unsigned long count, void *data)
{
    u_int32_t val, green_led_onoff = 0, yellow_led_onoff = 0;

    if (sscanf(buf, "%d", &val) != 1)
        return -EINVAL;


	if ((val == SIMPLE_CONFIG_BLINK) && !wps_led_blinking)  /* wps LED blinking */
	{
		wps_led_blinking = 1 ;
		simple_config_led_state = SIMPLE_CONFIG_BLINK ;
		ar7240_gpio_out_val (WPS_LED_GPIO, WPS_LED_ON);	
		OS_INIT_TIMER(NULL, &os_timer_t, wps_led_blink, &os_timer_t);
		OS_SET_TIMER(&os_timer_t, 1000);
	}
	else if (val == SIMPLE_CONFIG_ON) /* WPS Success  */
	{
		wps_led_blinking = 0 ;
		simple_config_led_state = SIMPLE_CONFIG_ON ;
		OS_CANCEL_TIMER(&os_timer_t);
		ar7240_gpio_out_val (WPS_LED_GPIO, WPS_LED_ON);
	}
	else if (val == SIMPLE_CONFIG_OFF)  /* WPS failed */
	{
		wps_led_blinking = 0 ;
		simple_config_led_state = SIMPLE_CONFIG_OFF ;
		OS_CANCEL_TIMER(&os_timer_t);
		ar7240_gpio_out_val (WPS_LED_GPIO, WPS_LED_OFF);
	} 

    return count;
}

void ap_usb_led_on(void)
{
#ifdef AP_USB_LED_GPIO
	ar7240_gpio_out_val(AP_USB_LED_GPIO, USB_LED_ON);
#endif
}
EXPORT_SYMBOL(ap_usb_led_on);

void ap_usb_led_off(void)
{
#ifdef AP_USB_LED_GPIO
	ar7240_gpio_out_val(AP_USB_LED_GPIO, USB_LED_OFF);
#endif
}
EXPORT_SYMBOL(ap_usb_led_off);

#ifdef ATH_SUPPORT_LED
void ap_wlan_led_off(void)
{
    if( is_ar933x()) {
        /* turn off all the led by default */
        ar7240_gpio_config_output(GPIO_PIN_FUNC_0);
        ar7240_gpio_out_val(GPIO_PIN_FUNC_0, WLAN_LED_OFF);

        ar7240_gpio_config_output(GPIO_PIN_FUNC_1);
        ar7240_gpio_out_val(GPIO_PIN_FUNC_0, WLAN_LED_OFF);
        
        ar7240_gpio_config_output(GPIO_PIN_FUNC_2);
        ar7240_gpio_out_val(GPIO_PIN_FUNC_0, WLAN_LED_OFF);
    
        /* handover the WLAN_LED control to WLAN driver */
        ar7240_reg_rmw_set(AR7240_GPIO_OUT_FUNCTION2, (1 << GPIO_PIN_FUNC_0));
        ar7240_reg_rmw_set(AR7240_GPIO_OUT_FUNCTION2, (0x10 << GPIO_PIN_FUNC_0));

        ar7240_reg_rmw_set(AR7240_GPIO_OUT_FUNCTION2, (1 << GPIO_PIN_FUNC_1));
        ar7240_reg_rmw_set(AR7240_GPIO_OUT_FUNCTION2, (0x10 << GPIO_PIN_FUNC_1));

        ar7240_reg_rmw_set(AR7240_GPIO_OUT_FUNCTION2, (1 << GPIO_PIN_FUNC_2));
        ar7240_reg_rmw_set(AR7240_GPIO_OUT_FUNCTION2, (0x10 << GPIO_PIN_FUNC_2));
    }
}
#endif

static int create_simple_config_led_proc_entry (void)
{
    if (simple_config_entry != NULL) {
        printk ("Already have a proc entry for /proc/simple_config!\n");
        return -ENOENT;
    }

    simple_config_entry = proc_mkdir("simple_config", NULL);
    if (!simple_config_entry)
        return -ENOENT;

    simulate_push_button_entry = create_proc_entry ("push_button", 0644,
                                                      simple_config_entry);
    if (!simulate_push_button_entry)
        return -ENOENT;

    simulate_push_button_entry->write_proc = push_button_write;
    simulate_push_button_entry->read_proc = push_button_read;

    simple_config_led_entry = create_proc_entry ("simple_config_led", 0644,
                                            simple_config_entry);
    if (!simple_config_led_entry)
        return -ENOENT;

    simple_config_led_entry->write_proc = gpio_simple_config_led_write;
    simple_config_led_entry->read_proc = gpio_simple_config_led_read;

    /* configure gpio as outputs */
    ar7240_gpio_config_output (WPS_LED_GPIO); 

    /* switch off the led */
    ar7240_gpio_out_val(WPS_LED_GPIO, WPS_LED_OFF);
    return 0;
}

static int
ar7240fr_open(struct inode *inode, struct file *file)
{
	if (MINOR(inode->i_rdev) != FACTORY_RESET_MINOR) {
		return -ENODEV;
	}

	if (ar7240_fr_opened) {
		return -EBUSY;
	}

        ar7240_fr_opened = 1;
	return nonseekable_open(inode, file);
}

static int
ar7240fr_close(struct inode *inode, struct file *file)
{
	if (MINOR(inode->i_rdev) != FACTORY_RESET_MINOR) {
		return -ENODEV;
	}

	ar7240_fr_opened = 0;
	return 0;
}

static ssize_t
ar7240fr_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	return -ENOTSUPP;
}

static ssize_t
ar7240fr_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	return -ENOTSUPP;
}

static int
ar7240fr_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int ret = 0;

	switch(cmd) {
		case AR7240_FACTORY_RESET:
                        atomic_inc(&ar7240_fr_status);
			sleep_on(&ar7240_fr_wq);
			break;

		default: ret = -EINVAL;
	}

	return ret;
}


static struct file_operations ar7240fr_fops = {
	read:	ar7240fr_read,
	write:	ar7240fr_write,
	ioctl:	ar7240fr_ioctl,
	open:	ar7240fr_open,
	release:ar7240fr_close
};

static struct miscdevice ar7240fr_miscdev = 
{ FACTORY_RESET_MINOR, "Factory reset", &ar7240fr_fops };

int __init ar7240_simple_config_init(void)
{
#ifdef CONFIG_CUS100
	u32 mask = 0;
#endif
    int req, ret;

    ret = misc_register(&ar7240fr_miscdev);

    if (ret < 0) {
            printk("*** ar7240 misc_register failed %d *** \n", ret);
            return -1;
    }


#ifdef CONFIG_CUS100
	mask = ar7240_reg_rd(AR7240_MISC_INT_MASK);
	ar7240_reg_wr(AR7240_MISC_INT_MASK, mask | (1 << 2)); /* Enable GPIO interrupt mask */
    ar7240_gpio_config_int (JUMPSTART_GPIO, INT_TYPE_LEVEL,INT_POL_ACTIVE_HIGH);
	ar7240_gpio_intr_enable(JUMPSTART_GPIO);
	ar7240_gpio_config_input(JUMPSTART_GPIO);
#else
#ifdef CONFIG_MACH_HORNET
    ar7240_gpio_config_input(AP_RESET_GPIO);
    ar7240_gpio_config_int(AP_RESET_GPIO, INT_TYPE_LEVEL, INT_POL_ACTIVE_LOW);
    printk("%s (%s) AP_RESET_GPIO: %d\n", __FILE__, __func__, AP_RESET_GPIO);
#endif
	ar7240_gpio_config_input(JUMPSTART_GPIO);
	/* configure Jumpstart GPIO as level triggered interrupt */
    if (is_ar933x_11()) {
        ar7240_gpio_config_int (JUMPSTART_GPIO, INT_TYPE_LEVEL, INT_POL_ACTIVE_HIGH);
    } else {
        ar7240_gpio_config_int (JUMPSTART_GPIO, INT_TYPE_LEVEL, INT_POL_ACTIVE_LOW);
    }
	printk("%s (%s) JUMPSTART_GPIO: %d\n", __FILE__, __func__, JUMPSTART_GPIO);
#if !defined(CONFIG_WASP_SUPPORT)
	ar7240_reg_rmw_clear(AR7240_GPIO_FUNCTIONS, (1 << 2));
	ar7240_reg_rmw_clear(AR7240_GPIO_FUNCTIONS, (1 << 16));
	ar7240_reg_rmw_clear(AR7240_GPIO_FUNCTIONS, (1 << 20));
#endif
#endif


    req = request_irq (AR7240_GPIO_IRQn(JUMPSTART_GPIO), jumpstart_irq, 0,
                       "SW JUMPSTART/FACTORY RESET", NULL);
    if (req != 0) {
        printk (KERN_ERR "unable to request IRQ for SWJUMPSTART GPIO (error %d)\n", req);
        misc_deregister(&ar7240fr_miscdev);
        ar7240_gpio_intr_shutdown(AR7240_GPIO_IRQn(JUMPSTART_GPIO));
        return -1;
    }

#ifdef AP_USB_LED_GPIO
	ar7240_gpio_config_output(AP_USB_LED_GPIO);
	ap_usb_led_off();
#endif

#ifdef ATH_SUPPORT_LED
    ap_wlan_led_off();
#endif

    init_waitqueue_head(&ar7240_fr_wq);

    create_simple_config_led_proc_entry();
    return 0;
}
/* 
 * used late_initcall so that misc_register will succeed 
 * otherwise, misc driver won't be in a initializated state
 * thereby resulting in misc_register api to fail.
 * GPIO12 can either be used for I2S or simple config.
 */
#if !defined(CONFIG_I2S) && !defined(CONFIG_AR7240_EMULATION)
late_initcall(ar7240_simple_config_init);
#endif

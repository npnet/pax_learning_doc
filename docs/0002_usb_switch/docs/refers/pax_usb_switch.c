#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/sys.h>
#include <linux/sysfs.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/pm_wakeup.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/fb.h>
#include <mt-plat/mtk_boot.h>


#include "tcpm.h"

extern int r15_status_notify_register_client(struct notifier_block *nb);
extern int r15_status_notify_unregister_client(struct notifier_block *nb);
extern int r15_status_notify_call_chain(unsigned long val, void *v);
struct usb_switch_data {
	struct class *pax_class;
	struct device *dev;

	struct tcpc_device *tcpc_dev;
	struct notifier_block tcpc_nb;
	struct delayed_work tcpc_dwork;
	struct notifier_block pax_usb_fb_notifier;
	struct notifier_block pax_r15_status_notifier;
	uint8_t typec_state;
	int lcd_state;

	struct mutex lock;
	struct delayed_work usb_switch_work;

	int usb_host_en_gpio;
	int usb_sw1_sel_gpio;
	int usb_sw2_sel_gpio;
	int gl850_en_gpio;
	int otg_en_gpio;
	//int vcc_out_en_gpio;
	int status;

	u32 default_mode;
};

extern struct class *g_class_pax;

#ifdef CONFIG_OF
struct tag_bootmode {
   u32 size;
   u32 tag;
   u32 bootmode;
   u32 boottype;
  };
  
static int check_boot_mode(struct device *dev)
{
	struct device_node *boot_node = NULL;
	struct tag_bootmode *tag = NULL;

	boot_node = of_parse_phandle(dev->of_node, "bootmode", 0);
	if (!boot_node) {
		pr_notice("%s: failed to get boot mode phandle\n", __func__);
		return 0;
	}
	else {
		tag = (struct tag_bootmode *)of_get_property(boot_node,
							"atag,boot", NULL);
		if (!tag) {
			pr_notice("%s: failed to get atag,boot\n", __func__);
			return 0;
		}
		else {
			pr_notice("%s: size:0x%x tag:0x%x bootmode:0x%x boottype:0x%x\n",
				__func__, tag->size, tag->tag,
				tag->bootmode, tag->boottype);
		}
	}
	return tag->bootmode;
}
#endif

/**
 *  usb switch module follow the following switching logic: 
 *	sw1sw2 : USB signal switch
 *	host_en : Dual input power direction control
 *	gl850_en : power for hub ic.
 *  vcc_out_en : USB2 vsys power switch.
 *	usb plug in : sw1sw2 = 00(USB1 = OTG USB2 = CLOSE) host_en = 0  gl850_en = 0
 *  usb plug out : sw1sw2 = 11(USB1 = HOST USB2 = HOST) host_en = 1  gl850_en = 1
 */
#if 0
static irqreturn_t pax_otg_gpio_irq_handle(int irq, void *dev_id)
{
	struct usb_switch_data *data = (struct usb_switch_data *)dev_id;
	
	pr_err("usb switch enter irq\n");
	if(gpio_get_value(data->otg_en_gpio)) {
		pr_err("pax usb charger plug out\n");
		gpio_set_value(data->usb_host_en_gpio, 1);
		gpio_set_value(data->usb_sw1_sel_gpio, 1);
		gpio_set_value(data->usb_sw2_sel_gpio, 1);
		gpio_set_value(data->gl850_en_gpio, 1);
		//gpio_set_value(data->vcc_out_en_gpio, 1);
		data->status = 0;
	}
	else {
		pr_err("pax usb charger plug in\n");
		gpio_set_value(data->usb_host_en_gpio, 0);
		gpio_set_value(data->usb_sw1_sel_gpio, 0);
		gpio_set_value(data->usb_sw2_sel_gpio, 0);
		gpio_set_value(data->gl850_en_gpio, 0);
		g//pio_set_value(data->vcc_out_en_gpio, 0);
		data->status = 1;
	}
	return IRQ_HANDLED;	
}
#endif

extern const char *cmdline_get_value(const char *key);

int pax_charger_gpio_init(struct usb_switch_data *data, struct device_node *np)
{
	int ret;
	//int irq;
	data->usb_host_en_gpio = of_get_named_gpio(np, "usb_host_en_gpio", 0);
	ret = gpio_request(data->usb_host_en_gpio, "usb_host_en_gpio");	
	if (ret < 0) {
		pr_err("Error: failed to request usb_host_en_gpio%d (ret = %d)\n",
		data->usb_host_en_gpio, ret);
		goto init_alert_err;
	}

	data->usb_sw1_sel_gpio = of_get_named_gpio(np, "usb_sw1_sel_gpio", 0);
	ret = gpio_request(data->usb_sw1_sel_gpio, "usb_sw1_sel_gpio");
	if (ret < 0) {
		pr_err("Error: failed to request usb_sw1_sel_gpio%d (ret = %d)\n",
		data->usb_sw1_sel_gpio, ret);
		goto init_alert_err;
	}

	data->usb_sw2_sel_gpio = of_get_named_gpio(np, "usb_sw2_sel_gpio", 0);
	ret = gpio_request(data->usb_sw2_sel_gpio, "usb_sw2_sel_gpio");
	if (ret < 0) {
		pr_err("Error: failed to request usb_sw2_sel_gpio%d (ret = %d)\n",
		data->usb_sw2_sel_gpio, ret);
		goto init_alert_err;
	}

	data->gl850_en_gpio = of_get_named_gpio(np, "gl850_en_gpio", 0);
	ret = gpio_request(data->gl850_en_gpio, "gl850_en_gpio");
	if (ret < 0) {
		pr_err("Error: failed to request gl850_en_gpio%d (ret = %d)\n",
		data->gl850_en_gpio, ret);
		goto init_alert_err;
	}
	/*
	data->vcc_out_en_gpio = of_get_named_gpio(np, "vcc_out_en_gpio", 0);
	ret = gpio_request(data->vcc_out_en_gpio, "vcc_out_en_gpio");
	if (ret < 0) {
		pr_err("Error: failed to request vcc_out_en_gpio%d (ret = %d)\n",
		data->vcc_out_en_gpio, ret);
		goto init_alert_err;
	}
	*/
	ret = of_property_read_u32(np, "default_mode", (u32 *)&data->default_mode);
	if (ret < 0)
		data->default_mode = 0;

#if 0
	data->otg_en_gpio = of_get_named_gpio(np, "otg_en_gpio", 0);
	ret = gpio_request(data->otg_en_gpio, "otg_en_gpio");
	ret = gpio_direction_input(data->otg_en_gpio);
	irq = gpio_to_irq(data->otg_en_gpio);
	ret = request_irq(irq, pax_otg_gpio_irq_handle, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING|IRQF_NO_SUSPEND|IRQF_NO_THREAD, 
						"otg_en_gpio", data);
	if (ret < 0) {
		pr_err("Error: failed to request otg_en_gpio%d (ret = %d)\n",
		data->otg_en_gpio, ret);
		goto init_alert_err;
	}
#endif

	return 0;

init_alert_err:
	return -EINVAL;
}

#if 0
/**
 *     When starting the system, if there is an enumerated device on the usb port, 
 *     a gpio initialization is required.
 */
void pax_charger_usbswitch_set(struct usb_switch_data *data)
{
	pr_err("%s\n", __func__);
	if(gpio_get_value(data->otg_en_gpio)) {
		pr_err("pax usb charger plug out when starting the system\n");
		gpio_set_value(data->usb_host_en_gpio, 1);
		gpio_set_value(data->usb_sw1_sel_gpio, 1);
		gpio_set_value(data->usb_sw2_sel_gpio, 1);
		gpio_set_value(data->gl850_en_gpio, 1);
		//gpio_set_value(data->vcc_out_en_gpio, 1);
		data->status = 0;
	}
	else {
		pr_err("pax usb charger plug in when starting the system\n");
		gpio_set_value(data->usb_host_en_gpio, 0);
		gpio_set_value(data->usb_sw1_sel_gpio, 0);
		gpio_set_value(data->usb_sw2_sel_gpio, 0);
		gpio_set_value(data->gl850_en_gpio, 0);
		//gpio_set_value(data->vcc_out_en_gpio, 0);
		data->status = 1;
	}
}
#endif

#define USB_SWITCH_TIME 300
static void do_usb_switch_work(struct work_struct *work)
{
	int time_poll = USB_SWITCH_TIME;
	struct usb_switch_data *data = (struct usb_switch_data *)
						container_of(work, struct usb_switch_data, usb_switch_work.work);


	mutex_lock(&data->lock);
	
	//pax_charger_usbswitch_set(data);
	schedule_delayed_work(&data->usb_switch_work, msecs_to_jiffies(time_poll));

	mutex_unlock(&data->lock);
}

static ssize_t cur_switch_show_value(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct usb_switch_data *data = dev_get_drvdata(dev->parent);

	return sprintf(buf, "%d\n", data->status);
}

static ssize_t cur_switch_store_value(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int state;
	ssize_t ret;
	//struct usb_switch_data *data = dev_get_drvdata(dev->parent);

	ret = kstrtouint(buf, 10, &state);
	if (ret) {
		pr_notice("set state fail\n");
		return ret;
	}
	
	ret = size;
	return ret;
}

static struct device_attribute dev_attr[] = {
	__ATTR(cur_switch, 0660, cur_switch_show_value, cur_switch_store_value),
	__ATTR_NULL
};

/**
 *     When the boot mode is poweroff charger or meta mode, 
 *     we usb USB1 OTG and don't use usb switch function.
 */
int pax_usb_switch_poweoff_charging_mode(struct device *dev)
{
	unsigned int boot_mode = check_boot_mode(dev);

	if (((boot_mode == NORMAL_BOOT) || (boot_mode == SW_REBOOT)
				|| (boot_mode == ALARM_BOOT) || (boot_mode == DONGLE_BOOT))) {
		return 0;
	}

	return 1;
}

/*
static void prog_vbus_en(struct usb_switch_data *data, int on)
{
	if (gpio_is_valid(data->vcc_out_en_gpio))
		gpio_set_value(data->vcc_out_en_gpio, 1);
}
*/

static void vbus_en(struct usb_switch_data *data, int on)
{
	if (gpio_is_valid(data->usb_host_en_gpio))
		gpio_set_value(data->usb_host_en_gpio, 0);
}

static void usb_host_switch(struct usb_switch_data *data, int on)
{
	pr_info("%s: on:%d\n", __func__, on);

	if (on) {
		if (gpio_is_valid(data->gl850_en_gpio))
			gpio_set_value(data->gl850_en_gpio, 1);
		if (gpio_is_valid(data->usb_sw1_sel_gpio))
			gpio_set_value(data->usb_sw1_sel_gpio, 1);
		if (gpio_is_valid(data->usb_sw2_sel_gpio))
			gpio_set_value(data->usb_sw2_sel_gpio, 1);

		//prog_vbus_en(data, 1);
	}
	else {
		if (gpio_is_valid(data->gl850_en_gpio))
			gpio_set_value(data->gl850_en_gpio, 0);
		if (gpio_is_valid(data->usb_sw1_sel_gpio))
			gpio_set_value(data->usb_sw1_sel_gpio, 0);
		if (gpio_is_valid(data->usb_sw2_sel_gpio))
			gpio_set_value(data->usb_sw2_sel_gpio, 0);

		//prog_vbus_en(data, 0);
	}
}

static int tcpc_notifier_call(struct notifier_block *nb,  unsigned long action, void *noti_data)
{
	struct tcp_notify *noti = noti_data;
	struct usb_switch_data *data = (struct usb_switch_data *)container_of(nb, struct usb_switch_data, tcpc_nb);

	pr_info("%s: action:%d\n", __func__, action);;

	switch(action) {
		case TCP_NOTIFY_SOURCE_VBUS:
			vbus_en(data, !!noti->vbus_state.mv);
			break;
		case TCP_NOTIFY_TYPEC_STATE:
			if ((noti->typec_state.old_state == TYPEC_UNATTACHED) &&
					(noti->typec_state.new_state == TYPEC_ATTACHED_SRC)) {
				usb_host_switch(data, 1);
			}
			else if ((noti->typec_state.old_state == TYPEC_UNATTACHED) &&
					(noti->typec_state.new_state == TYPEC_ATTACHED_SNK)) {
				usb_host_switch(data, 0);
			}
			else if ((noti->typec_state.old_state != TYPEC_UNATTACHED) &&
					(noti->typec_state.new_state == TYPEC_UNATTACHED)) {
				if (data->lcd_state == FB_BLANK_POWERDOWN) {
					usb_host_switch(data, 0);
				}
				else {
					usb_host_switch(data, !!data->default_mode);
				}
			}

			data->typec_state = noti->typec_state.new_state;

			break;
		case TCP_NOTIFY_DR_SWAP:
			if (noti->swap_state.new_role == PD_ROLE_UFP) {
				usb_host_switch(data, 0);
			}
			else if (noti->swap_state.new_role == PD_ROLE_DFP) {
				usb_host_switch(data, 1);
			}
			break;

		case TCP_NOTIFY_PR_SWAP:
			if (noti->swap_state.new_role == PD_ROLE_SINK) {
				vbus_en(data, 0);
			}
			else if (noti->swap_state.new_role == PD_ROLE_SOURCE) {
				vbus_en(data, 1);
			}
			break;
	}

	return NOTIFY_OK;
}

static void pax_tcpc_dev_init_work(struct work_struct *work)
{
	int ret = 0;
	struct usb_switch_data *data = (struct usb_switch_data *)container_of(work, struct usb_switch_data, tcpc_dwork.work);

	if (!data->tcpc_dev) {
		data->tcpc_dev = tcpc_dev_get_by_name("type_c_port0");
		if (!data->tcpc_dev) {
			schedule_delayed_work(&data->tcpc_dwork, msecs_to_jiffies(200));
			return;
		}
	}

	data->tcpc_nb.notifier_call = tcpc_notifier_call;
	ret = register_tcp_dev_notifier(data->tcpc_dev, &data->tcpc_nb,
			TCP_NOTIFY_TYPE_VBUS | TCP_NOTIFY_TYPE_USB | TCP_NOTIFY_TYPE_MISC);
	if (ret < 0) {
		schedule_delayed_work(&data->tcpc_dwork, msecs_to_jiffies(200));
		return;
	}
}

static void pax_tcpc_dev_init(struct usb_switch_data *data)
{
	data->lcd_state = FB_BLANK_UNBLANK;
	usb_host_switch(data, !!data->default_mode);

	INIT_DELAYED_WORK(&data->tcpc_dwork, pax_tcpc_dev_init_work);
	schedule_delayed_work(&data->tcpc_dwork, 0);
}

static int pax_usb_fb_notifier_call(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int blank;
	struct usb_switch_data *usb_sdata = (struct usb_switch_data *)container_of(
			self, struct usb_switch_data, pax_usb_fb_notifier);

	if (event != FB_EVENT_BLANK)
		return 0;

	blank = *(int *)evdata->data;

	switch (blank) {
		case FB_BLANK_UNBLANK:
			if (usb_sdata->typec_state == TYPEC_UNATTACHED)
				usb_host_switch(usb_sdata, 1);

			usb_sdata->lcd_state = blank;
			break;
		case FB_BLANK_POWERDOWN:
			if (usb_sdata->typec_state == TYPEC_UNATTACHED)
				usb_host_switch(usb_sdata, 0);

			usb_sdata->lcd_state = blank;
			break;
		default:
			break;
	}

	return 0;
}
#define SET_POWER_EN				_IOW('b', 0, int)
#define SET_POWER_STATUS				_IOW('b', 1, int)
extern void mt_usb_vbus_on(int delay);
extern void mt_usb_vbus_off(int delay);
static int pax_r15_status_notify_call(struct notifier_block *self, unsigned long event, void *value)
{
		int power_en = 0;
		int r15_status = 0;
	
	switch (event) {
		case SET_POWER_EN:
			power_en = *(int *)value;
			pr_err("PAX_SET_POWER_EN: %d\n", power_en);
			if (power_en) 
			{
				mt_usb_vbus_on(0);
				power_en = 0;
				r15_status = 1;
				
			}
			else
				mt_usb_vbus_off(0);
			break;

		default:
			break;
	};

	return NOTIFY_DONE;
}


extern const char *cmdline_get_value(const char *key);
static int pax_usb_swtich_probe(struct platform_device *pdev)
{
	int ret = 0,i = 0;
	struct usb_switch_data *data;
	struct device_node *np = pdev->dev.of_node;
	const char *terminal_name;
	//unsigned int time_interval = 500;

    //[FEATURE]-MOD-BEGIN by wugangnan@paxsz.com 2021-07-01, for M50 product no usb switch
    terminal_name = cmdline_get_value("androidboot.terminal_name");
    if(strcmp(terminal_name, "M50") == 0 || pax_usb_switch_poweoff_charging_mode(&pdev->dev))
    {
		pr_err( "product M50 and power off  && meta mode no usb switch module.\n");
		goto err0;
    }
    //[FEATURE]-MOD-END by wugangnan@paxsz.com 2021-07-01, for M50 product no usb switch
	
	pr_err("%s\n", __func__);

	data = devm_kzalloc(&pdev->dev, sizeof(struct usb_switch_data), GFP_KERNEL);
	if (!data) {
		pr_err("%s, alloc fail\n", __func__);
		return -ENOMEM;
	}

	/* resource */
	pax_charger_gpio_init(data,np);
	if (ret) {
		goto req_res_fail;
	}

	pax_tcpc_dev_init(data);

	if (data->default_mode) {
		data->pax_usb_fb_notifier.priority = 99;
		data->pax_usb_fb_notifier.notifier_call = pax_usb_fb_notifier_call;
		ret = fb_register_client(&data->pax_usb_fb_notifier);
	}
	
	data->pax_r15_status_notifier.notifier_call = pax_r15_status_notify_call;
	ret = r15_status_notify_register_client(&data->pax_r15_status_notifier);
	
	//pax_charger_usbswitch_set(data);

	if (g_class_pax != NULL) {
		data->pax_class = g_class_pax;
	} else {
		data->pax_class = class_create(THIS_MODULE, "pax");
	}
	
	//creat sysfs for debug /sys/class/pax/usb_switch/cur_switch
	data->dev = device_create(data->pax_class, &pdev->dev, 0, NULL, "usb_switch");
	platform_set_drvdata(pdev, data);
	
	for (i = 0; ; i++) {
		if (dev_attr[i].attr.name == NULL)
			break;
		device_create_file(data->dev, &dev_attr[i]);
	}

	INIT_DELAYED_WORK(&data->usb_switch_work, do_usb_switch_work);

	//schedule_delayed_work(&data->usb_switch_work, msecs_to_jiffies(time_interval));
	
	pr_info("%s: success.\n", __func__);
	return 0;

req_res_fail:
	devm_kfree(&pdev->dev, data);
	return ret;
err0:
    pr_err("usb_switch probe occured error \n");
    return ret;
}

static int pax_usb_swtich_remove(struct platform_device *pdev)
{
	struct usb_switch_data *data = platform_get_drvdata(pdev);
	int i;
	
	if (data) {
		cancel_delayed_work_sync(&data->usb_switch_work);
		gpio_free(data->usb_host_en_gpio);
		
		for (i = 0; ; i++) {
			if (dev_attr[i].attr.name == NULL)
				break;
			device_remove_file(data->dev, &dev_attr[i]);
		}
		mutex_destroy(&data->lock);

		devm_kfree(&pdev->dev, data);
	}

	return 0;
}


static const struct of_device_id pax_usb_switch_of_match[] = {
	{ .compatible = "pax,usb_switch" },
	{},
};

MODULE_DEVICE_TABLE(of, pax_usb_switch_of_match);


static int pax_usb_switch_suspend(struct device *dev)
{
	pr_err("==%s\n", __func__);
	return 0;
}

static int pax_usb_switch_resume(struct device *dev)
{
	pr_err("==%s\n", __func__);
	return 0;
}

static void pax_usb_switch_shutdown(struct platform_device *pdev)
{
	pr_err("==%s\n", __func__);
}

static const struct dev_pm_ops pax_usb_swtich_pm_ops = {
	.suspend = pax_usb_switch_suspend,
	.resume = pax_usb_switch_resume,
};

static struct platform_driver pax_usb_switch_driver = {
	.probe = pax_usb_swtich_probe,
	.remove = pax_usb_swtich_remove,
	.driver = {
		.name = "pax_usb_switch",
		.of_match_table = pax_usb_switch_of_match,
		.pm = &pax_usb_swtich_pm_ops,
	},
	.shutdown = pax_usb_switch_shutdown,
};


static int __init pax_usb_switch_init(void)
{
	return platform_driver_register(&pax_usb_switch_driver);
}

static void __exit pax_usb_switch_exit(void)
{
	platform_driver_unregister(&pax_usb_switch_driver);
}

/**
 * if listen typec event, must be late_init wait for pd core init;
 * in no_bat mode, must early then typec/pd, in case tyepc/pd cutoff vbus.
 */
#ifdef REGISTER_TYPEC_NOTIFY
late_initcall(pax_usb_switch_init);
#else
module_init(pax_usb_switch_init);
//subsys_initcall(pax_usb_switch_init);
#endif

module_exit(pax_usb_switch_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("PAX");
MODULE_DESCRIPTION("pax usb switch");

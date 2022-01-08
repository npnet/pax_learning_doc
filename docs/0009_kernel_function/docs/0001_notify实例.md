# 概述

notify实例

## 增加notify接口

* 增加notify接口`r15_status_notify.c`：

```c
/*
 *
 *  Copyright (C) 2006 Antonino Daplas <adaplas@pol.net>
 *
 *      2001 - Documented with DocBook
 *      - Brad Douglas <brad@neruo.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */
#include "r15_status_notify.h"

static BLOCKING_NOTIFIER_HEAD(r15_status_notify_list);

/**
 *      r15_status_notify_register_client - register a client notifier
 *      @nb: notifier block to callback on events
 */
int r15_status_notify_register_client(struct notifier_block *nb)
{
        return blocking_notifier_chain_register(&r15_status_notify_list, nb);
}
EXPORT_SYMBOL(r15_status_notify_register_client);

/**
 *      r15_status_notify_unregister_client - unregister a client notifier
 *      @nb: notifier block to callback on events
 */
int r15_status_notify_unregister_client(struct notifier_block *nb)
{
        return blocking_notifier_chain_unregister(&r15_status_notify_list, nb);
}
EXPORT_SYMBOL(r15_status_notify_unregister_client);

/**
 * r15_status_notify_call_chain - notify clients of r15_status_notify_events
 *
 */
int r15_status_notify_call_chain(unsigned long val, void *v)
{
        return blocking_notifier_call_chain(&r15_status_notify_list, val, v);
}
EXPORT_SYMBOL_GPL(r15_status_notify_call_chain);
```

* `r15_status_notify.h`:

```c
#ifndef __R15_STATUS_NOTIFY_H__
#define __R15_STATUS_NOTIFY_H__

#include <linux/notifier.h>
#include <linux/export.h>

#define SET_POWER_EN                            _IOW('b', 0, int)
#define SET_POWER_STATUS                                _IOW('b', 1, int)

enum r15_power_en {
        R15_POWER_DISABLE = 0,
        R15_POWER_ENABLE,
};

enum r15_status {
        R15_STATUS_OFFLINE,
        R15_STATUS_ONLINE,
};

enum pogo_dev_detect_type {
        POGO_DETECT_UNKNOWN,
        POGO_DETECT_NONE,
        POGO_DETECT_BY_EXT_PIN,
        POGO_DETECT_BY_CC,
};

extern int r15_status_notify_register_client(struct notifier_block *nb);  //驱动注册服务端
extern int r15_status_notify_unregister_client(struct notifier_block *nb);
extern int r15_status_notify_call_chain(unsigned long val, void *v);//发送消息

#endif
```

## 注册并监听

* `kernel-4.19/drivers/misc/pax/gpio/pax_gpio_control.c`:

```diff
#include "r15_status_notify.h"

 struct pax_gpio_desc {
        unsigned int func; /* GPIO_FUNC_INPUT/GPIO_FUNC_OUTPUT/GPIO_FUNC_INT */
@@ -106,6 +113,8 @@ struct pax_gpio_set {
        unsigned long out_gpio_value; /* pre time gpio value, per bit per gpio */

        struct delayed_work delay_work;
+       struct notifier_block gpio_nb;
 };

1.通知函数
+static int r15_power_vote(unsigned int value)
+{
+       int power_en = R15_POWER_DISABLE;
+
+       if (value == R15_ONLINE_TRADING) {
+               //power_en = R15_POWER_ENABLE;
+       }
+       else if (value == R15_ONLINE) {
+               power_en = R15_POWER_ENABLE;
+       }
+       else {
+               power_en = R15_POWER_DISABLE;
+       }
+
+       r15_status_notify_call_chain(SET_POWER_EN, &power_en);
+       PAX_GPIO_DBG("power_en: %d, \n", power_en);
+
+       return value;
+}

+static int r15_status_notify_call(struct notifier_block *self, unsigned long event, void *value)
+{
+       int power_en = 0;
+
+       switch (event) {
+               case SET_POWER_EN:
+                       power_en = *(int *)value;
+                       pr_err("SET_POWER_EN: %d\n", power_en);
+                       break;
+
+               default:
+                       break;
+       };
+
+       return NOTIFY_DONE;
+}

2.probe 注册监听
 static int pax_gpios_probe(struct platform_device *pdev)
 {
@@ -813,6 +866,10 @@ static int pax_gpios_probe(struct platform_device *pdev)
        /* delay work */
        INIT_DELAYED_WORK(&data->delay_work, work_queue_func);

+       /* R15 power notify*/
+       data->gpio_nb.notifier_call = r15_status_notify_call;
+       r15_status_notify_register_client(&data->gpio_nb);
```

也就是说，当调用r15_power_vote()函数将会通知所有注册的notify client，r15_status_notify_call将回调并打印。
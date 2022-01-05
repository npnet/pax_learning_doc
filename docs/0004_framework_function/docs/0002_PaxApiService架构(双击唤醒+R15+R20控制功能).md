# 概述
介绍PaxApiService架构

## 参考

* [Android中AIDL的使用详解](https://www.jianshu.com/p/d1fac6ccee98)

* [Android HIDL HAL 接口定义语言详解](https://blog.csdn.net/qq_19923217/article/details/88398660)

* [Android P HAL层添加HIDL实例（详细实现步骤）](https://blog.csdn.net/sinat_18179367/article/details/95940030)

## 架构图

![0002_架构图.png](images/0002_架构图.png)

* 涉及文件：

```log
kernel层：
 kernel-4.19/touchscreen/ilitek_limv/ilitek_main.c     |  12 +-
 kernel-4.19/drivers/misc/pax/gpio/pax_gpio_control.c
 
jni层： 
 paxdroid/external/pax/jni/libpaxapijni/pax_util_OsPaxApi.c  |  12 +
 paxdroid/external/pax/jni/libpaxapijni/pax_util_OsPaxApi.h  |  17 +
 paxdroid/external/pax/lib/libpaxapiclient/paxapiclient.cpp  |  22 +
 paxdroid/external/pax/lib/libpaxapiclient/paxapiclient.h    |   2 +
 
java服务层：
 paxdroid/frameworks/base/core/java/pax/util/IPaxApi.aidl      |   4 +-
 paxdroid/frameworks/base/m50api/java/pax/util/PayDevManager.java   |  91 ++
 paxdroid/frameworks/base/services/core/java/com/android/server/PaxApiService.java     |  34 +
 
hal层(gpio驱动类似)：
 paxdroid/device/paxdroid_mt6762.mk            |   1 +
 paxdroid/device/paxdroid_mt6762_vnd.mk        |   1 +
 paxdroid/device/sepolicy/vendor/file_contexts |   1 +
 paxdroid/hardware/interfaces/current.txt      |   2 +-
 paxdroid/hardware/interfaces/paxservice/1.0/IPaxApiService.hal         |   5 +
 paxdroid/hardware/interfaces/paxservice/1.0/default/PaxApiService.cpp  |   5 +
 paxdroid/hardware/interfaces/paxservice/1.0/default/PaxApiService.h    |   3 +
 paxdroid/hardware/interfaces/paxservice/1.0/default/gesture/PaxGesture.cpp        | 103 ++
 paxdroid/hardware/interfaces/paxservice/1.0/default/gesture/PaxGesture.h          |  11 +
 paxdroid/hardware/interfaces/paxservice/1.0/default/axdroid.hardware.paxservice@1.0-service.rc |   3 +
 paxdroid/hardware/libhardware/include/hardware/pax_gesture.h            |  82 ++
 paxdroid/hardware/libhardware/modules/gesture/pax_gesture.c | 180 ++++
```

## 增加双击唤醒功能

tp需要增加双击唤醒功能开关，功能已经在驱动层实现，现在需要增加hal接口及设置界面。

### 1.kernel层ioctrl接口

* `kernel-4.19/drivers/input/touchscreen/ilitek_limv/ilitek_platform_init.c`将生产/dev/pax_tp节点:

```diff
+#include <linux/miscdevice.h>
+
 struct ilitek_ts_data *ilitek_data;
 /*
    Data struct init. When the driver will be setup, the driver initial the data struct.
    If the memory alloc fail the function will return -ENOMEM.
    The log level be set ILITEK_DEFAULT_LOG_LEVEL.
  */
+
+static int tp_open(struct inode *inode, struct file *file)
+{
+	return 0;
+}
+
+static int tp_release(struct inode *inode, struct file *file)
+{
+	return 0;
+}
+
+static long tp_ioctl(struct file *file, unsigned int cmd,
+				unsigned long arg)
+{
+	void __user *user_data = (void __user *)arg;
+	int ret = 0;
+	int en = 1;
+
+	switch (cmd) {
+		case SET_GESTURE_ONOFF:
+			ret = copy_from_user(&en, user_data, sizeof(int));
+			if (ret < 0) {
+				pr_info("SET_GESTURE_ONOFF.\n");
+				ret = -1;
+				break;
+			}
+
+			pr_info("SET_GESTURE_ONOFF: %d\n", en);
+			ilitek_data->enable_gesture = en;
+			break;
+		case SET_DEFAULT:
+			ret = copy_from_user(&en, user_data, sizeof(int));
+			if (ret < 0) {
+				pr_info("SET_DEFAULT.\n");
+				ret = -1;
+				break;
+			}
+			pr_info("SET_DEFAULT: %d\n", en);
+			//tp_chg_vote(NC_DISABLE_CHG_BY_USER, !en);
+			break;
+		default:
+			pr_info("cmd: %u is not support.\n", cmd);
+			ret = -1;
+			break;
+	}
+
+	return ret;
+}
+
+#ifdef CONFIG_COMPAT
+static long tp_compat_ioctl(struct file *file,
+			unsigned int cmd, unsigned long arg)
+{
+	return tp_ioctl(file, cmd, arg);
+}
+#endif
+
+static const struct file_operations tp_fops = {
+	.owner = THIS_MODULE,
+	.open = tp_open,
+	.release = tp_release,
+	.unlocked_ioctl = tp_ioctl,
+#ifdef CONFIG_COMPAT
+	.compat_ioctl = tp_compat_ioctl,
+#endif
+};
+
+static struct miscdevice tp_miscdev = {
+	.minor      = MISC_DYNAMIC_MINOR,
+	.name		= ILITEK_TS_NAME,
+	.fops		= &tp_fops,
+};
+
+
 int ilitek_data_init(void) {
 	ilitek_data = kzalloc(sizeof(*ilitek_data), GFP_KERNEL);
 	if (ilitek_data == NULL) {
@@ -135,6 +211,8 @@ static struct i2c_driver tpd_i2c_driver = {
 	.detect = tpd_detect,
 };
 
+
+
 static int tpd_local_init(void)
 {
 	tp_log_info("TPD init device driver\n");
@@ -153,6 +231,7 @@ static int tpd_local_init(void)
 		tpd_button_setting(tpd_dts_data.tpd_key_num, tpd_dts_data.tpd_key_local, tpd_dts_data.tpd_key_dim_local);
 
 	tpd_type_cap = 1;
+	
 	tp_log_info("TPD init done\n");
 	return TPD_OK;
 }
@@ -403,6 +482,8 @@ static int __init ilitek_touch_driver_init(void)
 		i2c_del_driver(&ilitek_touch_device_driver);
 		return -ENODEV;
 	}
+
+	misc_register(&tp_miscdev);
```
### 2.新增HAL层Interface及module

```diff
1.增加包：
--- a/paxdroid/device/paxdroid_mt6762.mk
+++ b/paxdroid/device/paxdroid_mt6762.mk
@@ -98,6 +98,7 @@ USE_PAX_HELLO := AP
 #add by zengzp for paxservice
 PRODUCT_PACKAGES += \
     paxgpios.default \
+    paxgesture.default \

2.增加配置好的selinux节点属性：
--- a/paxdroid/device/sepolicy/vendor/file_contexts
+++ b/paxdroid/device/sepolicy/vendor/file_contexts
@@ -6,6 +6,7 @@
 /(vendor|system/vendor)/paxdroid(/.*)?     u:object_r:vendor_configs_file:s0
 
 /dev/pax_gpios                             u:object_r:pax_chr_device:s0
+/dev/ilitek_ts                             u:object_r:pax_chr_device:s0

3.增加hal层interface及module接口，涉及文件：
 paxdroid/hardware/interfaces/current.txt      |   2 +-
 paxdroid/hardware/interfaces/paxservice/1.0/IPaxApiService.hal         |   5 +
 paxdroid/hardware/interfaces/paxservice/1.0/default/PaxApiService.cpp  |   5 +
 paxdroid/hardware/interfaces/paxservice/1.0/default/PaxApiService.h    |   3 +
 paxdroid/hardware/interfaces/paxservice/1.0/default/gesture/PaxGesture.cpp        | 103 ++
 paxdroid/hardware/interfaces/paxservice/1.0/default/gesture/PaxGesture.h          |  11 +
 paxdroid/hardware/interfaces/paxservice/1.0/default/axdroid.hardware.paxservice@1.0-service.rc |   3 +
 paxdroid/hardware/libhardware/include/hardware/pax_gesture.h            |  82 ++
 paxdroid/hardware/libhardware/modules/gesture/pax_gesture.c | 180 ++++
 
 详细写一个ioctrl实现：
 +++ b/paxdroid/hardware/libhardware/modules/gesture/pax_gesture.c
@@ -0,0 +1,180 @@
+/*
+ * Copyright (C) 2013 The Android Open Source Project
+ *
+ * Licensed under the Apache License, Version 2.0 (the "License");
+ * you may not use this file except in compliance with the License.
+ * You may obtain a copy of the License at
+ *
+ *      http://www.apache.org/licenses/LICENSE-2.0
+ *
+ * Unless required by applicable law or agreed to in writing, software
+ * distributed under the License is distributed on an "AS IS" BASIS,
+ * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
+ * See the License for the specific language governing permissions and
+ * limitations under the License.
+ */
+
+#include <hardware/hardware.h>
+#include <hardware/pax_gesture.h>
+
+#include <stdio.h>
+#include <stdlib.h>
+#include <sys/types.h>
+#include <unistd.h>
+#include <sys/stat.h>
+#include <unistd.h>
+#include <errno.h>
+#include <fcntl.h>
+#include <malloc.h>
+#include <stdint.h>
+#include <dirent.h>
+#include <string.h>
+#include <pthread.h>
+#include <time.h>
+#include <stdlib.h>
+#include <log/log.h>
+
+#define TIMEOUT_STR_LEN         20
+#define LOG_NDEBUG 0
+#define LOG_TAG "pax_gesture_hal"
+static pthread_once_t g_init = PTHREAD_ONCE_INIT;
+static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
+
+#define PAX_TP_DEV			"/dev/ilitek_ts"
+
+/**
+ * device methods
+ */
+
+void init_globals(void)
+{
+    // init the mutex
+    pthread_mutex_init(&g_lock, NULL);
+}
+
+static bool device_exists(const char *file) {
+    int fd;
+
+    fd = open(file, O_RDWR);
+    if(fd < 0) {
+        return false;
+    }
+
+    close(fd);
+    return true;
+}
+
+static bool gesture_exists() {
+    return device_exists(PAX_TP_DEV);
+}
+
+static int open_pax_tp_dev(void)
+{
+	int fd = -1;
+
+	fd = open(PAX_TP_DEV, O_RDWR);
+
+	return fd;
+}
+
+static int pax_tp_ctl( unsigned long cmd, void *data)
+{
+	static int pax_tp_fd = -1;
+	int ret = -1;
+
+
+	if (pax_tp_fd < 0) {
+		pax_tp_fd = open_pax_tp_dev();
+
+		if (pax_tp_fd < 0) {
+			return -ENODEV;
+		}
+	}
+
+    pthread_mutex_lock(&g_lock);
+
+	switch(cmd) {
+		case SET_GESTURE_ONOFF:
+			{
+				int en = *(int *)data;
+				ret = ioctl(pax_tp_fd, cmd, &en);
+			}
+			break;
+		default:
+			break;
+	};
+
+    pthread_mutex_unlock(&g_lock);
+
+	return ret;
+}
+
+static int gesture_on(void)
+{
+	int en = 1;
+    return pax_tp_ctl( SET_GESTURE_ONOFF, &en);
+ 
+}
+
+static int gesture_off(void)
+{
+   	int en = 0;
+    return pax_tp_ctl( SET_GESTURE_ONOFF, &en);
+}
+
+static int gesture_close(hw_device_t *device)
+{
+    free(device);
+    return 0;
+}
+
+static int gesture_open(const hw_module_t* module, const char* id __unused,
+                      hw_device_t** device __unused) {
+
+    if (gesture_exists()) {
+        ALOGD("touchscreen gesture sys file exist");
+    }else {
+        ALOGE("touchscreen gesture sys file not exis. Cannot start gesture");
+        return -ENODEV;
+    }
+
+	pthread_once(&g_init, init_globals);
+	
+    gesture_device_t *gesturedev = malloc(sizeof(gesture_device_t));
+    memset(gesturedev, 0, sizeof(gesture_device_t));
+	
+    if (!gesturedev) {
+        ALOGE("Can not allocate memory for the gesture device");
+        return -ENOMEM;
+    }
+
+    gesturedev->common.tag = HARDWARE_DEVICE_TAG;
+    gesturedev->common.module = (hw_module_t *) module;
+    gesturedev->common.version = HARDWARE_DEVICE_API_VERSION(1,0);
+    gesturedev->common.close = gesture_close;
+
+	gesturedev->gesture_on = gesture_on;
+	gesturedev->gesture_off = gesture_off;
+
+    *device = (hw_device_t *) gesturedev;
+
+    return 0;
+}
+
+/*===========================================================================*/
+/* Default gesture HW module interface definition                           */
+/*===========================================================================*/
+
+static struct hw_module_methods_t gesture_module_methods = {
+    .open = gesture_open,
+};
+
+struct hw_module_t HAL_MODULE_INFO_SYM = {
+    .tag = HARDWARE_MODULE_TAG,
+    .module_api_version = GESTURE_API_VERSION,
+    .hal_api_version = HARDWARE_HAL_API_VERSION,
+    .id = GESTURE_HARDWARE_MODULE_ID,
+    .name = "pax gesture HAL",
+    .author = "The Paxdroid Open Source Project",
+    .methods = &gesture_module_methods,
+};
```
### 3.增加PaxApiService接口及对外PayDevManager aidl接口

```log
涉及文件：
 paxdroid/frameworks/base/core/java/pax/util/IPaxApi.aidl      |   4 +-
 paxdroid/frameworks/base/m50api/java/pax/util/PayDevManager.java   |  91 ++
 paxdroid/frameworks/base/services/core/java/com/android/server/PaxApiService.java     |  34 +
```
### R15接口添加见附件
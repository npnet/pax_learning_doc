# Android Manual Backlight

手动背光控制APP、Service、HIDL、Driver工作流程

## 参考文档

* [0184_Kernel_dump_stack.md](0184_Kernel_dump_stack.md)
* [0278_Android_Java_CallStack.md](0278_Android_Java_CallStack.md)
* [0368_Android_Auto_Brightness.md](0368_Android_Auto_Brightness.md)

## shell

* /sys/class/leds/lcd-backlight/
  ```
  brightness  device  max_brightness  power  subsystem  trigger  uevent
  ```
* cat brightness
  ```
  36
  ```
* echo 50 > brightness
  * kernel log
    ```
    [  217.021963] (5)[5713:sh]mt_mt65xx_led_set
    [  217.022041] -(5)[5713:sh]CPU: 5 PID: 5713 Comm: sh Tainted: P        W  O      4.19.127 #3
    [  217.022077] -(5)[5713:sh]Hardware name: MT6762V/WD (DT)
    [  217.022109] -(5)[5713:sh]Call trace:
    [  217.022171] -(5)[5713:sh] dump_backtrace+0x0/0x164
    [  217.022217] -(5)[5713:sh] show_stack+0x20/0x2c
    [  217.022264] -(5)[5713:sh] dump_stack+0xb8/0xf0
    [  217.022312] -(5)[5713:sh] mt_mt65xx_led_set+0x38/0x24c
    [  217.022352] -(5)[5713:sh] mt65xx_led_set+0x54/0x64
    [  217.022396] -(5)[5713:sh] led_set_brightness_nosleep+0x40/0x68
    [  217.022434] -(5)[5713:sh] led_set_brightness+0x30/0x74
    [  217.022472] -(5)[5713:sh] brightness_store+0x88/0xc0
    [  217.022516] -(5)[5713:sh] dev_attr_store+0x40/0x58
    [  217.022563] -(5)[5713:sh] sysfs_kf_write+0x50/0x68
    [  217.022606] -(5)[5713:sh] kernfs_fop_write+0x138/0x1d4
    [  217.022650] -(5)[5713:sh] __vfs_write+0x54/0x15c
    [  217.022692] -(5)[5713:sh] vfs_write+0xe4/0x1a0
    [  217.022732] -(5)[5713:sh] ksys_write+0x78/0xd8
    [  217.022772] -(5)[5713:sh] __arm64_sys_write+0x20/0x2c
    [  217.022816] -(5)[5713:sh] el0_svc_common+0x9c/0x14c
    [  217.022856] -(5)[5713:sh] el0_svc_handler+0x68/0x84
    [  217.022895] -(5)[5713:sh] el0_svc+0x8/0xc
    [  217.025055] (6)[5713:sh]mt_mt65xx_led_set before backlight_debug_log
    [  217.025118] (3)[54:kworker/3:1]mtk_leds backlight_debug_log(120) :[BL] Set Backlight directly T:217.25,L:50 map:50
    ```
  * logcat log
    ```
    10-25 17:08:20.183  1173  1253 V DisplayPowerController: Brightness [0.39998955] reason changing to: 'temporary', previous reason: 'manual'.
    10-25 17:08:20.190   548   548 D android.hardware.lights-service.mediatek: write 84 to /sys/class/leds/lcd-backlight/brightness, result: 0
    10-25 17:08:20.190   790   877 D AAL     : onBacklightChanged: 413/1023 -> 337/1023(phy:1349/4095)
    10-25 17:08:20.190   790   877 D AAL     : LABC: LABC after setTarget isSmoothBacklight=1
    10-25 17:08:20.205   548   548 D android.hardware.lights-service.mediatek: write 67 to /sys/class/leds/lcd-backlight/brightness, result: 0
    10-25 17:08:20.207   790   877 D AAL     : onBacklightChanged: 337/1023 -> 269/1023(phy:1077/4095)
    10-25 17:08:20.208   790   877 D AAL     : LABC: LABC after setTarget isSmoothBacklight=1
    ```

## dts

* kernel-4.19/arch/arm64/boot/dts/mediatek/M8.dts
  ```
  &odm {
       // ...省略
       led6:led@6 {
            compatible = "mediatek,lcd-backlight";
            led_mode = <5>;
            data = <1>;
            pwm_config = <0 3 0 0 0>;
       };
       // ...省略
  }
  ```
  * led_mode: 5
    * MT65XX_LED_MODE_CUST_BLS_PWM

## driver

`echo 50 > brightness`由mt65xx_led_set()进行处理，驱动注册及处理流程如下

```
* kernel-4.19/drivers/misc/mediatek/leds/mtk_leds_drv.c
  * static int __init mt65xx_leds_init(void)
    * ret = platform_device_register(&mt65xx_leds_device);
      * static struct platform_device mt65xx_leds_device
      * .name = "leds-mt65xx",
    * ret = platform_driver_register(&mt65xx_leds_driver);
      * static struct platform_driver mt65xx_leds_driver
        * .driver
          * .name = "leds-mt65xx",
        * .probe = mt65xx_leds_probe,
          * static int mt65xx_leds_probe(struct platform_device *pdev)
            * for (i = 0; i < MT65XX_LED_TYPE_TOTAL; i++)
              * g_leds_data[i]->cdev.brightness_set = mt65xx_led_set;
                * static void mt65xx_led_set(struct led_classdev *led_cdev, enum led_brightness level)
                  * mt_mt65xx_led_set(led_cdev, level);
                    * kernel-4.19/drivers/misc/mediatek/leds/mt6765/mtk_leds.c
                      * void mt_mt65xx_led_set(struct led_classdev *led_cdev, enum led_brightness level)
                        * #ifdef CONFIG_MTK_AAL_SUPPORT
                          * if (led_data->level != level)
                            * led_data->level = level;
                            * backlight_debug_log(led_data->level, level);
                              * ret = snprintf(buffer + strlen(buffer), 4095 - strlen(buffer), "T:%lld.%ld,L:%d map:%d ", cur_time_display, cur_time_mod, level, mappingLevel);
                              * pr_info("%s", buffer);
                            * disp_aal_notify_backlight_changed((((1 << MT_LED_INTERNAL_LEVEL_BIT_CNT) - 1) * level + 127) / 255);
                              * kernel-4.19/drivers/misc/mediatek/video/common/aal30/ddp_aal.c
                                * void disp_aal_notify_backlight_changed(int bl_1024)
                                  * backlight_brightness_set(bl_1024);
                                    * kernel-4.19/drivers/misc/mediatek/leds/mtk_leds_drv.c
                                      * int backlight_brightness_set(int level)
                                        * struct cust_mt65xx_led *cust_led_list = mt_get_cust_led_list();
                                          * struct cust_mt65xx_led *cust_led_list = get_cust_led_dtsi();
                                            * for (i = 0; i < MT65XX_LED_TYPE_TOTAL; i++)
                                              * switch (pled_dtsi[i].mode)
                                                * case MT65XX_LED_MODE_CUST_BLS_PWM:
                                                  * pled_dtsi[i].data = (long)disp_bls_set_backlight;
                                                    * kernel-4.19/drivers/misc/mediatek/video/common/pwm10/ddp_pwm.c
                                                      * int disp_bls_set_backlight(int level_1024)
                                                        * return disp_pwm_set_backlight(disp_pwm_get_main(), level_1024);
                                                          * ret = disp_pwm_set_backlight_cmdq(id, level_1024, NULL);
                                                            * index = index_of_pwm(id);
                                                            * level_1024 = disp_pwm_level_remap(id, level_1024);
                                                            * DISP_REG_MASK(cmdq, reg_base + DISP_PWM_CON_1_OFF, 1 << 16, 0x1fff << 16);
                                                            * disp_pwm_set_enabled(cmdq, id, 0);
                                                            * 到这里差不多了，不继续跟了
                                        * mt_mt65xx_led_set_cust(&cust_led_list[MT65XX_LED_TYPE_LCD], level);
                                          * int mt_mt65xx_led_set_cust(struct cust_mt65xx_led *cust, int level)
                                            * case MT65XX_LED_MODE_CUST_BLS_PWM:
                                              * case MT65XX_LED_MODE_CUST_BLS_PWM:
                                                * return ((cust_set_brightness) (cust->data)) (level);
                                                  * kernel-4.19/drivers/misc/mediatek/video/common/pwm10/ddp_pwm.c
                                                    * int disp_bls_set_backlight(int level_1024)
                                                      * 调用这个函数，上面mt_get_cust_led_list()中初始化的，这里不重复分析
              * ret = led_classdev_register(&pdev->dev, &g_leds_data[i]->cdev);
```

驱动调试diff

```diff
diff --git a/kernel-4.19/drivers/misc/mediatek/leds/mt6765/mtk_leds.c b/kernel-4.19/drivers/misc/mediatek/leds/mt6765/mtk_leds.c
index 01a701fab72..d778b1d0729 100644
--- a/kernel-4.19/drivers/misc/mediatek/leds/mt6765/mtk_leds.c
+++ b/kernel-4.19/drivers/misc/mediatek/leds/mt6765/mtk_leds.c
@@ -1,3 +1,4 @@
+#define DEBUG
 // SPDX-License-Identifier: GPL-2.0
 /*
  * Copyright (C) 2018 MediaTek Inc.
@@ -654,6 +655,7 @@ int mt_mt65xx_led_set_cust(struct cust_mt65xx_led *cust, int level)
                if (enable_met_backlight_tag())
                        output_met_backlight_tag(level);
 #endif
+               printk("MT65XX_LED_MODE_CUST_BLS_PWM\n");
                return ((cust_set_brightness) (cust->data)) (level);

        case MT65XX_LED_MODE_NONE:
@@ -668,6 +670,7 @@ void mt_mt65xx_led_work(struct work_struct *work)
        struct mt65xx_led_data *led_data =
            container_of(work, struct mt65xx_led_data, work);

+       printk("%s\n", __func__);
        pr_debug("%s:%d\n", led_data->cust.name, led_data->level);
        mutex_lock(&leds_mutex);
        mt_mt65xx_led_set_cust(&led_data->cust, led_data->level);
@@ -681,6 +684,8 @@ void mt_mt65xx_led_set(struct led_classdev *led_cdev, enum led_brightness level)
        /* unsigned long flags; */
        /* spin_lock_irqsave(&leds_lock, flags); */

+       printk("%s\n", __func__);
+       dump_stack();
 #ifdef CONFIG_MTK_AAL_SUPPORT
        if (led_data->level != level) {
                led_data->level = level;
@@ -697,6 +702,7 @@ void mt_mt65xx_led_set(struct led_classdev *led_cdev, enum led_brightness level)
                                    (level * CONFIG_LIGHTNESS_MAPPING_VALUE) /
                                    255;
                        }
+                       printk("%s before backlight_debug_log\n", __func__);
                        backlight_debug_log(led_data->level, level);
                        disp_pq_notify_backlight_changed((((1 <<
                                        MT_LED_INTERNAL_LEVEL_BIT_CNT)
```

kernel log

```
[  217.021963] (5)[5713:sh]mt_mt65xx_led_set
[  217.022041] -(5)[5713:sh]CPU: 5 PID: 5713 Comm: sh Tainted: P        W  O      4.19.127 #3
[  217.022077] -(5)[5713:sh]Hardware name: MT6762V/WD (DT)
[  217.022109] -(5)[5713:sh]Call trace:
[  217.022171] -(5)[5713:sh] dump_backtrace+0x0/0x164
[  217.022217] -(5)[5713:sh] show_stack+0x20/0x2c
[  217.022264] -(5)[5713:sh] dump_stack+0xb8/0xf0
[  217.022312] -(5)[5713:sh] mt_mt65xx_led_set+0x38/0x24c
[  217.022352] -(5)[5713:sh] mt65xx_led_set+0x54/0x64
[  217.022396] -(5)[5713:sh] led_set_brightness_nosleep+0x40/0x68
[  217.022434] -(5)[5713:sh] led_set_brightness+0x30/0x74
[  217.022472] -(5)[5713:sh] brightness_store+0x88/0xc0
[  217.022516] -(5)[5713:sh] dev_attr_store+0x40/0x58
[  217.022563] -(5)[5713:sh] sysfs_kf_write+0x50/0x68
[  217.022606] -(5)[5713:sh] kernfs_fop_write+0x138/0x1d4
[  217.022650] -(5)[5713:sh] __vfs_write+0x54/0x15c
[  217.022692] -(5)[5713:sh] vfs_write+0xe4/0x1a0
[  217.022732] -(5)[5713:sh] ksys_write+0x78/0xd8
[  217.022772] -(5)[5713:sh] __arm64_sys_write+0x20/0x2c
[  217.022816] -(5)[5713:sh] el0_svc_common+0x9c/0x14c
[  217.022856] -(5)[5713:sh] el0_svc_handler+0x68/0x84
[  217.022895] -(5)[5713:sh] el0_svc+0x8/0xc
[  217.025055] (6)[5713:sh]mt_mt65xx_led_set before backlight_debug_log
[  217.025118] (3)[54:kworker/3:1]mtk_leds backlight_debug_log(120) :[BL] Set Backlight directly T:217.25,L:50 map:50
```

## HIDL

Lights的HIDL接口分析

```
* vendor/mediatek/proprietary/hardware/liblights
  * aidl/default
    * Lights.h
      * char const*const LCD_FILE = "/sys/class/leds/lcd-backlight/brightness";
      * class Lights : public BnLights
        * #include <aidl/android/hardware/light/BnLights.h>
```

HIDL aidl定义

```java
// hardware/interfaces/light/aidl/android/hardware/light/ILights.aidl

@VintfStability
interface ILights {
    /**
     * Set light identified by id to the provided state.
     *
     * If control over an invalid light is requested, this method exists with
     * EX_UNSUPPORTED_OPERATION. Control over supported lights is done on a
     * device-specific best-effort basis and unsupported sub-features will not
     * be reported.
     *
     * @param id ID of logical light to set as returned by getLights()
     * @param state describes what the light should look like.
     */
    void setLightState(in int id, in HwLightState state);

    /**
     * Discover what lights are supported by the HAL implementation.
     *
     * @return List of available lights
     */
    HwLight[] getLights();
}
```

查看HIDL目录还是有什么aidl：hardware/interfaces/light/aidl/android/hardware/light

```
.
├── BrightnessMode.aidl
├── FlashMode.aidl
├── HwLight.aidl
├── HwLightState.aidl
├── ILights.aidl
└── LightType.aidl

0 directories, 6 files
```

aidl输出目录：out/soong/.intermediates/hardware/interfaces/light/aidl/android.hardware.light-cpp-source/gen/

* include/android/hardware/light
  ```
  .
  ├── BnBrightnessMode.h
  ├── BnFlashMode.h
  ├── BnHwLight.h
  ├── BnHwLightState.h
  ├── BnLightType.h
  ├── BnLights.h
  ├── BpBrightnessMode.h
  ├── BpFlashMode.h
  ├── BpHwLight.h
  ├── BpHwLightState.h
  ├── BpLightType.h
  ├── BpLights.h
  ├── BrightnessMode.h
  ├── FlashMode.h
  ├── HwLight.h
  ├── HwLightState.h
  ├── ILights.h
  └── LightType.h
  
  0 directories, 18 files
  ```
* android/hardware/light/
  ```
  .
  ├── BrightnessMode.cpp
  ├── BrightnessMode.cpp.d
  ├── FlashMode.cpp
  ├── FlashMode.cpp.d
  ├── HwLight.cpp
  ├── HwLight.cpp.d
  ├── HwLightState.cpp
  ├── HwLightState.cpp.d
  ├── ILights.cpp
  ├── ILights.cpp.d
  ├── LightType.cpp
  └── LightType.cpp.d
  
  0 directories, 12 files
  ```

HIDL Service叫什么名字？

* 先看注册的方法
  ```CPP
  // vendor/mediatek/proprietary/hardware/liblights/aidl/default
  
  int main() {
      ABinderProcess_setThreadPoolMaxThreadCount(0);
      std::shared_ptr<Lights> lights = ndk::SharedRefBase::make<Lights>();
  
      const std::string instance = std::string() + Lights::descriptor + "/default";  // <--
      binder_status_t status = AServiceManager_addService(lights->asBinder().get(), instance.c_str());
      CHECK(status == STATUS_OK);
  
      ABinderProcess_joinThreadPool();
      return EXIT_FAILURE;  // should not reached
  }
  ```
* service list | grep light
  ```
  14      android.hardware.light.ILights/default: [android.hardware.light.ILights]
  95      lights: [android.hardware.lights.ILightsManager]
  ```
  * 这里发现了一个新的aidl接口：ILightsManager
    * frameworks/base/core/java/android/hardware/lights/ILightsManager.aidl
    * frameworks/base/core/java/android/hardware/lights
      ```
      .
      ├── ILightsManager.aidl
      ├── Light.aidl
      ├── Light.java
      ├── LightState.aidl
      ├── LightState.java
      ├── LightsManager.java
      └── LightsRequest.java
    
      0 directories, 7 files
      ```

## LightsManager

LightsManager获取SystemLightsManager中的Context.LIGHTS_SERVICE服务实现如下

```java
// frameworks/base/core/java/android/hardware/lights/SystemLightsManager.java

public LightsManager(@NonNull Context context) throws ServiceNotFoundException {
    this(context, ILightsManager.Stub.asInterface(
        ServiceManager.getServiceOrThrow(Context.LIGHTS_SERVICE)));
}

/**
 * Creates a LightsManager with a provided service implementation.
 *
 * @hide
 */
@VisibleForTesting
public LightsManager(@NonNull Context context, @NonNull ILightsManager service) {
    mContext = Preconditions.checkNotNull(context);
    mService = Preconditions.checkNotNull(service);
}
```

向Context注册Context.LIGHTS_SERVICE实现如下

```java
// frameworks/base/core/java/android/app/SystemServiceRegistry.java

registerService(Context.LIGHTS_SERVICE, LightsManager.class,
    new CachedServiceFetcher<LightsManager>() {
        @Override
        public LightsManager createService(ContextImpl ctx)
            throws ServiceNotFoundException {
            return new SystemLightsManager(ctx);
        }});
```

亮度控制接口

```java
// frameworks/base/core/java/android/hardware/lights/SystemLightsManager.java

@RequiresPermission(Manifest.permission.CONTROL_DEVICE_LIGHTS)
@Override
public void requestLights(@NonNull LightsRequest request) {
    Preconditions.checkNotNull(request);
    if (!mClosed) {
        try {
            List<Integer> idList = request.getLights();
            List<LightState> stateList = request.getLightStates();
            int[] ids = new int[idList.size()];
            for (int i = 0; i < idList.size(); i++) {
                ids[i] = idList.get(i);
            }
            LightState[] states = new LightState[stateList.size()];
            for (int i = 0; i < stateList.size(); i++) {
                states[i] = stateList.get(i);
            }
            mService.setLightStates(getToken(), ids, states);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }
}
```

应用APP无法访问此接口，被系统隐藏

## LightsService

LightsService注册如下

```java
// frameworks/base/services/core/java/com/android/server/lights/LightsService.java

@Override
public void onStart() {
    publishLocalService(LightsManager.class, mService);
    publishBinderService(Context.LIGHTS_SERVICE, mManagerService);
}
```

查看lights服务

* dumpsys -l | grep light
  ```
    android.hardware.light.ILights/default
    lights
  ```
* dumpsys lights
  ```
  Service: aidl (android.hardware.light.ILights$Stub$Proxy@767a300)
  Lights:
    Light id=0 ordinal=0 color=ff1f1f1f
    Light id=1 ordinal=0 color=00000000
    Light id=2 ordinal=0 color=00000000
    Light id=3 ordinal=0 color=ff00ff00
    Light id=4 ordinal=0 color=00000000
    Light id=5 ordinal=0 color=00000000
  Session clients:
  ```
* dump
  ```java
  // frameworks/base/services/core/java/com/android/server/lights/LightsService.java
  
  @Override
  protected void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
      if (!DumpUtils.checkDumpPermission(getContext(), TAG, pw)) return;
  
      synchronized (LightsService.this) {
          if (mVintfLights != null) {
              pw.println("Service: aidl (" + mVintfLights.get() + ")");
          } else {
              pw.println("Service: hidl");
          }
  
          pw.println("Lights:");
          for (int i = 0; i < mLightsById.size(); i++) {
              final LightImpl light = mLightsById.valueAt(i);
              pw.println(String.format("  Light id=%d ordinal=%d color=%08x",
                      light.mHwLight.id, light.mHwLight.ordinal, light.getColor()));
          }
  
          pw.println("Session clients:");
          for (Session session : mSessions) {
              pw.println("  Session token=" + session.mToken);
              for (int i = 0; i < session.mRequests.size(); i++) {
                  pw.println(String.format("    Request id=%d color=%08x",
                          session.mRequests.keyAt(i),
                          session.mRequests.valueAt(i).getColor()));
              }
          }
      }
  }
  ```

LightsService获取HIDL接口，以及控制亮度原理

* 在构造函数中初始化HIDL接口；
* 应用层调用setLightStates()来和LightsService通信；

```
* frameworks/base/services/core/java/com/android/server/lights/LightsService.java
  * public LightsService(Context context)
    * this(context, new VintfHalCache(), Looper.myLooper());
      * private static class VintfHalCache implements Supplier<ILights>, IBinder.DeathRecipient
        * public synchronized ILights get()
          * if (mInstance == null)
            * IBinder binder = Binder.allowBlocking(ServiceManager.waitForDeclaredService(ILights.DESCRIPTOR + "/default"));
            * if (binder != null)
              * mInstance = ILights.Stub.asInterface(binder);
          * return mInstance;
            * LightsServer跟Light HIDL通信的Binder接口就获取到了
      * LightsService(Context context, Supplier<ILights> service, Looper looper)
        * mVintfLights = service.get() != null ? service : null;
          * LightsServer跟Light HIDL通信的Binder接口
        * populateAvailableLights(context);
          * if (mVintfLights != null)
            * populateAvailableLightsFromAidl(context);
              * for (HwLight hwLight : mVintfLights.get().getLights())
                * mLightsById.put(hwLight.id, new LightImpl(context, hwLight));
                  * private final class LightImpl extends LogicalLight
                    * private LightImpl(Context context, HwLight hwLight)
                      * mHwLight = hwLight;
                    * public void setBrightness(float brightness)  // 没有走这条线路
                      * setBrightness(brightness, BRIGHTNESS_MODE_USER);
                        * setLightLocked(color, LIGHT_FLASH_NONE, 0, 0, brightnessMode);
                          * setLightUnchecked(color, mode, onMS, offMS, brightnessMode);
                            * if (mVintfLights != null)
                              * mVintfLights.get().setLightState(mHwLight.id, lightState);
  * public void setLightStates(IBinder token, int[] lightIds, LightState[] lightStates)
    * session.setRequest(lightIds[i], lightStates[i]);
      * mRequests.put(lightId, state);
    * invalidateLightStatesLocked();
      * for (int i = 0; i < mLightsById.size(); i++)
        * light.setColor(state.getColor());
          * private final class LightImpl extends LogicalLight 
            * public void setColor(int color)
              * setLightLocked(color, LIGHT_FLASH_NONE, 0, 0, 0);
                * setLightUnchecked(color, mode, onMS, offMS, brightnessMode);
                  * if (mVintfLights != null)
                    * mVintfLights.get().setLightState(mHwLight.id, lightState);
                      * 接下来这里就到了HIDL层去了
```

## display updatePowerState

* BacklightAdapter是内部类
* 在BacklightAdapter构造函数中获取LightsManager，相当于也就获取了LightsService的Binder Client；
* 获取背光控制的Session；

```java
// frameworks/base/services/core/java/com/android/server/display/LocalDisplayAdapter.java

private final class LocalDisplayDevice extends DisplayDevice {
     // ...省略
     LocalDisplayDevice(IBinder displayToken, long physicalDisplayId,
          SurfaceControl.StaticDisplayInfo staticDisplayInfo,
          SurfaceControl.DynamicDisplayInfo dynamicInfo,
          SurfaceControl.DesiredDisplayModeSpecs modeSpecs, boolean isDefaultDisplay) {
     super(LocalDisplayAdapter.this, displayToken, UNIQUE_ID_PREFIX + physicalDisplayId,
               getContext());
     mPhysicalDisplayId = physicalDisplayId;
     mIsDefaultDisplay = isDefaultDisplay;
     updateDisplayPropertiesLocked(staticDisplayInfo, dynamicInfo, modeSpecs);
     mSidekickInternal = LocalServices.getService(SidekickInternal.class);
     mBacklightAdapter = new BacklightAdapter(displayToken, isDefaultDisplay,             // <----
               mSurfaceControlProxy);
     mDisplayDeviceConfig = null;
     }
     // ...省略
}

static class BacklightAdapter {
    private final IBinder mDisplayToken;
    private final LogicalLight mBacklight;
    private final boolean mUseSurfaceControlBrightness;
    private final SurfaceControlProxy mSurfaceControlProxy;

    private boolean mForceSurfaceControl = false;

    /**
     * @param displayToken Token for display associated with this backlight.
     * @param isDefaultDisplay {@code true} if it is the default display.
     */
    BacklightAdapter(IBinder displayToken, boolean isDefaultDisplay,
            SurfaceControlProxy surfaceControlProxy) {
        mDisplayToken = displayToken;
        mSurfaceControlProxy = surfaceControlProxy;

        mUseSurfaceControlBrightness = mSurfaceControlProxy
                .getDisplayBrightnessSupport(mDisplayToken);

        if (!mUseSurfaceControlBrightness && isDefaultDisplay) {
            LightsManager lights = LocalServices.getService(LightsManager.class);
            mBacklight = lights.getLight(LightsManager.LIGHT_ID_BACKLIGHT);
        } else {
            mBacklight = null;
        }
    }

    // Set backlight within min and max backlight values
    void setBacklight(float sdrBacklight, float sdrNits, float backlight, float nits) {
        if (mUseSurfaceControlBrightness || mForceSurfaceControl) {
            if (BrightnessSynchronizer.floatEquals(
                    sdrBacklight, PowerManager.BRIGHTNESS_INVALID_FLOAT)) {
                mSurfaceControlProxy.setDisplayBrightness(mDisplayToken, backlight);
            } else {
                mSurfaceControlProxy.setDisplayBrightness(mDisplayToken, sdrBacklight, sdrNits,
                        backlight, nits);
            }
        } else if (mBacklight != null) {
            mBacklight.setBrightness(backlight);
        }
    }

    void setVrMode(boolean isVrModeEnabled) {
        if (mBacklight != null) {
            mBacklight.setVrMode(isVrModeEnabled);
        }
    }

    void setForceSurfaceControl(boolean forceSurfaceControl) {
        mForceSurfaceControl = forceSurfaceControl;
    }

    @Override
    public String toString() {
        return "BacklightAdapter [useSurfaceControl=" + mUseSurfaceControlBrightness
                + " (force_anyway? " + mForceSurfaceControl + ")"
                + ", backlight=" + mBacklight + "]";
    }
}
```

使用如下diff快速跟踪调用关系

```diff
diff --git a/frameworks/base/services/core/java/com/android/server/display/DisplayPowerState.java b/frameworks/base/services/core/java/com/android/server/display/DisplayPowerState.java
index 68ae8ecae86..f0c77ca43d5 100644
--- a/frameworks/base/services/core/java/com/android/server/display/DisplayPowerState.java
+++ b/frameworks/base/services/core/java/com/android/server/display/DisplayPowerState.java
@@ -51,7 +51,7 @@ import java.io.PrintWriter;
 final class DisplayPowerState {
     private static final String TAG = "DisplayPowerState";

-    private static boolean DEBUG = SystemProperties.getBoolean("dbg.dms.dps", false);
+    private static boolean DEBUG = true;
     private static String COUNTER_COLOR_FADE = "ColorFadeLevel";

     private final Handler mHandler;
@@ -151,6 +151,10 @@ final class DisplayPowerState {
      */
     public void setScreenBrightness(float brightness) {
         if (mScreenBrightness != brightness) {
+
+            Exception e = new Exception("this is a log");
+            e.printStackTrace();
+
             if (DEBUG) {
                 Slog.d(TAG, "setScreenBrightness: brightness=" + brightness);
             }
diff --git a/frameworks/base/services/core/java/com/android/server/display/LocalDisplayAdapter.java b/frameworks/base/services/core/java/com/android/server/display/LocalDisplayAdapter.java
index fdf8a441fbd..c293f19cd39 100644
--- a/frameworks/base/services/core/java/com/android/server/display/LocalDisplayAdapter.java
+++ b/frameworks/base/services/core/java/com/android/server/display/LocalDisplayAdapter.java
@@ -59,7 +59,7 @@ import java.util.Objects;
  */
 final class LocalDisplayAdapter extends DisplayAdapter {
     private static final String TAG = "LocalDisplayAdapter";
-    private static final boolean DEBUG = SystemProperties.getBoolean("dbg.dms.lda", false);
+    private static final boolean DEBUG = true;
     private static final boolean MTK_DEBUG = "eng".equals(Build.TYPE);

     private static final String UNIQUE_ID_PREFIX = "local:";
@@ -715,6 +715,9 @@ final class LocalDisplayAdapter extends DisplayAdapter {
                                     + ", brightness=" + brightness + ")");
                         }

+                        Exception e = new Exception("this is a log");
+                        e.printStackTrace();
+
                         Trace.traceBegin(Trace.TRACE_TAG_POWER, "setDisplayBrightness("
                                 + "id=" + physicalDisplayId + ", brightness=" + brightness + ")");
                         try {
```

diff对应的log输出如下

```
10-26 07:00:50.281  1151  1235 V DisplayPowerController: Brightness [0.39998955] reason changing to: 'temporary', previous reason: 'manual'.
10-26 07:00:50.281  1151  1235 D DisplayPowerState: Screen ready
10-26 07:00:50.284  1151  1235 W System.err: java.lang.Exception: this is a log
10-26 07:00:50.284  1151  1235 W System.err:    at com.android.server.display.DisplayPowerState.setScreenBrightness(DisplayPowerState.java:155)
10-26 07:00:50.284  1151  1235 W System.err:    at com.android.server.display.DisplayPowerState$2.setValue(DisplayPowerState.java:116)
10-26 07:00:50.284  1151  1235 W System.err:    at com.android.server.display.DisplayPowerState$2.setValue(DisplayPowerState.java:113)
10-26 07:00:50.284  1151  1235 W System.err:    at com.android.server.display.RampAnimator.animateTo(RampAnimator.java:71)
10-26 07:00:50.284  1151  1235 W System.err:    at com.android.server.display.DisplayPowerController.animateScreenBrightness(DisplayPowerController.java:1356)
10-26 07:00:50.285  1151  1235 W System.err:    at com.android.server.display.DisplayPowerController.updatePowerState(DisplayPowerController.java:1092)
10-26 07:00:50.285  1151  1235 W System.err:    at com.android.server.display.DisplayPowerController.access$600(DisplayPowerController.java:92)
10-26 07:00:50.285  1151  1235 W System.err:    at com.android.server.display.DisplayPowerController$DisplayControllerHandler.handleMessage(DisplayPowerController.java:1976)
10-26 07:00:50.285  1151  1235 W System.err:    at android.os.Handler.dispatchMessage(Handler.java:106)
10-26 07:00:50.285  1151  1235 W System.err:    at android.os.Looper.loop(Looper.java:223)
10-26 07:00:50.285  1151  1235 W System.err:    at android.os.HandlerThread.run(HandlerThread.java:67)
10-26 07:00:50.285  1151  1235 W System.err:    at com.android.server.ServiceThread.run(ServiceThread.java:44)
10-26 07:00:50.285  1151  1235 D DisplayPowerState: setScreenBrightness: brightness=0.33660948
10-26 07:00:50.287  1151  1235 D DisplayPowerState: Requesting new screen state: state=ON, backlight=0.33660948
10-26 07:00:50.287  1151  1235 D DisplayPowerState: Screen ready
10-26 07:00:50.287  1151  3438 D DisplayPowerState: Updating screen state: state=ON, backlight=0.33660948
10-26 07:00:50.287  1151  1235 D DisplayPowerState: Screen ready
10-26 07:00:50.287  1151  3438 D LocalDisplayAdapter: setDisplayBrightness(id=0, brightness=0.33660948)
10-26 07:00:50.287  1151  3438 W System.err: java.lang.Exception: this is a log
10-26 07:00:50.288  1151  3438 W System.err:    at com.android.server.display.LocalDisplayAdapter$LocalDisplayDevice$1.setDisplayBrightness(LocalDisplayAdapter.java:718)
10-26 07:00:50.288  1151  3438 W System.err:    at com.android.server.display.LocalDisplayAdapter$LocalDisplayDevice$1.run(LocalDisplayAdapter.java:648)
10-26 07:00:50.288  1151  3438 W System.err:    at com.android.server.display.DisplayManagerService.requestGlobalDisplayStateInternal(DisplayManagerService.java:592)
10-26 07:00:50.288  1151  3438 W System.err:    at com.android.server.display.DisplayManagerService.access$4700(DisplayManagerService.java:163)
10-26 07:00:50.288  1151  3438 W System.err:    at com.android.server.display.DisplayManagerService$LocalService$1.requestDisplayState(DisplayManagerService.java:2543)
10-26 07:00:50.288  1151  3438 W System.err:    at com.android.server.display.DisplayPowerState$PhotonicModulator.run(DisplayPowerState.java:445)
10-26 07:00:50.290   792   866 D AAL     : onBacklightChanged: 413/1023 -> 345/1023(phy:1381/4095)
10-26 07:00:50.290   526   526 D android.hardware.lights-service.mediatek: write 86 to /sys/class/leds/lcd-backlight/brightness, result: 0
10-26 07:00:50.291  1151  1235 D DisplayPowerState: Screen ready
```

updatePowerState()是怎么处理到背光的

```
* frameworks/base/services/core/java/com/android/server/display/DisplayManagerService.java
  * private void addDisplayPowerControllerLocked(LogicalDisplay display)
    * final DisplayPowerController displayPowerController = new DisplayPowerController(mContext, mDisplayPowerCallbacks, mPowerHandler, mSensorManager, mDisplayBlanker, display, mBrightnessTracker, brightnessSetting, () -> handleBrightnessChange(display));
      * frameworks/base/services/core/java/com/android/server/display/DisplayPowerController.java
        * public DisplayPowerController(Context context, DisplayPowerCallbacks callbacks, Handler handler, SensorManager sensorManager, DisplayBlanker blanker, LogicalDisplay logicalDisplay, BrightnessTracker brightnessTracker, BrightnessSetting brightnessSetting, Runnable onBrightnessChangeRunnable)
        * private void initialize(int displayState)
          * mPowerState = new DisplayPowerState(mBlanker, mColorFadeEnabled ? new ColorFade(mDisplayId) : null, mDisplayId, displayState);
            * frameworks/base/services/core/java/com/android/server/display/DisplayPowerState.java
              * public void setScreenBrightness(float brightness)
          * mScreenBrightnessRampAnimator = new DualRampAnimator<>(mPowerState, DisplayPowerState.SCREEN_BRIGHTNESS_FLOAT, DisplayPowerState.SCREEN_SDR_BRIGHTNESS_FLOAT);
            * DisplayPowerState.SCREEN_BRIGHTNESS_FLOAT
              * frameworks/base/services/core/java/com/android/server/display/DisplayPowerState.java
                * public static final FloatProperty<DisplayPowerState> SCREEN_BRIGHTNESS_FLOAT = new FloatProperty<DisplayPowerState>("screenBrightnessFloat")
                  * public void setValue(DisplayPowerState object, float value)
                    * object.setScreenBrightness(value);
                      * 下面详细跟，这里不继续往下跟了
          * mScreenBrightnessRampAnimator.setListener(mRampAnimatorListener);
        * private void updatePowerState()
          * animateScreenBrightness(animateValue, sdrAnimateValue, rampSpeed);
            * mScreenBrightnessRampAnimator.animateTo(target, sdrTarget, rate)
              * frameworks/base/services/core/java/com/android/server/display/RampAnimator.java
                * public boolean animateTo(float firstTarget, float secondTarget, float rate)
                  * final boolean firstRetval = mFirst.animateTo(firstTarget, rate);
                    * mProperty.setValue(mObject, target);
                      * DisplayPowerState.SCREEN_BRIGHTNESS_FLOAT
                        * frameworks/base/services/core/java/com/android/server/display/DisplayPowerState.java
                          * public static final FloatProperty<DisplayPowerState> SCREEN_BRIGHTNESS_FLOAT = new FloatProperty<DisplayPowerState>("screenBrightnessFloat")
                            * public void setValue(DisplayPowerState object, float value)
                              * object.setScreenBrightness(value);
                                * public void setScreenBrightness(float brightness)
                                  * mScreenBrightness = brightness;
                                  * scheduleScreenUpdate();
                                    * postScreenUpdateThreadSafe();
                                      * mHandler.post(mScreenUpdateRunnable);
                                        * private final Runnable mScreenUpdateRunnable = new Runnable()
                                          * float brightnessState = mScreenState != Display.STATE_OFF && mColorFadeLevel > 0f ? mScreenBrightness : PowerManager.BRIGHTNESS_OFF_FLOAT;
                                          * mPhotonicModulator.setState(mScreenState, brightnessState, sdrBrightnessState)
                                            * mLock.notifyAll();
                                              * 肯定有一个地方在等待这个notify释放
                                                * frameworks/base/services/core/java/com/android/server/display/DisplayPowerState.java
                                                  * public void run()
                                                    * mLock.wait();
                                                    * mBlanker.requestDisplayState(mDisplayId, state, brightnessState, sdrBrightnessState);
                                                      * frameworks/base/services/core/java/com/android/server/display/DisplayManagerService.java
                                                        * private final DisplayBlanker mDisplayBlanker = new DisplayBlanker()
                                                          * public synchronized void requestDisplayState(int displayId, int state, float brightness, float sdrBrightness)
                                                            * if (state != Display.STATE_OFF)
                                                              * requestDisplayStateInternal(displayId, state, brightness, sdrBrightness);
                                                                * runnable = updateDisplayStateLocked(mLogicalDisplayMapper.getDisplayLocked(displayId).getPrimaryDisplayDeviceLocked());
                                                                  * return device.requestDisplayStateLocked(state, brightnessPair.brightness, brightnessPair.sdrBrightness);
                                                                    * frameworks/base/services/core/java/com/android/server/display/LocalDisplayAdapter.java
                                                                      * public Runnable requestDisplayStateLocked(final int state, final float brightnessState, final float sdrBrightnessState)
                                                                        * return new Runnable() { }
                                                                          * public void run()
                                                                            * setDisplayBrightness(brightnessState, sdrBrightnessState);
                                                                              * mBacklightAdapter.setBacklight(sdrBacklight, sdrNits, backlight, nits);
                                                                                * mBacklight.setBrightness(backlight);
                                                                * runnable.run();
                  * final boolean secondRetval = mSecond.animateTo(secondTarget, rate);
          * Slog.v(TAG, "Brightness [" + brightnessState + "] reason changing to: '" + mBrightnessReasonTemp.toString(brightnessAdjustmentFlags) + "', previous reason: '" + mBrightnessReason + "'.");
```

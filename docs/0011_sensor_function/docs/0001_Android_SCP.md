# Android SCP

SCP(Sensor-hub Control Processor)，CHRE(Context Hub Runtime Environment), System Companion Processor (SCP)

## 参考文档

* [0241_Android_Sensors.md](0241_Android_Sensors.md)
  * [0241-DS6000-AZ6A-DMT-V4.0EN_Sensors_Programming_Guide.pdf](refers/0241-DS6000-AZ6A-DMT-V4.0EN_Sensors_Programming_Guide.pdf)
* [0334_Android_SCP_IPI.md](0334_Android_SCP_IPI.md)
* [0331-CS6873-BD9E-PGD-V1.2EN_MT6873_SCP_Development_Guide.pdf](refers/0331-CS6873-BD9E-PGD-V1.2EN_MT6873_SCP_Development_Guide.pdf)

## 代码

* vendor/mediatek/proprietary/tinysys/freertos/source

## Module驱动分析

### ACC Module驱动

* 一个传感器就是一个Module，一般用Overlay的形式，因为支持兼容多种厂家的芯片，Module只能是一种设备
* vendor/mediatek/proprietary/tinysys/freertos/source
  * project/CM4_A/mt6765/k62v1_64_bsp/cust/accGyro/cust_accGyro.c
    ```CPP
    #include "cust_accGyro.h"

    struct accGyro_hw cust_accGyro_hw[] __attribute__((section(".cust_accGyro"))) = {
    #ifdef CFG_LIS2HH12_SUPPORT
        {
            .name = "lis2hh12",
            .i2c_num = 0,
            .direction = 7,
            .i2c_addr = {0x68, 0},
            .eint_num = 12,
        },
    #endif

    };
    ```
  * middleware/contexthub/MEMS_Driver/accGyro/lis2hh12.c
    ```
    * int lis2hh12Init(void)
      * mTask.hw = get_cust_accGyro("lis2hh12");  获取客制化信息
    * MODULE_DECLARE(lis2hh12, SENS_TYPE_ACCEL, lis2hh12Init);
    ```
  * project/CM4_A/mt6765/platform/feature_config/chre.mk
    ```Makefile
    ifeq ($(CFG_LIS2HH12_SUPPORT),yes)
    C_FILES  += $(SENDRV_DIR)/accGyro/lis2hh12.c
    endif
    ```
  * project/CM4_A/mt6765/k62v1_64_bsp/ProjectConfig.mk
    * CFG_LIS2HH12_SUPPORT = yes

### Module驱动初始化调用

* 一个线程就是一个APP，一个APP会检查所有同类的Module，讲道理，最终只有一个Module工作
* vendor/mediatek/proprietary/tinysys/freertos/source/kernel/service/common/include/module_init.h
  ```CPP
  #ifndef _MODULE_INIT_H_
  #define _MODULE_INIT_H_
  #ifdef CFG_MODULE_INIT_SUPPORT
  
  struct module_init_s {
      int tags;
      int (*init_func)(void);
  };
  
  #define MODULE_DECLARE(name, tag, func) \
      struct module_init_s module_init_##name __attribute__((section(".module_init"))) = { \
          .tags = tag, \
          .init_func = func, \
      }
  
  extern void module_init(int tag);
  #endif
  #endif
  ```
* vendor/mediatek/proprietary/tinysys/freertos/source/kernel/service/common/src/module_init.c
  ```CPP
  #include "module.h"
  
  extern const struct module_init_s __module_start[];
  extern const struct module_init_s __module_end[];
  
  void module_init(int tag)
  {
      const struct module_init_s *mod;
  
      for (mod = __module_start; mod < __module_end; mod++) {
          if (mod->tags == tag) {
              if ((mod->init_func()) < 0)
                  continue;
              else
                  break;
          }
      }
  }
  ```


* vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/accGyro/accGyro.c
  ```CPP
  project/CM4_A/mt6765/k62v1_64_pax_eea/ProjectConfig.mk
  34:CFG_OVERLAY_INIT_SUPPORT = yes

  static bool startTask(uint32_t taskId)
  {
  // ...省略
  
  #ifndef CFG_OVERLAY_INIT_SUPPORT  
      /* Register sensor fsm last */
      module_init(SENS_TYPE_ACCEL);
  #else
      extern void accGyroOverlayRemap(void);
      accGyroOverlayRemap();  跑这里
  #endif
  
      return true;
  }

  INTERNAL_APP_INIT(APP_ID_MAKE(APP_ID_VENDOR_MTK, MTK_APP_ID_WRAP(SENS_TYPE_ACCEL, 0, 0)), 0, startTask, endTask, handleEvent);
  ```
* vendor/mediatek/proprietary/hardware/contexthub/firmware/inc/seos.h
  ```CPP
  #ifndef INTERNAL_APP_INIT
  #define INTERNAL_APP_INIT(_id, _ver, _init, _end, _event)                               \
  static const struct AppHdr         \
  __attribute__((used,section (".internal_app_init"))) mAppHdr = {      \
      .hdr.magic   = APP_HDR_MAGIC,                                                       \
      .hdr.fwVer   = APP_HDR_VER_CUR,                                                     \
      .hdr.fwFlags = FL_APP_HDR_INTERNAL | FL_APP_HDR_APPLICATION,                        \
      .hdr.appId   = (_id),                                                               \
      .hdr.appVer  = (_ver),                                                              \
      .hdr.payInfoType = LAYOUT_APP,                                                      \
      .vec.init    = (uint32_t)(_init),                                                   \
      .vec.end     = (uint32_t)(_end),                                                    \
      .vec.handle  = (uint32_t)(_event)                                                   \
  }
  
  
  #endif
  #ifndef APP_INIT
  #define APP_INIT(_ver, _init, _end, _event)                                             \
  extern const struct AppFuncs _mAppFuncs;                                                \
  const struct AppFuncs SET_EXTERNAL_APP_ATTRIBUTES(used, section (".app_init"),          \
  visibility("default")) _mAppFuncs = {                                                   \
      .init   = (_init),                                                                  \
      .end    = (_end),                                                                   \
      .handle = (_event)                                                                  \
  };                                                                                      \
  const uint32_t SET_EXTERNAL_APP_VERSION(used, section (".app_version"),                 \
  visibility("default")) _mAppVer = _ver
  #endif
  ```

### 启动APP线程

* vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/platform/system.ld.template
  ```ld
  .text :
      {
  
          . = ALIGN(4);
          *(.text)           /* .text sections (code) */
          *(.text*)          /* .text* sections (code) */
          @@Keep_Symbols@@
          *(.rodata)         /* .rodata sections (constants, strings, etc.) */
          *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
          . = ALIGN(8);
          __internal_app_start = ABSOLUTE(.);
          KEEP(*(.internal_app_init)) ;
          __internal_app_end = ABSOLUTE(.);
          . = ALIGN(4);
          KEEP(*(.scp_exported_funcs))
          *(.glue_7)         /* glue arm to thumb code */
          *(.glue_7t)        /* glue thumb to arm code */
          *(.eh_frame)
          __commands_start = .;
          KEEP(*(.commands))
          __commands_end = .;
  
  
          KEEP(*(.init))
          KEEP(*(.fini))
  
          . = ALIGN(4);
          _etext = .;        /* define a global symbols at end of code */
          _exit = .;
      } > RTOS
  ```
* 操作系统创建APP流程
  ```
  * vendor/mediatek/proprietary/hardware/contexthub/firmware/src/seos.c
    * void osMainInit(void)
      * osStartTasks();
        * for (i = 0, app = platGetInternalAppList(&nApps); i < nApps; i++, app++)
          * app = platGetInternalAppList(&nApps)
            * vendor/mediatek/proprietary/hardware/contexthub/firmware/links/plat/inc/plat.h
              * return &__internal_app_start;                 <-- 所有任务APP开始的地方
          * if (osStartApp(app))
        * status = osExtAppStartApps(APP_ID_ANY);
  ```

## Overlay驱动分析

![0331-SCP_Overlay.png](images/0331-SCP_Overlay.png)

## Overlay调用分析

* Purpose: customers need two materials. One type sensor may come from two different vendor. If putting these two driver in SCP SRAM to do auto detect, it consumes SRAM. So MTK solution locates two drivers in the DRAM and one driver is loaded when SCP boots.
  * 意思就是为了兼容多种同类型的芯片，最好采用这种模式；
* Overlay驱动加载流程
  ```
  * vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/accGyro/accGyro.c
    * static bool startTask(uint32_t taskId)
      * accGyroOverlayRemap();
        * vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k61v1_64_bsp/cust/overlay/overlay.c
          * void accGyroOverlayRemap(void)
            * ACC_GYRO_OVERLAY_REMAP(lis2hh12);
              * #define ACC_GYRO_OVERLAY_REMAP(name) SENSOR_OVERLAY_REMAP(name, accGyroEnd)
                * #define SENSOR_OVERLAY_REMAP(name, label)
                  * ptr = TINYSYS_OVERLAY_SELECT(name);
                   * vendor/mediatek/proprietary/tinysys/freertos/source/drivers/CM4_A/mt6765/overlay/inc/mtk_overlay_init.h
                     * #define TINYSYS_OVERLAY_SELECT(ovl_name)
                       * overlay_select_ret = tinysys_overlay_select(#ovl_name);
                         * vendor/mediatek/proprietary/tinysys/freertos/source/drivers/CM4_A/mt6765/overlay/src/mtk_overlay_init.c
                           * void * tinysys_overlay_select(char* name)
                             * for (mod = __overlay_struct_start; mod < __overlay_struct_end; mod++)
                               * PRINTF_E("mod=%p\n",mod);
                               * PRINTF_E("mod->name=%s\n",mod->name);
                               * if (!strcmp(mod->name,name))
                                 * ret = scp_copy_overlay(mod);
  ```
  * 每种APP是一个线程（task），每个线程要跑一种传感器，但是整个系统包含多个传感器驱动，需要一个一个检查，直到可以运行的那个
* vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/platform/system.ld.template
  ```
      /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
      {
          . = ALIGN(4);
          __data_start = ABSOLUTE(.);
          _sdata = .;        /* create a global symbol at data start */
          *(.data)           /* .data sections */
          *(.data*)          /* .data* sections */
  
          . = ALIGN(4);
          __module_start = .;
          KEEP(*(.module_init*))
          __module_end = .;
          . = ALIGN(4);
  #ifdef CFG_OVERLAY_INIT_SUPPORT
          __overlay_struct_start = .;
          KEEP(*(.overlay_init*))
          __overlay_struct_end = .;
          . = ALIGN(4);
  #endif
          __cust_alsps_start = .;
          KEEP(*(.cust_alsps*))
          __cust_alsps_end = .;
          . = ALIGN(4);
          __cust_accGyro_start = .;
          KEEP(*(.cust_accGyro*))
          __cust_accGyro_end = .;
          . = ALIGN(4);
          __cust_baro_start = .;
          KEEP(*(.cust_baro*))
          __cust_baro_end = .;
          . = ALIGN(4);
          __cust_mag_start = .;
          KEEP(*(.cust_mag*))
          __cust_mag_end = .;
          . = ALIGN(4);
          __cust_sar_start = .;
          KEEP(*(.cust_sar*))
          __cust_sar_end = .;
          . = ALIGN(4);
          _edata = .;        /* define a global symbol at data end */
          __data_end = ABSOLUTE(.);
      } > RTOS
  ```
  * KEEP(\*(.overlay_init\*))
* vendor/mediatek/proprietary/tinysys/freertos/source/drivers/CM4_A/mt6765/overlay/inc/mtk_overlay_init.h
  ```CPP
  #define OVERLAY_DECLARE(ovl_name, ovl_section, ovl_data) \
      struct overlay_init_s overlay_init_##ovl_name __attribute__((section(".overlay_init"))) = { \
          .overlay_section = ovl_section, \
          .data = ovl_data, \
      }
  
  #define OVERLAY_INIT(ovl_name) do { \
      extern char __load_start_##ovl_name; \
      extern char __load_stop_##ovl_name; \
      extern struct overlay_init_s overlay_init_##ovl_name; \
      overlay_init_##ovl_name.name = #ovl_name; \
      overlay_init_##ovl_name.overlay_load_start = &__load_start_##ovl_name; \
      overlay_init_##ovl_name.overlay_load_end = &__load_stop_##ovl_name; \
      } while(0)
  ```
* vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/accGyro/lis2hh12.c
  ```CPP
  #ifndef CFG_OVERLAY_INIT_SUPPORT
  MODULE_DECLARE(lis2hh12, SENS_TYPE_ACCEL, lis2hh12Init);
  #else
  #include "mtk_overlay_init.h"
  OVERLAY_DECLARE(lis2hh12, OVERLAY_ID_ACCGYRO, lis2hh12Init);
  #endif
  ```
* vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/ProjectConfig.mk
  ```
  CFG_MTK_VOW_SUPPORT = no
  
  CFG_CHRE_SUPPORT = yes
  CFG_CONTEXTHUB_FW_SUPPORT = yes
  CFG_ALSPS_SUPPORT = yes
  CFG_CM36558_SUPPORT = yes
  CFG_MAGNETOMETER_SUPPORT = yes
  CFG_MAG_CALIBRATION_IN_AP = yes
  CFG_AKM09918_SUPPORT = yes
  CFG_BAROMETER_SUPPORT = yes
  CFG_BMP280_SUPPORT = yes
  CFG_ACCGYRO_SUPPORT = yes
  CFG_LIS2HH12_SUPPORT = yes
  CFG_STEP_COUNTER_SUPPORT = yes
  CFG_STEP_DETECTOR_SUPPORT = yes
  CFG_SIGNIFICANT_MOTION_SUPPORT = yes
  CFG_WAKEUP_SUPPORT = yes
  CFG_LIFT_SUPPORT = yes
  CFG_LIFT_PUTDOWN_SUPPORT = yes
  CFG_MOTION_SUPPORT = yes
  CFG_WIN_ORIENTATION_SUPPORT = yes
  CFG_OVERLAY_INIT_SUPPORT = yes
  CFG_OVERLAY_DEBUG_SUPPORT = yes
  ```
  * CFG_OVERLAY_INIT_SUPPORT = yes

## Overlay Code存储分析

* ld配置：vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/project.ld
  ```
  /*SCP sram size*/
  _SCP_SRAM_SIZE = 0x40000;
  _SCP_RTOS_START = 0x800;
  /* reserved memory areas*/
  _SCP_IPC_SHARE_BUFFER_ADDR = _SCP_RTOS_START - 0x240;
  _CNN_TO_SCP_SHARE_BUFFER_ADDR = _SCP_IPC_SHARE_BUFFER_ADDR - @@CFG_CNN_TO_SCP_BUF_SIZE@@;
  _SCP_TO_CNN_SHARE_BUFFER_ADDR = _CNN_TO_SCP_SHARE_BUFFER_ADDR - @@CFG_SCP_TO_CNN_BUF_SIZE@@;
  
  /* Specify the memory areas */
  MEMORY {
          LOADER(rwx)     : ORIGIN = 0x00000000, LENGTH =    2K   /*loader for recovery use*/
  #ifdef CFG_CACHE_SUPPORT
          RTOS(rwx)       : ORIGIN = 0x00000800, LENGTH =  238K   /* 256k - loader(2K) - cache(8K+8K) */
          DRAM(rwx)       : ORIGIN = 0x00080000, LENGTH = 1024K   /* shift overlay section if enlarge cache region is required */
  #else
          RTOS(rwx)       : ORIGIN = 0x00000800, LENGTH =  254K   /* 256k - loader(2K) */
  #endif
      }
  ```
* vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/platform/system.ld.template
  ```ld
  #include "tinysys_config.h"
  #include "overlay_sensor.h"
  #ifdef CFG_CACHE_SUPPORT
  #include "cache_ld.h"
  #endif
  
  // # ...省略
  
  #ifdef CFG_OVERLAY_INIT_SUPPORT
      __overlay_ws_01_start_lda = __next_start;
      /* overlay image define */
  OVERLAY __overlay_ws_01_start :
      NOCROSSREFS AT(__overlay_ws_01_start_lda)
      {
          OVERLAY0
      }
      __overlay_ws_01_end = ABSOLUTE(.);
  OVERLAY __overlay_ws_01_end :
      NOCROSSREFS
      {
          OVERLAY1
      }
      __overlay_ws_02_end = ABSOLUTE(.);
  OVERLAY __overlay_ws_02_end :
      NOCROSSREFS
      {
          OVERLAY2
      }
      __overlay_ws_03_end = ABSOLUTE(.);
  OVERLAY __overlay_ws_03_end :
      NOCROSSREFS
      {
          OVERLAY3
      }
      __overlay_ws_04_end = ABSOLUTE(.);
  OVERLAY __overlay_ws_04_end :
      NOCROSSREFS
      {
          OVERLAY4
      }
      __overlay_ws_05_end = ABSOLUTE(.);
  OVERLAY __overlay_ws_05_end :
      NOCROSSREFS
      {
          OVERLAY5
      }
      __overlay_ws_06_end = ABSOLUTE(.);
  OVERLAY __overlay_ws_06_end :
      NOCROSSREFS
      {
          OVERLAY6
      }
      __overlay_ws_07_end = ABSOLUTE(.);
  OVERLAY __overlay_ws_07_end :
      NOCROSSREFS
      {
          OVERLAY7
      }
      __overlay_ws_08_end = ABSOLUTE(.);
  OVERLAY __overlay_ws_08_end :
      NOCROSSREFS
      {
          OVERLAY8
      }
      __overlay_ws_09_end = ABSOLUTE(.);
  OVERLAY __overlay_ws_09_end :
      NOCROSSREFS
      {
          OVERLAY9
      }
      __overlay_ws_10_end = ABSOLUTE(.);
      /*overlay image end */
  #endif
  ```
  * OVERLAY0
* OVERLAY0解析: vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/inc/overlay_sensor.h
  ```C
  #define OVERLAY_SECTION_TEXT (.text* .data* .rodata* .bss*)
  #define OVERLAY_ONE_OBJECT(tag, file) .tag { *file.o OVERLAY_SECTION_TEXT }
  #define OVERLAY_LIB_OBJECT(tag, lib, file) .tag { lib.a: file.o OVERLAY_SECTION_TEXT }
  #define OVERLAY_THREE_OBJECT(tag, file1, file2, file3) \
          .tag { *file1.o OVERLAY_SECTION_TEXT } \
          .tag { *file2.o OVERLAY_SECTION_TEXT } \
          .tag { *file3.o OVERLAY_SECTION_TEXT }
  #define OVERLAY_SIX_OBJECT(tag, file1, file2, file3, file4, file5, file6, file7) \
          .tag { *file1.o OVERLAY_SECTION_TEXT } \
          .tag { *file2.o OVERLAY_SECTION_TEXT } \
          .tag { *file3.o OVERLAY_SECTION_TEXT } \
          .tag { *file4.o OVERLAY_SECTION_TEXT } \
          .tag { *file5.o OVERLAY_SECTION_TEXT } \
          .tag { *file6.o OVERLAY_SECTION_TEXT } \
          .tag { *file7.o OVERLAY_SECTION_TEXT }
  
  
  /*****************************************************************************
  * Overlay0: ACCGYRO
  *****************************************************************************/
  #define OVERLAY_SECTION_ACCGYRO                    \
      OVERLAY_ONE_OBJECT(lis2hh12, lis2hh12)         \
      OVERLAY_ONE_OBJECT(bmi160, bmi160)
  
  #ifdef OVERLAY_SECTION_ACCGYRO
  #define OVERLAY0 OVERLAY_SECTION_ACCGYRO
  #else
  #define OVERLAY0
  #endif  // OVERLAY_SECTION_ACCGYRO
  ```
* 未添加代码区error
  ```
  out/target/product/k62v1_64_bsp/obj/TINYSYS_OBJ/tinysys-scp_intermediates/freertos/source/CM4_A/project/CM4_A/mt6765/k62v1_64_bsp/cust/overlay/overlay.o: In function `accGyroOverlayRemap':
  vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/overlay/overlay.c:94: undefined reference to `__load_start_bmi160'
  vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/overlay/overlay.c:94: undefined reference to `__load_stop_bmi160'
  collect2: error: ld returned 1 exit status
  make: *** [build/config_cm4.mk:155: out/target/product/k62v1_64_bsp/obj/TINYSYS_OBJ/tinysys-scp_intermediates/freertos/source/CM4_A/tinysys-scp-CM4_A.elf] Error 1
  make: Leaving directory 'vendor/mediatek/proprietary/tinysys/freertos/source'
  [ 97% 1037/1063] //vendor/mediatek/proprietary/frameworks/ml/nn/runtime:libneuropilot strip libneuropilot.so
  [ 97% 1038/1063] //vendor/mediatek/proprietary/frameworks/ml/nn/runtime:libneuropilot strip libneuropilot.so [arm]
  ninja: build stopped: subcommand failed.
  17:47:40 ninja failed with: exit status 1
  ```
  * OVERLAY_ONE_OBJECT(bmi160, bmi160)
* Overlay声明：vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/overlay/overlay.c
  ```
  void accGyroOverlayRemap(void)
  {
  ACC_GYRO_OVERLAY_REMAP_START
      ACC_GYRO_OVERLAY_REMAP(lis2hh12);
      ACC_GYRO_OVERLAY_REMAP(bmi160);
  ACC_GYRO_OVERLAY_REMAP_END
  
      return;
  }
  ```
  * 前面是将代码放到Overlay代码区，这里是声明代码引用的地方

### Overlay代码超出限制

* error
  ```
  SCP: accGyro(27347>27000) is out of memory limitation
  ```
  * 代码块大小限制: vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/platform/Setting.ini
    ```
    accGyro:27000
    ```
    * accGyro:28000
* 上面accGyro只是一个bmi160就超出了默认代码大小，如果有很多个，这里需要另外调整；

### Overlay bmi160 spi log

本来以为是I2C驱动，结果一跑，是SPI驱动，尴尬，不过算是验证了跑通整个修改流程

```
[0.005]mod=0x259b4
[0.005]mod->name=bmi160
[0.005]ovl select
[0.005]&__overlay_struct_start=0x259b4
[0.005]&__overlay_struct_end=0x25a2c
[0.005]name =bmi160
[0.005]start=0x80000
[0.005]stop =0x83718
[0.005]size =14104
[0.006]section=0
[0.006]data=0x36df5
[0.006]scp_region_info:
[0.006]ap_loader_start=9f900000
[0.006]ap_loader_size=438
[0.006]ap_firmware_start=9f900800
[0.006]ap_firmware_size=3b800
[0.006]load from dram addr=ff980000
[0.006]load to sec01 addr=0x366e0
[0.011]ovl select done
[0.011]_ovly_table[0].vma = 366e0
[0.011]_ovly_table[0].size = 3718
[0.011]_ovly_table[0].lma = 80000
[0.011]_ovly_table[0].mapped = 1
[0.011]bmi160Init
[0.011]acc spi_num: 0
[0.011]acc map[0]:1, map[1]:0, map[2]:2, sign[0]:-1, sign[1]:-1, sign[2]:-1
[0.011]ipi-31 dump spm state, 0x1
[0.011]ipi busy,owner:31
[0.011]bmi160 overlay remap fail
```

### 修改添加bmi160 SPI驱动

驱动已经存在，需要添加驱动操作：

* 打开驱动宏；
* 配置驱动参数；
* 声明Overlay检索代码；
* 将代码放入Overlay代码区；
* Overlay代码代码限制修改，没有超出就不用修改；

```diff
diff --git a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/ProjectConfig.mk b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/ProjectConfig.mk
index 54182fc04a5..0a472b505ba 100755
--- a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/ProjectConfig.mk
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/ProjectConfig.mk
@@ -22,7 +22,8 @@ CFG_AKM09918_SUPPORT = yes
 CFG_BAROMETER_SUPPORT = yes
 CFG_BMP280_SUPPORT = yes
 CFG_ACCGYRO_SUPPORT = yes
-CFG_LIS2HH12_SUPPORT = yes
+#CFG_LIS2HH12_SUPPORT = yes
+CFG_BMI160_SUPPORT = yes
 CFG_STEP_COUNTER_SUPPORT = yes
 CFG_STEP_DETECTOR_SUPPORT = yes
 CFG_SIGNIFICANT_MOTION_SUPPORT = yes
diff --git a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/accGyro/cust_accGyro.c b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/accGyro/cust_accGyro.c
index ad054e666d8..c78ffdb01a3 100755
--- a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/accGyro/cust_accGyro.c
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/accGyro/cust_accGyro.c
@@ -11,4 +11,14 @@ struct accGyro_hw cust_accGyro_hw[] __attribute__((section(".cust_accGyro"))) =
     },
 #endif

+#ifdef CFG_BMI160_SUPPORT
+    {
+        .name = "bmi160",
+        .i2c_num = 0,
+        .direction = 7,
+        .i2c_addr = {0x68, 0},
+        .eint_num = 12,
+    },
+#endif
+
 };
diff --git a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/overlay/overlay.c b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/overlay/overlay.c
index 37ea139ab77..498030479f3 100755
--- a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/overlay/overlay.c
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/overlay/overlay.c
@@ -90,7 +90,7 @@ do {                                                           \
 void accGyroOverlayRemap(void)
 {
 ACC_GYRO_OVERLAY_REMAP_START
-    ACC_GYRO_OVERLAY_REMAP(lis2hh12);
+    ACC_GYRO_OVERLAY_REMAP(bmi160);
 ACC_GYRO_OVERLAY_REMAP_END

     return;
diff --git a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/inc/overlay_sensor.h b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/inc/overlay_sensor.h
index bfffcfc54fb..c28891cedc6 100755
--- a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/inc/overlay_sensor.h
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/inc/overlay_sensor.h
@@ -24,7 +24,7 @@
 * Overlay0: ACCGYRO
 *****************************************************************************/
 #define OVERLAY_SECTION_ACCGYRO                    \
-    OVERLAY_ONE_OBJECT(lis2hh12, lis2hh12)
+    OVERLAY_ONE_OBJECT(bmi160, bmi160)

 #ifdef OVERLAY_SECTION_ACCGYRO
 #define OVERLAY0 OVERLAY_SECTION_ACCGYRO
diff --git a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/platform/Setting.ini b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/platform/Setting.ini
index 26955e1723b..0ca74476807 100644
--- a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/platform/Setting.ini
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/platform/Setting.ini
@@ -106,7 +106,7 @@ SPI:7470
 Timer:2000
 UART:400
 WDT:450
-accGyro:27000
+accGyro:28000
 activity:31000
 activity-no-baro:10
 alsps:18000

```

### mmc5603调试

* 步骤和上面bmi160是一样的；
* 系统默认有mmc5603，但是貌似比较复杂，默认MT6762没有配置好，没有FAE协助，配置环境复杂；
  * vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/mmc5603.c
  * vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/mmc5603.h
* 问FAE要了驱动，貌似校正库不在SCP这边，所以配置编译环境比较容易，如下是运行log；
  ```
  [0.022]mod=0x25a04
  [0.022]mod->name=bmi160
  [0.022]mod=0x25a18
  [0.022]mod->name=cm36558
  [0.022]mod=0x25a2c
  [0.022]mod->name=fakeAls
  [0.022]mod=0x25a40
  [0.022]mod->name=fakePs
  [0.022]mod=0x25a54
  [0.022]mod->name=bmp280
  [0.022]mod=0x25a68
  [0.022]mod->name=mmc5603
  [0.022]ovl select
  [0.022]&__overlay_struct_start=0x25a04
  [0.022]&__overlay_struct_end=0x25a7c
  [0.022]name =mmc5603
  [0.022]start=0x83718
  [0.022]stop =0x845c5
  [0.022]size =3757
  [0.023]section=1
  [0.023]data=0x39e95
  [0.023]scp_region_info:
  [0.023]ap_loader_start=9f900000
  [0.023]ap_loader_size=438
  [0.023]ap_firmware_start=9f900800
  [0.023]ap_firmware_size=3b800
  [0.023]load from dram addr=ff983718
  [0.024]load to sec02 addr=0x39e48
  [0.024]ovl select done
  [0.025]_ovly_table[1].vma = 39e48
  [0.025]_ovly_table[1].size = ead
  [0.025]_ovly_table[1].lma = 83718
  [0.025]_ovly_table[1].mapped = 1
  [0.025]driver version : 1.00.20176, mag i2c_num: 0, i2c_addr: 0x30
  [0.025]mag map[0]:0, map[1]:1, map[2]:2, sign[0]:-1, sign[1]:1, sign[2]:-1
  [0.025][I2C-SCP] 809: best_mul 12 sample_div 2 step_div 6 min_div 11
  [0.025][I2C-SCP] 809: best_mul 13 sample_div 1 step_div 13 min_div 13
  [0.025]mmc5603Init read id:0x10 suceess!!!
  [0.025]mmc5603: auto detect success
  [0.025]mmc5603 overlay remap success
  ```
* 不过内核阶段也可以注册成功，讲道理不应该，kernel log：
  ```
  <7>[    8.512926] .(4)[1:swapper/0]<MAG> [name:mag&]mag_init
  <7>[    8.512978] .(4)[1:swapper/0]<MAG> [name:mag&]mag_probe ++++!!
  <7>[    8.513077] .(4)[1:swapper/0]<MAG> [name:mag&]mag_context_alloc_object start
  <7>[    8.519145] .(1)[262:dc_trim_thread][name:mtk_soc_codec_6357&]open_trim_bufferhardware_withspk(), enable 1, buffer_on 1
  <7>[    8.519159] .(1)[262:dc_trim_thread][name:mtk_soc_codec_6357&]TurnOnDacPower()
  <7>[    8.519194] .(1)[262:dc_trim_thread][name:mtk_soc_codec_6357&]-PMIC DCXO XO_AUDIO_EN_M enable, DCXO_CW14 = 0xa2b5
  <7>[    8.519929] .(1)[262:dc_trim_thread][name:mtk_soc_codec_6357&]setDlMtkifSrc(), enable = 1, freq = 48000
  <7>[    8.522723] .(4)[1:swapper/0]<MAG> [name:mag&]mag_context_alloc_object end
  <7>[    8.522782] .(4)[1:swapper/0]<MAG> [name:mag&]mag_real_driver_init start
  <7>[    8.522833] .(4)[1:swapper/0]<MAG> [name:mag&] i=0
  <7>[    8.522885] .(4)[1:swapper/0]<MAG> [name:mag&] mag try to init driver mmc5603x
  <4>[    8.525022] .(4)[1:swapper/0][name:mmc5603x&]<<-MMC5603X INFO->> mmc5603x_i2c_probe: enter probe,driver version=V80.97.00.10
  <4>[    8.527271] .(4)[1:swapper/0][name:mmc5603x&]<<-MMC5603X INFO->> direction =0
  <4>[    8.548931] .(5)[1:swapper/0][name:mmc5603x&]<<-MMC5603X INFO->> [mmc5603x] product_id[0] = 16
  <7>[    8.565476] .(5)[1:swapper/0]<MAG> [name:mag&]mag register data path div: 1024
  <4>[    8.565544] .(5)[1:swapper/0][name:mmc5603x&]<<-MMC5603X INFO->> mmc5603X IIC probe successful !
  <6>[    8.566963] .(5)[1:swapper/0][name:bootprof&][name:bootprof&]BOOTPROF:      8566.945635:probe: probe=i2c_device_probe drv=mmc5603x(mmc5603x_i2c_driver)    42.043539ms
  <7>[    8.571671] .(5)[1:swapper/0]<MAG> [name:mag&] mag real driver mmc5603x probe ok
  <7>[    8.571686] .(5)[1:swapper/0]<MAG> [name:mag&]mag_probe OK !!
  <7>[    8.571708] .(5)[1:swapper/0]initcall mag_init+0x0/0x674 returned 0 after 57408 usecs
  ```

### SCP使能传感器流程

暂时找不到AP如何发信息过来的，主要是目前我们的系统没有配置开启SensorHub，所以没法跟踪分析

```
* vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/magnetometer.c
  * static void handleEvent(uint32_t evtType, const void* evtData)
    * case EVT_SENSOR_EVENT
      * handleSensorEvent(evtData);
        * case CHIP_SAMPLING_DONE:
          * processPendingEvt();
            * if (mTask.pendingConfig)
              * else if (mTask.pendConfig.enable == 1 && !mTask.magNowOn)
                * sensorOpsMag.sensorPower(true, NULL);
                  * static const struct SensorOps sensorOpsMag
                    * DEC_OPS_CFG_SELFTEST(sensorPowerMag, sensorFirmwareMag, sensorRateMag, sensorFlushMag, sensorCfgMag, sensorSelfTestMag),
                      * static bool sensorPowerMag(bool on, void *cookie)
                        * magPowerCalculate(on);
                          * setMagHwEnable(on);
                            * sensorFsmRunState(&dataInfo, &mTask.fsm, (const void *)CHIP_ENABLE, &i2cCallback, &spiCallback);
                              * vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/sensorFsm.c
                                * int sensorFsmRunState(struct transferDataInfo *dataInfo, struct fsmCurrInfo *fsmInfo, const void *state, I2cCallbackF i2cCallBack, SpiCbkF spiCallBack)
                                  * cmd = sensorFsmFindCmd(fsmInfo->mSensorFsm, fsmInfo->mSensorFsmSize, state);
                                    * vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/mmc5603.c
                                      * static struct sensorFsm mmc5603Fsm[]
                                        * sensorFsmCmd(STATE_ENABLE, STATE_ENABLE_DONE, mmc5603Enable)
                                          * osLog(LOG_INFO, "%s :  MMC5603 enable mag\n", __func__);
```

## 默认scp log分析

驱动基本上都是采用overlay的形式，需要用的时候才拷贝到单片机SRAM中运行，每次都要轮询所有的驱动，直到找到设备驱动

```
----- timezone:GMT
isable	no	inactive
[0.005]62		8(8)		0	0	disable	no	inactive
[0.005]63		8(8)		0	0	disable	no	inactive
[0.005]in project_init
[0.005]ram dump: 0x369bc
[0.005]send ready IPI
[0.005]log_info dram=36598 buf=365bc start=365b8 end=365b4 maxlen=400
[0.006]Logger init done
[0.006]SCP mtk_wdt_init: WDT_CFGREG=0x800fffff!!
[0.006]Scheduler start...
[0.006]task:IDLE was created(traceTASK_CREATE()) 
[0.006] task:Tmr S was created(traceTASK_CREATE()) 
[0.006]SEOS Initializing
[0.006][MPEKlib]: MPE_CAL_A_VER_18082801
[0.006]mod=0x25c8c
[0.006]mod->name=lis2hh12
[0.006]ovl select
[0.006]&__overlay_struct_start=0x25c8c
[0.006]&__overlay_struct_end=0x25d04
[0.006]name =lis2hh12
[0.006]start=0x80000
[0.006]stop =0x82291
[0.006]size =8849
[0.006]section=0
[0.006]data=0x36d31
[0.006]scp_region_info:
[0.006]ap_loader_start=9f900000
[0.006]ap_loader_size=438
[0.006]ap_firmware_start=9f900800
[0.006]ap_firmware_size=3b800
[0.006]load from dram addr=ff980000
[0.007]load to sec01 addr=0x36a10
[0.010]ovl select done
[0.010]_ovly_table[0].vma = 36a10
[0.010]_ovly_table[0].size = 2291
[0.010]_ovly_table[0].lma = 80000
[0.010]_ovly_table[0].mapped = 1
[0.010]acc spi_num: 0
[0.010]acc map[0]:1, map[1]:0, map[2]:2, sign[0]:-1, sign[1]:-1, sign[2]:-1
[0.010]lis2hh12 overlay remap fail
[0.010]mod=0x25c8c
[0.010]mod->name=lis2hh12
[0.010]mod=0x25ca0
[0.010]mod->name=cm36558
[0.010]ovl select
[0.010]&__overlay_struct_start=0x25c8c
[0.010]&__ov
[0.013][I2C-SCP] 645: i2c0: err = -121
[0.013][I2C-SCP] 735: i2c0:addr=0x51,ACK error=0x2,retry=0
[0.013][I2C-SCP] 680: I2c0:a2,0,1,3a,1
[I2C-SCrt=9f900800
[0.015]ap_firmware_size=3b800
[0.015]load from dram addr=ff985718
[0.015]load to sec05 addr=0x3b00c
[0.015]ovl select done
[0.015]_ovly_table[4].vma = 3b00c
[0.015]_ovly_table[4].size = 5c
[0.015]_ovly_table[4].lma = 85718
[0.015]_ovly_table[4].mapped = 1
[0.015]fakePs: auto detect success!
[0.015]fakePs overlay remap success
[0.015]mod=0x25c8c
[0.015]mod->name=lis2hh12
[0.015]mod=0x25ca0
[0.015]mod->name=cm36558
[0.015]mod=0x25cb4
[0.015]mod->name=fakeAls
[0.015]mod=0x25cc8
[0.015]mod->name=fakePs
[0.015]mod=0x25cdc
[0.015]mod->name=bmp280
[0.015]ovl select
[0.015]&__overlay_struct_start=0x25c8c
[0.015]&__overlay_struct_end=0x25d04
[0.015]name =bmp280
[0.015]start=0x84d70
[0.015]stop =0x85718
[0.015]size =2472
[0.015]section=3
[0.015]data=0x3a689
[0.015]scp_region_info:
[0.015]ap_loader_start=9f900000
[0.015]ap_loader_size=438
[0.015]ap_firmware_start=9f900800
[0.015]ap_firmware_size=3b800
[0.015]load from dram addr=ff984d70
[0.015]load to sec04 addr=0x3a664
1
[I2C-SCP] 2,a,5,d,0,1
[I2C-SCP] 3,0,0,0,1,1
[I2C-SCP] 6
[0.017][I2C-SCP] 645: i2c0: err = -121
[0.017][I2C-SCP] 735: i2c0:addr=0x77,ACK error=0x2,retry=0
[0.017][I2C-SCP] 680: I2c0:ee,0,1,3a,1
[I2C-SCP] 2,a,5,d,0,1
[I2C-SCP] 3,0,0,0,1,1
[I2C-SCP] 6
[0.017][I2C-SCP] 645: i2c0: err = -121
[0.017]bmp280 overlay remap fail
[0.017]mod=0x25c8c
[0.017]mod->name=lis2hh12
[0.017]mod=0x25ca0
[0.017]mod->name=cm36558
[0.017]mod=0x25cb4
[0.017]mod->name=fakeAls
[0.017]mod=0x25cc8
[0.017]mod->name=fakePs
[0.017]mod=0x25cdc
[0.017]mod->name=bmp280
[0.017]mod=0x25cf0
[0.017]mod->name=akm09918
[0.017]ovl select
[0.017]&__overlay_struct_start=0x25c8c
[0.017]&__overlay_struct_end=0x25d04
[0.017]name =akm09918
[0.017]start=0x82291
[0.017]stop =0x82b38
[0.0_info:
[0.017]ap_loader_start=9f900000
[0.017]ap_loader_size=438
[0.017]ap_firmware_start=9f900800
[0.017]ap_firmware_size=3b800
[0.017]load from dram addr=ff982291
[0.018]load to sec02 addr=0x38ca1
[0.019]ovl select done
[0.019]_ovly_table[1].vma = 38ca1
[0.019]_ovly_table[1].size = 8a7
[0.019]_ovly_table[1].lma = 82291
[0.019]_ovly_table[1].mapped = 1
[0.019]mag i2c_num: 0, i2c_addr: 0xc
[0.019]mag map[0]:0, map[1]:1, map[2]:2, sign[0]:-1, sign[1]:1, sign[2]:-1
[0.019][I2C-SCP] 809: best_mul 12 sample_div 2 step_div 6 min_div 11
[0.019][I2C-SCP] 809: best_mul 13 sample_div 1 step_div 13 min_div 13
[0.019][I2C-SCP] 735: i2c0:addr=0xc,ACK error=0x2,retry=0
[0.019][I2C-SCP] 680: I2c0:18,0,1,3a,1
[I2C-SCP] 2,a,5,d,0,1
[I2C-SCP] a3,0,0,0,1,2
[I2C-SCP] 6
[0.019][I2C-SCP] 645: i2c0: err = -121
[0.019][I2C-SCP] 735: i2c0:addr=0xc,ACK error=0x2,retry=0
[0.019][I2C-SCP] 680: I2c0:18,0,1,3a,1
[I2C-SCP] 2,a,5,d,0,1
[I2C-SCP] 3,0,0,0,1,2
[I2C-SCP] 6
[0.019][I2C-SCP] 645: i2c0: err = -121
[0.019][I2C-SCP] 735: i2c0:addr=0xc,ACK error=0x3,retry=0
[0.019][I2C-SCP] 680: I2c0:18,0,0,3a,1
[I2C-SCP] 2,a,5,d,0,1
[I2C-SCP] 3,0,0,0,1,2
[I2C-SCP] 6
[0.020][I2C-SCP] 645: i2c0: err = -121
[0.020]akm09918 overlay remap fail
[0.020]contexthub_fw_start tid: 268
[0.021]alsps: app start
[0.021]initSensors: Proximity not ready!
[0.021]LIFT EVT_APP_START
[0.021]TILT EVT_APP_START
[0.021]STEP_RECOGNITION EVT_APP_START
[0.021]alsPs: init done
```
  * [0.013][I2C-SCP] 735: i2c0:addr=0x51,ACK error=0x2,retry=0
  * [0.017][I2C-SCP] 735: i2c0:addr=0x77,ACK error=0x2,retry=0
  * [0.019][I2C-SCP] 735: i2c0:addr=0xc,ACK error=0x3,retry=0
  * 用逻辑分析仪看I2C1数据，可以抓到上面的通信数据，也就是说这个bus，Linux系统和单片机系统都可以使用，因为目前我们传感器驱动在Linux系统这边；

## Linux Driver调试

* menuconfig
  * CONFIG_I2C_CHARDEV=y
* https://github.com/ZengjfOS/Android/tree/i2c-tools
  * i2cdetect -y 1

## SCP I2C Master

* vendor/mediatek/proprietary/tinysys/freertos/source/drivers/CM4_A/mt6765/i2c/src/i2cchre-plat.c
  ```CPP
  int i2cMasterRequest(uint32_t busId, uint32_t speedInHz) {
      if (busId >= ARRAY_SIZE(mMtI2cDevs))
          return -EINVAL;
  
      struct MtI2cDev *pdev = &mMtI2cDevs[busId];
      struct I2cMtState *state = &pdev->state;
  
      I2CDBG("%s: %d\n", __func__, __LINE__);
  
      if (state->mode == MT_I2C_DISABLED) {
          state->mode = MT_I2C_MASTER;
          state->timerHandle = 0;
          state->timeOut = 2000000000;
  
          pdev->next = 2;
          pdev->last = 1;
          atomicBitsetInit(mXfersValid, I2C_MAX_QUEUE_DEPTH);
          atomicWriteByte(&state->masterState, MT_I2C_MASTER_IDLE);
          pdev->speedInHz = speedInHz;
  
          /* Need enable clock */
          i2c_clock_enable(pdev);
          mtI2cSpeedSet(pdev);
          request_irq(I2C0_IRQn, i2c0_irq_handler, "I2C0");
  #ifdef CFG_VCORE_DVFS_SUPPORT
          scp_wakeup_src_setup(I2C0_WAKE_CLK_CTRL, 1);
  #endif
          i2c_clock_disable(pdev);
      } else {
          return -EBUSY;
      }
      /* LOG here */
      return 0;
  }
  ```

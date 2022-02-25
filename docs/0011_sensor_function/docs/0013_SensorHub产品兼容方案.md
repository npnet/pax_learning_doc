# 概述

SensorHub兼容M50、M8功能开发。

## 软件方案

* 1.因为scp里面无法通过boardid判断是哪种机型，需要kernel往下发。kernel hub驱动检测到scp起来后会发送一段DRAM CFG数据给scp，通过这次发送可以将产品型号数据发下去，具体代码如下：

* `kernel-4.19/drivers/misc/mediatek/sensors-1.0/sensorHub/SCP_nanoHub/SCP_nanoHub.c`:
```C++
* static int sensorHub_probe(struct platform_device *pdev)
  * scp_ipi_registration(IPI_SENSOR,SCP_sensorHub_IPI_handler, "SCP_sensorHub");
    * SCP_sensorHub_IPI_handler(int id, //ipi中断处理函数
      * SCP_sensorHub_find_cmd(rsp->rsp.action); //寻找指令类型
        * SCP_sensorHub_notify_cmd //scp通知中断处理函数
          * case SCP_INIT_DONE:
          * WRITE_ONCE(scp_chre_ready, true); //重要，写chre初始化完成变量
          * atomic_set(&power_status, SENSOR_POWER_UP);
  * scp_A_register_notify(&sensorHub_ready_notifier);
    * sensorHub_ready_event(struct notifier_block *this, //通知链处理函数
      * if (event == SCP_EVENT_READY) {
      * scp_power_monitor_notify(SENSOR_POWER_UP, ptr);
        * WRITE_ONCE(scp_system_ready, true); //重要，写scp初始化完成变量
  * task_power_reset = kthread_run(sensorHub_power_up_work,
    * sensorHub_power_up_loop(data); //for循环
      * wait_event(power_reset_wait,READ_ONCE(scp_system_ready) && READ_ONCE(scp_chre_ready)); //等待scp_system_ready和scp_chre_ready变量完成
        * sensor_send_dram_info_to_hub(); //send dram information to scp 加这里  
```

* 2.理解一下ipi通信主要的几种数据类型：

```C++
static const struct SCP_sensorHub_Cmd SCP_sensorHub_Cmds[] = {
	SCP_SENSORHUB_CMD(SENSOR_HUB_ACTIVATE,
		SCP_sensorHub_enable_cmd),
	SCP_SENSORHUB_CMD(SENSOR_HUB_SET_DELAY,
		SCP_sensorHub_set_delay_cmd),
	SCP_SENSORHUB_CMD(SENSOR_HUB_GET_DATA,
		SCP_sensorHub_get_data_cmd),
	SCP_SENSORHUB_CMD(SENSOR_HUB_BATCH,
		SCP_sensorHub_batch_cmd),
	SCP_SENSORHUB_CMD(SENSOR_HUB_SET_CONFIG,
		SCP_sensorHub_set_cfg_cmd),
	SCP_SENSORHUB_CMD(SENSOR_HUB_SET_CUST,
		SCP_sensorHub_set_cust_cmd),
	SCP_SENSORHUB_CMD(SENSOR_HUB_BATCH_TIMEOUT,
		SCP_sensorHub_batch_timeout_cmd),
	SCP_SENSORHUB_CMD(SENSOR_HUB_SET_TIMESTAMP,
		SCP_sensorHub_set_timestamp_cmd),
	SCP_SENSORHUB_CMD(SENSOR_HUB_NOTIFY,
		SCP_sensorHub_notify_cmd),
};
宏定义：
#define    SENSOR_HUB_ACTIVATE		0
#define    SENSOR_HUB_SET_DELAY		1
#define    SENSOR_HUB_GET_DATA		2
#define    SENSOR_HUB_BATCH			3
#define    SENSOR_HUB_SET_CONFIG	4
#define    SENSOR_HUB_SET_CUST		5
#define    SENSOR_HUB_NOTIFY		6
#define    SENSOR_HUB_BATCH_TIMEOUT 7
#define    SENSOR_HUB_SET_TIMESTAMP	8

#define    SENSOR_HUB_POWER_NOTIFY	9

    /* SCP_NOTIFY EVENT */
    #define    SCP_INIT_DONE			0
    #define    SCP_FIFO_FULL			1
    #define    SCP_NOTIFY				2
    #define    SCP_BATCH_TIMEOUT		3
    #define	   SCP_DIRECT_PUSH          4

数据结构：
union SCP_SENSOR_HUB_DATA {
	struct SCP_SENSOR_HUB_REQ req;
	struct SCP_SENSOR_HUB_RSP rsp;
	struct SCP_SENSOR_HUB_ACTIVATE_REQ activate_req;
	struct SCP_SENSOR_HUB_ACTIVATE_RSP activate_rsp;
	struct SCP_SENSOR_HUB_SET_DELAY_REQ set_delay_req;
	struct SCP_SENSOR_HUB_SET_DELAY_RSP set_delay_rsp;
	struct SCP_SENSOR_HUB_GET_DATA_REQ get_data_req;
	struct SCP_SENSOR_HUB_GET_DATA_RSP get_data_rsp;
	struct SCP_SENSOR_HUB_BATCH_REQ batch_req;
	struct SCP_SENSOR_HUB_BATCH_RSP batch_rsp;
	struct SCP_SENSOR_HUB_SET_CONFIG_REQ set_config_req;
	struct SCP_SENSOR_HUB_SET_CONFIG_RSP set_config_rsp;
	struct SCP_SENSOR_HUB_SET_CUST_REQ set_cust_req;
	struct SCP_SENSOR_HUB_SET_CUST_RSP set_cust_rsp;
	struct SCP_SENSOR_HUB_NOTIFY_RSP notify_rsp;
};

具体数据结构：
struct SCP_SENSOR_HUB_REQ {
	uint8_t sensorType;
	uint8_t action;
	uint8_t reserve[2];
	uint32_t data[11];
};

struct SCP_SENSOR_HUB_RSP {
	uint8_t sensorType;
	uint8_t action;
	int8_t errCode;
	uint8_t reserve[1];
	/* uint32_t    reserved[9]; */
};

struct SCP_SENSOR_HUB_ACTIVATE_REQ {
	uint8_t sensorType;
	uint8_t action;
	uint8_t reserve[2];
	uint32_t enable;	/* 0 : disable ; 1 : enable */
	/* uint32_t    reserved[9]; */
};

#define SCP_SENSOR_HUB_ACTIVATE_RSP SCP_SENSOR_HUB_RSP
/* typedef SCP_SENSOR_HUB_RSP SCP_SENSOR_HUB_ACTIVATE_RSP; */

struct SCP_SENSOR_HUB_SET_DELAY_REQ {
	uint8_t sensorType;
	uint8_t action;
	uint8_t reserve[2];
	uint32_t delay;		/* ms */
	/* uint32_t    reserved[9]; */
};

#define SCP_SENSOR_HUB_SET_DELAY_RSP  SCP_SENSOR_HUB_RSP
/* typedef SCP_SENSOR_HUB_RSP SCP_SENSOR_HUB_SET_DELAY_RSP; */

struct SCP_SENSOR_HUB_GET_DATA_REQ {
	uint8_t sensorType;
	uint8_t action;
	uint8_t reserve[2];
	/* uint32_t    reserved[10]; */
};

struct SCP_SENSOR_HUB_GET_DATA_RSP {
	uint8_t sensorType;
	uint8_t action;
	int8_t errCode;
	uint8_t reserve[1];
	/* struct data_unit_t data_t; */
	union {
		int8_t int8_Data[0];
		int16_t int16_Data[0];
		int32_t int32_Data[0];
	} data;
};

struct SCP_SENSOR_HUB_BATCH_REQ {
	uint8_t sensorType;
	uint8_t action;
	uint8_t flag;
	uint8_t reserve[1];
	uint32_t period_ms;	/* batch reporting time in ms */
	uint32_t timeout_ms;	/* sampling time in ms */
	/* uint32_t    reserved[7]; */
};

#define SCP_SENSOR_HUB_BATCH_RSP SCP_SENSOR_HUB_RSP
/* typedef SCP_SENSOR_HUB_RSP SCP_SENSOR_HUB_BATCH_RSP; */

struct SCP_SENSOR_HUB_SET_CONFIG_REQ {
	uint8_t sensorType;
	uint8_t action;
	// [NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-02-23, add terminal boardid cfg
	//uint8_t reserve[2];
	uint8_t reserve[1];
	uint8_t terminal_boardid;
	// [NEW FEATURE]-END by wugangnan@paxsz.com 2022-02-23, add terminal boardid cfg
	/* struct sensorFIFO   *bufferBase; */
	uint32_t bufferBase;/* use int to store buffer DRAM base LSB 32 bits */
	uint32_t bufferSize;
	uint64_t ap_timestamp;
	uint64_t arch_counter;
	/* uint32_t    reserved[8]; */
};
```

* 3.思路就是使用sensor_send_dram_info_to_hub函数往下发机型，增加一个byte就行了，kernel这边如下修改：

```diff
 static int sensor_send_dram_info_to_hub(void)
 {                              /* call by init done workqueue */
        struct SCP_sensorHub_data *obj = obj_data;
@@ -1227,6 +1262,9 @@ static int sensor_send_dram_info_to_hub(void)
        data.set_config_req.action = SENSOR_HUB_SET_CONFIG;
        data.set_config_req.bufferBase =
                (unsigned int)(obj->shub_dram_phys & 0xFFFFFFFF);
+       // [NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-02-23, add terminal boardid cfg to scp
+    data.set_config_req.terminal_boardid = get_terminal_type();
+       // [NEW FEATURE]-END by wugangnan@paxsz.com 2022-02-23, add terminal boardid cfg to scp
 }

 --- a/kernel-4.19/drivers/misc/mediatek/sensors-1.0/sensorHub/inc_v1/SCP_sensorHub.h
+++ b/kernel-4.19/drivers/misc/mediatek/sensors-1.0/sensorHub/inc_v1/SCP_sensorHub.h
@@ -342,7 +342,11 @@ struct SCP_SENSOR_HUB_BATCH_REQ {
 struct SCP_SENSOR_HUB_SET_CONFIG_REQ {
        uint8_t sensorType;
        uint8_t action;
-       uint8_t reserve[2];
+       // [NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-02-23, add terminal boardid cfg
+       //uint8_t reserve[2];
+       uint8_t reserve[1];
+       uint8_t terminal_boardid;
+       // [NEW FEATURE]-END by wugangnan@paxsz.com 2022-02-23, add terminal boardid cfg
```

* 4.scp这边接收IPI流程如下,上层传下来的数据格式是SENSOR_HUB_SET_CONFIG：

```C++
* INTERNAL_APP_INIT(contextHubFwHandleEvent); 
 * case EVT_IPI_RX: {
 * contextHubHandleIpiRxEvent();
   * contextHubFindCmd(mTask.ipi_req.action);
     * contextHubFwSetCfg //因为上层传下来的数据格式是SENSOR_HUB_SET_CONFIG
       * set_cfg_req = (SCP_SENSOR_HUB_SET_CONFIG_REQ *)req;  //获取数据
       * pax_Set_Terminal_Type(set_cfg_req->terminal_boardid); //设置机型
```

* 5.所以对应scp也需要修改：

```diff
--- a/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/contexthub_fw.h
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/contexthub_fw.h
@@ -108,6 +108,14 @@
 #define GRV_INCREASE_NUM_AP             1000000
 #define GRAV_INCREASE_NUM_AP            1000
 #define LINEARACCEL_INCREASE_NUM_AP     1000
+
+typedef enum {
+       TERMINAL_M50_PLASTIC,
+       TERMINAL_M50_METAL,
+       TERMINAL_M8
+}terminal_type_t;
+
+
 typedef bool (*SimpleQueueForciblyDiscardCbkF)(void *data, bool onDelete); //return false to reject
 struct mtkActiveSensor {
     uint64_t latency;
@@ -416,7 +424,10 @@ typedef struct {
     uint8_t    sensorType;
     uint8_t    action;
     uint8_t    event;
-    uint8_t    reserve;
+    // [NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-02-23, add terminal boardid cfg
+    //uint8_t    reserve;
+    uint8_t    terminal_boardid;
+    // [NEW FEATURE]-END by wugangnan@paxsz.com 2022-02-23, add terminal boardid cfg
     uint32_t   data[11];
 } SCP_SENSOR_HUB_REQ;

@@ -478,7 +489,11 @@ typedef SCP_SENSOR_HUB_RSP SCP_SENSOR_HUB_BATCH_RSP;
 typedef struct {
     uint8_t            sensorType;
     uint8_t            action;
-    uint8_t            reserve[2];
+    // [NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-02-23, add terminal boardid cfg
+    //uint8_t            reserve[2];
+    uint8_t            reserve[1];
+    uint8_t            terminal_boardid;
+    // [NEW FEATURE]-END by wugangnan@paxsz.com 2022-02-23, add terminal boardid cfg
     struct sensorFIFO   *bufferBase;
     uint32_t            bufferSize;
     uint64_t            ap_timestamp;
@@ -722,5 +737,7 @@ uint8_t chreTypeToMtkType(uint8_t sensortype);
 uint8_t mtkTypeToChreType(uint8_t sensortype);
 void registerDownSampleInfo(int mtkType, uint32_t rate);
 void registerHwSampleInfo(int chreType, uint32_t hwDelay);
+int pax_Get_Terminal_Type(void);

--- a/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/contexthub_fw.c
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/contexthub_fw.c
 static int contextHubFwSetCfg(SCP_SENSOR_HUB_REQ *req,
                               struct data_unit_t *data)
 {
@@ -1526,6 +1554,10 @@ static int contextHubFwSetCfg(SCP_SENSOR_HUB_REQ *req,
 #ifdef CFG_VCORE_DVFS_SUPPORT
     dvfs_enable_DRAM_resource(SENS_MEM_ID);
 #endif
+
+    // [NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-02-23, add terminal boardid cfg to scp
+    pax_Set_Terminal_Type(set_cfg_req->terminal_boardid);
+       // [NEW FEATURE]-END by wugangnan@paxsz.com 2022-02-23, add terminal boardid cfg to scp
```

## scp驱动功能实现

* 主要思路是driver里面增加一个FSM函数，开机的时候去获取机型就好：

```diff
diff --git a/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/cust_mag.h b/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/cust_mag.h
old mode 100644
new mode 100755
index 5560d93a820..b8021586e6f
--- a/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/cust_mag.h
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/cust_mag.h
@@ -7,6 +7,10 @@ struct mag_hw {
     const char *name;
     int i2c_num;    /*!< the i2c bus used by the chip */
     int direction;  /*!< the direction of the chip */
+    // [NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-02-23, scp Compatible with both models
+    int direction_m50;
+    int direction_m8;
+    // [NEW FEATURE]-END by wugangnan@paxsz.com 2022-02-23, scp Compatible with both models
     unsigned char   i2c_addr[M_CUST_I2C_ADDR_NUM];
 };
 struct mag_hw* get_cust_mag(const char *name);
diff --git a/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/magnetometer.c b/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/magnetometer.c
old mode 100644
new mode 100755
index 1118c30f715..87e3d5f31dd
--- a/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/magnetometer.c
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/magnetometer.c
@@ -458,6 +458,7 @@ static bool sensorCfgMag(void *data, void *cookie)
     int32_t *values = data;
     float offset[AXES_NUM];
     int32_t caliParameter[6] = {0};
+    struct transferDataInfo dataInfo;
 
     osLog(LOG_ERROR, "sensorCfgMag:\n");
     osLog(LOG_ERROR, "values: %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld\n",
@@ -479,6 +480,10 @@ static bool sensorCfgMag(void *data, void *cookie)
     caliParameter[4] = values[7];
     caliParameter[5] = values[8];
 
+    // [NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-02-23, add chip cfg to scp Compatible with both models
+    sensorFsmRunState(&dataInfo, &mTask.fsm, (const void *)CHIP_MAG_CFG, &i2cCallback, &spiCallback);
+    // [NEW FEATURE]-END by wugangnan@paxsz.com 2022-02-23, add chip cfg to scp Compatible with both models
+
     if (mTask.caliAPI.caliApiSetCaliParam != NULL) {
         err = mTask.caliAPI.caliApiSetCaliParam(&caliParameter[0]);
         if (err < 0) {
@@ -869,6 +874,14 @@ static void handleSensorEvent(const void *state)
             break;
         }
 
+        case CHIP_MAG_CFG_DONE: {
+            sensorFsmReleaseState(&mTask.fsm);
+            mTask.nowState = CHIP_IDLE;
+            osLog(LOG_INFO, "mag: cfg done\n");
+            processPendingEvt();
+            break;
+        }
+
         case CHIP_SAMPLING_DONE: {
             mark_timestamp(SENS_TYPE_MAG, SENS_TYPE_MAG, SAMPLE_DONE, rtcGetTime());
             sensorFsmReleaseState(&mTask.fsm);
diff --git a/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/magnetometer.h b/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/magnetometer.h
old mode 100644
new mode 100755
index b054e148c24..088d8b3bbc9
--- a/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/magnetometer.h
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/magnetometer.h
@@ -49,6 +49,8 @@ enum MagState {
     CHIP_INIT_DONE,
     CHIP_IDLE,
     CHIP_RESET,
+    CHIP_MAG_CFG,
+    CHIP_MAG_CFG_DONE,
 };
 struct magData {
     float x, y, z;
diff --git a/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/mmc5603.c b/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/mmc5603.c
index 5a4c35ba59f..386135d5443 100755
--- a/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/mmc5603.c
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/mmc5603.c
@@ -57,6 +57,10 @@ enum MMC5603State
     STATE_IDLE = CHIP_IDLE,
 
     STATE_FUSE_EN = CHIP_RESET,
+    // [NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-02-23, add chip cfg to scp Compatible with both models
+    STATE_MAG_CFG = CHIP_MAG_CFG,
+    STATE_MAG_CFG_DONE = CHIP_MAG_CFG_DONE,
+    // [NEW FEATURE]-END by wugangnan@paxsz.com 2022-02-23, add chip cfg to scp Compatible with both models
     STATE_FUSE_RD,
     STATE_CORE,
     STATE_CHIP_RESET,
@@ -234,6 +238,47 @@ static int mmc5603Selftest(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *
 }
 
 #endif
+
+// [NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-02-23, add terminal boardid cfg to scp Compatible with both models
+static int mmc5603MagCfg(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
+                           void *inBuf, uint8_t inSize, uint8_t elemInSize,
+                           void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
+{
+	int pax_Terminal_Type;
+	int ret;
+
+	if (mTask.cvt.map[AXIS_X] == 0 && mTask.cvt.map[AXIS_Y] == 0 && mTask.cvt.map[AXIS_Z] == 0)
+	{
+		pax_Terminal_Type = pax_Get_Terminal_Type();
+		if (pax_Terminal_Type == TERMINAL_M8)
+		{
+			mTask.hw->direction = mTask.hw->direction_m8;
+		}
+		else if(pax_Terminal_Type == TERMINAL_M50_PLASTIC)
+		{
+			mTask.hw->direction = mTask.hw->direction_m50;
+		}
+		else if(pax_Terminal_Type == TERMINAL_M50_METAL)
+		{
+			mTask.hw->direction = mTask.hw->direction_m50;
+		}
+		else {
+			osLog(LOG_ERROR, "invalid Terminal_Type: %d\n", pax_Terminal_Type);
+		}
+
+	    if (0 != (ret = sensorDriverGetConvert(mTask.hw->direction, &mTask.cvt)))
+	    {
+	        osLog(LOG_ERROR, "invalid direction: %d\n", mTask.hw->direction);
+	    }
+		osLog(LOG_ERROR, "mag map[0]:%d, map[1]:%d, map[2]:%d, sign[0]:%d, sign[1]:%d, sign[2]:%d\n\r",
+			  mTask.cvt.map[AXIS_X], mTask.cvt.map[AXIS_Y], mTask.cvt.map[AXIS_Z],
+			  mTask.cvt.sign[AXIS_X], mTask.cvt.sign[AXIS_Y], mTask.cvt.sign[AXIS_Z]);
+	}
+	sensorFsmEnqueueFakeI2cEvt(i2cCallBack, next_state, SUCCESS_EVT);
+	return 0;
+}
+// [NEW FEATURE]-END by wugangnan@paxsz.com 2022-02-23, add terminal boardid cfg to scp Compatible with both models
+
 static void magGetSensorInfo(struct sensorInfo_t *data)
 {
     memcpy(&data->mag_dev_info, &mTask.mag_dev_info, sizeof(struct mag_dev_info_t));
@@ -472,7 +517,7 @@ static int mmc5603Convert(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *n
     int32_t idata[AXES_NUM];
     uint64_t timestamp = 0;
 
-    //osLog(LOG_ERROR, "mmc5603Convert direction: %d\n", mTask.hw->direction);
+    osLog(LOG_ERROR, "mmc5603Convert direction: %d pax_Get_Terminal_Type: %d mTask.hw->direction_m8 = %d mTask.hw->direction_m50 = %d\n", mTask.hw->direction ,pax_Get_Terminal_Type(),mTask.hw->direction_m8,mTask.hw->direction_m50);
 
 #if 1
     osLog(LOG_ERROR, "%s mmc5603 raw reg data: \n"
@@ -496,10 +541,17 @@ static int mmc5603Convert(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *n
     remap_data[mTask.cvt.map[AXIS_X]] = mTask.cvt.sign[AXIS_X] * data[0].x;
     remap_data[mTask.cvt.map[AXIS_Y]] = mTask.cvt.sign[AXIS_Y] * data[0].y;
     remap_data[mTask.cvt.map[AXIS_Z]] = mTask.cvt.sign[AXIS_Z] * data[0].z;
-    //printf("%s:: remaped: %f %f %f;;;raw: %f %f %f\n",__func__,(double)remap_data[0],(double)remap_data[1],(double)remap_data[2],(double)data[0].x,(double)data[0].y,(double)data[0].z);
-    data[0].x = remap_data[AXIS_X] * 100.0f / 1024;
-    data[0].y = remap_data[AXIS_Y] * 100.0f / 1024;
-    data[0].z = remap_data[AXIS_Z] * 100.0f / 1024;
+
+    //data[0].x = remap_data[AXIS_X] * 100.0f / 1024;
+    //data[0].y = remap_data[AXIS_Y] * 100.0f / 1024;
+    //data[0].z = remap_data[AXIS_Z] * 100.0f / 1024;
+    //[BUGFIX]-BEGIN by (wugangnan@paxsz.com), use kernel Calculation formula to cali data
+    data[0].x = remap_data[AXIS_X]  / 10;
+    data[0].y = remap_data[AXIS_Y]  / 10;
+    data[0].z = remap_data[AXIS_Z]  / 10;
+    //[BUGFIX]-END by (wugangnan@paxsz.com), use kernel Calculation formula to cali data
+
+    printf("%s:: remaped: %f %f %f;;;raw: %f %f %f\n",__func__,(double)remap_data[0],(double)remap_data[1],(double)remap_data[2],(double)data[0].x,(double)data[0].y,(double)data[0].z);
 
     mTask.factoryData.ix = (int32_t)(data[0].x * MAGNETOMETER_INCREASE_NUM_AP);
     mTask.factoryData.iy = (int32_t)(data[0].y * MAGNETOMETER_INCREASE_NUM_AP);
@@ -542,6 +594,8 @@ static struct sensorFsm mmc5603Fsm[] = {
 #ifdef MEMSIC_SELFTEST
     sensorFsmCmd(STATE_SELFTEST, STATE_SELFTEST_DONE, mmc5603Selftest),
 #endif    
+    sensorFsmCmd(STATE_MAG_CFG, STATE_MAG_CFG_DONE, mmc5603MagCfg),
+
 };
 
 static int mmc5603Init(void)
@@ -560,6 +614,8 @@ static int mmc5603Init(void)
     mTask.i2c_addr = mTask.hw->i2c_addr[0];
     osLog(LOG_ERROR, "driver version : %s, mag i2c_num: %d, i2c_addr: 0x%x\n", MMC5603_DRIVER_VERSION, mTask.hw->i2c_num, mTask.i2c_addr);
 
+    // [NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-02-24, put mag direction compatible Function in mmc5603MagCfg
+	/*
     if (0 != (ret = sensorDriverGetConvert(mTask.hw->direction, &mTask.cvt)))
     {
         osLog(LOG_ERROR, "invalid direction: %d\n", mTask.hw->direction);
@@ -567,6 +623,8 @@ static int mmc5603Init(void)
     osLog(LOG_ERROR, "mag map[0]:%d, map[1]:%d, map[2]:%d, sign[0]:%d, sign[1]:%d, sign[2]:%d\n\r",
           mTask.cvt.map[AXIS_X], mTask.cvt.map[AXIS_Y], mTask.cvt.map[AXIS_Z],
           mTask.cvt.sign[AXIS_X], mTask.cvt.sign[AXIS_Y], mTask.cvt.sign[AXIS_Z]);
+	*/
+	// [NEW FEATURE]-END by wugangnan@paxsz.com 2022-02-24, put mag direction compatible Function in mmc5603MagCfg
 
     i2cMasterRequest(mTask.hw->i2c_num, I2C_SPEED);
     mTask.txBuf[0] = MMC5603_REG_PRODUCT;
@@ -589,7 +647,9 @@ static int mmc5603Init(void)
 #endif
         mTask.mag_dev_info.layout = 0x00;
         mTask.mag_dev_info.deviceid = 0x10;
-        strncpy(mTask.mag_dev_info.libname, "memsic", sizeof(mTask.mag_dev_info.libname));
+        // [NEW FEATURE]-BEGIN by wugangnan@paxsz.com 2022-02-24, put Calibration library compatible Function in hal,not in scp driver
+        //strncpy(mTask.mag_dev_info.libname, "memsic", sizeof(mTask.mag_dev_info.libname));
+        // [NEW FEATURE]-END by wugangnan@paxsz.com 2022-02-24, put Calibration library compatible Function in hal,not in scp driver
         magSensorRegister();
         magRegisterInterruptMode(MAG_UNFIFO);
         registerMagDriverFsm(mmc5603Fsm, ARRAY_SIZE(mmc5603Fsm));
```
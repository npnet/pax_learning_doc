# Android SCP IPI

Inter-Processor Communication (IPC) and Inter-Processor Interrupt (IPI)

* IPC is a communication bridge between AP and SCP.  AP/SCP sends an IPC interrupt to SCP/AP to inform to collect the commmunication mesesages in the shared buffer.
* sensorhub和nanohub可以不同，也可以相同，nanohub接口可以是I2C/SPI/IPI，sensorhub貌似只能是IPI；

## 参考文档

* [0001_Android_SCP.md](0001_Android_SCP.md)

## SCP使能传感器流程

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

## nanohub处理IPI中断

* kernel-4.9/drivers/misc/mediatek/sensors-1.0/sensorHub/SCP_nanoHub/SCP_nanoHub.c
  ```CPP
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
  
  const struct SCP_sensorHub_Cmd *
      SCP_sensorHub_find_cmd(uint32_t packetReason)
  {
      int i;
      const struct SCP_sensorHub_Cmd *cmd;
  
      for (i = 0; i < ARRAY_SIZE(SCP_sensorHub_Cmds); i++) {
          cmd = &SCP_sensorHub_Cmds[i];
          if (cmd->reason == packetReason)
              return cmd;
      }
      return NULL;
  }

  static void SCP_sensorHub_IPI_handler(int id,
      void *data, unsigned int len)
  {
      SCP_SENSOR_HUB_DATA_P rsp = (SCP_SENSOR_HUB_DATA_P) data;
      const struct SCP_sensorHub_Cmd *cmd;
  
      if (len > SENSOR_IPI_SIZE) {
          pr_err("SCP_sensorHub_IPI_handler len=%d error\n", len);
          return;
      }
      /*pr_err("sensorType:%d, action=%d event:%d len:%d\n",
       * rsp->rsp.sensorType, rsp->rsp.action, rsp->notify_rsp.event, len);
       */
      cmd = SCP_sensorHub_find_cmd(rsp->rsp.action);
      if (cmd != NULL)
          cmd->handler(rsp, len);
      else
          pr_err("cannot find cmd!\n");
  }

  static int sensorHub_probe(struct platform_device *pdev)
  {
      // ...省略
      /* register ipi interrupt handler */
      scp_ipi_registration(IPI_SENSOR,
          SCP_sensorHub_IPI_handler, "SCP_sensorHub");
      // ...省略
  }
  ```
* kernel-4.9/drivers/misc/mediatek/scp/v01/scp_ipi.c
  ```CPP
  /*
   * API let apps can register an ipi handler to receive IPI
   * @param id:      IPI ID
   * @param handler:  IPI handler
   * @param name:  IPI name
   */
  enum scp_ipi_status scp_ipi_registration(enum ipi_id id,
      void (*ipi_handler)(int id, void *data, unsigned int len),
      const char *name)
  {
      if (id < SCP_NR_IPI) {
          scp_ipi_desc[id].name = name;
  
          if (ipi_handler == NULL)
              return SCP_IPI_ERROR;
  
          scp_ipi_desc[id].handler = ipi_handler;
          return SCP_IPI_DONE;
      } else {
          return SCP_IPI_ERROR;
      }
  }
  EXPORT_SYMBOL_GPL(scp_ipi_registration);
  ```

## AP sensorHub发送IPI请求

```
* kernel-4.9/drivers/misc/mediatek/sensors-1.0/sensorHub/SCP_nanoHub/SCP_nanoHub.c
  * int sensor_set_cmd_to_hub(uint8_t sensorType, CUST_ACTION action, void *data)
    * err = scp_sensorHub_req_send(&req, &len, 1);
      * ret = SCP_sensorHub_ipi_txrx((unsigned char *)data);
        * return scp_ipi_txrx(txrxbuf, SENSOR_IPI_SIZE, txrxbuf, SENSOR_IPI_SIZE);
          * ipi_message_init(&m);
          * ipi_message_add_tail(&t, &m);
            * list_add_tail(&t->transfer_list, &m->transfers);
          * status =  __ipi_xfer(&m);
  * static int __init SCP_sensorHub_init(void)
    * INIT_WORK(&master.work, ipi_work);
      * static void ipi_work(struct work_struct *work)
        * list_for_each_entry(t, &m->transfers, transfer_list)
          * status = ipi_txrx_bufs(t);
            * status = scp_ipi_send(IPI_SENSOR, (unsigned char *)hw->tx, hw->len, 0, SCP_A_ID);
              * kernel-4.9/drivers/misc/mediatek/scp/v01/scp_ipi.c
                * enum scp_ipi_status scp_ipi_send(enum ipi_id id, void *buf, unsigned int  len, unsigned int wait, enum scp_core_id scp_id)
```

其中发送IPI请求主要函数为`sensor_set_cmd_to_hub`:

```c++
enum CUST_ACTION {
	CUST_ACTION_SET_CUST = 1,
	CUST_ACTION_SET_CALI,
	CUST_ACTION_RESET_CALI,
	CUST_ACTION_SET_TRACE,
	CUST_ACTION_SET_DIRECTION,
	CUST_ACTION_SHOW_REG,
	CUST_ACTION_GET_RAW_DATA,
	CUST_ACTION_SET_PS_THRESHOLD,
	CUST_ACTION_SHOW_ALSLV,
	CUST_ACTION_SHOW_ALSVAL,
	CUST_ACTION_SET_FACTORY,
	CUST_ACTION_GET_SENSOR_INFO,
};

int sensor_set_cmd_to_hub(uint8_t sensorType,
	enum CUST_ACTION action, void *data)
{
	union SCP_SENSOR_HUB_DATA req;
	int len = 0, err = 0;
	struct SCP_SENSOR_HUB_GET_RAW_DATA *pGetRawData;

	req.get_data_req.sensorType = sensorType;
	req.get_data_req.action = SENSOR_HUB_SET_CUST;

	if (atomic_read(&power_status) == SENSOR_POWER_DOWN) {
		pr_err("scp power down, we can not access scp\n");
		return -1;
	}

	switch (sensorType) {
	case ID_ACCELEROMETER:
		req.set_cust_req.sensorType = ID_ACCELEROMETER;
		req.set_cust_req.action = SENSOR_HUB_SET_CUST;
		switch (action) {
		case CUST_ACTION_RESET_CALI:

       ...省略...
```


## SCP contexthub处理IPI请求

在SCP这边，不管是nanohub，还是sensorhub，都是contexthub，IPI事件处理函数主要是`contextHubFwHandleEvent`，处理发送和接收事件，处理完成后将用`contextHubIpiRxAck`函数向AP发送ACK。

```
* vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/contexthub_fw.c
  * INTERNAL_APP_INIT(APP_ID_MAKE(APP_ID_VENDOR_MTK, MTK_APP_ID_WRAP(SENS_TYPE_INVALID, 0, 0)), 0, contextHubFwStart, contextHubFwEnd, contextHubFwHandleEvent);
    * static bool contextHubFwStart(uint32_t tid)
      * ipi_ret = scp_ipi_registration(IPI_SENSOR, contextHubIpiHandler, "chre_fw_ipi");
        * static void contextHubIpiHandler(int id, void *data, unsigned int len)
          * osEnqueuePrivateEvt(EVT_IPI_RX, NULL, NULL, mTask.id);
            * 进入OS系统消息处理，最终传到下面这个函数
              * static void contextHubFwHandleEvent(uint32_t evtType, const void* evtData)
                * case EVT_IPI_RX:
                  * contextHubHandleIpiRxEvent();
                    * cmd = contextHubFindCmd(mTask.ipi_req.action);
                      * const static struct ContextHubCmd mContextHubCmds[]
                        * CONTEXTHUB_CMD(SENSOR_HUB_SET_CUST, contextHubFwSetCust, contextHubFwSetCustAck),
                          * static int contextHubFwSetCust(SCP_SENSOR_HUB_REQ *req, struct data_unit_t *data)
                            * set_cust_req = (SCP_SENSOR_HUB_SET_CUST_REQ *)req;
                            * mtkType = apIdToMtkType(set_cust_req->sensorType);
                            * ret = contextHubDispatchCust(mtkTypeToChreType(mtkType), set_cust_req);
                    * ret = cmd->handler(&mTask.ipi_req, &data);
                    * ret = contextHubIpiRxAck(cmd, &mTask.ipi_req, &data, ret);
```

其中处理IPI请求主要函数是`contextHubDispatchCust`：

```c
typedef enum {
    CUST_ACTION_SET_CUST = 1,
    CUST_ACTION_SET_CALI,
    CUST_ACTION_RESET_CALI,
    CUST_ACTION_SET_TRACE,
    CUST_ACTION_SET_DIRECTION,
    CUST_ACTION_SHOW_REG,
    CUST_ACTION_GET_RAW_DATA,
    CUST_ACTION_SET_PS_THRESHOLD,
    CUST_ACTION_SHOW_ALSLV,
    CUST_ACTION_SHOW_ALSVAL,
    CUST_ACTION_SET_FACTORY,
    CUST_ACTION_GET_SENSOR_INFO,
} CUST_ACTION;

static int contextHubDispatchCust(uint8_t sensType, SCP_SENSOR_HUB_SET_CUST_REQ *req)
{
    int ret = 0;
    CUST_SET_REQ_P cust_req = &req->cust_set_req;

    switch (cust_req->cust.action) {
        case CUST_ACTION_SET_CALI:
            ret = sensorCoreWriteCalibration(sensType, cust_req->setCali.int32_data);
            break;
        case CUST_ACTION_RESET_CALI:
            ret = sensorCoreResetCalibration(sensType);
            break;
        case CUST_ACTION_SET_PS_THRESHOLD:
            ret = sensorCoreSetThreshold(sensType, cust_req->setPSThreshold.threshold);
            break;
        case CUST_ACTION_SET_TRACE:
            ret = sensorCoreSetDebugTrace(sensType, cust_req->setTrace.trace);
            break;
        case CUST_ACTION_GET_SENSOR_INFO:
            ret = sensorCoreGetSensorInfo(sensType, &cust_req->getInfo.sensorInfo);
            break;
        default:
            break;
    }
    return ret;
}
```

## sensorhub nanohub IPI通信验证

开启内核相关配置

```diff
diff --git a/kernel-4.9/arch/arm64/configs/k62v1_64_bsp_debug_defconfig b/kernel-4.9/arch/arm64/configs/k62v1_64_bsp_debug_defconfig
index 382f89e769c..a359e8ac81b 100755
--- a/kernel-4.9/arch/arm64/configs/k62v1_64_bsp_debug_defconfig
+++ b/kernel-4.9/arch/arm64/configs/k62v1_64_bsp_debug_defconfig
@@ -152,8 +152,8 @@ CONFIG_CUSTOM_KERNEL_GYROSCOPE=y
 CONFIG_MTK_BMI160GY_NEW=y
 # CONFIG_MTK_GYROHUB is not set
 CONFIG_CUSTOM_KERNEL_MAGNETOMETER=y
-CONFIG_MTK_MMC5603X=y
-# CONFIG_MTK_MAGHUB is not set
+#CONFIG_MTK_MMC5603X=y
+CONFIG_MTK_MAGHUB=y
 CONFIG_MTK_HWMON=y
 # CONFIG_CUSTOM_KERNEL_BAROMETER is not set
 # CONFIG_MTK_BAROHUB is not set
@@ -173,7 +173,7 @@ CONFIG_MTK_HWMON=y
 # CONFIG_MTK_DEVICE_ORIENTATION_HUB is not set
 # CONFIG_MTK_MOTION_DETECT_HUB is not set
 # CONFIG_MTK_TILTDETECTHUB is not set
-# CONFIG_CUSTOM_KERNEL_SENSORHUB is not set
+CONFIG_CUSTOM_KERNEL_SENSORHUB=y
 CONFIG_MTK_MD1_SUPPORT=9
 CONFIG_MTK_C2K_LTE_MODE=0
 CONFIG_MTK_COMBO=y
@@ -312,6 +312,7 @@ CONFIG_SERIAL_8250_RUNTIME_UARTS=2
 CONFIG_SERIAL_8250_MT6577=y
 # CONFIG_HW_RANDOM is not set
 CONFIG_I2C_MTK=y
+CONFIG_I2C_CHARDEV=y
 CONFIG_SPI=y
 CONFIG_SPI_MT65XX=y
 CONFIG_MTK_GAUGE_VERSION=30
@@ -417,8 +418,8 @@ CONFIG_ANDROID_LOW_MEMORY_KILLER=y
 CONFIG_ANDROID_INTF_ALARM_DEV=y
 CONFIG_ION=y
 CONFIG_MTK_ION=y
-# CONFIG_NANOHUB is not set
-# CONFIG_NANOHUB_MTK_IPI is not set
+CONFIG_NANOHUB=y
+CONFIG_NANOHUB_MTK_IPI=y
 CONFIG_MAILBOX=y
 CONFIG_MTK_CMDQ_MBOX=y
 CONFIG_IIO=y
```

替换系统默认的mmc5603代码

* vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/MEMS_Driver/magnetometer/
  * mmc5603.c
  * mmc5603.h

添加接收中断，并输出打印，配置mmc5603驱动

```diff
diff --git a/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/contexthub_fw.c b/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/contexthub_fw.c
index 437cc525e9d..7999ffe446a 100644
--- a/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/contexthub_fw.c
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/middleware/contexthub/contexthub_fw.c
@@ -1693,6 +1693,7 @@ static void contextHubHandleIpiRxEvent(void)
     struct data_unit_t data;
     const struct ContextHubCmd *cmd;
 
+    osLog(LOG_ERROR, "ERR: contextHubIpiRx!\n");
     cmd = contextHubFindCmd(mTask.ipi_req.action);
     if (cmd == NULL) {
         osLog(LOG_ERROR, "contextHubHandleIpiRxEvent cannot find cmd\n");
diff --git a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/ProjectConfig.mk b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/ProjectConfig.mk
index 54182fc04a5..48b8f7e332c 100755
--- a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/ProjectConfig.mk
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/ProjectConfig.mk
@@ -18,7 +18,8 @@ CFG_ALSPS_SUPPORT = yes
 CFG_CM36558_SUPPORT = yes
 CFG_MAGNETOMETER_SUPPORT = yes
 CFG_MAG_CALIBRATION_IN_AP = yes
-CFG_AKM09918_SUPPORT = yes
+# CFG_AKM09918_SUPPORT = yes
+CFG_MMC5603_SUPPORT = yes
 CFG_BAROMETER_SUPPORT = yes
 CFG_BMP280_SUPPORT = yes
 CFG_ACCGYRO_SUPPORT = yes
diff --git a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/magnetometer/cust_mag.c b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/magnetometer/cust_mag.c
index 4fa23bd1756..1b62d50dfba 100755
--- a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/magnetometer/cust_mag.c
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/magnetometer/cust_mag.c
@@ -9,4 +9,13 @@ struct mag_hw cust_mag_hw[] __attribute__((section(".cust_mag"))) = {
         .i2c_addr = {0x0c, 0},
     },
 #endif
+
+#ifdef CFG_MMC5603_SUPPORT
+    {
+        .name = "mmc5603",
+        .i2c_num = 0,
+        .direction = 4,
+        .i2c_addr = {0x30, 0},
+    },
+#endif
 };
diff --git a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/overlay/overlay.c b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/overlay/overlay.c
index 37ea139ab77..710872d8ef3 100755
--- a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/overlay/overlay.c
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/k62v1_64_bsp/cust/overlay/overlay.c
@@ -102,7 +102,7 @@ ACC_GYRO_OVERLAY_REMAP_END
 void magOverlayRemap(void)
 {
 MAG_OVERLAY_REMAP_START
-    MAG_OVERLAY_REMAP(akm09918);
+    MAG_OVERLAY_REMAP(mmc5603);
 MAG_OVERLAY_REMAP_END
 
     return;
diff --git a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/platform/feature_config/chre.mk b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/platform/feature_config/chre.mk
index 5f028392eaf..9e7bd22cd0e 100644
--- a/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/platform/feature_config/chre.mk
+++ b/vendor/mediatek/proprietary/tinysys/freertos/source/project/CM4_A/mt6765/platform/feature_config/chre.mk
@@ -194,6 +194,10 @@ endif
 ifeq ($(CFG_AKM09915_SUPPORT),yes)
 C_FILES  += $(SENDRV_DIR)/magnetometer/akm09915.c
 endif
+
+ifeq ($(CFG_MMC5603_SUPPORT),yes)
+C_FILES  += $(SENDRV_DIR)/magnetometer/mmc5603.c
+endif
 endif
 
 ######## accgyro support ########
```

使用磁感传感器APP获取磁感数据，查看SCP log是否有对应的log，log如下，可以看到SCP地磁传感器工作了

* [0334-scp_log_boot_normal.txt](refers/0334-scp_log_boot_normal.txt)
  * scp启动的时候的log
* [0334-scp_log_mag.txt](refers/0334-scp_log_mag.txt)
  * scp和AP通信时候的log
    ```
    [330.712]ERR: contextHubIpiRx!
    [330.712]sync time scp:330712873096, ap:334816981788, offset:4105494230
    [335.156]hostintf: 335156769722, chreType:8, rate:15359, latency:0, cmd:0!
    [335.161]hostintf: 335161255414, chreType:8, rate:15359, latency:0, cmd:1!
    [335.161]sensorPowerMag on:1, nowOn:0
    [335.161]mmc5603Enable :  MMC5603 enable mag
    [335.161]mag: enable done
    [335.161]sensorRateMag, rate:16384, latency:66670592
    [335.161]mag intrMode: MU, timerDelay:62500000
    [335.161]mag: ratechg done
    [335.225]mmc5603Convert mmc5603 raw reg data: 
    [0]:128, [1]:96
    [2]:124, [3]:227
    [4]:127, [5]:1
    ```
    * [335.156]hostintf: 335156769722, chreType:8, rate:15359, latency:0, cmd:0!
      * 话说这个hostintf又是另外一个接口
* hostintf分析，使用IPI通道进行数据通信，而是不是使用I2C或者SPI通信作为数据通道
  ```
  * vendor/mediatek/proprietary/hardware/contexthub/firmware/src/hostIntf.c
    * INTERNAL_APP_INIT(APP_ID_MAKE(APP_ID_VENDOR_GOOGLE, 0), 0, hostIntfRequest, hostIntfRelease, hostIntfHandleEvent);
      * static bool hostIntfRequest(uint32_t tid)
        * mComm = platHostIntfInit();
          * vendor/mediatek/proprietary/hardware/contexthub/firmware/links/plat/src/hostIntf.c
            '''
            * const struct HostIntfComm *platHostIntfInit() 
              * return hostIntfIPIInit();
                * vendor/mediatek/proprietary/hardware/contexthub/firmware/links/plat/src/hostIntfIPI.c
                  * const struct HostIntfComm *hostIntfIPIInit(void)
                    * return &gIPIComm;
            '''
          * vendor/mediatek/proprietary/hardware/contexthub/firmware/links/variant/inc/variant.h
            * #define PLATFORM_HOST_INTF_IPI_BUS
  ```
* 对应的Linux dmesg
  ```
  <7>[  339.334272]  (5)[665:sensors@2.0-ser]<MAG> [name:mag&]mag_store_batch 1,0,66667000,0
  <7>[  339.335941]  (0)[665:sensors@2.0-ser]<MAG> [name:mag&]mag_store_batch done: 0
  <7>[  339.338219]  (0)[665:sensors@2.0-ser]<MAG> [name:mag&]mag_store_active buf=1
  <7>[  339.338252]  (0)[665:sensors@2.0-ser][Msensor] [name:maghub&]magnetic enable value = 1
  <7>[  339.340416]  (0)[665:sensors@2.0-ser]<MAG> [name:mag&]mag_store_active done
  ```
  * <7>[  339.338252]  (0)[665:sensors@2.0-ser][Msensor] [name:maghub&]magnetic enable value = 1
    ```
    * kernel-4.9/drivers/misc/mediatek/sensors-1.0/magnetometer/maghub/maghub.c
      * static int maghub_m_setPowerMode(bool enable)
        * pr_debug("magnetic enable value = %d\n", enable);
        * res = sensor_enable_to_hub(ID_MAGNETIC, enable);
          * kernel-4.9/drivers/misc/mediatek/sensors-1.0/sensorHub/SCP_nanoHub/SCP_nanoHub.c
            * int sensor_enable_to_hub(uint8_t handle, int enabledisable)
              * ret = nanohub_external_write((const uint8_t *)&cmd, sizeof(struct ConfigCmd));
                * kernel-4.9/drivers/staging/nanohub/main.c
                  * ssize_t nanohub_external_write(const char *buffer, size_t length)
                    * nanohub_comms_tx_rx_retrans (data, CMD_COMMS_WRITE, buffer, length, &ret_data, sizeof(ret_data), false, 10, 10)
                      * kernel-4.9/drivers/staging/nanohub/comms.c
                        * int nanohub_comms_tx_rx_retrans(struct nanohub_data *data, u32 cmd, const u8 *tx, u8 tx_len, u8 *rx, size_t rx_len, bool user, int retrans_cnt, int retrans_delay)
                          * data->comms.open(data);
                          * ret = nanohub_comms_tx_rx(data, pad, packet_size, seq, rx, rx_len);
                          * data->comms.close(data);
    * kernel-4.9/drivers/staging/nanohub/main.c
      '''
      static int __init nanohub_init(void)
      {
          int ret = 0;
  
          sensor_class = class_create(THIS_MODULE, "nanohub");
          if (IS_ERR(sensor_class)) {
              ret = PTR_ERR(sensor_class);
              pr_err("nanohub: class_create failed; err=%d\n", ret);
          }
          if (!ret)
              major = __register_chrdev(0, 0, ID_NANOHUB_MAX, "nanohub",
                            &nanohub_fileops);
  
          if (major < 0) {
              ret = major;
              major = 0;
              pr_err("nanohub: can't register; err=%d\n", ret);
          }
  
      #ifdef CONFIG_NANOHUB_I2C
          if (ret == 0)
              ret = nanohub_i2c_init();
      #endif
      #ifdef CONFIG_NANOHUB_SPI
          if (ret == 0)
              ret = nanohub_spi_init();
      #endif
      #ifdef CONFIG_NANOHUB_MTK_IPI
              ret = nanohub_ipi_init();
      #endif
          pr_info("nanohub: loaded; ret=%d\n", ret);
          return ret;
      }

      '''
    ```
    * CONFIG_NANOHUB_I2C
    * CONFIG_NANOHUB_SPI
    * CONFIG_NANOHUB_MTK_IPI
      * kernel-4.9/arch/arm64/configs/k62v1_64_bsp_debug_defconfig
        * CONFIG_NANOHUB_MTK_IPI=y

## Linux Driver I2C bus是否可访问
  
* i2cdetect -y 1
  ```
       0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
  00:          -- -- -- -- -- -- -- -- -- -- -- -- --
  10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  20: -- -- -- UU -- -- -- -- -- -- -- -- -- -- -- --
  30: 30 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  60: -- -- -- -- -- -- -- -- UU -- -- -- -- -- -- --
  70: -- -- -- -- -- -- -- --
  ```
* 不管是否打开地磁APP访问，还是不打开，都能检测道0x30(MMC5603)设备节点，那么表示是I2C bus是共用的，没有做隔离

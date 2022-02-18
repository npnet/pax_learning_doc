# Accel Sensor 分析

Accel Sensor 驱动分析


## 参考

* [sensor_all_in_one.pdf](refer/sensor_all_in_one.pdf)

* [CS6853-BD9D-PGD-V1.7EN_MT6853_SCP_Development_Guide.pdf](CS6853-BD9D-PGD-V1.7EN_MT6853_SCP_Development_Guide.pdf refer/CS6853-BD9D-PGD-V1.7EN_MT6853_SCP_Development_Guide.pdf)

## 代码分析

* dts：

```
accel sensor:
&cust_accel_sc7a20 {
		i2c_num					= <1>;
		i2c_addr				= <0x18 0 0 0>;
		direction				= <7>;
		power_id				= <0xffff>;
		power_vol				= <0>;
		firlen					= <0>;
		is_batch_supported			= <0>;
};

&i2c1 {
	gsensor_sc7a20@18 {
		compatible = "mediatek,gsensor_sc7a20";
		reg = <0x18>;
		status = "okay";
	};
}
```

## debug调试

* acc sensorhub调通后，scp打印如下：

```log
----- timezone:Asia/Shanghai
8 overlay remap fail

[0.017]contexthub_fw_start tid: 268

[0.017]accGyro: app start

[0.017]sc7a20ResetRead

[0.017]alsps: app start

[0.017]initSensors:   not ready!

[0.017]LIFT EVT_APP_START

[0.017]TILT EVT_APP_START

[0.017]STEP_RECOGNITION EVT_APP_START

[0.017]alsPs: init done

[0.017]read before sc7a20ResetWrite

[0.028]sc7a20DeviceId

[0.028]sc7a20 acc reso: 0, sensitivity: 1024

[0.028]sc7a20RegisterCore deviceId 0x11


[0.028]accGyro: init done

[0.518]initSensors: alloc blocks number:219

[0.520]get dram phy addr=0x8d000000,size=1048520, maxEventNumber:23830

[0.520]get dram phy rp=0,wp=0

[2.029]frequency request: 65535 MHz => 250 MHz

[2.829]sync time scp:2829537083, ap:4316042615, offset:1486968532

[8.765]hostintf: 8765761097, chreType:1, rate:0, latency:0, cmd:3!

[8.765]sensorCfgAcc:

[8.765]bias: 0.000000, 0.000000, 0.000000

[8.765]cali: 0, 0, 0

[8.765][MPEKlib]: MPE_CAL_A_VER_18082801

[8.765]sc7a20AccCfgCali: cfgData[0]:0, cfgData[1]:0, cfgData[2]:0

[8.765]acc: cfg done

```
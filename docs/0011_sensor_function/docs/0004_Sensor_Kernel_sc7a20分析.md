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
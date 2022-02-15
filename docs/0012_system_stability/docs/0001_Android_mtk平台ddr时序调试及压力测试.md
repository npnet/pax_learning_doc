# Android memery

由于memory 时序容易受到走线、电源系统及温度等因素的干扰，导致memory接口的不稳定，从而造成几率性不能下载、无法开机或者系统不稳定的情况；
使用ETT工具，自动同步memory设备，调试一组最优化的memory时序设定用于量产，同时检测memory接口的稳定性,避免因时序不稳定导致的系列问题；
因此，在导入新layout或者memory chip的时候都请跑一遍ETT。

## 参考文档

* [0001_MT6765_62_ETT_&_stress_test_reference_for_LP4X_V1.6.pdf](refer/0001_MT6765_62_ETT_&_stress_test_reference_for_LP4X_V1.6.pdf)

## 方法

* 具体请参考其中文档MT6765_62 ETT & stress test reference for LP4X V1.6.pdf，提出了以下几个测试点：

- ETT test step by step (ETT测试主要是调试一组最优化的memory时序设定用于量产，需要空板(emmc未烧录)+焊接串口) 这块主要是软件人员配合硬件在新DDR上做验证
- MTK Eye-Scan Function (判断ETT测试结果)

## 实验过程

### 1.ett测试条件

* 未烧录过的单板
* 下载过程序的板子请务必先Format whole flash
* 工具:支持MT6765_62平台的Flash tool(W1748以后版本)
* ETT BIN说明:https://online.mediatek.com/qvl/_layouts/15/mol/qvl/ext/QVLHomeExternal.aspx 下载对应平台、对应memory的ETT bin。
* 必须用电源给VBAT供电，高低温环境下测试不能使用电池
* 手机上的NTC需要下拉10K电阻到GND， 模拟电池本身的NTC

### 2.测试过程

* 组合键Ctrl+Alt+A调出flash tool的brom Adapter选择ETT bin设置start address(0x204000)勾上Jump点击download；

![0001_ett.png](images/0001_ett.png)

烧录后发现无打印，咨询MTK后，发现串口TX/RX实际电压为1.8v，因为下载`MT6765_ETT-enc_KMDX60018M_B425_RELEASE.bin`后并没有打开3.3v电平转换开关，我们用的3.3v串口，电平不匹配，原理图如下，需要硬件打开以下两个供电：

IO_3V3(GPIO_EXT1 B:GPIO164)需要打开：

![0001_IO3.3.png](images/0001_IO3.3.png)

GPIO_EXT8 B:GPIO171 串口电平转换：

![0001_1.8-3.3.png](images/0001_1.8-3.3.png)


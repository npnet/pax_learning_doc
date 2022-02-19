# 传感器原理

Sensor架构介绍

## 参考

* [0007_sensor_all_in_one.pdf](refers/sensor_all_in_one.pdf)

## 软件架构图

![0007_软件架构1.png](images/0007_软件架构1.png)

![0007_架构2.png](images/0007_架构2.png)

新架构中的每个sensor都对应于一个CPP file：

![0007_每个.png](images/0007_每个.png)

## HIDL层架构

### 平台与相关文件

* 平台：M8 MT6765 Android11 sensor-1.0
* 相关文件:
  * vendor\mediatek\proprietary\hardware\sensor\sensors-1.0，最终编译成vendor/lib64/hw/sensors.mt6765.so，不同硬件平台可能保存的名称和路径不一致
  * vendor\mediatek\proprietary\hardware\sensor\hidl\2.0，最终编译成vendor/bin/hw/android.hardware.sensors@2.0-service-mediatek
  * hardware\interfaces\sensors\2.0\ISensors.hal，google HIDL接口定义

### 相关编译配置

* HIDL 编译配置

* `vendor\mediatek\proprietary\hardware\sensor\hidl\2.0\Android.mk`:

```Makefile
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.sensors@2.0-service-mediatek
LOCAL_INIT_RC := android.hardware.sensors@2.0-service-mediatek.rc
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_OWNER := mtk
LOCAL_SRC_FILES := \
    Sensors.cpp \
    service.cpp 

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libhardware \
    libhwbinder \
    libbase \
    libutils \
    libhidlbase \
    libhidltransport \
    android.hardware.sensors@1.0 \
    android.hardware.sensors@2.0 \
    libfmq \
    libpower 

LOCAL_STATIC_LIBRARIES := \
    android.hardware.sensors@1.0-convert \
    multihal 

LOCAL_REQUIRED_MODULES += \
    sensors.$(TARGET_BOARD_PLATFORM) 

include $(BUILD_EXECUTABLE)
```

* `device/mediatek/vendor/common/device.mk`:

```
PRODUCT_PACKAGES += \
        android.hardware.sensors@2.0-service-mediatek
```

* `device/mediatek/vendor/common/project_manifest/manifest_sensor_hidl_v2.xml`:
```xml
<manifest version="1.0" type="device">
    <hal format="hidl">
        <name>android.hardware.sensors</name>
        <transport>hwbinder</transport>
        <impl level="generic"></impl>
        <version>2.0</version>
        <interface>
            <name>ISensors</name>
            <instance>default</instance>
        </interface>
    </hal>
</manifest>
``` 

* device目录下相关宏配置

主要是device/mediateksample/$project/ProjectConfig.mk文件

```log
device/mediateksample/k62v1_64_pax/ProjectConfig.mk
MTK_SENSOR_ARCHITECTURE = 1.0
MTK_SENSORS_1_0 = yes
```

* hal层SensorList对应传感器类型添加

### HIDL启动流程

* `vendor/mediatek/proprietary/hardware/sensor/hidl/2.0/android.hardware.sensors@2.0-service-mediatek.rc`系统起来后init进程会加载vendor/etc/init里的rc文件:
```
启动详情请参考：
https://www.jianshu.com/p/769c58285c22?utm_source=oschina-app

文件系统地址：
vendor/etc/init/android.hardware.sensors@2.0-service-mediatek.rc

service vendor.sensors-hal-2-0 /vendor/bin/hw/android.hardware.sensors@2.0-service-mediatek
    class main
    user system
    group system
    rlimit rtprio 10 10
```

#### 1.sensor数据流poll机制

```
* vendor\mediatek\proprietary\hardware\sensor\hidl\2.0\service.cpp
  * main()
    * android::sp<ISensors> sensors = new Sensors()          //进入HIDL Seneors.cpp中的构造函数
      * Sensors::Sensors()                                 //构造函数
        * //hw_get_module会通过SENSORS_HARDWARE_MODULE_ID找到对应HAL模块sensors-1.0/sensor.cpp  ->  /vendor/lib64/hw/sensors.mt6765.so
          * hw_get_module(SENSORS_HARDWARE_MODULE_ID,(hw_module_t const **)&mSensorModule)
            * sensors_open_1()                           //打开sensor,调用Hal层sensors-1.0/sensors.cpp里面的open_sensors()函数，这里就从HIDL层进入到snesor Hal层
              * mRunThread = std::thread(startThread, this)    //启动poll线程(这里可以知道其实sensor还没有打开，poll线程就已经启动在源源不断获取数据)
                * Sensors::startThread()
                  * Sensors::poll()
                    * while(mRunThreadEnable.load())     //while()循环持续得去获取Hal层传递上来的数据 
                      * mSensorDevice->poll            //进入Hal层调用sensors.cpp里面的poll__poll函数去获取数据
                      * convertFromSensorEvents()
                      * postEvents()
                        * while (1)                                       
                          * mEventQueue->writeBlocking()//通过while死循环将event数据源源不断写入到FMQ(快速消息队列)
    * sensors->registerAsService()       //注册sensor service
```

* `hardware/interfaces/sensors/1.0/ISensors.hal`:其他关键方法：

```
Sensors::initialize()
Sensors::getSensorsList()       //获取sensor列表，通过mSensorModule->get_sensors_list()进入Hal层的sensors.cpp->sensors__get_sensors_list()方法
Sensors::batch(int32_t sensor_handle, int64_t sampling_period_ns, int64_t max_report_latency_ns)    //通过mSensorDevice->batch进入Hal层sensors.cpp->poll__batch()方法
Sensors::activate(int32_t sensor_handle, bool enabled)      //通过mSensorDevice->activate进入Hal层sensors.cpp->poll__activate()方法
Sensors::flush(int32_t sensor_handle)                       //通过mSensorDevice->flush进入Hal层sensors.cpp->poll__flush()方法
```

### 2.open_sensors()函数了解open工作流程

* 流程分析：

```
//由上面HIDL层工作流程分析中的sensors_open_1()可知，会调用Hal层sensors.cpp  .open = open_sensors, 由此入口正式进入到Hal层
* open = open_sensors
    * open_sensors()
        * init_sensors()
            * mSensorList = SensorList::getInstance();
                * SensorList *mInterface = new SensorList
                    * SensorList::SensorList()
                        * SensorList::initSensorList()         //初始化sensor列表，这里通过相关宏配置来实现是否需要添加对应传感器到SensorList
            * mSensorList->getSensorList(&list)
                * SensorList::getSensorList
            * dev = sensors_poll_context_t::getInstance()                   //获取sensors_poll_context_t实例，sensors_poll_context_t的作用是统一相关操作snesor的接口
                * getInstance()--->new sensors_poll_context_t               //SensorContext.cpp,创建sensors_poll_context_t对象
                    * sensors_poll_context_t::sensors_poll_context_t()      //开始创建各个sensor的实例对象
                        * mSensors[accel] = new AccelerationSensor();
                            * AccelerationSensor::AccelerationSensor()      //Acceleration.cpp

            * dev->device.activate        = poll__activate;
            * dev->device.setDelay        = poll__setDelay;
            * dev->device.poll            = poll__poll;                     //HIDL层进入到Hal层 mSensorDevice相关调用接口
            * dev->device.batch           = poll__batch;
            * dev->device.flush           = poll__flush;

            * mSensorManager = SensorManager::getInstance();                //获取SensorManager实例对象
                * SensorManager *sensors = new SensorManager
                    * SensorManager::SensorManager()
            * mSensorManager->addSensorsList(list, count);                  //将获取到的sensor列表添加到mSensorList容器里面
                * SensorManager::addSensorsList()
            * mNativeConnection = mSensorManager->createSensorConnection(numFds);
                * SensorManager::createSensorConnection()
            * mSensorManager->setNativeConnection(mNativeConnection);       //将本地sensor连接保存到SensorManager
                * SensorManager::setNativeConnection()
            * mSensorManager->setSensorContext(dev);
                * SensorManager::setSensorContext()                         //设置sensor上下文
            * mVendorInterface = VendorInterface::getInstance();            //这些应该是进入校正库的相关调用接口
                * VendorInterface *mInterface = new VendorInterface
                    * VendorInterface::VendorInterface()
                        * fd = TEMP_FAILURE_RETRY(open("/sys/class/sensor/m_mag_misc/maglibinfo", O_RDWR))  //通过maglibinfo节点绑定的fops去找到校正库
                        * len = TEMP_FAILURE_RETRY(read(fd, &libinfo, sizeof(struct mag_libinfo_t)))
            * mMtkInterface = MtkInterface::getInstance();
            * mSensorCalibration = SensorCalibration::getInstance();        //sensor校正相关
            * mDirectChannelManager = DirectChannelManager::getInstance();
                            
//思考：sensors.cpp只是定义了poll__batch()函数，那么是在哪里调用的？？？
//--->framework-native-services-sensorservice-SensorService.cpp:  SensorService::enable()  sensor->batch()会通过hidl调用到Hal层的poll__batch() 
sensors.cpp：
* poll__batch()                          
    * SensorManager::batch()
        * sensors_poll_context_t::batch()
            * AccelerationSensor::batch()    //操作/sys/class/sensor/m_acc_misc/accbatch节点,进入到内核空间调用驱动accel.c->accbatch_store()函数

//--->framework-native-services-sensorservice-SensorService.cpp:  SensorService::enable()  sensor->activate()会通过hidl调用到Hal层的poll__activate()
sensors.cpp：                     
* poll__activate()                           //sensor使能函数
    * SensorManager::activate()
        * sensors_poll_context_t::activate()
            * AccelerationSensor::enable()   //操作/sys/class/sensor/m_acc_misc/accactive节点，进入到内核空间调用驱动accel.c->accactive_store()函数

//启动poll线程后就一直在循环读取数据
sensors.cpp                     
* poll__poll()           //AccelerationSensor::enable打开加速度传感器后，上层就一直在循环调用poll__poll()获取传感器数据
    * SensorManager::pollEvent()
        * sensors_poll_context_t::pollEvent()                       //轮询事件,启动轮询机制，监听sensorlist[]中文件描述符，等待事件上报 
            * AccelerationSensor::readEvents()                      //读取事件
                * SensorEventCircularReader::fill()                 //将下方read(mReadFd,..)读取到的数据保存在mBuffer
                    * read(mReadFd,..)   //mReadFd就是/dev/m_acc_misc文件描述符，m_acc_misc设备节点绑定了fops,通过read()进入到内核空间调用驱动accel_read()
                        * SensorEventCircularReader::readEvent()    //将上面mBuffer里面的数据保存在 *event
                * AccelerationSensor::processEvent()                //事件处理与校正，上面获取到的*event数据,将(sensor_event *event)转换成(sensors_event_t mPendingEvent)
                    * SensorEventCircularReader::next()             //获取下一次的数据
```


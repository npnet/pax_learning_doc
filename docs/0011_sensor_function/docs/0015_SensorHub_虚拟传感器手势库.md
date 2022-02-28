# 概述

SensorHub 虚拟传感器介绍。

## 分析

* `chre.mk`:

```C++
######## FLP support ########
ifeq ($(CFG_FLP_SUPPORT),yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/ccci
INCLUDES += -I$(SOURCE_DIR)/drivers/common/ccci
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/service
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/service/contexthub_flp.c
CFG_CNN_TO_SCP_BUF_SIZE = 0x8c
CFG_SCP_TO_CNN_BUF_SIZE = 0x8c
endif

######## geofence sensor support ########
ifeq ($(CFG_GEOFENCE_SUPPORT),yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/ccci
INCLUDES += -I$(SOURCE_DIR)/drivers/common/ccci
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/geofence_adaptor.c
endif

######## stepRecognition--STEP_COUNTER ########
ifeq ($(CFG_STEP_COUNTER_SUPPORT),yes)
CFLAGS += -D_STEP_COUNTER_
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/pedometer/v2
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/stepRecognition.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/pedometer/v2 -lpedometer
endif

######## stepRecognition--STEP_DETECTOR ########
ifeq ($(CFG_STEP_DETECTOR_SUPPORT),yes)
CFLAGS += -D_STEP_DETECTOR_
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/pedometer/v2
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/stepRecognition.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/pedometer/v2 -lpedometer
endif

######## stepRecognition--SIGNIFICANT_MOTION ########
ifeq ($(CFG_SIGNIFICANT_MOTION_SUPPORT),yes)
CFLAGS += -D_SMD_ENABLE_
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/pedometer/v2
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/stepRecognition.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/pedometer/v2 -lpedometer
endif

######## gestureRecognition--SHAKE ########
ifeq ($(CFG_SHAKE_SUPPORT),yes)
CFLAGS += -D_SHAKE_ENABLE_
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/common
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/gesture
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/gestureRecognition.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/common -llecommon
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/gesture -llegesture
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/gesture -llegestureshake
endif

######## gestureRecognition--SNAPSHOT ########
ifeq ($(CFG_SNAPSHOT_SUPPORT),yes)
CFLAGS += -D_SNAPSHOT_ENABLE_
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/common
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/gesture
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/activity
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/gestureRecognition.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/common -lmath
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/common -llecommon
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/gesture -llegesture
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/gesture -llegesturesnapshot
endif

######## gestureRecognition--ANSWERCALL ########
ifeq ($(CFG_ANSWERCALL_SUPPORT),yes)
CFLAGS += -D_ANSWERCALL_ENABLE_
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/answercall
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/tilt
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/answercall_detector.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/answercall -lanswercall
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/tilt -ldircommon
endif

######## gestureRecognition--TILT ########
ifeq ($(CFG_TILT_SUPPORT),yes)
CFLAGS += -D_TILT_ENABLE_
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/tilt
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/tilt_detector.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/tilt -ltilt
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/tilt -ldircommon
endif

######### activity_baro ############
ifeq ($(CFG_ACTIVITY_BARO_SUPPORT), yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/activity/
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/tilt
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/common
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/activity.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/common -lmath
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/common -llecommon
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/tilt -ltilt
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/tilt -ldircommon
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/activity -llecontext
endif

######### activity_no_baro ############
ifeq ($(CFG_ACTIVITY_NO_BARO_SUPPORT), yes)
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/activity_no_baro -llecontext_no_baro
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/activity_no_baro
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/activity_no_baro.c
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/common
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/common -llecommon
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/common -lmath
endif

######## stationary ########
ifeq ($(CFG_STATIONARY_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/common
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/tilt
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/situation
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/stationary_adaptor.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/common -lmath
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/tilt -ldircommon
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/situation -lstationary
endif

######## winOrientation ########
ifeq ($(CFG_WIN_ORIENTATION_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/win_orientation.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
endif

######## freefallRecognition ########
ifeq ($(CFG_FREEFALL_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/common
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/gesture
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/freefallRecognition.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/common -llecommon
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/gesture -llegesture
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/freefall -llegesturefreefall
endif

######## inPocket ########
ifeq ($(CFG_INPOCKET_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/common
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/in_pocket.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/common -lmath
endif

######## flat ########
ifeq ($(CFG_FLAT_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/common
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/flat.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/common -lmath
endif

######## anyMotion/noMotion ########
ifeq ($(CFG_MOTION_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/common
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/motion
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/motion_adaptor.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/common -lmath
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/motion -lmotion
endif

######## wakeup ########
ifeq ($(CFG_WAKEUP_SUPPORT),yes)
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/wakeup.c
endif

######## glance ########
ifeq ($(CFG_GLANCE_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/wakeup_gesture
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/glanceDetect.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/wakeup_gesture -lwakeupgesture
endif

######## e_glance ########
ifeq ($(CFG_E_GLANCE_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/wakeup_gesture
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/eGlanceDetect.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/wakeup_gesture -lwakeupgesture
endif

######## pickup ########
ifeq ($(CFG_PICKUP_SUPPORT),yes)
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/pickup.c
endif

######## Lift and putdown ########
ifeq ($(CFG_LIFT_PUTDOWN_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/wakeup_gesture
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/liftpdDetect.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/wakeup_gesture -lwakeupgesture
endif

######## floorCount ########
ifeq ($(CFG_FLOOR_COUNT_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/pedometer
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/floor_count
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/floor_count.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/pedometer -lpedometer
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/floor_count -lfloorcount
endif

######## gestureRecognition--LIFT ########
ifeq ($(CFG_LIFT_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/wakeup_gesture
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/liftDetect.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/wakeup_gesture -lwakeupgesture
endif

######## gestureRecognition--FLIP ########
ifeq ($(CFG_FLIP_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/flip
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/algo/tilt
INCLUDES += -I$(SOURCE_DIR)/middleware/contexthub/VIRT_Driver
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/flipDetect.c
C_FILES  += $(SOURCE_DIR)/middleware/contexthub/VIRT_Driver/algoDataResample.c
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/flip -lflip
LIBFLAGS += -L$(SOURCE_DIR)/middleware/contexthub/algo/tilt -ldircommon
endif
```

* 各虚拟传感器算法都是闭源，可以看到algo目录下的文件结构，很多.a文件，有使用说明：

```
wugn@jcrj-tf-compile:algo$ tree
.
├── activity
│   ├── context.h
│   ├── context_private.h
│   ├── context_setting.h
│   ├── context_setting_private.h
│   ├── liblecontext.a
│   ├── NOTICE.txt
│   └── README
├── activity_no_baro
│   ├── context.h
│   ├── context_private.h
│   ├── context_setting.h
│   ├── context_setting_private.h
│   ├── liblecontext_no_baro.a
│   ├── NOTICE.txt
│   └── README
├── answercall
│   ├── answercall.h
│   ├── libanswercall.a
│   ├── NOTICE.txt
│   └── README
├── auto_cali
│   ├── API_sensor_calibration.h
│   ├── libksensor.a
│   └── libksensor_nogyro.a
├── common
│   ├── activity_algorithm.h
│   ├── data_buffer.h
│   ├── extract_feature.h
│   ├── feature_setting.h
│   ├── feature_struct.h
│   ├── init_learning.h
│   ├── kiss_fft_cust.h
│   ├── _kiss_fft_guts.h
│   ├── kiss_fft.h
│   ├── kiss_fftr.h
│   ├── liblecommon.a
│   ├── libmath.a
│   ├── math_method.h
│   ├── NOTICE.txt
│   ├── README
│   ├── rf_function.h
│   ├── signal.h
│   └── simu_arm_math.h
├── flip
│   ├── flip.h
│   ├── libflip.a
│   ├── NOTICE.txt
│   └── README
├── floor_count
│   ├── floor_count.h
│   └── libfloorcount.a
├── fusion
│   ├── inc
│   │   ├── mpe_Array_multiply.h
│   │   ├── mpe_Attitude.h
│   │   ├── mpe_cm4_API.h
│   │   ├── mpe_DR.h
│   │   ├── mpe_Gesture.h
│   │   ├── mpe_Matching.h
│   │   └── mpe_math.h
│   ├── libmpe.a
│   └── readme
├── gesture
│   ├── gesture_flip_predictor.h
│   ├── gesture_freefall_predictor.h
│   ├── gesture.h
│   ├── gesture_pickup_predictor.h
│   ├── gesture_private.h
│   ├── gesture_setting.h
│   ├── gesture_setting_private.h
│   ├── gesture_shake_predictor.h
│   ├── gesture_snapshot_predictor.h
│   ├── liblegesture.a
│   ├── liblegestureflip.a
│   ├── liblegesturefreefall.a
│   ├── liblegesturepickup.a
│   ├── liblegestureshake.a
│   ├── liblegesturesnapshot.a
│   ├── NOTICE.txt
│   └── README
├── glance
│   ├── glance_common.h
│   ├── glance.h
│   ├── glance_setting.h
│   ├── glance_util.h
│   ├── libglance.a
│   ├── NOTICE.txt
│   └── README
├── lift
│   ├── liblift.a
│   ├── lift_common_sep.h
│   ├── lift.h
│   ├── lift_setting.h
│   ├── NOTICE.txt
│   └── README
├── motion
│   ├── libmotion.a
│   ├── motion.h
│   ├── NOTICE.txt
│   └── README
├── pedometer
│   ├── v1
│   │   ├── libpedometer.a
│   │   ├── NOTICE.txt
│   │   ├── pedometer_constant.h
│   │   ├── pedometer.h
│   │   ├── pedometer_private.h
│   │   ├── pedometer_util.h
│   │   ├── README
│   │   └── v3_decision_tree.h
│   └── v2
│       ├── libpedometer.a
│       ├── NOTICE.txt
│       ├── pedometer_constant.h
│       ├── pedometer.h
│       ├── pedometer_private.h
│       ├── pedometer_utils.h
│       ├── README
│       └── v3_decision_tree.h
├── situation
│   ├── libstationary.a
│   ├── NOTICE.txt
│   ├── README
│   └── stationary.h
├── tilt
│   ├── libdircommon.a
│   ├── libtilt.a
│   ├── NOTICE.txt
│   ├── README
│   ├── tilt_common.h
│   └── tilt.h
├── timestamp_cali
│   ├── API_timestamp_calibration.h
│   └── libktimestamp.a
└── wakeup_gesture
    ├── gesture.h
    ├── gesture_setting.h
    ├── gesture_util.h
    ├── libwakeupgesture.a
    ├── NOTICE.txt
    └── README
```

# Android Brightness Backlight

光感传感器和背光关系分析，最终通过Monotone Cubic Spline拟合

## 参考文档

* [0368_Android_Auto_Brightness.md](0010_Android_Auto_Brightness)
* [0434_A_Manual_Backlight.md](0011_A_Manual_Backlight.md)

## config

vendor/mediatek/proprietary/packages/overlay/vendor/FrameworkResOverlay/brightness_adaptive_support/res/values/config.xml

```xml
<?xml version="1.0" encoding="utf-8"?>
<!--
/*
** Copyright 2009, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
-->

<!-- These resources are around just to allow their values to be customized
     for different hardware and product builds.  Do not translate. -->
<resources xmlns:xliff="urn:oasis:names:tc:xliff:document:1.2">

    <!-- Flag indicating whether the we should enable the automatic brightness in Settings.
         Software implementation will be used if config_hardware_auto_brightness_available is not set -->
    <bool name="config_automatic_brightness_available">true</bool>

    <!-- Array of light sensor LUX values to define our levels for auto backlight brightness support.
         The N entries of this array define N + 1 control points as follows:
         (1-based arrays)

         Point 1:            (0, value[1]):             lux <= 0
         Point 2:     (level[1], value[2]):  0        < lux <= level[1]
         Point 3:     (level[2], value[3]):  level[2] < lux <= level[3]
         ...
         Point N+1: (level[N], value[N+1]):  level[N] < lux

         The control points must be strictly increasing.  Each control point
         corresponds to an entry in the brightness backlight values arrays.
         For example, if LUX == level[1] (first element of the levels array)
         then the brightness will be determined by value[2] (second element
         of the brightness values array).

         Spline interpolation is used to determine the auto-brightness
         backlight values for LUX levels between these control points.

         Must be overridden in platform specific overlays -->
    <integer-array name="config_autoBrightnessLevels">
        <item>128</item>
        <item>256</item>
        <item>384</item>
        <item>512</item>
        <item>640</item>
        <item>768</item>
        <item>896</item>
        <item>1024</item>
        <item>2048</item>
        <item>4096</item>
        <item>6144</item>
        <item>8192</item>
        <item>10240</item>
        <item>12288</item>
        <item>14336</item>
        <item>16384</item>
        <item>18432</item>
    </integer-array>

    <!-- Array of output values for LCD backlight corresponding to the LUX values
         in the config_autoBrightnessLevels array.  This array should have size one greater
         than the size of the config_autoBrightnessLevels array.
         The brightness values must be between 0 and 255 and be non-decreasing.
         This must be overridden in platform specific overlays -->
    <integer-array name="config_autoBrightnessLcdBacklightValues">
        <item>8</item>
        <item>64</item>
        <item>98</item>
        <item>104</item>
        <item>110</item>
        <item>116</item>
        <item>122</item>
        <item>128</item>
        <item>134</item>
        <item>182</item>
        <item>255</item>
        <item>255</item>
        <item>255</item>
        <item>255</item>
        <item>255</item>
        <item>255</item>
        <item>255</item>
        <item>255</item>
    </integer-array>

    <!-- Indicate whether to allow the device to suspend when the screen is off
         due to the proximity sensor.  This resource should only be set to true
         if the sensor HAL correctly handles the proximity sensor as a wake-up source.
         Otherwise, the device may fail to wake out of suspend reliably.
         The default is false. -->
    <bool name="config_suspendWhenScreenOffDueToProximity">true</bool>
</resources>
```

获取上面数组数据

```java
// frameworks/base/services/core/java/com/android/server/display/BrightnessMappingStrategy.java

@Nullable
public static BrightnessMappingStrategy create(Resources resources) {
    float[] luxLevels = getLuxLevels(resources.getIntArray(
            com.android.internal.R.array.config_autoBrightnessLevels));
    int[] brightnessLevelsBacklight = resources.getIntArray(
            com.android.internal.R.array.config_autoBrightnessLcdBacklightValues);
    float[] brightnessLevelsNits = getFloatArray(resources.obtainTypedArray(
            com.android.internal.R.array.config_autoBrightnessDisplayValuesNits));
    float autoBrightnessAdjustmentMaxGamma = resources.getFraction(
            com.android.internal.R.fraction.config_autoBrightnessAdjustmentMaxGamma,
            1, 1);

    float[] nitsRange = getFloatArray(resources.obtainTypedArray(
            com.android.internal.R.array.config_screenBrightnessNits));
    int[] backlightRange = resources.getIntArray(
            com.android.internal.R.array.config_screenBrightnessBacklight);

    long shortTermModelTimeout = resources.getInteger(
            com.android.internal.R.integer.config_autoBrightnessShortTermModelTimeout);
    Slog.e(TAG, "PhysicalMappingStrategy");

    Exception e = new Exception("this is a log");
    e.printStackTrace();

    Slog.e(TAG, "luxLevels" + Arrays.toString(luxLevels));
    Slog.e(TAG, "brightnessLevelsBacklight" + Arrays.toString(brightnessLevelsBacklight));
    Slog.e(TAG, "nitsRange" + Arrays.toString(nitsRange));
    Slog.e(TAG, "backlightRange" + Arrays.toString(backlightRange));
    Slog.e(TAG, "brightnessLevelsNits" + Arrays.toString(brightnessLevelsNits));

    if (isValidMapping(nitsRange, backlightRange)
            && isValidMapping(luxLevels, brightnessLevelsNits)) {
        int minimumBacklight = resources.getInteger(
                com.android.internal.R.integer.config_screenBrightnessSettingMinimum);
        int maximumBacklight = resources.getInteger(
                com.android.internal.R.integer.config_screenBrightnessSettingMaximum);
        if (backlightRange[0] > minimumBacklight
                || backlightRange[backlightRange.length - 1] < maximumBacklight) {
            Slog.w(TAG, "Screen brightness mapping does not cover whole range of available " +
                    "backlight values, autobrightness functionality may be impaired.");
        }
        BrightnessConfiguration.Builder builder = new BrightnessConfiguration.Builder(
                luxLevels, brightnessLevelsNits);
        builder.setShortTermModelTimeoutMillis(shortTermModelTimeout);
        builder.setShortTermModelLowerLuxMultiplier(SHORT_TERM_MODEL_THRESHOLD_RATIO);
        builder.setShortTermModelUpperLuxMultiplier(SHORT_TERM_MODEL_THRESHOLD_RATIO);
        Slog.e(TAG, "PhysicalMappingStrategy");
        return new PhysicalMappingStrategy(builder.build(), nitsRange, backlightRange,
                autoBrightnessAdjustmentMaxGamma);
    } else if (isValidMapping(luxLevels, brightnessLevelsBacklight)) {
        Slog.e(TAG, "SimpleMappingStrategy");
        return new SimpleMappingStrategy(luxLevels, brightnessLevelsBacklight,
                autoBrightnessAdjustmentMaxGamma, shortTermModelTimeout);
    } else {
        return null;
    }
}
```

添加的log输出diff

```diff
diff --git a/frameworks/base/services/core/java/com/android/server/display/BrightnessMappingStrategy.java b/frameworks/base/services/core/java/com/android/server/display/BrightnessMappingStrategy.java
index 6f12155c5ec..f715ca9dbe9 100644
--- a/frameworks/base/services/core/java/com/android/server/display/BrightnessMappingStrategy.java
+++ b/frameworks/base/services/core/java/com/android/server/display/BrightnessMappingStrategy.java
@@ -79,6 +79,16 @@ public abstract class BrightnessMappingStrategy {

         long shortTermModelTimeout = resources.getInteger(
                 com.android.internal.R.integer.config_autoBrightnessShortTermModelTimeout);
+        Slog.e(TAG, "PhysicalMappingStrategy");
+
+        Exception e = new Exception("this is a log");
+        e.printStackTrace();
+
+        Slog.e(TAG, "luxLevels" + Arrays.toString(luxLevels));
+        Slog.e(TAG, "brightnessLevelsBacklight" + Arrays.toString(brightnessLevelsBacklight));
+        Slog.e(TAG, "nitsRange" + Arrays.toString(nitsRange));
+        Slog.e(TAG, "backlightRange" + Arrays.toString(backlightRange));
+        Slog.e(TAG, "brightnessLevelsNits" + Arrays.toString(brightnessLevelsNits));

         if (isValidMapping(nitsRange, backlightRange)
                 && isValidMapping(luxLevels, brightnessLevelsNits)) {
@@ -96,9 +106,11 @@ public abstract class BrightnessMappingStrategy {
             builder.setShortTermModelTimeoutMillis(shortTermModelTimeout);
             builder.setShortTermModelLowerLuxMultiplier(SHORT_TERM_MODEL_THRESHOLD_RATIO);
             builder.setShortTermModelUpperLuxMultiplier(SHORT_TERM_MODEL_THRESHOLD_RATIO);
+            Slog.e(TAG, "PhysicalMappingStrategy");
             return new PhysicalMappingStrategy(builder.build(), nitsRange, backlightRange,
                     autoBrightnessAdjustmentMaxGamma);
         } else if (isValidMapping(luxLevels, brightnessLevelsBacklight)) {
+            Slog.e(TAG, "SimpleMappingStrategy");
             return new SimpleMappingStrategy(luxLevels, brightnessLevelsBacklight,
                     autoBrightnessAdjustmentMaxGamma, shortTermModelTimeout);
         } else {
```

logcat输出上面数据以便确认

```
E BrightnessMappingStrategy: PhysicalMappingStrategy
I HealthServiceWrapper: health: HealthServiceWrapper listening to instance default
E BrightnessMappingStrategy: luxLevels[0.0, 128.0, 256.0, 384.0, 512.0, 640.0, 768.0, 896.0, 1024.0, 2048.0, 4096.0, 6144.0, 8192.0, 10240.0, 12288.0, 14336.0, 16384.0, 18432.0]
E BrightnessMappingStrategy: brightnessLevelsBacklight[8, 64, 98, 104, 110, 116, 122, 128, 134, 182, 255, 255, 255, 255, 255, 255, 255, 255]
E BrightnessMappingStrategy: nitsRange[]
E BrightnessMappingStrategy: backlightRange[]
E BrightnessMappingStrategy: brightnessLevelsNits[]
D StatsPullAtomService: Registering pullers with statsd
E BrightnessMappingStrategy: SimpleMappingStrategy
```

如下是BrightnessMappingStrategy初始化过程，采用SimpleMappingStrategy策略，其中采用MonotoneCubicSpline算法

```
* frameworks/base/services/core/java/com/android/server/display/BrightnessMappingStrategy.java
  * public static BrightnessMappingStrategy create(Resources resources)
    * float[] luxLevels = getLuxLevels(resources.getIntArray(com.android.internal.R.array.config_autoBrightnessLevels));
    * int[] brightnessLevelsBacklight = resources.getIntArray(com.android.internal.R.array.config_autoBrightnessLcdBacklightValues);
    * } else if (isValidMapping(luxLevels, brightnessLevelsBacklight)) {
      * return new SimpleMappingStrategy(luxLevels, brightnessLevelsBacklight, autoBrightnessAdjustmentMaxGamma, shortTermModelTimeout);
        * private static class SimpleMappingStrategy extends BrightnessMappingStrategy
          * private SimpleMappingStrategy(float[] lux, int[] brightness, float maxGamma, long timeout)
            * for (int i = 0; i < N; i++)
              * mLux[i] = lux[i];
              * mBrightness[i] = normalizeAbsoluteBrightness(brightness[i]);
              * computeSpline();
                * Pair<float[], float[]> curve = getAdjustedCurve(mLux, mBrightness, mUserLux, mUserBrightness, mAutoBrightnessAdjustment, mMaxGamma);
                * mSpline = Spline.createSpline(curve.first, curve.second);
                  * frameworks/base/core/java/android/util/Spline.java
                    * public static Spline createSpline(float[] x, float[] y)
                      * if (isMonotonic(y))
                        * return createMonotoneCubicSpline(x, y);
                          * public static class MonotoneCubicSpline extends Spline
                            * public MonotoneCubicSpline(float[] x, float[] y)
                            * return new MonotoneCubicSpline(x, y);
```

## dumpsys display

* dumpsys display | grep mUserBrightness
* dumpsys display
  ```
  SimpleMappingStrategy
    mSpline=MonotoneCubicSpline{[(0.0, 0.027559055: 0.0017224409), (128.0, 0.2480315: 0.0013841044), (256.0, 0.38188976: 5.302923E-4), (384.0, 0.4055118: 1.5908774E-4), (512.0, 0.42913386: 1.8454727E-4),
  (640.0, 0.4527559: 1.8454727E-4), (768.0, 0.47637796: 1.8454727E-4), (896.0, 0.5: 1.8454716E-4), (1024.0, 0.52362204: 1.8454721E-4), (2048.0, 0.71259844: 1.6244003E-4), (4096.0, 1.0: 0.0), (6144.0, 1.0:
   0.0), (8192.0, 1.0: 0.0), (10240.0, 1.0: 0.0), (12288.0, 1.0: 0.0), (14336.0, 1.0: 0.0), (16384.0, 1.0: 0.0), (18432.0, 1.0: 0.0)]}
    mMaxGamma=3.0
    mAutoBrightnessAdjustment=0.0
    mUserLux=-1.0
    mUserBrightness=-1.0
  ```
  * mSpline=MonotoneCubicSpline{...}
    ```java
    // frameworks/base/core/java/android/util/Spline.java
  
    // For debugging.
    @Override
    public String toString() {
        StringBuilder str = new StringBuilder();
        final int n = mX.length;
        str.append("MonotoneCubicSpline{[");
        for (int i = 0; i < n; i++) {
            if (i != 0) {
                str.append(", ");
            }
            str.append("(").append(mX[i]);
            str.append(", ").append(mY[i]);
            str.append(": ").append(mM[i]).append(")");
        }
        str.append("]}");
        return str.toString();
    }
    ```

## Monotone Cubic Spline

pip3 install scipy

![0435-Android_Monotone_Cubic_Spline.png](images/0435-Android_Monotone_Cubic_Spline.png)

```python
import sys
import numpy as np
from scipy.interpolate import CubicSpline
from matplotlib import pyplot as plt


# 示例数据，从Android中通过dumpsys display命令，获取的SimpleMappingStrategy实例变量MonotoneCubicSpline数据
data = [
    (0.0, 0.027559055, 0.0017224409), 
    (128.0, 0.2480315, 0.0013841044), 
    (256.0, 0.38188976, 5.302923E-4), 
    (384.0, 0.4055118, 1.5908774E-4), 
    (512.0, 0.42913386, 1.8454727E-4), 
    (640.0, 0.4527559, 1.8454727E-4), 
    (768.0, 0.47637796, 1.8454727E-4), 
    (896.0, 0.5, 1.8454716E-4), 
    (1024.0, 0.52362204, 1.8454721E-4), 
    (2048.0, 0.71259844, 1.6244003E-4), 
    (4096.0, 1.0, 0.0), 
    # (6144.0, 1.0, 0.0), 
    # (8192.0, 1.0, 0.0), 
    # (10240.0, 1.0, 0.0), 
    # (12288.0, 1.0, 0.0), 
    # (14336.0, 1.0, 0.0), 
    # (16384.0, 1.0, 0.0), 
    # (18432.0, 1.0, 0.0)
]

def main(argv=None) :

    X1 = []
    Y1 = []
    for i in range(len(data)):
        X1.append(data[i][0])
        # Brightness量程(0 ~ 1)转为PWM量程(0 ~ 255)
        # Y1.append(data[i][1] * 255)
        # 不转换PWM量程(0 ~ 255)的原因是和Android数据方便对比，Android默认Brightness量程(0 ~ 1)
        Y1.append(data[i][1])
    
    # 生成曲线
    cs = CubicSpline(X1, Y1)

    if len(argv) == 2:
        # 单独计算一个值
        print("input lux: " +  argv[1])
        print("Brightness: " + "{:.2f}".format(cs(int(argv[1]))))
    else:
        # 生成曲线x轴坐标点
        xs = np.arange(0, X1[len(X1) - 1], 1)

        # 绘制曲线，有x轴坐标点生成y坐标点：cs(xs)
        plt.plot(xs, cs(xs), label="S")
        # 绘制点以及文本
        for i in range(len(data)):
            # 绘制点
            plt.plot(X1[i], Y1[i], 'o')
            # 加入字符显示的偏移
            plt.text(X1[i] - 15, Y1[i] + 0.02, str(X1[i]) + ", " + "{:.2f}".format(Y1[i]), fontsize=9, rotation=90)

        plt.xlabel("X LUX")
        plt.ylabel("Y PWM(0 ~ 255)")
        plt.title("Android (Light Sensor) & (LCD PWM) Mapper")
        plt.show()

if __name__ == "__main__" :
    sys.exit(main(sys.argv))
```

* print(cs(78.4594))输出：0.16927823732647743
  * Android Logcat输入如下：
    ```
    D AutomaticBrightnessController: setAmbientLux(78.4594)
    D AutomaticBrightnessController: updateAmbientLux: Darkened: mBrighteningLuxThreshold=86.305336, mAmbientLightRingBuffer=[460.0 / 7998ms, 360.0 / 504ms, 0.0 / 1502ms], mAm
    D AutomaticBrightnessController: updateAutoBrightness: mScreenAutoBrightness=0.4194107, newScreenAutoBrightness=0.1689984
    ```
* 为了方便调试，最后还是转换成PWM量程(0 ~ 255)比较合适，实际查看亮度值如下
  * lux: 550
    * print(cs(550)) PWM量程值: 111.35041199897756
    * cat /sys/class/leds/lcd-backlight/brightness
      ```
      107
      ```
    * 和107很接近了
  * lux: 460
    * print(cs(550)) PWM量程值: 106.56775399154981
    * cat /sys/class/leds/lcd-backlight/brightness
      ```
      107
      ```
    * 和107很接近了
  * lux: 180
    * print(cs(550)) PWM量程值: 80.75037983146315
    * cat /sys/class/leds/lcd-backlight/brightness
      ```
      81
      ```
    * 和81很接近了

## Light Sensor

注册传感器

```java
// frameworks/base/services/core/java/com/android/server/display/AutomaticBrightnessController.java

private final SensorEventListener mLightSensorListener = new SensorEventListener() {
    @Override
    public void onSensorChanged(SensorEvent event) {
        if (mLightSensorEnabled) {
            final long time = SystemClock.uptimeMillis();
            final float lux = event.values[0];
            handleLightSensorEvent(time, lux);
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        // Not used.
    }
};

private boolean setLightSensorEnabled(boolean enable) {
    if (enable) {
        if (!mLightSensorEnabled) {
            mLightSensorEnabled = true;
            mLightSensorEnableTime = SystemClock.uptimeMillis();
            mCurrentLightSensorRate = mInitialLightSensorRate;
            registerForegroundAppUpdater();
            mSensorManager.registerListener(mLightSensorListener, mLightSensor,
                    mCurrentLightSensorRate * 1000, mHandler);
            return true;
        }
    } else if (mLightSensorEnabled) {
        mLightSensorEnabled = false;
        mAmbientLuxValid = !mResetAmbientLuxAfterWarmUpConfig;
        mScreenAutoBrightness = PowerManager.BRIGHTNESS_INVALID_FLOAT;
        mRecentLightSamples = 0;
        mAmbientLightRingBuffer.clear();
        mCurrentLightSensorRate = -1;
        mHandler.removeMessages(MSG_UPDATE_AMBIENT_LUX);
        unregisterForegroundAppUpdater();
        mSensorManager.unregisterListener(mLightSensorListener);
    }
    return false;
}
```

分析diff

```diff
diff --git a/frameworks/base/services/core/java/com/android/server/display/DisplayPowerController.java b/frameworks/base/services/core/java/com/android/server/display/DisplayPowerController.java
index 32cde50d5ec..6aed7693b53 100644
--- a/frameworks/base/services/core/java/com/android/server/display/DisplayPowerController.java
+++ b/frameworks/base/services/core/java/com/android/server/display/DisplayPowerController.java
@@ -95,7 +95,7 @@ final class DisplayPowerController implements AutomaticBrightnessController.Call
     private static final String SCREEN_ON_BLOCKED_TRACE_NAME = "Screen on blocked";
     private static final String SCREEN_OFF_BLOCKED_TRACE_NAME = "Screen off blocked";

-    private static final boolean DEBUG = SystemProperties.getBoolean("dbg.dms.dpc", false);
+    private static final boolean DEBUG = true;
     private static final boolean MTK_DEBUG = "eng".equals(Build.TYPE);
     private static final boolean DEBUG_PRETEND_PROXIMITY_SENSOR_ABSENT = false;

@@ -763,6 +763,9 @@ final class DisplayPowerController implements AutomaticBrightnessController.Call
         int brightnessAdjustmentFlags = 0;
         mBrightnessReasonTemp.set(null);

+        Exception e = new Exception("this is a log");
+        e.printStackTrace();
+
         synchronized (mLock) {
             mPendingUpdatePowerStateLocked = false;
             if (mPendingRequestLocked == null) {
diff --git a/frameworks/base/services/core/java/com/android/server/display/DisplayPowerState.java b/frameworks/base/services/core/java/com/android/server/display/DisplayPowerState.java
index 68ae8ecae86..54b1ca112b8 100644
--- a/frameworks/base/services/core/java/com/android/server/display/DisplayPowerState.java
+++ b/frameworks/base/services/core/java/com/android/server/display/DisplayPowerState.java
@@ -51,7 +51,7 @@ import java.io.PrintWriter;
 final class DisplayPowerState {
     private static final String TAG = "DisplayPowerState";

-    private static boolean DEBUG = SystemProperties.getBoolean("dbg.dms.dps", false);
+    private static boolean DEBUG = true;
     private static String COUNTER_COLOR_FADE = "ColorFadeLevel";

     private final Handler mHandler;
@@ -155,6 +155,9 @@ final class DisplayPowerState {
                 Slog.d(TAG, "setScreenBrightness: brightness=" + brightness);
             }

+            Exception e = new Exception("this is a log");
+            e.printStackTrace();
+
             mScreenBrightness = brightness;
             if (mScreenState != Display.STATE_OFF) {
                 mScreenReady = false;
diff --git a/frameworks/base/services/core/java/com/android/server/display/RampAnimator.java b/frameworks/base/services/core/java/com/android/server/display/RampAnimator.java
index 7916d816dc9..9ea3706fca9 100644
--- a/frameworks/base/services/core/java/com/android/server/display/RampAnimator.java
+++ b/frameworks/base/services/core/java/com/android/server/display/RampAnimator.java
@@ -98,6 +98,9 @@ final class RampAnimator<T> {
         final boolean changed = (mTargetValue != target);
         mTargetValue = target;

+        Exception e = new Exception("this is a log");
+        e.printStackTrace();
+
         // Start animating.
         if (!mAnimating && target != mCurrentValue) {
             mAnimating = true;
```

Logcat输出的diff log

```
10-27 06:25:51.096  1145  1224 D AutomaticBrightnessController: calculateAmbientLux: totalWeight=1.9005E7, newAmbientLux=78.4594
10-27 06:25:51.096  1145  1224 D AutomaticBrightnessController: setAmbientLux(78.4594)
10-27 06:25:51.097  1145  1224 D AutomaticBrightnessController: updateAmbientLux: Darkened: mBrighteningLuxThreshold=86.305336, mAmbientLightRingBuffer=[460.0 / 7998ms, 360.0 / 504ms, 0.0 / 1502ms], mAm
bientLux=78.4594
10-27 06:25:51.097  1145  1224 D AutomaticBrightnessController: updateAutoBrightness: mScreenAutoBrightness=0.4194107, newScreenAutoBrightness=0.1689984
10-27 06:25:51.098  1145  1224 D AutomaticBrightnessController: updateAmbientLux: Scheduling ambient lux update for 9052090520 (in 497 ms)
10-27 06:25:51.098  1145  1224 W System.err: java.lang.Exception: this is a log
10-27 06:25:51.099  1145  1224 W System.err:    at com.android.server.display.DisplayPowerController.updatePowerState(DisplayPowerController.java:766)
10-27 06:25:51.101  1145  1224 W System.err:    at com.android.server.display.DisplayPowerController.access$600(DisplayPowerController.java:92)
10-27 06:25:51.102  1145  1224 W System.err:    at com.android.server.display.DisplayPowerController$DisplayControllerHandler.handleMessage(DisplayPowerController.java:1952)
10-27 06:25:51.103  1145  1224 W System.err:    at android.os.Handler.dispatchMessage(Handler.java:106)
10-27 06:25:51.104  1145  1224 W System.err:    at android.os.Looper.loop(Looper.java:223)
10-27 06:25:51.104  1145  1224 W System.err:    at android.os.HandlerThread.run(HandlerThread.java:67)
10-27 06:25:51.104  1145  1224 W System.err:    at com.android.server.ServiceThread.run(ServiceThread.java:44)
10-27 06:25:51.106  1145  1224 V SettingsProvider: name : screen_brightness_float appId : 1000
10-27 06:25:51.108  1145  1224 D DisplayPowerController: Animating brightness: target=0.1689984, rate=0.2352941
10-27 06:25:51.111  1145  1145 V SettingsProvider: name : screen_brightness appId : 1000
10-27 06:25:51.112  1145  1224 D DisplayPowerController: Unfinished business...
10-27 06:25:51.113  1145  1224 W System.err: java.lang.Exception: this is a log
10-27 06:25:51.113  1145  1224 W System.err:    at com.android.server.display.DisplayPowerController.updatePowerState(DisplayPowerController.java:766)
10-27 06:25:51.114  1145  1224 W System.err:    at com.android.server.display.DisplayPowerController.access$600(DisplayPowerController.java:92)
10-27 06:25:51.114  1145  1224 W System.err:    at com.android.server.display.DisplayPowerController$DisplayControllerHandler.handleMessage(DisplayPowerController.java:1952)
10-27 06:25:51.114  1145  1224 W System.err:    at android.os.Handler.dispatchMessage(Handler.java:106)
10-27 06:25:51.114  1145  1224 W System.err:    at android.os.Looper.loop(Looper.java:223)
10-27 06:25:51.114  1145  1224 W System.err:    at android.os.HandlerThread.run(HandlerThread.java:67)
10-27 06:25:51.114  1145  1224 W System.err:    at com.android.server.ServiceThread.run(ServiceThread.java:44)
10-27 06:25:51.115  1145  1224 V SettingsProvider: name : screen_brightness_float appId : 1000
10-27 06:25:51.116  1145  1224 D DisplayPowerController: Animating brightness: target=0.1689984, rate=0.2352941
10-27 06:25:51.119  1145  1224 W System.err: java.lang.Exception: this is a log
10-27 06:25:51.120  1145  1145 V SettingsProvider: name : screen_brightness appId : 1000
10-27 06:25:51.120  1145  1224 W System.err:    at com.android.server.display.DisplayPowerController.updatePowerState(DisplayPowerController.java:766)
10-27 06:25:51.121  1145  1224 W System.err:    at com.android.server.display.DisplayPowerController.access$600(DisplayPowerController.java:92)
10-27 06:25:51.121  1145  1224 W System.err:    at com.android.server.display.DisplayPowerController$DisplayControllerHandler.handleMessage(DisplayPowerController.java:1952)
10-27 06:25:51.122  1145  1224 W System.err:    at android.os.Handler.dispatchMessage(Handler.java:106)
10-27 06:25:51.122  1145  1224 W System.err:    at android.os.Looper.loop(Looper.java:223)
10-27 06:25:51.123  1145  1224 W System.err:    at android.os.HandlerThread.run(HandlerThread.java:67)
10-27 06:25:51.123  1145  1224 W System.err:    at com.android.server.ServiceThread.run(ServiceThread.java:44)
10-27 06:25:51.124  1145  1224 V SettingsProvider: name : screen_brightness_float appId : 1000
10-27 06:25:51.125  1145  1224 D DisplayPowerController: Animating brightness: target=0.1689984, rate=0.2352941
10-27 06:25:51.126  1145  1224 D DisplayPowerState: setScreenBrightness: brightness=0.41628808
10-27 06:25:51.127  1145  1224 W System.err: java.lang.Exception: this is a log
10-27 06:25:51.128  1145  1224 W System.err:    at com.android.server.display.DisplayPowerState.setScreenBrightness(DisplayPowerState.java:158)
10-27 06:25:51.128  1145  1224 W System.err:    at com.android.server.display.DisplayPowerState$2.setValue(DisplayPowerState.java:116)
10-27 06:25:51.129  1145  1224 W System.err:    at com.android.server.display.DisplayPowerState$2.setValue(DisplayPowerState.java:113)
10-27 06:25:51.129  1145  1224 W System.err:    at com.android.server.display.RampAnimator$1.run(RampAnimator.java:161)
10-27 06:25:51.130  1145  1224 W System.err:    at android.view.Choreographer$CallbackRecord.run(Choreographer.java:974)
10-27 06:25:51.130  1145  1224 W System.err:    at android.view.Choreographer.doCallbacks(Choreographer.java:797)
10-27 06:25:51.130  1145  1224 W System.err:    at android.view.Choreographer.doFrame(Choreographer.java:728)
10-27 06:25:51.131  1145  1224 W System.err:    at android.view.Choreographer$FrameDisplayEventReceiver.run(Choreographer.java:959)
10-27 06:25:51.131  1145  1224 W System.err:    at android.os.Handler.handleCallback(Handler.java:938)
10-27 06:25:51.132  1145  1224 W System.err:    at android.os.Handler.dispatchMessage(Handler.java:99)
10-27 06:25:51.132  1145  1224 W System.err:    at android.os.Looper.loop(Looper.java:223)
10-27 06:25:51.133  1145  1224 W System.err:    at android.os.HandlerThread.run(HandlerThread.java:67)
10-27 06:25:51.133  1145  1224 W System.err:    at com.android.server.ServiceThread.run(ServiceThread.java:44)
10-27 06:25:51.134  1145  1224 D DisplayPowerState: Requesting new screen state: state=ON, backlight=0.41628808
10-27 06:25:51.135  1145  1224 D DisplayPowerState: Screen ready
10-27 06:25:51.135  1145  3232 D DisplayPowerState: Updating screen state: state=ON, backlight=0.41628808
10-27 06:25:51.135  1145  1224 D DisplayPowerState: Screen ready
10-27 06:25:51.136   525   525 D android.hardware.lights-service.mediatek: write 107 to /sys/class/leds/lcd-backlight/brightness, result: 0
10-27 06:25:51.138   773   844 D AAL     : onBacklightChanged: 433/1023 -> 429/1023(phy:1717/4095)
10-27 06:25:51.138   773   844 D AAL     : LABC: LABC after setTarget isSmoothBacklight=1
```

光感传感器工作流程

```
* frameworks/base/services/core/java/com/android/server/display/AutomaticBrightnessController.java
  * private boolean setLightSensorEnabled(boolean enable)
    * mSensorManager.registerListener(mLightSensorListener, mLightSensor, mCurrentLightSensorRate * 1000, mHandler);
      * private final SensorEventListener mLightSensorListener = new SensorEventListener() { }
        * public void onSensorChanged(SensorEvent event)
          * handleLightSensorEvent(time, lux);
            * applyLightSensorMeasurement(time, lux);
              * mAmbientLightRingBuffer.push(time, lux);
              * mLastObservedLux = lux;
              * mLastObservedLuxTime = time;
            * updateAmbientLux(time);
              * setAmbientLux(fastAmbientLux);
              * updateAutoBrightness(true /* sendUpdate */, false /* isManuallySet */);
                * float value = mBrightnessMapper.getBrightness(mAmbientLux, mForegroundAppPackageName, mForegroundAppCategory);
                * float newScreenAutoBrightness = clampScreenBrightness(value);
                * mScreenAutoBrightness = newScreenAutoBrightness;
                  * 后面使用这个值更新屏幕亮度
                * if (sendUpdate)
                  * mCallbacks.updateBrightness();
                    * frameworks/base/services/core/java/com/android/server/display/DisplayPowerController.java
                      * public void updateBrightness()
                        * sendUpdatePowerState();
                          * sendUpdatePowerStateLocked();
                            * Message msg = mHandler.obtainMessage(MSG_UPDATE_POWER_STATE);
                            * mHandler.sendMessage(msg)
                              * private final class DisplayControllerHandler extends Handler
                                * public void handleMessage(Message msg)
                                  * case MSG_UPDATE_POWER_STATE:
                                    * updatePowerState()
                                      * if (Float.isNaN(brightnessState))
                                        * if (autoBrightnessEnabled)
                                          * brightnessState = mAutomaticBrightnessController.getAutomaticScreenBrightness();
                                            * frameworks/base/services/core/java/com/android/server/display/AutomaticBrightnessController.java
                                              * public float getAutomaticScreenBrightness()
                                                * return mScreenAutoBrightness;
                                          * newAutoBrightnessAdjustment = mAutomaticBrightnessController.getAutomaticScreenBrightnessAdjustment();
                                            * frameworks/base/services/core/java/com/android/server/display/AutomaticBrightnessController.java
                                              * public float getAutomaticScreenBrightnessAdjustment()
                                                * return mBrightnessMapper.getAutoBrightnessAdjustment();
                                                  * frameworks/base/services/core/java/com/android/server/display/BrightnessMappingStrategy.java
                                                    * private static class SimpleMappingStrategy extends BrightnessMappingStrategy
                                                      * public float getAutoBrightnessAdjustment()
                                                        * return mAutoBrightnessAdjustment;
                                          * float animateValue = clampScreenBrightness(brightnessState);
                                          * animateScreenBrightness(animateValue, sdrAnimateValue, rampSpeed);
                                            * if (DEBUG)
                                              * Slog.d(TAG, "Animating brightness: target=" + target + ", sdrTarget=" + sdrTarget + ", rate=" + rate);
                                            * mScreenBrightnessRampAnimator.animateTo(target, sdrTarget, rate)
                                              * frameworks/base/services/core/java/com/android/server/display/RampAnimator.java
                                                * public boolean animateTo(float target, float rate)
                                                  * if (!mAnimating && target != mCurrentValue)
                                                    * postAnimationCallback();
                                                      * mChoreographer.postCallback(Choreographer.CALLBACK_ANIMATION, mAnimationCallback, null);
                                                        * private final Runnable mAnimationCallback = new Runnable()
                                                          * public void run()
                                                            * mProperty.setValue(mObject, mCurrentValue);
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
                                                            * if (mTargetValue != mCurrentValue)
                                                              * postAnimationCallback();
```

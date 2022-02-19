# Android Auto Brightness

理解SystemUI中的亮度调节以及自动亮度调节调用流程

## Java打印函数栈

```Java
Exception e = new Exception("stack dump");
e.printStackTrace();
```

## Settings

* vendor/mediatek/proprietary/packages/apps/SettingsLib
* vendor/mediatek/proprietary/packages/apps/MtkSettings

### layout

* src/main/res/xml/auto_brightness_detail.xml
  ```xml
  <PreferenceScreen
      xmlns:android="http://schemas.android.com/apk/res/android"
      xmlns:settings="http://schemas.android.com/apk/res-auto"
      android:key="auto_brightness_detail"
      android:title="@string/auto_brightness_title">
  
      <com.android.settings.widget.VideoPreference
          android:key="auto_brightness_video"
          android:title="@string/summary_placeholder"
          settings:animation="@raw/aab_brightness"
          settings:preview="@drawable/aab_brightness"
          settings:controller="com.android.settings.widget.VideoPreferenceController"/>
  
      <!-- Cross-listed item, if you change this, also change it in power_usage_summary.xml -->
      <com.android.settingslib.RestrictedSwitchPreference
          android:key="auto_brightness"
          android:title="@string/auto_brightness_title"
          android:summary="@string/auto_brightness_summary"
          settings:keywords="@string/keywords_display_auto_brightness"
          settings:controller="com.android.settings.display.AutoBrightnessDetailPreferenceController"
          settings:useAdminDisabledSummary="true"
          settings:userRestriction="no_config_brightness"
          settings:allowDividerAbove="true" />
  
  </PreferenceScreen>
  ```

### Settings处理代码

* src/main/java/com/android/settings/display/AutoBrightnessDetailPreferenceController.java
  ```Java
  package com.android.settings.display;
  
  import android.content.Context;
  
  public class AutoBrightnessDetailPreferenceController extends AutoBrightnessPreferenceController {
  
      // ...省略
  }
  ```
* src/main/java/com/android/settings/display/AutoBrightnessPreferenceController.java
  ```Java
  package com.android.settings.display;
  
  import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE;
  import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE_AUTOMATIC;
  import static android.provider.Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL;
  
  // ...省略
  
  public class AutoBrightnessPreferenceController extends TogglePreferenceController {
  
      private final String SYSTEM_KEY = SCREEN_BRIGHTNESS_MODE;
      private final int DEFAULT_VALUE = SCREEN_BRIGHTNESS_MODE_MANUAL;
  
      public AutoBrightnessPreferenceController(Context context, String key) {
          super(context, key);
      }
  
      @Override
      public boolean isChecked() {
          return Settings.System.getInt(mContext.getContentResolver(),
                  SYSTEM_KEY, DEFAULT_VALUE) != DEFAULT_VALUE;
      }
  
      @Override
      public boolean setChecked(boolean isChecked) {
          Settings.System.putInt(mContext.getContentResolver(), SYSTEM_KEY,
                  isChecked ? SCREEN_BRIGHTNESS_MODE_AUTOMATIC : DEFAULT_VALUE);
          return true;
      }
  
      // ...省略
  }
  ```

## logcat log

```
08-13 09:34:52.954  1160  1244 V SettingsProvider: name : screen_brightness_float appId : 1000
08-13 09:34:52.954  1160  1244 V DisplayPowerController: Brightness [0.04] reason changing to: 'automatic', previous reason: 'manual'.
```

## Settings自动调光开启调用流程

## 函数调用栈

```
08-13 09:50:07.930   952  1033 E PowerManagerService: onChange() uri: content://settings/system/screen_brightness_mode
08-13 09:50:07.933   952  1033 W System.err: java.lang.Exception: stack dump
08-13 09:50:07.934   952  1033 W System.err:    at com.android.server.display.DisplayPowerController.sendUpdatePowerStateLocked(DisplayPowerController.java:681)
08-13 09:50:07.934   952  1033 W System.err:    at com.android.server.display.DisplayPowerController.requestPowerState(DisplayPowerController.java:652)
08-13 09:50:07.934   952  1033 W System.err:    at com.android.server.display.DisplayManagerService$LocalService.requestPowerState(DisplayManagerService.java:2560)
08-13 09:50:07.934   952  1033 W System.err:    at com.android.server.power.PowerManagerService.updateDisplayPowerStateLocked(PowerManagerService.java:2935)
08-13 09:50:07.934   952  1033 W System.err:    at com.android.server.power.PowerManagerService.updatePowerStateLocked(PowerManagerService.java:2001)
08-13 09:50:07.934   952  1033 W System.err:    at com.android.server.power.PowerManagerService.handleSettingsChangedLocked(PowerManagerService.java:1277)
08-13 09:50:07.934   952  1033 W System.err:    at com.android.server.power.PowerManagerService.access$3200(PowerManagerService.java:125)
08-13 09:50:07.934   952  1033 W System.err:    at com.android.server.power.PowerManagerService$SettingsObserver.onChange(PowerManagerService.java:4452)
08-13 09:50:07.934   952  1033 W System.err:    at android.database.ContentObserver.onChange(ContentObserver.java:169)
08-13 09:50:07.934   952  1033 W System.err:    at android.database.ContentObserver.onChange(ContentObserver.java:187)
08-13 09:50:07.934   952  1033 W System.err:    at android.database.ContentObserver.onChange(ContentObserver.java:200)
08-13 09:50:07.934   952  1033 W System.err:    at android.database.ContentObserver.lambda$dispatchChange$0$ContentObserver(ContentObserver.java:282)
08-13 09:50:07.934   952  1033 W System.err:    at android.database.-$$Lambda$ContentObserver$MgqiYb2qvgLhoXTioYXq9MvvpNk.run(Unknown Source:10)
08-13 09:50:07.934   952  1033 W System.err:    at android.os.Handler.handleCallback(Handler.java:938)
08-13 09:50:07.935   952  1033 W System.err:    at android.os.Handler.dispatchMessage(Handler.java:99)
08-13 09:50:07.935   952  1033 W System.err:    at android.os.Looper.loop(Looper.java:223)
08-13 09:50:07.935   952  1033 W System.err:    at android.os.HandlerThread.run(HandlerThread.java:67)
08-13 09:50:07.935   952  1033 W System.err:    at com.android.server.ServiceThread.run(ServiceThread.java:44)
08-13 09:50:07.935   952  1033 V DisplayPowerController: msg.what: 1
08-13 09:50:07.935   952  1033 W System.err: java.lang.Exception: stack dump
08-13 09:50:07.935   952  1033 W System.err:    at com.android.server.display.AutomaticBrightnessController.configure(AutomaticBrightnessController.java:321)
08-13 09:50:07.936   952  1033 W System.err:    at com.android.server.display.DisplayPowerController.updatePowerState(DisplayPowerController.java:948)
08-13 09:50:07.936   952  1033 W System.err:    at com.android.server.display.DisplayPowerController.access$600(DisplayPowerController.java:92)
08-13 09:50:07.936   952  1033 W System.err:    at com.android.server.display.DisplayPowerController$DisplayControllerHandler.handleMessage(DisplayPowerController.java:1948)
08-13 09:50:07.936   952  1033 W System.err:    at android.os.Handler.dispatchMessage(Handler.java:106)
08-13 09:50:07.936   952  1033 W System.err:    at android.os.Looper.loop(Looper.java:223)
08-13 09:50:07.936   952  1033 W System.err:    at android.os.HandlerThread.run(HandlerThread.java:67)
08-13 09:50:07.936   952  1033 W System.err:    at com.android.server.ServiceThread.run(ServiceThread.java:44)
```

### 调用流程如下

```
* frameworks/base/services/core/java/com/android/server/power/PowerManagerService.java
  * public void systemReady(IAppOpsService appOps)
    * resolver.registerContentObserver(Settings.System.getUriFor( Settings.System.SCREEN_BRIGHTNESS_MODE), false, mSettingsObserver, UserHandle.USER_ALL);
      * mSettingsObserver = new SettingsObserver(mHandler);
        * private final class SettingsObserver extends ContentObserver
          * public void onChange(boolean selfChange, Uri uri)
            * handleSettingsChangedLocked();
              * updateSettingsLocked();
                * final ContentResolver resolver = mContext.getContentResolver();
                * mScreenBrightnessModeSetting = Settings.System.getIntForUser(resolver, Settings.System.SCREEN_BRIGHTNESS_MODE, Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL, UserHandle.USER_CURRENT);
              * updatePowerStateLocked();
                * final boolean displayBecameReady = updateDisplayPowerStateLocked(dirtyPhase2);
                  * mDisplayReady = mDisplayManagerInternal.requestPowerState(mDisplayPowerRequest, mRequestWaitForNegativeProximity);
                    * frameworks/base/services/core/java/com/android/server/display/DisplayManagerService.java
                      * public boolean requestPowerState(DisplayPowerRequest request, boolean waitForNegativeProximity) 
                        * return mDisplayPowerController.requestPowerState(request, waitForNegativeProximity);
                          * frameworks/base/services/core/java/com/android/server/display/DisplayPowerController.java
                            * public boolean requestPowerState(DisplayPowerRequest request, boolean waitForNegativeProximity) 
                                * sendUpdatePowerStateLocked();
                                  * Message msg = mHandler.obtainMessage(MSG_UPDATE_POWER_STATE);
                                    * private final class DisplayControllerHandler extends Handler
                                        * updatePowerState();
                                          * mAutomaticBrightnessController.configure(autoBrightnessEnabled, mBrightnessConfiguration, mLastUserSetScreenBrightness, userSetBrightnessChanged, autoBrightnessAdjustment, autoBrightnessAdjustmentChanged, mPowerRequest.policy);
                                            * private boolean setLightSensorEnabled(boolean enable)
```

### 相关代码如下

* 监听Settings数据变化是在PowerManagerService中处理的；
* 然后跳转到DisplayManagerService中处理；

```Java
// frameworks/base/services/core/java/com/android/server/power/PowerManagerService.java


public void systemReady(IAppOpsService appOps) {
    synchronized (mLock) {
        // ...省略
        mSettingsObserver = new SettingsObserver(mHandler);
        // ...省略
    }

    final ContentResolver resolver = mContext.getContentResolver();
    mConstants.start(resolver);

    // Register for settings changes.
    // ...省略
    resolver.registerContentObserver(Settings.System.getUriFor(
            Settings.System.SCREEN_BRIGHTNESS_MODE),
            false, mSettingsObserver, UserHandle.USER_ALL);
    resolver.registerContentObserver(Settings.System.getUriFor(
            Settings.System.SCREEN_AUTO_BRIGHTNESS_ADJ),
            false, mSettingsObserver, UserHandle.USER_ALL);
    // ...省略
}

private final class SettingsObserver extends ContentObserver {
    public SettingsObserver(Handler handler) {
        super(handler);
    }

    @Override
    public void onChange(boolean selfChange, Uri uri) {
        synchronized (mLock) {
            handleSettingsChangedLocked();
        }
    }
}

private void handleSettingsChangedLocked() {
    updateSettingsLocked();
    updatePowerStateLocked();
}

private void updateSettingsLocked() {
    final ContentResolver resolver = mContext.getContentResolver();

    // ...省略

    mScreenBrightnessModeSetting = Settings.System.getIntForUser(resolver,
            Settings.System.SCREEN_BRIGHTNESS_MODE,
            Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL, UserHandle.USER_CURRENT);

    mDirty |= DIRTY_SETTINGS;
}

private void updatePowerStateLocked() {
    // ...省略
    try {
        // ...省略

        // Phase 3: Update display power state.
        final boolean displayBecameReady = updateDisplayPowerStateLocked(dirtyPhase2);

        // ...省略
    } finally {
        Trace.traceEnd(Trace.TRACE_TAG_POWER);
    }
}

private boolean updateDisplayPowerStateLocked(int dirty) {
    final boolean oldDisplayReady = mDisplayReady;
    if ((dirty & (DIRTY_WAKE_LOCKS | DIRTY_USER_ACTIVITY | DIRTY_WAKEFULNESS
            | DIRTY_ACTUAL_DISPLAY_POWER_STATE_UPDATED | DIRTY_BOOT_COMPLETED
            | DIRTY_SETTINGS | DIRTY_SCREEN_BRIGHTNESS_BOOST | DIRTY_VR_MODE_CHANGED |
            DIRTY_QUIESCENT)) != 0) {
        // ...省略

        mDisplayReady = mDisplayManagerInternal.requestPowerState(mDisplayPowerRequest,
                mRequestWaitForNegativeProximity);
        mRequestWaitForNegativeProximity = false;

        // ...省略
    }
}
```

* DisplayManagerService服务调用mDisplayPowerController处理；

```Java
// frameworks/base/services/core/java/com/android/server/display/DisplayManagerService.java

@Override
public boolean requestPowerState(DisplayPowerRequest request,
        boolean waitForNegativeProximity) {
    synchronized (mSyncRoot) {
        return mDisplayPowerController.requestPowerState(request,
                waitForNegativeProximity);
    }
}
```

* DisplayManagerService服务调用mDisplayPowerController处理；
* mDisplayPowerController最终调用mAutomaticBrightnessController来配置自动背光调节；

```Java
// frameworks/base/services/core/java/com/android/server/display/DisplayPowerController.java

public boolean requestPowerState(DisplayPowerRequest request,
        boolean waitForNegativeProximity) {
    // ...省略

    synchronized (mLock) {
        // ...省略

        if (changed && !mPendingRequestChangedLocked) {
            mPendingRequestChangedLocked = true;
            sendUpdatePowerStateLocked();
        }

        // ...省略
    }
}

private void sendUpdatePowerStateLocked() {
    if (!mPendingUpdatePowerStateLocked) {
        mPendingUpdatePowerStateLocked = true;
        Message msg = mHandler.obtainMessage(MSG_UPDATE_POWER_STATE);
        mHandler.sendMessage(msg);
    }
}

private final class DisplayControllerHandler extends Handler {
    // ...省略

    @Override
    public void handleMessage(Message msg) {
        Slog.v(TAG, "msg.what: " + msg.what);
        switch (msg.what) {
            case MSG_UPDATE_POWER_STATE:
                updatePowerState();
                break;

            // ...省略
        }
    }
}

private void updatePowerState() {
    // ...省略

    // Configure auto-brightness.
    if (mAutomaticBrightnessController != null) {
        hadUserBrightnessPoint = mAutomaticBrightnessController.hasUserDataPoints();
        mAutomaticBrightnessController.configure(autoBrightnessEnabled,
                mBrightnessConfiguration,
                mLastUserSetScreenBrightness,
                userSetBrightnessChanged, autoBrightnessAdjustment,
                autoBrightnessAdjustmentChanged, mPowerRequest.policy);
    }

    // ...省略
}
```

* mAutomaticBrightnessController开启传感器数据监听；

```Java
// frameworks/base/services/core/java/com/android/server/display/AutomaticBrightnessController.java

public void configure(boolean enable, @Nullable BrightnessConfiguration configuration,
        float brightness, boolean userChangedBrightness, float adjustment,
        boolean userChangedAutoBrightnessAdjustment, int displayPolicy) {
    // While dozing, the application processor may be suspended which will prevent us from
    // receiving new information from the light sensor. On some devices, we may be able to
    // switch to a wake-up light sensor instead but for now we will simply disable the sensor
    // and hold onto the last computed screen auto brightness.  We save the dozing flag for
    // debugging purposes.

    boolean dozing = (displayPolicy == DisplayPowerRequest.POLICY_DOZE);
    boolean changed = setBrightnessConfiguration(configuration);
    changed |= setDisplayPolicy(displayPolicy);
    if (userChangedAutoBrightnessAdjustment) {
        changed |= setAutoBrightnessAdjustment(adjustment);
    }
    if (userChangedBrightness && enable) {
        // Update the brightness curve with the new user control point. It's critical this
        // happens after we update the autobrightness adjustment since it may reset it.
        changed |= setScreenBrightnessByUser(brightness);
    }
    final boolean userInitiatedChange =
            userChangedBrightness || userChangedAutoBrightnessAdjustment;
    if (userInitiatedChange && enable && !dozing) {
        prepareBrightnessAdjustmentSample();
    }
    changed |= setLightSensorEnabled(enable && !dozing);
    if (changed) {
        updateAutoBrightness(false /*sendUpdate*/, userInitiatedChange);
    }
}

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

## SystemUI Brightness

* vendor/mediatek/proprietary/packages/apps/SystemUI/src/com/android/systemui/settings/BrightnessDialog.java
  * Settings里亮度调节调用的是这个界面出来
* vendor/mediatek/proprietary/packages/apps/SystemUI/src/com/android/systemui/qs/QSPanel.java
  * 快速设置界面

### Layout

```xml
<!-- frameworks/base/packages/SystemUI/res/layout/quick_settings_brightness_dialog.xml -->

<LinearLayout xmlns:android="http://schemas.android.com/apk/res/android"
    xmlns:systemui="http://schemas.android.com/apk/res-auto"
    android:layout_height="wrap_content"
    android:layout_width="match_parent"
    android:layout_gravity="center_vertical"
    android:paddingLeft="16dp"
    android:paddingRight="16dp"
    style="@style/BrightnessDialogContainer">

    <com.android.systemui.settings.ToggleSliderView
        android:id="@+id/brightness_slider"
        android:layout_width="0dp"
        android:layout_height="48dp"
        android:layout_gravity="center_vertical"
        android:layout_weight="1"
        android:contentDescription="@string/accessibility_brightness"
        android:importantForAccessibility="no"
        systemui:text="@string/status_bar_settings_auto_brightness_label" />

</LinearLayout>
```

### QS Panel

BrightnessController控制着brightness slider事件出发

```Java
// vendor/mediatek/proprietary/packages/apps/SystemUI/src/com/android/systemui/qs/QSPanel.java

protected void addViewsAboveTiles() {
    mBrightnessView = LayoutInflater.from(mContext).inflate(
        R.layout.quick_settings_brightness_dialog, this, false);
    addView(mBrightnessView);
    mBrightnessController = new BrightnessController(getContext(),
            findViewById(R.id.brightness_slider), mBroadcastDispatcher);
}

```

BrightnessController从onChanged事件获取当前亮度，并交给DisplayManagerService进行处理

```Java
// vendor/mediatek/proprietary/packages/apps/SystemUI/src/com/android/systemui/settings/BrightnessController.java

@Override
public void onChanged(ToggleSlider toggleSlider, boolean tracking, boolean automatic,
        int value, boolean stopTracking) {
    if (mExternalChange) return;

    if (mSliderAnimator != null) {
        mSliderAnimator.cancel();
    }

    final float minBacklight;
    final float maxBacklight;
    final int metric;
    final String settingToChange;

    if (mIsVrModeEnabled) {
        metric = MetricsEvent.ACTION_BRIGHTNESS_FOR_VR;
        minBacklight = mMinimumBacklightForVr;
        maxBacklight = mMaximumBacklightForVr;
        settingToChange = Settings.System.SCREEN_BRIGHTNESS_FOR_VR_FLOAT;
    } else {
        metric = mAutomatic
                ? MetricsEvent.ACTION_BRIGHTNESS_AUTO
                : MetricsEvent.ACTION_BRIGHTNESS;
        minBacklight = mMinimumBacklight;
        maxBacklight = mMaximumBacklight;
        settingToChange = Settings.System.SCREEN_BRIGHTNESS_FLOAT;
    }
    final float valFloat = MathUtils.min(convertGammaToLinearFloat(value,
            minBacklight, maxBacklight),
            1.0f);
    if (stopTracking) {
        // TODO(brightnessfloat): change to use float value instead.
        MetricsLogger.action(mContext, metric,
                BrightnessSynchronizer.brightnessFloatToInt(mContext, valFloat));

    }
    setBrightness(valFloat);
    if (!tracking) {
        AsyncTask.execute(new Runnable() {
                public void run() {
                    Settings.System.putFloatForUser(mContext.getContentResolver(),
                            settingToChange, valFloat, UserHandle.USER_CURRENT);
                }
            });
    }

    for (BrightnessStateChangeCallback cb : mChangeCallbacks) {
        cb.onBrightnessLevelChanged();
    }
}

private void setBrightness(float brightness) {
    mDisplayManager.setTemporaryBrightness(brightness);
}
```

mDisplayManager是Binder Proxy，通过Binder通信调用到DisplayManagerService

```Java
// frameworks/base/services/core/java/com/android/server/display/DisplayManagerService.java

@Override // Binder call
public void setTemporaryBrightness(float brightness) {
    mContext.enforceCallingOrSelfPermission(
            Manifest.permission.CONTROL_DISPLAY_BRIGHTNESS,
            "Permission required to set the display's brightness");
    final long token = Binder.clearCallingIdentity();
    try {
        synchronized (mSyncRoot) {
            mDisplayPowerController.setTemporaryBrightness(brightness);
        }
    } finally {
        Binder.restoreCallingIdentity(token);
    }
}
```

最终由DisplayControllerHandler来处理消息

```Java
// frameworks/base/services/core/java/com/android/server/display/DisplayPowerController.java

public void setTemporaryBrightness(float brightness) {
    Message msg = mHandler.obtainMessage(MSG_SET_TEMPORARY_BRIGHTNESS,
            Float.floatToIntBits(brightness), 0 /*unused*/);
    msg.sendToTarget();
}

private final class DisplayControllerHandler extends Handler {
    // ...省略

    @Override
    public void handleMessage(Message msg) {
        Slog.v(TAG, "msg.what: " + msg.what);
        switch (msg.what) {
            // ...省略

            case MSG_CONFIGURE_BRIGHTNESS:
                mBrightnessConfiguration = (BrightnessConfiguration)msg.obj;
                updatePowerState();
                break;

            case MSG_SET_TEMPORARY_BRIGHTNESS:
                // TODO: Should we have a a timeout for the temporary brightness?
                mTemporaryScreenBrightness = Float.intBitsToFloat(msg.arg1);
                updatePowerState();
                break;

            case MSG_SET_TEMPORARY_AUTO_BRIGHTNESS_ADJUSTMENT:
                mTemporaryAutoBrightnessAdjustment = Float.intBitsToFloat(msg.arg1);
                updatePowerState();
                break;
        }
    }
}
```

## Auto Brightness Adjustment

* 当设置了自动调光，那么当我们又去设置背光亮度的时候，会重置自动调光起点，第一次打开调光是80%的样子
* 在DisplayPowerController中，每一次调用updatePowerState()更新状态时，都会对AutomaticBrightnessController进行配置
* 在下面逻辑中，hadUserBrightnessPoint表示是否在自动背光打开的情况下拖动亮度条调节过亮度，判断依据是BrightnessMappingStrategy中的mUserLux成员，它表示用户在开启自动背光后手动设置亮度时的Lux值：
* 然后开始调用configure()方法进行配置
  * userChangedBrightness:表示用户是否手动通过拖动亮度条设置过亮度
  * userChangedAutoBrightnessAdjustment:表示自动背光调整值adjustment是否发生变化

```Java
// frameworks/base/services/core/java/com/android/server/display/DisplayPowerController.java

private void updatePowerState() {
    // ...省略

    // If the brightness is already set then it's been overridden by something other than the
    // user, or is a temporary adjustment.
    boolean userInitiatedChange = (Float.isNaN(brightnessState))
            && (autoBrightnessAdjustmentChanged || userSetBrightnessChanged);
    boolean hadUserBrightnessPoint = false;
    // Configure auto-brightness.
    if (mAutomaticBrightnessController != null) {
        hadUserBrightnessPoint = mAutomaticBrightnessController.hasUserDataPoints();
        mAutomaticBrightnessController.configure(autoBrightnessEnabled,
                mBrightnessConfiguration,
                mLastUserSetScreenBrightness,
                userSetBrightnessChanged, autoBrightnessAdjustment,
                autoBrightnessAdjustmentChanged, mPowerRequest.policy);
    }

    // ...省略
}
```

将当前亮度值写入调光策略

```Java
// frameworks/base/services/core/java/com/android/server/display/AutomaticBrightnessController.java

// The mapper to translate ambient lux to screen brightness in the range [0, 1.0].
private final BrightnessMappingStrategy mBrightnessMapper;

private boolean setAutoBrightnessAdjustment(float adjustment) {
    return mBrightnessMapper.setAutoBrightnessAdjustment(adjustment);
}

```

使用的是SimpleMappingStrategy，如下是其dump方法

```Java
// frameworks/base/services/core/java/com/android/server/display/BrightnessMappingStrategy.java

private static class SimpleMappingStrategy extends BrightnessMappingStrategy {
    // ...省略

    @Override
    public void dump(PrintWriter pw) {
        pw.println("SimpleMappingStrategy");
        pw.println("  mSpline=" + mSpline);
        pw.println("  mMaxGamma=" + mMaxGamma);
        pw.println("  mAutoBrightnessAdjustment=" + mAutoBrightnessAdjustment);
        pw.println("  mUserLux=" + mUserLux);
        pw.println("  mUserBrightness=" + mUserBrightness);
    }

    // ...省略
}
```

使用`adb shell dumpsys display`命令，查看当前调光策略数据

```
SimpleMappingStrategy
  mSpline=MonotoneCubicSpline{[(0.0, 3.902439E-5: 1.5609757E-4), (128.0, 0.020019513: 1.5609755E-4), (256.0, 0.04: 0.0), (384.0, 0.04: 0.0), (459.99997, 0.04: 0.0), (512.0, 0.044519294: 8.690936E-5), (640.0, 0.055643678: 8.6909204E-5), (768.0, 0.06676805: 8.690925E-5), (896.0, 0.077892445: 8.690939E-5), (1024.0, 0.089016855: 8.690938E-5), (2048.0, 0.17801198: 8.6909306E-5), (4096.0, 0.35600224: 8.690934E-5), (6144.0, 0.53399265: 8.690921E-5), (8192.0, 0.71198237: 8.690923E-5), (10240.0, 0.88997287: 7.0316804E-5), (12288.0, 1.0: 0.0), (14336.0, 1.0: 0.0), (16384.0, 1.0: 0.0), (18432.0, 1.0: 0.0)]}
  mMaxGamma=3.0
  mAutoBrightnessAdjustment=-1.0
  mUserLux=459.99997
  mUserBrightness=0.04
```

SimpleMappingStrategy默认值是：

```
SimpleMappingStrategy
  mSpline=MonotoneCubicSpline{[(0.0, 0.027559055: 0.0017224409), (128.0, 0.2480315: 0.0013841044), (256.0, 0.38188976: 5.302923E-4), (384.0, 0.4055118: 1.5908774E-4), (512.0, 0.42913386: 1.8454727E-4), (640.0, 0.4527559: 1.8454727E-4), (768.0, 0.47637796: 1.8454727E-4), (896.0, 0.5: 1.8454716E-4), (1024.0, 0.52362204: 1.8454721E-4), (2048.0, 0.71259844: 1.6244003E-4), (4096.0, 1.0: 0.0), (6144.0, 1.0: 0.0), (8192.0, 1.0: 0.0), (10240.0, 1.0: 0.0), (12288.0, 1.0: 0.0), (14336.0, 1.0: 0.0), (16384.0, 1.0: 0.0), (18432.0, 1.0: 0.0)]}
  mMaxGamma=3.0
  mAutoBrightnessAdjustment=0.0
  mUserLux=-1.0
  mUserBrightness=-1.0
```

* 目前的问题就是：当我们关闭Auto Brightness的时候，SimpleMappingStrategy中的数据并不会恢复到默认值，这样导致下一次打开Auto Brightness的时候会保持上一次的设定参数；
* 如下所示，SimpleMappingStrategy中提供了clearUserDataPoints()来清理数据；

```Java
frameworks/base/services/core/java/com/android/server/display/BrightnessMappingStrategy.java

private static class SimpleMappingStrategy extends BrightnessMappingStrategy {
    // ...省略

    @Override
    public void clearUserDataPoints() {
        if (mUserLux != -1) {
            if (mLoggingEnabled) {
                Slog.d(TAG, "clearUserDataPoints: " + mAutoBrightnessAdjustment + " => 0");
                PLOG.start("clear user data points")
                        .logPoint("user data point", mUserLux, mUserBrightness);
            }
            mAutoBrightnessAdjustment = 0;
            mUserLux = -1;
            mUserBrightness = -1;
            computeSpline();
        }
    }

    // ...省略
}
```

在关Auto Brightness的地方，清理一下数据，diff如下

```diff
diff --git a/frameworks/base/services/core/java/com/android/server/display/DisplayPowerController.java b/frameworks/base/services/core/java/com/android/server/display/DisplayPowerController.java
index 46d738f580c..f7dc3a46e4d 100644
--- a/frameworks/base/services/core/java/com/android/server/display/DisplayPowerController.java
+++ b/frameworks/base/services/core/java/com/android/server/display/DisplayPowerController.java
@@ -1115,6 +1115,12 @@ final class DisplayPowerController implements AutomaticBrightnessController.Call
             Slog.v(TAG, "Brightness [" + brightnessState + "] reason changing to: '"
                     + mBrightnessReasonTemp.toString(brightnessAdjustmentFlags)
                     + "', previous reason: '" + mBrightnessReason + "'.");
+
+            if (mBrightnessReason.reason == BrightnessReason.REASON_AUTOMATIC
+                    && mBrightnessReasonTemp.reason == BrightnessReason.REASON_MANUAL) {
+                mAutomaticBrightnessController.resetShortTermModel();
+            }
+
             mBrightnessReason.set(mBrightnessReasonTemp);
         }

```

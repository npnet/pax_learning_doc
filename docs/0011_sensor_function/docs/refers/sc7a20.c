/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <seos.h>
#include <util.h>
#include <sensors.h>
#include <plat/inc/rtc.h>

#include <accGyro.h>
#include <contexthub_core.h>
#include <cust_accGyro.h>
#include <mt_gpt.h>

// Jake.L, DATE20190401, Add calibration interface, DATE20190401-01 START
#ifndef CFG_MTK_CALIBRATION_V1_0
#include <API_sensor_calibration.h>
#else
#include "calibration/factory_calibration/factory_cal.h"
#endif
// Jake.L, DATE20190401-01 END

#define I2C_SPEED                       400000

// Jake.L, DATE20190318, add contexthub interface, DATE20190318-01 LINE
#define ACC_NAME     "sc7a20"
#define LOG_TAG     "sc7a20 "

// Jake.L, DATE20190401, NOTE, DATE20190401-01 LINE
#define ACC_CALI


#define SC7A20_cali_self                        //djq, 2019-05-28

//  add by djq 2017-08-02 start
#ifdef SC7A20_cali_self
#define Z_OFF_DEFAULT  (0)          //djq 2019-08-27
#define XY_THR_N      (-200)
#define XY_THR_P      (200)
#define Z_THR_MIN_N   (-774)
#define Z_THR_MIN_P   (774)
#define Z_THR_MAX_N   (-1270)
#define Z_THR_MAX_P   (1270)
#define SUM_DOT       (50)
#define THRESHOLD_VAL (40)
#endif
//  add by djq 2017-08-02 end

/* SC7A20 Register Map  (Please refer to SC7A20 Specifications) */
#include "sc7a20_registers.h"

enum SC7A20State {
    STATE_SAMPLE = CHIP_SAMPLING,
    STATE_CONVERT = CHIP_CONVERT,
    STATE_SAMPLE_DONE = CHIP_SAMPLING_DONE,
    STATE_ACC_ENABLE = CHIP_ACC_ENABLE,
    STATE_ACC_ENABLE_DONE = CHIP_ACC_ENABLE_DONE,
    STATE_ACC_DISABLE = CHIP_ACC_DISABLE,
    STATE_ACC_DISABLE_DONE = CHIP_ACC_DISABLE_DONE,
    STATE_ACC_RATECHG = CHIP_ACC_RATECHG,
    STATE_ACC_RATECHG_DONE = CHIP_ACC_RATECHG_DONE,
    // Jake.L, DATE20190401, Add calibration interface, DATE20190401-01 START
    #if defined (ACC_CALI)
    STATE_ACC_CALI = CHIP_ACC_CALI,
    STATE_ACC_CALI_DONE = CHIP_ACC_CALI_DONE,
    STATE_ACC_CFG = CHIP_ACC_CFG,
    STATE_ACC_CFG_DONE = CHIP_ACC_CFG_DONE,
    #endif  /* ACC_CALI */
    // Jake.L, DATE20190401-01 END
    STATE_INIT_DONE = CHIP_INIT_DONE,
    STATE_IDLE = CHIP_IDLE,
    STATE_RESET_R = CHIP_RESET,
    STATE_RESET_W,
    STATE_DEVID,
    STATE_ENPOWER_W,
    STATE_ACC_TURNOFF_W,
    STATE_CALC_RESO,
    STATE_CORE,
    STATE_ACC_RATE,
};

struct scale_factor {
    unsigned char  whole;
    unsigned char  fraction;
};

struct acc_data_resolution {
    struct scale_factor scalefactor;
    int                 sensitivity;
};

static struct acc_data_resolution sc7a20g_data_resolution[] = {
    {{ 0, 9}, 1024},
};


#define SILAN_SC7A20_FILTER                 1
#ifdef SILAN_SC7A20_FILTER

typedef struct FilterChannelTag{
    int16_t sample_l;
    int16_t sample_h;

    int16_t flag_l;
    int16_t flag_h;

} FilterChannel;

typedef struct Silan_core_channel_s{
    int16_t filter_param_l;
    int16_t filter_param_h;
    int16_t filter_threhold;
    FilterChannel  sl_channel[3];
} Silan_core_channel;

#endif	/*__SILAN_SC7A20_FILTER__*/

#ifdef SILAN_SC7A20_FILTER
	Silan_core_channel	core_channel;
#endif

#define MAX_I2C_PER_PACKET  8
#define SC7A20_DATA_LEN   6
#define MAX_RXBUF 512
#define MAX_TXBUF (MAX_RXBUF / MAX_I2C_PER_PACKET)
static struct SC7A20Task {
    bool accPowerOn;
    /* txBuf for i2c operation, fill register and fill value */
    uint8_t txBuf[MAX_TXBUF];
    /* rxBuf for i2c operation, receive rawdata */
    uint8_t rxBuf[MAX_RXBUF];
    uint8_t deviceId;
    uint8_t waterMark;
    uint32_t accRate;
    uint16_t accRateDiv;

    uint64_t sampleTime;
    struct transferDataInfo dataInfo;
    struct accGyroDataPacket accGyroPacket;
    /* data for factory */
    struct TripleAxisDataPoint accFactoryData;

    // Jake.L, DATE20190401, NOTE, DATE20190401-01 LINE
    #if defined (ACC_CALI)
    bool startCali;
    int32_t accuracy;
    float staticCali[AXES_NUM];
    #endif  /* ACC_CALI */
    int32_t accSwCali[AXES_NUM];
    struct acc_data_resolution *accReso;
    struct accGyro_hw *hw;
    struct sensorDriverConvert cvt;
    uint8_t i2c_addr;
    bool autoDetectDone;

    // Jake.L, DATE20190318, add contexthub interface, DATE20190318-01 LINE
    int32_t debug_trace;
} mTask;
static struct SC7A20Task *sc7a20DebugPoint;

#ifdef SC7A20_cali_self
void find_max_min(int16_t *array, int len, int16_t *max, int16_t *min)
{
	int16_t temp_max = *array;
	int16_t temp_min = *array;

	int i = 0;
	for(i = 0; i < len; i++)
	{
		if(*(array + i) > temp_max)
			temp_max = *(array + i);

		if(*(array + i) < temp_min)
			temp_min = *(array + i);
	}

	*max = temp_max;
	*min = temp_min;
}
#endif


#ifdef SILAN_SC7A20_FILTER
static int16_t filter_average(int16_t preAve, int16_t sample, int16_t Filter_num, int16_t* flag)
{
	if (*flag == 0)
	{
		preAve = sample;
		*flag = 1;
	}

	return preAve + (sample - preAve) / Filter_num;
}

static int16_t silan_filter_process(FilterChannel* fac, int16_t sample)
{
	if(fac == NULL)
	{
		return 0;
	}

	fac->sample_l = filter_average(fac->sample_l, sample, core_channel.filter_param_l, &fac->flag_l);
	fac->sample_h = filter_average(fac->sample_h, sample, core_channel.filter_param_h, &fac->flag_h);
	if (abs(fac->sample_l - fac->sample_h) > core_channel.filter_threhold)
	{
		fac->sample_h = fac->sample_l;
	}

	return fac->sample_h;
}
#endif /* ! SILAN_SC7A20_FILTER */

//djq, 2019-05-28
static int sc7a20ResetRead(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
                         void *inBuf, uint8_t inSize, uint8_t elemInSize,
                         void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
{
    osLog(LOG_INFO, "sc7a20ResetRead\n");
    mTask.txBuf[0] = SC7A20_CTRL_REG4;
    return i2cMasterTxRx(mTask.hw->i2c_num, mTask.i2c_addr, mTask.txBuf, 1,
                         mTask.rxBuf, 1, i2cCallBack,
                         next_state);
}

static int sc7a20ResetWrite(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
                         void *inBuf, uint8_t inSize, uint8_t elemInSize,
                         void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
{
    osLog(LOG_INFO, "read before sc7a20ResetWrite\n");

//    mTask.txBuf[0] = SC7A20_CTRL_REG2;
//    mTask.txBuf[1] = mTask.rxBuf[0] | SC7A20_CTRL_REG2_SRST;
//    return i2cMasterTx(mTask.hw->i2c_num, mTask.i2c_addr, mTask.txBuf, 2, i2cCallBack,
//                       next_state);

// start:SDO pull-up resistor close off
    mTask.txBuf[0] = 0x1E;
    mTask.txBuf[1] = 0x05;
    i2cMasterTxRx(mTask.hw->i2c_num, mTask.i2c_addr, mTask.txBuf, 2, NULL, 0, i2cCallBack, (void *)CHIP_IDLE);

    mTask.txBuf[0] = 0x57;
    i2cMasterTxRx(mTask.hw->i2c_num, mTask.i2c_addr, mTask.txBuf, 1, mTask.rxBuf, 1, i2cCallBack, (void *)CHIP_IDLE);

    mTask.rxBuf[0] = mTask.rxBuf[0] | 0x08;

	mTask.txBuf[0] = 0x57;
    mTask.txBuf[1] = mTask.rxBuf[0];
    i2cMasterTxRx(mTask.hw->i2c_num, mTask.i2c_addr, mTask.txBuf, 2, NULL, 0, i2cCallBack, (void *)CHIP_IDLE);

    mTask.txBuf[0] = 0x1E;
    mTask.txBuf[1] = 0x00;
    i2cMasterTxRx(mTask.hw->i2c_num, mTask.i2c_addr, mTask.txBuf, 2, NULL, 0, i2cCallBack, (void *)CHIP_IDLE);
// end:SDO pull-up resistor close off

    mTask.txBuf[0] = SC7A20_CTRL_REG4;
    mTask.txBuf[1] = 0x88;    //BDU set 1; BLE set 0; HR set 1; scale = 2g
    return i2cMasterTx(mTask.hw->i2c_num, mTask.i2c_addr, mTask.txBuf, 2, i2cCallBack,
                       next_state);
}

static int sc7a20DeviceId(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
                         void *inBuf, uint8_t inSize, uint8_t elemInSize,
                         void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
{
    /* Wait sensor reset to be ready  */
    mdelay(10);
    osLog(LOG_INFO, "sc7a20DeviceId\n");

    mTask.txBuf[0] = SC7A20_WHO_AM_I ;
    return i2cMasterTxRx(mTask.hw->i2c_num, mTask.i2c_addr, mTask.txBuf, 1,
                         &mTask.deviceId, 1, i2cCallBack,
                         next_state);
}
static int sc7a20PowerRead(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
                         void *inBuf, uint8_t inSize, uint8_t elemInSize,
                         void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
{
    osLog(LOG_INFO, "sc7a20PowerRead\n");
    mTask.txBuf[0] = SC7A20_CTRL_REG1;
    return i2cMasterTxRx(mTask.hw->i2c_num, mTask.i2c_addr, mTask.txBuf, 1,
                         mTask.rxBuf, 1, i2cCallBack,
                         next_state);
}
static int sc7a20PowerEnableWrite(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
                         void *inBuf, uint8_t inSize, uint8_t elemInSize,
                         void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
{
    osLog(LOG_INFO, "sc7a20PowerEnableWrite: 0x%x\n", mTask.rxBuf[0]);
    mTask.txBuf[0] = SC7A20_CTRL_REG1;
    mTask.txBuf[1] = mTask.rxBuf[0] | (0x47);      //djq, 2019-05-58, 50Hz default
    return i2cMasterTx(mTask.hw->i2c_num, mTask.i2c_addr, mTask.txBuf, 2, i2cCallBack,
                       next_state);
}
static int sc7a20PowerDisableWrite(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
                         void *inBuf, uint8_t inSize, uint8_t elemInSize,
                         void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
{
    osLog(LOG_INFO, "sc7a20PowerDisableWrite: 0x%x\n", mTask.rxBuf[0]);
    if (mTask.accPowerOn) {
        osLog(LOG_INFO, "sc7a20PowerDisableWrite should not disable, acc:%d\n",
            mTask.accPowerOn);
        sensorFsmEnqueueFakeI2cEvt(i2cCallBack, next_state, SUCCESS_EVT);
    } else {
        mTask.txBuf[0] = SC7A20_CTRL_REG1;
        mTask.txBuf[1] = mTask.rxBuf[0] & ~0xf0;  //djq, 2019-05-58, power down; 0Hz
        return i2cMasterTx(mTask.hw->i2c_num, mTask.i2c_addr, mTask.txBuf, 2, i2cCallBack,
                           next_state);
    }
    return 0;
}

static int sc7a20gEnable(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
                         void *inBuf, uint8_t inSize, uint8_t elemInSize,
                         void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
{
    osLog(LOG_INFO, "sc7a20gEnable\n");

    mTask.accPowerOn = true;
    return sc7a20PowerRead(i2cCallBack, spiCallBack, next_state,
        inBuf, inSize, elemInSize, outBuf, outSize, elemOutSize);
}
static int sc7a20gDisable(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
                         void *inBuf, uint8_t inSize, uint8_t elemInSize,
                         void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
{
    int ret = 0;
    struct accGyroCntlPacket cntlPacket;

    ret = rxControlInfo(&cntlPacket, inBuf, inSize, elemInSize, outBuf, outSize, elemOutSize);
    if (ret < 0) {
        sensorFsmEnqueueFakeI2cEvt(i2cCallBack, next_state, ERROR_EVT);
        osLog(LOG_INFO, "sc7a20gDisable, rx water_mark err\n");
        return -1;
    }

    mTask.accPowerOn = false;
    mTask.accRate = 0;
    return sc7a20PowerRead(i2cCallBack, spiCallBack, next_state,
        inBuf, inSize, elemInSize, outBuf, outSize, elemOutSize);
}
static int sc7a20gRate(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
                         void *inBuf, uint8_t inSize, uint8_t elemInSize,
                         void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
{
    int ret = 0;
    uint32_t sample_rate, water_mark;
    struct accGyroCntlPacket cntlPacket;

    ret = rxControlInfo(&cntlPacket, inBuf, inSize, elemInSize, outBuf, outSize, elemOutSize);
    if (ret < 0) {
        sensorFsmEnqueueFakeI2cEvt(i2cCallBack, next_state, ERROR_EVT);
        osLog(LOG_ERROR, "sc7a20gRate, rx inSize and elemSize error\n");
        return -1;
    }
    sample_rate = cntlPacket.rate;
    water_mark = cntlPacket.waterMark;

    if (sample_rate > 0 && sample_rate < 6000) {
        mTask.accRate = 5000;
        mTask.accRateDiv = 224;
    } else if (sample_rate > 9000 && sample_rate < 11000) {
        mTask.accRate = 10000;
        mTask.accRateDiv = 111;
    } else if (sample_rate > 16000 && sample_rate < 17000) {
        mTask.accRate = 15000;
        mTask.accRateDiv = 74;
    } else if (sample_rate > 40000 && sample_rate < 60000) {
        mTask.accRate = 51100;
        mTask.accRateDiv = 21;
    } else if (sample_rate > 90000 && sample_rate < 110000) {
        mTask.accRate = 102300;
        mTask.accRateDiv = 10;
    } else if (sample_rate > 190000 && sample_rate < 210000) {
        mTask.accRate = 225000;
        mTask.accRateDiv = 4;
    } else {
        mTask.accRate = sample_rate;
        mTask.accRateDiv = 1125000 / sample_rate - 1;
    }

    //TODO: write rate to sensor

    mTask.waterMark = water_mark;
    osLog(LOG_INFO, "sc7a20gRate: %d, minAccGyroDelay:%d, water_mark:%d\n", mTask.accRate, water_mark);
    sensorFsmEnqueueFakeI2cEvt(i2cCallBack, next_state, SUCCESS_EVT);
    return ret;
}
static int sc7a20CalcReso(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
                         void *inBuf, uint8_t inSize, uint8_t elemInSize,
                         void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
{
    uint8_t reso = 0;
    //osLog(LOG_INFO, "sc7a20CalcReso:acc(0x%x), gyro(0x%x)\n", mTask.rxBuf[0], mTask.rxBuf[1]);
    reso = 0x00;
    // value from sensor
    if (reso < sizeof(sc7a20g_data_resolution) / sizeof(sc7a20g_data_resolution[0])) {
        mTask.accReso = &sc7a20g_data_resolution[reso];
        osLog(LOG_INFO, LOG_TAG"acc reso: %d, sensitivity: %d\n", reso, mTask.accReso->sensitivity);
    }
    sensorFsmEnqueueFakeI2cEvt(i2cCallBack, next_state, SUCCESS_EVT);
    return 0;
}
static void accGetCalibration(int32_t *cali, int32_t size)
{
    cali[AXIS_X] = mTask.accSwCali[AXIS_X];
    cali[AXIS_Y] = mTask.accSwCali[AXIS_Y];
    cali[AXIS_Z] = mTask.accSwCali[AXIS_Z];
    osLog(LOG_INFO, LOG_TAG"accGetCalibration cali x:%d, y:%d, z:%d\n", cali[AXIS_X], cali[AXIS_Y], cali[AXIS_Z]);
}
static void accSetCalibration(int32_t *cali, int32_t size)
{
    mTask.accSwCali[AXIS_X] = cali[AXIS_X];
    mTask.accSwCali[AXIS_Y] = cali[AXIS_Y];
    mTask.accSwCali[AXIS_Z] = cali[AXIS_Z];
    osLog(LOG_INFO, LOG_TAG"accSetCalibration cali x:%d, y:%d, z:%d\n", mTask.accSwCali[AXIS_X],
        mTask.accSwCali[AXIS_Y], mTask.accSwCali[AXIS_Z]);
}
static void accGetData(void *sample)
{
    struct TripleAxisDataPoint *tripleSample = (struct TripleAxisDataPoint *)sample;
    tripleSample->ix = mTask.accFactoryData.ix;
    tripleSample->iy = mTask.accFactoryData.iy;
    tripleSample->iz = mTask.accFactoryData.iz;
}

// Jake.L, DATE20190401, Add calibration interface, DATE20190401-01 START
#if defined (ACC_CALI)
static int sc7a20AccCali(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
                            void *inBuf, uint8_t inSize, uint8_t elemInSize,
                            void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
{
#ifndef CFG_MTK_CALIBRATION_V1_0
    float bias[AXES_NUM] = {0};
#endif

    mTask.startCali = true;
    osLog(LOG_INFO, "sc7a20AccCali\n");
#ifndef CFG_MTK_CALIBRATION_V1_0
    Acc_init_calibration(bias);
#else
    accFactoryCalibrateInit(&mTask.accFactoryCal);
#endif
	sensorFsmEnqueueFakeI2cEvt(i2cCallBack, next_state, SUCCESS_EVT);
    return 0;
}

static int sc7a20AccCfgCali(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
                            void *inBuf, uint8_t inSize, uint8_t elemInSize,
                            void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
{
    int ret = 0;
    struct accGyroCaliCfgPacket caliCfgPacket;

    ret = rxCaliCfgInfo(&caliCfgPacket, inBuf, inSize, elemInSize, outBuf, outSize, elemOutSize);

    if (ret < 0) {
       sensorFsmEnqueueFakeSpiEvt(spiCallBack, next_state, ERROR_EVT);
       osLog(LOG_ERROR, "sc7a20AccCfgCali, rx inSize and elemSize error\n");
       return -1;
    }
    osLog(LOG_INFO, "sc7a20AccCfgCali: cfgData[0]:%d, cfgData[1]:%d, cfgData[2]:%d\n",
       caliCfgPacket.caliCfgData[0], caliCfgPacket.caliCfgData[1], caliCfgPacket.caliCfgData[2]);

    mTask.staticCali[AXIS_X] = (float)caliCfgPacket.caliCfgData[0] / 1000;
    mTask.staticCali[AXIS_Y] = (float)caliCfgPacket.caliCfgData[1] / 1000;
    mTask.staticCali[AXIS_Z] = (float)caliCfgPacket.caliCfgData[2] / 1000;

    sensorFsmEnqueueFakeI2cEvt(i2cCallBack, next_state, SUCCESS_EVT);
    return 0;
}
#endif  /* ACC_CALI */
// Jake.L, DATE20190401-01 END


// Jake.L, DATE20190318, add contexthub interface, DATE20190318-01 START
static void sc7a20SetDebugTrace(int32_t trace) {
    mTask.debug_trace = trace;
    osLog(LOG_ERROR, "%s ==> trace:%d\n", __func__, mTask.debug_trace);
}

static void accGetSensorInfo(struct sensorInfo_t *data)
{
    strncpy(data->name, ACC_NAME, sizeof(data->name));
}
// Jake.L, DATE20190318-01 END

static int sc7a20RegisterCore(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
                         void *inBuf, uint8_t inSize, uint8_t elemInSize,
                         void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
{
    struct sensorCoreInfo mInfo;

    osLog(LOG_INFO, "sc7a20RegisterCore deviceId 0x%x\n\r", mTask.deviceId);

    // Jake.L, DATE20190318, set null for internal function point, DATE20190318-01 LINE
    memset(&mInfo, 0x00, sizeof(struct sensorCoreInfo));

    if (mTask.deviceId != SC7A20_WHO_AM_I_WIA_ID) {
        osLog(LOG_INFO, "sc7a20RegisterCore deviceId 0x%x\n\r", mTask.deviceId);
        return 0;
    }

    /* Register sensor Core */
    mInfo.sensType = SENS_TYPE_ACCEL;
    mInfo.gain = GRAVITY_EARTH_1000;
    mInfo.sensitivity = mTask.accReso->sensitivity;
    mInfo.cvt = mTask.cvt;
    mInfo.getCalibration = accGetCalibration;
    mInfo.setCalibration = accSetCalibration;
    mInfo.getData = accGetData;
    // Jake.L, DATE20190318, add contexthub interface, DATE20190318-01 START
    mInfo.setDebugTrace = sc7a20SetDebugTrace;
    mInfo.getSensorInfo = accGetSensorInfo;
    // Jake.L, DATE20190318-01 END
    sensorCoreRegister(&mInfo);

    sensorFsmEnqueueFakeI2cEvt(i2cCallBack, next_state, SUCCESS_EVT);
    return 0;
}
static int sc7a20Sample(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
                         void *inBuf, uint8_t inSize, uint8_t elemInSize,
                         void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
{
    int ret = 0;
    //osLog(LOG_INFO, "sc7a20Sample\n\r");

    ret = rxTransferDataInfo(&mTask.dataInfo, inBuf, inSize, elemInSize, outBuf, outSize, elemOutSize);
    if (ret < 0) {
        sensorFsmEnqueueFakeI2cEvt(i2cCallBack, next_state, ERROR_EVT);
        osLog(LOG_ERROR, "sc7a20Sample, rx dataInfo error\n");
        return -1;
    }

    mTask.sampleTime = rtcGetTime();

    mTask.txBuf[0] = SC7A20_XOUT_L | 0x80;    //djq, 2019-05-28
    ret = i2cMasterTxRx(mTask.hw->i2c_num, mTask.i2c_addr, &mTask.txBuf[0], 1,
                         &mTask.rxBuf[0], 6, i2cCallBack,
                         next_state);
    if (ret != 0)
        osLog(LOG_INFO, "SC7A20_XOUT_L err\n");

    return 0;
}
#if defined (ACC_CALI)
static int sc7a20gConvert(uint8_t *databuf, struct accGyroData *data)
{
    int32_t raw_data[AXES_NUM];
    int32_t remap_data[AXES_NUM];
    // TODO: update shift based on 14/12bit data, 2/4
    uint8_t right_shift = 4;

    int32_t caliResult[AXES_NUM] = {0};
    float calibrated_data_output[AXES_NUM] = {0};
    float temp_data[AXES_NUM] = {0};
    int16_t status = 0;

	//  add by djq 2017-08-02 start
#ifdef SC7A20_cali_self
	static int16_t axis_x_off = 0;  			//jq 2017-08-07
	static int16_t axis_y_off = 0;  			//jq 2017-08-07
	static int16_t axis_z_off = Z_OFF_DEFAULT;  //jq 2017-08-07
	static uint16_t index = 0;
	static int16_t x_value[SUM_DOT] ={0};
	static int16_t y_value[SUM_DOT] ={0};
	static int16_t z_value[SUM_DOT] ={0};

	int flag_x = 0;
	int flag_y = 0;
	int flag_z = 0;

	int i = 0;

	int16_t x_min = 0;
	int16_t x_max = 0;
	int16_t y_min = 0;
	int16_t y_max = 0;
	int16_t z_min = 0;
	int16_t z_max = 0;

	int32_t temp_x_value = 0;
	int32_t temp_y_value = 0;
	int32_t temp_z_value = 0;
#endif

    //osLog(LOG_INFO, "sc7a20gConvert\n\r");
    raw_data[AXIS_X] = (int16_t)((databuf[AXIS_X * 2 + 1] << 8) | databuf[AXIS_X * 2]);
    raw_data[AXIS_Y] = (int16_t)((databuf[AXIS_Y * 2 + 1] << 8) | databuf[AXIS_Y * 2]);
    raw_data[AXIS_Z] = (int16_t)((databuf[AXIS_Z * 2 + 1] << 8) | databuf[AXIS_Z * 2]);

    /* shift data to correct format */
    raw_data[AXIS_X] = raw_data[AXIS_X] >> right_shift;
    raw_data[AXIS_Y] = raw_data[AXIS_Y] >> right_shift;
    raw_data[AXIS_Z] = raw_data[AXIS_Z] >> right_shift;
    osLog(LOG_INFO, "sc7a20 acc1 rawdata x:%d, y:%d, z:%d\n", raw_data[AXIS_X], raw_data[AXIS_Y], raw_data[AXIS_Z]);

#ifdef SC7A20_cali_self
    	if(index < SUM_DOT)
		{
	    	if( raw_data[AXIS_X] > XY_THR_N && raw_data[AXIS_X] < XY_THR_P)
	    		flag_x = 1;
			else
				flag_x = 0;

	    	if( raw_data[AXIS_Y] > XY_THR_N && raw_data[AXIS_Y] < XY_THR_P)
	    		flag_y = 1;
			else
				flag_y = 0;

        	if((raw_data[AXIS_Z] > Z_THR_MAX_N && raw_data[AXIS_Z] < Z_THR_MIN_N)  || (raw_data[AXIS_Z] > Z_THR_MIN_P && raw_data[AXIS_Z] < Z_THR_MAX_P))
	    		flag_z = 1;
			else
				flag_z = 0;

        	if(flag_x == 1 && flag_y == 1 && flag_z == 1)
        	{
				x_value[index] = raw_data[AXIS_X];
				y_value[index] = raw_data[AXIS_Y];
				z_value[index] = raw_data[AXIS_Z];
				index = index + 1;
        	}
        	else
            	index = 0;
		}

        if(index == SUM_DOT)
        {
			find_max_min(x_value, SUM_DOT, &x_max, &x_min);
			find_max_min(y_value, SUM_DOT, &y_max, &y_min);
			find_max_min(z_value, SUM_DOT, &z_max, &z_min);

			if( ((x_max - x_min) < THRESHOLD_VAL)  && ((y_max - y_min) < THRESHOLD_VAL) && ((z_max - z_min) < THRESHOLD_VAL))
			{

				temp_x_value = 0;
				for(i = 0; i < SUM_DOT; i++)
				{
					temp_x_value += x_value[i];
				}
				temp_x_value = temp_x_value / SUM_DOT;
				axis_x_off = 0 - (int16_t)temp_x_value;

				temp_y_value = 0;
				for(i = 0; i < SUM_DOT; i++)
				{
					temp_y_value += y_value[i];
				}
				temp_y_value = temp_y_value / SUM_DOT;
				axis_y_off = 0 - (int16_t)temp_y_value;

				temp_z_value = 0;
				for(i = 0; i < SUM_DOT; i++)
				{
					temp_z_value += z_value[i];
				}
				temp_z_value = temp_z_value / SUM_DOT;

				if(temp_z_value > Z_THR_MAX_N && temp_z_value < Z_THR_MIN_N)
					axis_z_off = -1024 - (int16_t)temp_z_value;
				else
					axis_z_off = 1024 - (int16_t)temp_z_value;
			    
				index += 1;		

			}
			else
			{
			    index = 0; 
			}


		}
		raw_data[AXIS_X] +=  axis_x_off;
	    raw_data[AXIS_Y] +=  axis_y_off;
	    raw_data[AXIS_Z] +=  axis_z_off;
#endif

#ifdef SILAN_SC7A20_FILTER

	raw_data[AXIS_X] = silan_filter_process(&core_channel.sl_channel[0], raw_data[AXIS_X]);
	raw_data[AXIS_Y] = silan_filter_process(&core_channel.sl_channel[1], raw_data[AXIS_Y]);
	raw_data[AXIS_Z] = silan_filter_process(&core_channel.sl_channel[2], raw_data[AXIS_Z]);

#endif

    raw_data[AXIS_X] = raw_data[AXIS_X] + mTask.accSwCali[AXIS_X];
    raw_data[AXIS_Y] = raw_data[AXIS_Y] + mTask.accSwCali[AXIS_Y];
    raw_data[AXIS_Z] = raw_data[AXIS_Z] + mTask.accSwCali[AXIS_Z];

    /*remap coordinate*/
    remap_data[mTask.cvt.map[AXIS_X]] = mTask.cvt.sign[AXIS_X] * raw_data[AXIS_X];
    remap_data[mTask.cvt.map[AXIS_Y]] = mTask.cvt.sign[AXIS_Y] * raw_data[AXIS_Y];
    remap_data[mTask.cvt.map[AXIS_Z]] = mTask.cvt.sign[AXIS_Z] * raw_data[AXIS_Z];

    /* Out put the degree/second(mo/s) */
    data->sensType = SENS_TYPE_ACCEL;
    data->x = (float)remap_data[AXIS_X] * GRAVITY_EARTH_SCALAR / mTask.accReso->sensitivity;
    data->y = (float)remap_data[AXIS_Y] * GRAVITY_EARTH_SCALAR / mTask.accReso->sensitivity;
    data->z = (float)remap_data[AXIS_Z] * GRAVITY_EARTH_SCALAR / mTask.accReso->sensitivity;
    #if 0
    osLog(LOG_INFO, "acc rawdata x:%f, y:%f, z:%f cali[%f, %f, %f]\n",
        (double)data->x, (double)data->y, (double)data->z,
        (double)mTask.staticCali[AXIS_X], (double)mTask.staticCali[AXIS_Y], (double)mTask.staticCali[AXIS_Z]);
    #endif  /* #if 0 */

    temp_data[AXIS_X] = data->x;
    temp_data[AXIS_Y] = data->y;
    temp_data[AXIS_Z] = data->z;

    if (UNLIKELY(mTask.startCali)) {
        #ifndef CFG_MTK_CALIBRATION_V1_0
        status = Acc_run_factory_calibration_timeout(0,
            temp_data, calibrated_data_output, (int *)&mTask.accuracy, rtcGetTime());
        /*
        osLog(LOG_INFO, LOG_TAG"ACC cali %lld: input(%f, %f, %f), output[%f, %f, %f], %d, %d\n",
                rtcGetTime(),
                (double)temp_data[AXIS_X], (double)temp_data[AXIS_Y], (double)temp_data[AXIS_Z],
                (double)calibrated_data_output[AXIS_X], (double)calibrated_data_output[AXIS_Y], (double)calibrated_data_output[AXIS_Z],
                status, mTask.accuracy);
        */
        if (status != 0) {
            mTask.startCali = false;
            if (status > 0) {
                osLog(LOG_INFO, LOG_TAG"ACC cali detect shake %lld\n", rtcGetTime());
                caliResult[AXIS_X] = (int32_t)(mTask.staticCali[AXIS_X] * 1000);
                caliResult[AXIS_Y] = (int32_t)(mTask.staticCali[AXIS_Y] * 1000);
                caliResult[AXIS_Z] = (int32_t)(mTask.staticCali[AXIS_Z] * 1000);
                accGyroSendCalibrationResult(SENS_TYPE_ACCEL, (int32_t *)&caliResult[0], (uint8_t)status);
            } else
                osLog(LOG_INFO, LOG_TAG"ACC cali time out %lld\n", rtcGetTime());
        } else if (mTask.accuracy == 3) {
            mTask.startCali = false;
            mTask.staticCali[AXIS_X] = calibrated_data_output[AXIS_X] - temp_data[AXIS_X];
            mTask.staticCali[AXIS_Y] = calibrated_data_output[AXIS_Y] - temp_data[AXIS_Y];
            mTask.staticCali[AXIS_Z] = calibrated_data_output[AXIS_Z] - temp_data[AXIS_Z];
            caliResult[AXIS_X] = (int32_t)(mTask.staticCali[AXIS_X] * 1000);
            caliResult[AXIS_Y] = (int32_t)(mTask.staticCali[AXIS_Y] * 1000);
            caliResult[AXIS_Z] = (int32_t)(mTask.staticCali[AXIS_Z] * 1000);
            accGyroSendCalibrationResult(SENS_TYPE_ACCEL, (int32_t *)&caliResult[0], (uint8_t)status);

            osLog(LOG_INFO, LOG_TAG"ACC cali done %lld:caliResult[0]:%d, caliResult[1]:%d, caliResult[2]:%d, offset[0]:%f, offset[1]:%f, offset[2]:%f\n",
                rtcGetTime(), caliResult[AXIS_X], caliResult[AXIS_Y], caliResult[AXIS_Z],
                (double)mTask.staticCali[AXIS_X], (double)mTask.staticCali[AXIS_Y], (double)mTask.staticCali[AXIS_Z]);
        }
        #endif
    }

#ifndef CFG_MTK_CALIBRATION_V1_0
            data->x = temp_data[AXIS_X] + mTask.staticCali[AXIS_X];
            data->y = temp_data[AXIS_Y] + mTask.staticCali[AXIS_Y];
            data->z = temp_data[AXIS_Z] + mTask.staticCali[AXIS_Z];
#else
            data->x = raw_data[AXIS_X] - mTask.staticCali[AXIS_X];
            data->y = raw_data[AXIS_Y] - mTask.staticCali[AXIS_Y];
            data->z = raw_data[AXIS_Z] - mTask.staticCali[AXIS_Z];
#endif

    mTask.accFactoryData.ix = (int32_t)(data->x * ACCELEROMETER_INCREASE_NUM_AP);
    mTask.accFactoryData.iy = (int32_t)(data->y * ACCELEROMETER_INCREASE_NUM_AP);
    mTask.accFactoryData.iz = (int32_t)(data->z * ACCELEROMETER_INCREASE_NUM_AP);
    //osLog(LOG_INFO, "acc rawdata x:%f, y:%f, z:%f\n", (double)data->x, (double)data->y, (double)data->z);

    return 0;
}
#else
static int sc7a20gConvert(uint8_t *databuf, struct accGyroData *data)
{
    int32_t raw_data[AXES_NUM];
    int32_t remap_data[AXES_NUM];
    // TODO: update shift based on 14/12bit data, 2/4
    uint8_t right_shift = 4;
    //osLog(LOG_INFO, "sc7a20gConvert\n\r");

	//  add by djq 2017-08-02 start
#ifdef SC7A20_cali_self
	static int16_t axis_x_off = 0;  			//jq 2017-08-07
	static int16_t axis_y_off = 0;  			//jq 2017-08-07
	static int16_t axis_z_off = Z_OFF_DEFAULT;  //jq 2017-08-07
	static uint16_t index = 0;
	static int16_t x_value[SUM_DOT] ={0};
	static int16_t y_value[SUM_DOT] ={0};
	static int16_t z_value[SUM_DOT] ={0};

	int flag_x = 0;
	int flag_y = 0;
	int flag_z = 0;

	int i = 0;

	int16_t x_min = 0;
	int16_t x_max = 0;
	int16_t y_min = 0;
	int16_t y_max = 0;
	int16_t z_min = 0;
	int16_t z_max = 0;

	int32_t temp_x_value = 0;
	int32_t temp_y_value = 0;
	int32_t temp_z_value = 0;
#endif

    raw_data[AXIS_X] = (int16_t)((databuf[AXIS_X * 2 + 1] << 8) | databuf[AXIS_X * 2]);
    raw_data[AXIS_Y] = (int16_t)((databuf[AXIS_Y * 2 + 1] << 8) | databuf[AXIS_Y * 2]);
    raw_data[AXIS_Z] = (int16_t)((databuf[AXIS_Z * 2 + 1] << 8) | databuf[AXIS_Z * 2]);

    /* shift data to correct format */
    raw_data[AXIS_X] = raw_data[AXIS_X] >> right_shift;
    raw_data[AXIS_Y] = raw_data[AXIS_Y] >> right_shift;
    raw_data[AXIS_Z] = raw_data[AXIS_Z] >> right_shift;
    osLog(LOG_INFO, "sc7a20 acc2 rawdata x:%d, y:%d, z:%d\n", raw_data[AXIS_X], raw_data[AXIS_Y], raw_data[AXIS_Z]);

#ifdef SC7A20_cali_self
    	if(index < SUM_DOT)
		{
	    	if( raw_data[AXIS_X] > XY_THR_N && raw_data[AXIS_X] < XY_THR_P)
	    		flag_x = 1;
			else
				flag_x = 0;

	    	if( raw_data[AXIS_Y] > XY_THR_N && raw_data[AXIS_Y] < XY_THR_P)
	    		flag_y = 1;
			else
				flag_y = 0;

        	if((raw_data[AXIS_Z] > Z_THR_MAX_N && raw_data[AXIS_Z] < Z_THR_MIN_N)  || (raw_data[AXIS_Z] > Z_THR_MIN_P && raw_data[AXIS_Z] < Z_THR_MAX_P))
	    		flag_z = 1;
			else
				flag_z = 0;

        	if(flag_x == 1 && flag_y == 1 && flag_z == 1)
        	{
				x_value[index] = raw_data[AXIS_X];
				y_value[index] = raw_data[AXIS_Y];
				z_value[index] = raw_data[AXIS_Z];
				index = index + 1;
        	}
        	else
            	index = 0;
		}

        if(index == SUM_DOT)
        {
			find_max_min(x_value, SUM_DOT, &x_max, &x_min);
			find_max_min(y_value, SUM_DOT, &y_max, &y_min);
			find_max_min(z_value, SUM_DOT, &z_max, &z_min);

			if( ((x_max - x_min) < THRESHOLD_VAL)  && ((y_max - y_min) < THRESHOLD_VAL) && ((z_max - z_min) < THRESHOLD_VAL))
			{

				temp_x_value = 0;
				for(i = 0; i < SUM_DOT; i++)
				{
					temp_x_value += x_value[i];
				}
				temp_x_value = temp_x_value / SUM_DOT;
				axis_x_off = 0 - (int16_t)temp_x_value;

				temp_y_value = 0;
				for(i = 0; i < SUM_DOT; i++)
				{
					temp_y_value += y_value[i];
				}
				temp_y_value = temp_y_value / SUM_DOT;
				axis_y_off = 0 - (int16_t)temp_y_value;

				temp_z_value = 0;
				for(i = 0; i < SUM_DOT; i++)
				{
					temp_z_value += z_value[i];
				}
				temp_z_value = temp_z_value / SUM_DOT;

				if(temp_z_value > Z_THR_MAX_N && temp_z_value < Z_THR_MIN_N)
					axis_z_off = -1024 - (int16_t)temp_z_value;
				else
					axis_z_off = 1024 - (int16_t)temp_z_value;
					
			    index += 1;		

			}
			else
			{
			    index = 0; 
			}

		}
		raw_data[AXIS_X] +=  axis_x_off;
	    raw_data[AXIS_Y] +=  axis_y_off;
	    raw_data[AXIS_Z] +=  axis_z_off;

#endif

#ifdef SILAN_SC7A20_FILTER

	raw_data[AXIS_X] = silan_filter_process(&core_channel.sl_channel[0], raw_data[AXIS_X]);
	raw_data[AXIS_Y] = silan_filter_process(&core_channel.sl_channel[1], raw_data[AXIS_Y]);
	raw_data[AXIS_Z] = silan_filter_process(&core_channel.sl_channel[2], raw_data[AXIS_Z]);

#endif

    raw_data[AXIS_X] = raw_data[AXIS_X] + mTask.accSwCali[AXIS_X];
    raw_data[AXIS_Y] = raw_data[AXIS_Y] + mTask.accSwCali[AXIS_Y];
    raw_data[AXIS_Z] = raw_data[AXIS_Z] + mTask.accSwCali[AXIS_Z];

    /*remap coordinate*/
    remap_data[mTask.cvt.map[AXIS_X]] = mTask.cvt.sign[AXIS_X] * raw_data[AXIS_X];
    remap_data[mTask.cvt.map[AXIS_Y]] = mTask.cvt.sign[AXIS_Y] * raw_data[AXIS_Y];
    remap_data[mTask.cvt.map[AXIS_Z]] = mTask.cvt.sign[AXIS_Z] * raw_data[AXIS_Z];

    /* Out put the degree/second(mo/s) */
    data->sensType = SENS_TYPE_ACCEL;
    data->x = (float)remap_data[AXIS_X] * GRAVITY_EARTH_SCALAR / mTask.accReso->sensitivity;
    data->y = (float)remap_data[AXIS_Y] * GRAVITY_EARTH_SCALAR / mTask.accReso->sensitivity;
    data->z = (float)remap_data[AXIS_Z] * GRAVITY_EARTH_SCALAR / mTask.accReso->sensitivity;
    mTask.accFactoryData.ix = (int32_t)(data->x * ACCELEROMETER_INCREASE_NUM_AP);
    mTask.accFactoryData.iy = (int32_t)(data->y * ACCELEROMETER_INCREASE_NUM_AP);
    mTask.accFactoryData.iz = (int32_t)(data->z * ACCELEROMETER_INCREASE_NUM_AP);
    //osLog(LOG_INFO, "acc rawdata x:%f, y:%f, z:%f\n", (double)data->x, (double)data->y, (double)data->z);

    return 0;
}
#endif  /* ACC_CALI */
static int sc7a20Convert(I2cCallbackF i2cCallBack, SpiCbkF spiCallBack, void *next_state,
                         void *inBuf, uint8_t inSize, uint8_t elemInSize,
                         void *outBuf, uint8_t *outSize, uint8_t *elemOutSize)
{
    struct accGyroData *data = mTask.accGyroPacket.outBuf;
    uint8_t accEventSize = 0, gyroEventSize = 0;
    uint8_t *databuf;

    //osLog(LOG_INFO, "sc7a20Convert");
    databuf = &mTask.rxBuf[0];
    accEventSize++;
    sc7a20gConvert(databuf, &data[0]);

    #ifndef CFG_MTK_CALIBRATION_V1_0
    /*if startcali true , can't send to runtime cali in parseRawData to accGyro*/
    #if defined (ACC_CALI)
    if (mTask.startCali) {
        accEventSize = 0;
    }
    #endif  /* ACC_CALI */
    #endif

    txTransferDataInfo(&mTask.dataInfo, accEventSize, gyroEventSize, mTask.sampleTime, data, 29.0f);

    sensorFsmEnqueueFakeI2cEvt(i2cCallBack, next_state, SUCCESS_EVT);
    return 0;
}
static struct sensorFsm sc7a20Fsm[] = {
    sensorFsmCmd(STATE_SAMPLE, STATE_CONVERT, sc7a20Sample),
    sensorFsmCmd(STATE_CONVERT, STATE_SAMPLE_DONE, sc7a20Convert),
    /* acc enable state */
    sensorFsmCmd(STATE_ACC_ENABLE, STATE_ENPOWER_W, sc7a20gEnable),
    sensorFsmCmd(STATE_ENPOWER_W, STATE_ACC_ENABLE_DONE, sc7a20PowerEnableWrite),
    /* acc disable state */
    sensorFsmCmd(STATE_ACC_DISABLE, STATE_ACC_TURNOFF_W, sc7a20gDisable),
    sensorFsmCmd(STATE_ACC_TURNOFF_W, STATE_ACC_DISABLE_DONE, sc7a20PowerDisableWrite),
    /* acc rate state */
    sensorFsmCmd(STATE_ACC_RATECHG,  STATE_ACC_RATECHG_DONE, sc7a20gRate),

    // Jake.L, DATE20190401, Add Add calibration fsm state, DATE20190401-01 START
    #if defined (ACC_CALI)
    /* acc calibration */
    sensorFsmCmd(STATE_ACC_CALI, STATE_ACC_CALI_DONE, sc7a20AccCali),
    sensorFsmCmd(STATE_ACC_CFG, STATE_ACC_CFG_DONE, sc7a20AccCfgCali),
    #endif  /* ACC_CALI */
    // Jake.L, DATE20190401-01 END

    // TODO - write rate to sensor
    /* init state */
    sensorFsmCmd(STATE_RESET_R, STATE_RESET_W, sc7a20ResetRead),
    sensorFsmCmd(STATE_RESET_W, STATE_DEVID, sc7a20ResetWrite),
    sensorFsmCmd(STATE_DEVID, STATE_CALC_RESO, sc7a20DeviceId),
    sensorFsmCmd(STATE_CALC_RESO, STATE_CORE, sc7a20CalcReso),
    sensorFsmCmd(STATE_CORE, STATE_INIT_DONE, sc7a20RegisterCore),
};
void sc7a20TimerCbkF(void)
{
    //osLog(LOG_INFO, "sc7a20TimerCbkF\n");
}
static void i2cAutoDetect(void *cookie, size_t tx, size_t rx, int err)
{
    if (err == 0)
        osLog(LOG_INFO, "sc7a20: auto detect success:0x%x\n", mTask.deviceId);
    else
        osLog(LOG_ERROR, "sc7a20: auto detect error (%d)\n", err);

    mTask.autoDetectDone = true;
}
int sc7a20Init(void)
{
    int ret = 0;
    //osLog(LOG_INFO, "sc7a20: sc7a20Init\n");
    osLog(LOG_ERROR, "---[cyamon]------sc7a20: sc7a20Init\n");

    sc7a20DebugPoint = &mTask;
    insertMagicNum(&mTask.accGyroPacket);
    mTask.hw = get_cust_accGyro("sc7a20");
    if (NULL == mTask.hw) {
        osLog(LOG_ERROR, "---[cyamon]------sc7a20:get_cust_acc_hw fail\n");
        return 0;
    }
    mTask.i2c_addr = mTask.hw->i2c_addr[0];
    osLog(LOG_ERROR, "---[cyamon]------sc7a20: acc i2c_num: %d, i2c_addr: 0x%x\n", mTask.hw->i2c_num, mTask.i2c_addr);

    if (0 != (ret = sensorDriverGetConvert(mTask.hw->direction, &mTask.cvt))) {
        osLog(LOG_ERROR, "invalid direction: %d\n", mTask.hw->direction);
    }
    osLog(LOG_ERROR, "acc map[0]:%d, map[1]:%d, map[2]:%d, sign[0]:%d, sign[1]:%d, sign[2]:%d\n\r",
        mTask.cvt.map[AXIS_X], mTask.cvt.map[AXIS_Y], mTask.cvt.map[AXIS_Z],
        mTask.cvt.sign[AXIS_X], mTask.cvt.sign[AXIS_Y], mTask.cvt.sign[AXIS_Z]);
    i2cMasterRequest(mTask.hw->i2c_num, I2C_SPEED);

    mTask.autoDetectDone = false;
    mTask.txBuf[0] = SC7A20_WHO_AM_I ;
    ret = i2cMasterTxRx(mTask.hw->i2c_num, mTask.i2c_addr, mTask.txBuf, 1,
                        &mTask.deviceId, 1, i2cAutoDetect, NULL);

#ifdef SILAN_SC7A20_FILTER
	/* configure default filter param */
	core_channel.filter_param_l  = 2;
	core_channel.filter_param_h  = 8;
	core_channel.filter_threhold = 50;   //4G scale: 25; 2G scale: 50

	{
		int j = 0;
		for(j = 0; j < 3; j++)
		{
			core_channel.sl_channel[j].sample_l = 0;
			core_channel.sl_channel[j].sample_h = 0;
			core_channel.sl_channel[j].flag_l 	= 0;
			core_channel.sl_channel[j].flag_h 	= 0;
		}
	}
#endif

 osLog(LOG_ERROR, "---[cyamon]---sc7a20---ret=[%x]\n", ret);
    if (ret != 0) {
        osLog(LOG_ERROR, "sc7a20: auto detect i2cMasterTxRx error (%d)\n", ret);
        ret = -1;
        goto err_out;
    }

    /* wait i2c rxdata */
    if(!mTask.autoDetectDone)
        mdelay(1);
    if(!mTask.autoDetectDone)
        mdelay(3);

    osLog(LOG_ERROR, "---[cyamon]---sc7a20---deviceId=[%x]\n", mTask.deviceId);
    if (mTask.deviceId != SC7A20_WHO_AM_I_WIA_ID) {
        osLog(LOG_ERROR, "sc7a20: auto detect error wai 0x%x\n", mTask.deviceId);
        ret = -1;
        goto err_out;
    }

    osLog(LOG_INFO, "sc7a20: auto detect success wai 0x%x\n", mTask.deviceId);
    accSensorRegister();
    registerAccGyroInterruptMode(ACC_GYRO_FIFO_UNINTERRUPTIBLE);
    registerAccGyroDriverFsm(sc7a20Fsm, ARRAY_SIZE(sc7a20Fsm));
    registerAccGyroTimerCbk(sc7a20TimerCbkF);

err_out:
    return ret;

}
#ifndef CFG_OVERLAY_INIT_SUPPORT
MODULE_DECLARE(sc7a20, SENS_TYPE_ACCEL, sc7a20Init);
#else
#include "mtk_overlay_init.h"
OVERLAY_DECLARE(sc7a20, OVERLAY_WORK_00, sc7a20Init);
#endif


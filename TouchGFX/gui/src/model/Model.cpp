#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <gui/watch/WatchUiData.hpp>

#ifndef SIMULATOR
#include "max30102_service.h"
#endif

Model::Model() : modelListener(0), tickCounter(125)
{

}

void Model::tick()
{
    if (modelListener != 0 && (tickCounter % 15U) == 0U)
    {
        WatchUi::WatchSnapshot snapshot = WatchUi::sampleSnapshot(tickCounter);

#ifndef SIMULATOR
        const Max30102ServiceSnapshot sensor = Max30102Service_GetSnapshot();

        snapshot.sensorReady = sensor.sensorReady;
        snapshot.fingerDetected = sensor.fingerDetected;
        snapshot.sensorValid = sensor.fingerDetected && sensor.heartValid && sensor.spo2Valid;
        snapshot.rawRed = sensor.rawRed;
        snapshot.rawIr = sensor.rawIr;
        snapshot.validSamples = sensor.validSamples;
        snapshot.partId = sensor.partId;
        snapshot.fifoWritePtr = sensor.fifoWritePtr;
        snapshot.fifoReadPtr = sensor.fifoReadPtr;
        snapshot.fifoAvailable = sensor.fifoAvailable;
        snapshot.modeConfig = sensor.modeConfig;
        snapshot.spo2Config = sensor.spo2Config;
        snapshot.led1Pa = sensor.led1Pa;
        snapshot.led2Pa = sensor.led2Pa;
        snapshot.fifoConfig = sensor.fifoConfig;
        snapshot.interruptEnable1 = sensor.interruptEnable1;
        snapshot.interruptEnable2 = sensor.interruptEnable2;
        snapshot.overflowCounter = sensor.overflowCounter;
        snapshot.maxFifoAvailable = sensor.maxFifoAvailable;
        snapshot.samplesReadTotal = sensor.samplesReadTotal;
        snapshot.emptyFifoPolls = sensor.emptyFifoPolls;
        snapshot.sampleEngineTestCode = sensor.sampleEngineTestCode;
        snapshot.sampleEngineFifoAfterKick = sensor.sampleEngineFifoAfterKick;
        snapshot.sampleEngineReadCode = sensor.sampleEngineReadCode;
        snapshot.sampleEngineRawRed = sensor.sampleEngineRawRed;
        snapshot.sampleEngineRawIr = sensor.sampleEngineRawIr;
        snapshot.fifoReadAttempts = sensor.fifoReadAttempts;
        snapshot.i2cReady = sensor.i2cReady;
        snapshot.max30102Ack = sensor.max30102Ack;
        snapshot.busDeviceCount = sensor.busDeviceCount;
        snapshot.busFirstAddress = sensor.busFirstAddress;
        snapshot.i2cReadyStatus = sensor.i2cReadyStatus;
        snapshot.i2cError = sensor.i2cError;
        snapshot.fifoReadFailures = sensor.fifoReadFailures;
        snapshot.diagnosticReadFailures = sensor.diagnosticReadFailures;

        if (sensor.heartValid && sensor.heartBpm <= 255U)
        {
            snapshot.heartRate = static_cast<uint8_t>(sensor.heartBpm);
        }
        else
        {
            snapshot.heartRate = 0U;
        }

        if (sensor.spo2Valid && sensor.spo2X10 != 0U)
        {
            snapshot.spo2Percent = static_cast<uint8_t>((sensor.spo2X10 + 5U) / 10U);
        }
        else
        {
            snapshot.spo2Percent = 0U;
        }

        snapshot.temperatureC = static_cast<int8_t>(sensor.temperatureX10 / 10);
#endif

        modelListener->watchDataUpdated(snapshot);
    }
    ++tickCounter;
}

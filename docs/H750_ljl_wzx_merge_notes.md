# ljl 合并 wzx 后的数据链路说明

本文记录 `origin/wzx` 合并到 `ljl` 后，心率血氧和步行数据在 H750 TouchGFX 工程里的分工。

## 当前结论

- 心率血氧继续使用 MAX30102 链路。
- 步数、步行距离、实时速度、平均速度使用 `wzx` 的 IMU/WalkMetrics 链路。
- H750 端 IMU 数据只走 USART1 无线接收，不启用 I2C fallback。
- TouchGFX 仍保持 6 屏结构：`lock`、`home`、`screen1`、`walk`、`run`、`rope`。

## 心率血氧链路

MAX30102 仍由 H750 本机读取：

```text
MAX30102
  -> I2C1 PB8/PB9
  -> Max30102Service_Poll()
  -> Max30102Service_GetSnapshot()
  -> TouchGFX Model
  -> Presenter
  -> walk/run/rope View
```

显示规则保持不变：

- `SENSOR ERR`：设备或读取异常。
- `PLACE FINGER`：设备存在，但未检测到手指。
- `MEASURING`：检测到手指，但还没有稳定有效结果。
- `HR <bpm>` 和 `<spo2>%`：有效结果。

## 步行数据链路

`wzx` 的实时运动数据走无线串口：

```text
F103 IMU 工程
  -> USART1 输出 IMU 文本帧
  -> DL-20 无线模块
  -> H750 USART1 PA10/PA9
  -> IMU_Sensor_OnWirelessByte()
  -> IMU_Sensor_Read()
  -> WalkMetrics_Update()
  -> walkView
```

H750 等待的数据格式是：

```text
IMU,<ms>,<ax_mg>,<ay_mg>,<az_mg>,<gx_mrad>,<gy_mrad>,<gz_mrad>\r\n
```

`walk` 页目前使用这条链路刷新：

- `TIME`
- `NOW`
- `AVG`
- `STEP`
- 主数值距离 `KM`

心率血氧和步行数据是两条独立链路。心率血氧不依赖 IMU，步数也不依赖 MAX30102。

## 为什么关闭 I2C fallback

`fallback` 是备用读取路径，意思是：如果无线 IMU 数据没有到，代码还可以尝试从 H750 本机 I2C 直接读 IMU。

这次合并后保持：

```c
#define IMU_ENABLE_I2C_FALLBACK 0U
```

原因是 `wzx` 的 I2C fallback 使用 I2C4，并且会占用 `PB8/PB9`；但当前 `ljl` 中 `PB8/PB9` 已经是 MAX30102 的 `I2C1_SCL/I2C1_SDA`。

如果打开这个 fallback，会出现同一组引脚同时被两个外设方案占用的问题：

```text
PB8/PB9
  -> 当前 ljl: MAX30102 I2C1
  -> wzx fallback: IMU I2C4
```

所以当前工程选择：

```text
MAX30102: I2C1 PB8/PB9
IMU/步数: USART1 PA10/PA9 无线输入
I2C fallback: 关闭
```

## 调试判断顺序

如果心率血氧不刷新，优先查 MAX30102：

1. `ACK/ADDR`
2. `PART_ID`
3. `FIFO`
4. `rawRed/rawIr`
5. `Max30102Service_Poll()` 是否持续运行

如果步数不刷新，优先查 IMU 无线链路：

1. F103 是否持续输出 `IMU,...` 行。
2. DL-20 两端是否配对，波特率是否都是 115200。
3. H750 USART1 接线是否为 `PA10=RX`、`PA9=TX`，TX/RX 是否交叉。
4. `IMU_Sensor_OnWirelessByte()` 是否收到字节。
5. `IMU_Sensor_Read()` 是否返回样本。
6. `walkView::updateWalkMetrics()` 是否进入 `WalkMetrics_Update()`。

如果以后确实要让 H750 本机 I2C 直接读取 IMU，需要重新选一组不与 MAX30102、QSPI、LCD 冲突的引脚，再单独打开 fallback，不能直接复用 PB8/PB9。

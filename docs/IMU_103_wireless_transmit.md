# STM32F103 IMU 无线发送改造说明

这份说明给 `D:\WZX\IDE_workplace\IMU` 工程使用。当前 F103 工程已经能通过 I2C1 读取 IMU，但任务里只调用了 `SENSOR_Read_AccGyro()`，没有把数据通过 USART1 发给 DL-20，所以 H750 手表端收不到步行算法需要的数据，步数不会增加。

## 目标链路

```text
F103 IMU 工程
  I2C1 读取 IMU
  USART1 输出 IMU 文本帧
    |
    v
DL-20 模块 A ))) 2.4G ((( DL-20 模块 B
    |
    v
H750 手表 USART1 接收
  解析 IMU 文本帧
  喂给 WalkMetrics 步行算法
```

H750 端当前等待的数据格式是：

```text
IMU,<ms>,<ax_mg>,<ay_mg>,<az_mg>,<gx_mrad>,<gy_mrad>,<gz_mrad>\r\n
```

单位要求：

- `ax_mg/ay_mg/az_mg`：加速度，单位 mg，也就是 `g * 1000`
- `gx_mrad/gy_mrad/gz_mrad`：角速度，单位 mrad/s，也就是 `rad/s * 1000`
- 串口参数：`115200, 8N1`
- 建议发送频率：50Hz，也就是每 20ms 一帧

示例数据：

```text
IMU,123456,12,-38,998,120,-45,30
```

## F103 代码修改点

在 `Core/Src/main.c` 的 `USER CODE BEGIN Includes` 区域增加：

```c
#include <string.h>
```

在 `USER CODE BEGIN 4` 区域增加以下辅助函数：

```c
static int32_t ScaleFloatToI32(float value, float scale)
{
    float scaled = value * scale;
    return (scaled >= 0.0f) ? (int32_t)(scaled + 0.5f) : (int32_t)(scaled - 0.5f);
}

static void SENSOR_SendWirelessFrame(void)
{
    char line[96];
    int len;

    int32_t ax_mg = ScaleFloatToI32(sensor_data.Accel_X, 1000.0f);
    int32_t ay_mg = ScaleFloatToI32(sensor_data.Accel_Y, 1000.0f);
    int32_t az_mg = ScaleFloatToI32(sensor_data.Accel_Z, 1000.0f);

    int32_t gx_mrad = ScaleFloatToI32(sensor_data.Gyro_X, 1000.0f);
    int32_t gy_mrad = ScaleFloatToI32(sensor_data.Gyro_Y, 1000.0f);
    int32_t gz_mrad = ScaleFloatToI32(sensor_data.Gyro_Z, 1000.0f);

    len = snprintf(line,
                   sizeof(line),
                   "IMU,%lu,%ld,%ld,%ld,%ld,%ld,%ld\r\n",
                   (unsigned long)HAL_GetTick(),
                   (long)ax_mg,
                   (long)ay_mg,
                   (long)az_mg,
                   (long)gx_mrad,
                   (long)gy_mrad,
                   (long)gz_mrad);

    if ((len > 0) && (len < (int)sizeof(line)))
    {
        HAL_UART_Transmit(&huart1, (uint8_t*)line, (uint16_t)len, 20);
    }
}
```

然后把 `StartTask02()` 的循环从当前的 100ms 读取一次：

```c
for(;;) {
    SENSOR_Read_AccGyro();
    osDelay(100);
}
```

改成 20ms 读取并发送一次：

```c
for(;;) {
    SENSOR_Read_AccGyro();
    SENSOR_SendWirelessFrame();
    osDelay(20);
}
```

## DL-20 接线

F103 侧：

```text
DL-20 VCC -> F103 3.3V
DL-20 GND -> F103 GND
DL-20 TX  -> F103 PA10 / USART1_RX
DL-20 RX  -> F103 PA9  / USART1_TX
```

H750 侧：

```text
DL-20 VCC -> H750 3.3V
DL-20 GND -> H750 GND
DL-20 TX  -> H750 PA10 / USART1_RX
DL-20 RX  -> H750 PA9  / USART1_TX
```

注意：

- TX/RX 必须交叉。
- 两边必须共地。
- 建议都用 3.3V 供电和 3.3V 串口电平。

## DL-20 初始设置

两个 DL-20 模块需要设置成同一组点对点通信：

- 模式：点对点
- 一个设为 A 端，另一个设为 B 端
- 信道一致
- 波特率：115200
- 数据位/校验/停止位：8N1

进入配置模式的方法：按住模块按键再上电。

不建议用广播模式，因为资料里说明广播模式有约 5% 丢包。

## 上板前快速验证

先不要直接接 H750，建议先这样测 F103 发射端：

1. F103 的 `PA9/PA10/GND` 接 USB-TTL。
2. 串口助手打开 `115200, 8N1`。
3. 烧录 F103。
4. 正常应持续看到类似：

```text
IMU,1020,5,-22,1003,12,-4,31
IMU,1040,7,-20,1001,16,-3,28
```

如果串口助手看不到连续 `IMU,...` 行，问题在 F103 发送侧或接线。

如果 USB-TTL 能看到，换成 DL-20 后 H750 仍不计步，再排查：

- 两个 DL-20 是否一个 A 端一个 B 端
- 两边波特率是否都是 115200
- TX/RX 是否交叉
- H750 端是否已经进入 WALK 并按下 START
- 拿板子随手甩几下不一定等价于真实步行，后续可先做更宽松的测试阈值

## H750 调试屏结果解释

如果 H750 的 WALK 调试区显示：

```text
B/L/O 持续增加
E 保持 0
A 0 0 0
G 0 0 0
AGE 很小
```

含义是：H750 已经收到完整 `IMU,...` 行，并且格式解析成功；问题不在 H750 串口接收，也不是格式解析失败，而是收到的载荷数值就是 0。

这种情况下优先查 F103：

1. `SENSOR_Read_AccGyro()` 是否读失败。当前 F103 示例代码读失败会把 `Accel_X/Y/Z` 和 `Gyro_X/Y/Z` 全部置 0。
2. F103 的 IMU I2C 接线是否正确，尤其是 `PB6=SCL`、`PB7=SDA`、`3.3V`、`GND`。
3. F103 串口助手直连 `PA9/PA10` 时，是否能看到非 0 的 `IMU,...` 行。
4. 如果串口助手里也是全 0，先不要接 DL-20，直接修 F103 传感器读取。
5. 如果串口助手里非 0，但 H750 上全 0，再查 DL-20 是否有回环、广播配置、或者两端是否都在发送同名 `IMU,...` 数据。

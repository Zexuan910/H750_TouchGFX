触摸补充说明

本版本已经补全 CST816T 触摸：

1. 默认引脚
   PB6 -> I2C1_SCL
   PB7 -> I2C1_SDA
   CST816T 地址：0x15

2. 已新增文件
   Core/Inc/i2c.h
   Core/Src/i2c.c
   Core/Inc/cst816t.h
   Core/Src/cst816t.c
   TouchGFX/target/STM32TouchController.cpp 已改为读取 CST816T 坐标。

3. main.c 初始化顺序
   MX_GPIO_Init();
   MX_CRC_Init();
   MX_SPI1_Init();
   MX_TIM1_Init();
   MX_I2C1_Init();
   UI_Init();

4. 如果触摸没反应，先检查硬件
   - CST816T 的 SCL 是否接 PB6
   - CST816T 的 SDA 是否接 PB7
   - VCC 是否是 3.3V
   - GND 是否共地
   - I2C 上拉是否存在
   - CST816T 地址是否为 0x15

5. 如果触摸坐标方向不对，只改 Core/Inc/cst816t.h：
   CST816T_SWAP_XY
   CST816T_MIRROR_X
   CST816T_MIRROR_Y

例如上下反了：
   #define CST816T_MIRROR_Y 1U

左右反了：
   #define CST816T_MIRROR_X 1U

横竖轴反了：
   #define CST816T_SWAP_XY 1U

6. 如果你的触摸不是接 PB6/PB7
   需要修改 Core/Src/i2c.c 里的 I2C GPIO 初始化，或者回 CubeMX 把实际引脚配置为 I2C1/I2C2/I2C4。

注意：SCREEN_COLOR 原工程没有触摸驱动，所以这部分是新补的。显示部分仍按 SCREEN_COLOR 的 SPI1/PJ6/PJ7/PJ8/PA8 配置。

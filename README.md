# STM32N657 IMX219 摄像头工程

这个工程是一个两级启动示例：

```text
FSBL -> Appli
```

FSBL 负责初始化外部 Flash/启动链路并跳转到 Appli。Appli 初始化 GPIO、USART3、I2C1 和 DCMIPP/CSI，然后驱动 Sony IMX219 输出 `1640x1232 RAW10`，通过 DCMIPP PIPE1 转成 `640x480 RGB565`，并通过串口转储一帧用于验证。

## 编译

在工程根目录执行：

```powershell
D:/gunwin32/gnuwin32/bin/make.exe -C Makefile/FSBL
D:/gunwin32/gnuwin32/bin/make.exe -C Makefile/Appli
```

也可以使用系统 PATH 中的 `make`：

```powershell
make -C Makefile/FSBL
make -C Makefile/Appli
```

Appli 的 ELF 默认输出到：

```text
Makefile/Appli/build/fsbl_appli_led_usart_baseline_Appli.elf
```

## 下载和抓图

下载并运行 Appli：

```powershell
powershell -ExecutionPolicy Bypass -File tools/flash_run.ps1
```

串口等待 `RGB565_UART_DUMP_BEGIN` 后抓取一帧并转 PNG：

```powershell
powershell -ExecutionPolicy Bypass -File tools/capture_uart_rgb565_dump.ps1 -Port COMx
```

把 `COMx` 换成板子的 USART3 串口号。抓到的 `.bin/.png/.log` 会写入 `captures/`，该目录是测试输出目录，已经被 `.gitignore` 忽略，不作为工程源码提交。

## 主程序结构

`main.c` 只做系统初始化和调用摄像头驱动：

```c
MX_GPIO_Init();
MX_USART3_UART_Init();
MX_I2C1_Init();

if (CameraDriver_InitAndStart(&hi2c1) != HAL_OK)
{
  Error_Handler();
}

while (1)
{
  CameraDriver_Task();
}
```

摄像头相关入口在：

- `Appli/Core/Inc/camera_driver.h`
- `Appli/Core/Src/camera_driver.c`
- `Appli/Core/Inc/camera_pipeline.h`
- `Appli/Core/Src/camera_pipeline.c`
- `Drivers/BSP/Components/imx219/imx219.h`
- `Drivers/BSP/Components/imx219/imx219.c`

`camera_driver` 是给应用层使用的接口；`camera_pipeline` 负责 DCMIPP/CSI/帧缓冲/串口转储；`imx219` 是传感器寄存器驱动。

## 调曝光、模拟增益、WB、Gamma

推荐从默认参数开始改：

```c
CameraDriver_ImageControl_t ctrl;

CameraDriver_GetDefaultImageControl(&ctrl);
ctrl.exposure_lines = 0x01F4U;
ctrl.analog_gain = 0x80U;
ctrl.digital_gain = 0x0100U;
ctrl.wb_red_gain = 137500000U;
ctrl.wb_green_gain = CAMERA_DRIVER_WB_GAIN_1X;
ctrl.wb_blue_gain = 125000000U;
ctrl.gamma_enable = 0U;

(void)CameraDriver_ApplyImageControl(&ctrl);
```

也可以直接调用一个函数：

```c
(void)CameraDriver_SetImageControl(0x01F4U,
                                   0x80U,
                                   0x0100U,
                                   137500000U,
                                   CAMERA_DRIVER_WB_GAIN_1X,
                                   125000000U,
                                   0U);
```

这些参数可以在 `CameraDriver_InitAndStart()` 前调用作为预设，也可以在摄像头运行后调用在线修改。含义如下：

- `exposure_lines`：IMX219 coarse integration time，必须小于 `FRAME_LENGTH - 4`。
- `analog_gain`：IMX219 模拟增益寄存器值。
- `digital_gain`：IMX219 数字增益，`0x0100` 表示 1x。
- `wb_*_gain`：DCMIPP PIPE1 ISP exposure 的三通道白平衡增益，`100000000` 表示 1x。
- `gamma_enable`：`0` 关闭 DCMIPP Gamma，非 0 打开。

## IMX219 1640x1232 RAW10 到 640x480 RGB565

当前链路如下：

1. IMX219 初始化为 `IMX219_R1640_1232` 和 `IMX219_RAW10`，2 lane CSI。
2. 传感器进入低速链路配置：DCMIPP PHY 使用 `BT_450`，IMX219 `op_pll_mult = 0x0039`。
3. CSI 虚拟通道 0 配置为 RAW10，PIPE1 强制 RAW10 数据类型。
4. PIPE1 执行 RawBayer2RGB，Bayer 顺序为 `RGGB`，输出 RGB 数据。
5. PIPE1 可选应用 WB/Gamma。
6. DCMIPP downsize 从 RawBayer2RGB 后的有效尺寸缩放到 `640x480`。
7. Pixel packer 输出 `RGB565_1`，行 pitch 按 16 字节对齐。
8. 帧缓冲固定在 `0x34082000`，默认帧大小为 `640 * 480 * 2 = 614400` 字节。

### 缩放方式

当前程序不是在 C 代码里手动“隔一个像素抽一个点”，也不是手动计算“两点均值”。实际缩放交给 STM32N6 的 DCMIPP PIPE1 downsize 硬件完成，软件只配置源尺寸、目标尺寸、比例寄存器和分频因子。

当前有效尺寸分两步变化：

```text
IMX219 RAW10 Bayer:        1640 x 1232
RawBayer2RGB 后有效尺寸:    820 x 616
DCMIPP downsize 输出:       640 x 480 RGB565
```

代码里对应配置在 `Appli/Core/Src/camera_pipeline.c`：

```c
downsize_conf.HSize = 640;
downsize_conf.VSize = 480;
downsize_conf.HRatio = DCMIPP_DOWNSIZE_RATIO(820, 640);
downsize_conf.VRatio = DCMIPP_DOWNSIZE_RATIO(616, 480);
downsize_conf.HDivFactor = DCMIPP_DOWNSIZE_DIV_FACTOR(820, 640);
downsize_conf.VDivFactor = DCMIPP_DOWNSIZE_DIV_FACTOR(616, 480);
HAL_DCMIPP_PIPE_SetDownsizeConfig(&hdcmipp, DCMIPP_PIPE1, &downsize_conf);
HAL_DCMIPP_PIPE_EnableDownsize(&hdcmipp, DCMIPP_PIPE1);
```

按当前宏计算，实际写入的近似参数是：

```text
H_RATIO = 820 * 8192 / 640 = 10496
V_RATIO = 616 * 8192 / 480 = 10513
H_DIV   = 1024 * 640 / 820 = 799
V_DIV   = 1024 * 480 / 616 = 797
```

所以从程序角度看，它是 DCMIPP 的硬件比例缩放。硬件内部具体滤波方式由 DCMIPP downsize 模块实现，HAL 接口没有暴露“均值/抽点/双线性”等模式选择；当前工程没有做软件均值或软件抽点。

## 移植到其他工程

1. 拷贝 `Drivers/BSP/Components/imx219/` 到目标工程。
2. 拷贝 `Appli/Core/Inc/camera_driver.h`、`camera_pipeline.h` 和对应 `.c` 文件。
3. 在目标工程 Makefile 或 IDE 工程中加入：
   - `Appli/Core/Src/camera_driver.c`
   - `Appli/Core/Src/camera_pipeline.c`
   - `Drivers/BSP/Components/imx219/imx219.c`
4. 保证包含路径有：
   - `Appli/Core/Inc`
   - `Drivers/BSP/Components/imx219`
   - `Drivers/STM32N6xx_HAL_Driver/Inc`
   - `Drivers/CMSIS/...`
5. 目标工程需要提供这些外设和符号：
   - `I2C_HandleTypeDef hi2c1`
   - USART3 的 `__io_putchar`
   - `EN_MODULE_GPIO_Port` / `EN_MODULE_Pin`
   - DCMIPP、CSI、GPDMA、GPIO、RIF/RISAF 初始化
6. 在 `main.c` 初始化 GPIO/USART/I2C 后调用：

```c
if (CameraDriver_InitAndStart(&hi2c1) != HAL_OK)
{
  Error_Handler();
}
```

循环里调用：

```c
CameraDriver_Task();
```

移植时优先检查帧缓冲地址 `CAMERA_PIPELINE_BUFFER_ADDRESS` 和 RISAF/GPDMA 安全属性。当前工程使用 `0x34082000`，目标工程如果 RAM 布局不同，需要同步修改 linker/RISAF/缓存维护相关配置。

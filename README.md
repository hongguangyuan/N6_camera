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
2. 当前按 IMX219 2-lane 参考链路配置：DCMIPP PHY 使用 `BT_950`，IMX219 使用默认 `op_pll_mult = 0x0072`。
3. CSI 虚拟通道 0 配置为 RAW10，PIPE1 强制 RAW10 数据类型。
4. PIPE1 执行 RawBayer2RGB，Bayer 顺序为 `RGGB`，输出 RGB 数据。
5. PIPE1 可选应用 WB/Gamma。
6. DCMIPP downsize 从 RawBayer2RGB 后的有效尺寸缩放到 `640x480`。
7. Pixel packer 输出 `RGB565_1`，行 pitch 按 16 字节对齐。
8. 帧缓冲固定在 `0x34082000`，默认帧大小为 `640 * 480 * 2 = 614400` 字节。

当前 IMX219 帧时序按 24fps 高速链路测试配置：

```text
LINE_LENGTH  = 3560
FRAME_LENGTH = 2134
LINE_TIME    = 19528 ns
FRAME_TIME   = 19528 ns * 2134 = 41.673 ms
FPS          = 1000 / 41.673 = 23.996 fps，约 24 fps
```

注意：`pipe1-rgb565-continuous-24fps-20260610` 分支里的 `FRAME_LENGTH = 1067` 是 640x480 传感器输入模式使用的参数，不能直接套到当前 `IMX219_R1640_1232`。当前输入高度是 1232 行，`FRAME_LENGTH` 必须大于 1232，否则传感器帧时序会不完整，容易出现横纹、亮条或画面错乱。

调试记录：低速链路 `op_pll_mult = 0x0039` 下，`LINE_LENGTH = 3560` 会出现横纹/错行；`LINE_LENGTH = 8000` 可恢复真实画面，但帧率只有约 6.675fps。该路径不是 24fps 目标路径，仅用于确认 DCMIPP/PIPE1 能完整写帧。

参考 Linux 主线 IMX219 驱动，`1640x1232` RAW10 使用 2x2 binning，`LINE_LENGTH = 3560`，默认 link frequency 为 `456 MHz`，对应 2-lane D-PHY 约 `912 Mbps/lane`。因此当前 24fps 调试不再使用低速链路，而是使用默认 PLL，并把 DCMIPP PHY 设到接近 912Mbps 的 `BT_950` 档。`BT_900` 实测 UART dump 中 `0xA5A5` 预填充值占比约 96.875%，说明 PIPE1 没有完整写出帧。

`BT_950` 实测日志中 `csi_err1/csi_err2 = 0`，并且 line-byte 计数在第 1、124、616、1232 行都跟随帧数递增，说明 CSI 输入链路已经稳定收到 RAW10 帧；但 `P1SR` 出现 `OVRF`，RGB565 buffer 仍约 96.875% 是 `0xA5A5` 预填值。因此当前 24fps 调试改成 snapshot 单帧模式，避免连续采集让 PIPE1 长时间 overrun。若单帧仍失败，下一步应拆分 PIPE1 路径，例如先抓 RAW/GRAY 或关闭部分处理，定位 RawBayer2RGB、downsize、RGB565 pack/write 中是哪一级在高速输入下溢出。

2026-07-03 更新：24fps snapshot RGB565 仍基本是预填值，因此当前默认调试开关临时切到 `CAMERA_PIPELINE_RAW_GRAY_DEBUG = 1U`。此模式仍保持 IMX219 默认高速链路和 `BT_950`，但关闭 RawBayer2RGB/downsize/RGB565 组合路径，使用 PIPE1 `MONO_Y8_G8_1` 输出 `GRAY8`，用于确认高速 RAW10 输入能否在更轻的 PIPE1 写内存路径下完整落帧。抓图脚本会输出 `uart_gray8_*.bin/png`。

2026-07-03 再更新：GRAY8 仍约 96.987% 为 `0xA5`，且 `P1SR` 仍带 `OVRF`，说明问题仍在 PIPE1 这侧。当前默认调试开关继续切到 `CAMERA_PIPELINE_PIPE0_RAW10P_DEBUG = 1U`，绕开 PIPE1，改用 PIPE0 抓前 300 行 RAW10 packed 数据。抓图脚本会输出 `uart_raw10p_*.bin/png`；如果 PIPE0 RAW10P 可以正常变化，说明 CSI 和内存写入基础路径可用，后续重点回到 PIPE1 的格式/吞吐配置。

2026-07-03 RAW10P 结果：PIPE0 抓到的数据不再是预填，`0xA5` 占比低于 1%，并且数据会达到 `0x000..0x3FF`；但字节流形态更像低 10 位放在 16-bit little-endian 中，而不是标准 5-byte RAW10 packed。因此当前调试开关改成 `CAMERA_PIPELINE_PIPE0_RAW16_DEBUG = 1U`，`CAMERA_PIPELINE_PIPE0_RAW10P_DEBUG = 0U`，只抓左侧 `1024x300` RAW16，输出约 `1024 * 300 * 2 = 614400` 字节，便于确认真实 RAW 画面。

2026-07-03 RAW16 结果：`1024x300` RAW16 dump 中 `0xA5` 占比为 0，`min_raw10=65`、`max_raw10=341`，且 `csi_err1/csi_err2 = 0`，PIPE0 无 overrun。由此确认 IMX219 默认高速链路、CSI 接收、PIPE0 到内存写入是可用的。RAW16 预览图仍有斜纹/横纹，主要用于判断链路是否写入，不作为最终图像质量依据；当前 RGB565/GRAY8 失败点集中在 PIPE1 后处理/写出路径的 `OVRF`。

2026-07-03 PIPE1 降载试验：当前默认开关改为 `CAMERA_PIPELINE_RAW_GRAY_DEBUG = 1U`、`CAMERA_PIPELINE_PIPE1_GRAY_CROP_ONLY_DEBUG = 1U`，关闭 PIPE0 RAW16，保留高速 `BT_950`。此模式使用 PIPE1 直接做 `RAW10 -> GRAY8`，关闭 RawBayer2RGB 和 downsize，只裁 `640x240` 小窗口写内存，输出 `153600` 字节，用于验证 PIPE1 在最轻路径下是否仍会 `OVRF`。

2026-07-03 PIPE1 小窗口结果：`640x240` GRAY8 仍约 96.073% 为 `0xA5`，每行只写入约 24-28 字节，`P1SR` 仍为 `0x00020087`，说明小窗口 crop 没有解决 PIPE1 `OVRF`。随后改为 `CAMERA_PIPELINE_PIPE1_GRAY_DECIM_ONLY_DEBUG = 1U`、`CAMERA_PIPELINE_PIPE1_GRAY_CROP_ONLY_DEBUG = 0U`，在 PIPE1 入口强制 1/2 decimation，输出 `820x616` GRAY8。

2026-07-03 PIPE1 入口 decimation 结果：`820x616` GRAY8 中 `0xA5` 为 `504440 / 512512`，约 98.43%，仍是预填。由此判断，当前 `1640x1232` 高速 RAW10 输入下，PIPE1 即使只做 GRAY8 和入口 1/2 decimation 也会 `OVRF`。当前调试开关改为 `CAMERA_PIPELINE_SENSOR_640X480_DEBUG = 1U`、`CAMERA_PIPELINE_RAW_GRAY_DEBUG = 1U`、`CAMERA_PIPELINE_PIPE1_GRAY_DECIM_ONLY_DEBUG = 0U`：IMX219 直接输出 `640x480 RAW10`，DCMIPP 仍使用默认高速链路 `BT_950`，PIPE1 只做 `RAW10 -> GRAY8`，用于确认 PIPE1 在传感器端小输入下能否完整写出。预期 UART 输出为 `GRAY8_UART_DUMP_BEGIN width=640 height=480 stride=640 bytes=307200`。

2026-07-03 PIPE1 小输入结果：`640x480` GRAY8 中 `0xA5` 为 `295108 / 307200`，约 96.06%，仍是预填。因此当前不再继续优先调 PIPE1，默认调试开关改为 `CAMERA_PIPELINE_SENSOR_640X480_DEBUG = 1U`、`CAMERA_PIPELINE_PIPE0_RAW16_DEBUG = 1U`、`CAMERA_PIPELINE_RAW_GRAY_DEBUG = 0U`。此模式使用 IMX219 `640x480 RAW10`、DCMIPP `BT_950`、PIPE0 RAW16 写内存，预期 UART 输出为 `RAW16_UART_DUMP_BEGIN width=640 height=480 stride=1280 bytes=614400`。若 PIPE0 完整写入，再在 PIPE0 RAW 基础上做软件 Bayer -> RGB565，绕开 PIPE1。

2026-07-03 PIPE0 小输入结果：`640x480` RAW16 中 `min_raw10=57`、`max_raw10=266`、`high6_nonzero=0`，画面来自真实场景，说明 PIPE0 路径稳定；但 `IMX219_R640_480` 是传感器中心裁剪，视角很小。当前默认调试改为 `CAMERA_PIPELINE_SENSOR_640X480_DEBUG = 0U`、`CAMERA_PIPELINE_PIPE0_RAW16_DEBUG = 1U`、`CAMERA_PIPELINE_PIPE0_SW_RGB565_DEBUG = 1U`：IMX219 回到 `1640x1232` binned 模式，PIPE0 抓中心 `640x480` RAW16 窗口，然后 CPU 原地映射成灰度 `RGB565` 再 UART 输出。预期 UART 输出恢复为 `RGB565_UART_DUMP_BEGIN width=640 height=480 stride=1280 bytes=614400`。这一步先跑通绕开 PIPE1 的 RGB565 输出；后续再把中心窗口升级为全视场降采样和彩色 Bayer demosaic。

2026-07-06 更新：第一版 PIPE0 软件 RGB565 使用固定 `black=48`、`white=300`，在 `1640x1232` 中心窗口下输出全白，抓到的 RGB565 全部为 `0xFFFF`。当前已改成按本帧 RAW10 直方图自动拉伸：`black_p1` 使用约 1% 分位，`white_p99` 使用约 99% 分位，避免不同场景亮度变化时整帧饱和。UART 日志会打印 `PIPE0 software RGB565: gray raw10 min=... max=... black_p1=... white_p99=...`。

2026-07-06 再更新：中心 crop `(hstart=500, vstart=376)` 下 PIPE0 软件转换日志显示 `min=341 max=341`，最终 RGB565 全部为 `0x0000`。这说明该中心 crop 在当前 RAW10/PIPE0 配置下读到的是常数帧，不是有效 RAW 画面。当前先把 PIPE0 crop 起点恢复为 `(0,0)`，沿用之前已验证过的左上 RAW16 写入路径，优先跑通 PIPE0 RAW -> 软件 RGB565。

2026-07-06 PIPE0 软件 RGB565 结果：左上 `(0,0)` RAW 路径重新有动态范围，日志显示 `min=63 max=341`，但直接把 Bayer 每个采样点当灰度会出现明显横纹/棋盘纹，看起来像解析错误。当前已改为 2x2 Bayer block 平均后再写回 2x2 RGB565 灰度块，UART 输出尺寸仍为 `640x480`，但会先去掉 Bayer CFA 条纹。日志中的转换标记改为 `bayer2x2avg gray`。

2026-07-06 全视场降载试验：上一版 PIPE0 只抓 `1640x1232` 输入左上角 `640x480`，所以画面左侧发黑且视角很窄。随后试过 PIPE0 对完整 `1640x1232` 做 byte/line 1/2 decimation，再写入 `820x616 RAW16` 工作帧，但输出出现大面积斜杠/黑白噪声，判断为横向 `DCMIPP_BSM_DATA_OUT_2` 破坏 RAW10/RAW16 字节或像素相位。当前默认链路改为横向不做 byte decimation，只裁左侧 `820` 宽；纵向做 line 1/2 decimation，得到 `820x616 RAW16` 工作帧，再由 CPU 转成 `640x480 RGB565`。UART 输出仍为 `RGB565_UART_DUMP_BEGIN width=640 height=480 stride=1280 bytes=614400`。

2026-07-06 原地转换修正：`820x616 RAW16` 工作帧和 `640x480 RGB565` 输出帧共用 `0x34082000`，两块内存有重叠。第一版从右下往左上写，会提前覆盖中间部分尚未读取的 RAW 数据，表现为上半屏黑白噪声、下半屏还能看到真实画面。当前改为从左上往右下转换，保证每个源 RAW 点先读取再被 RGB565 输出覆盖。

2026-07-06 PIPE0 stride 修正：横向不抽 byte 后，黑白噪声明显减少，但画面仍整幅斜着走。查 HAL 后确认 `Pipe_Config()` 对 PIPE0 只配置 frame rate，PIPE0 没有 `P0PPM0PR` pitch 寄存器，因此 P0 实际按连续行写入，`820xRAW16` 的真实行距是 `1640` 字节，而不是软件原先使用的 16 字节对齐 `1648`。当前已把 PIPE0 RAW 工作帧 stride 改为 `1640`，避免每行多跳 8 字节造成斜线。

2026-07-06 灰度拉伸修正：stride 修正后画面已经正常，但整体偏灰。原因是旧版自动拉伸用单个 RAW 点建立直方图，而最终输出用 2x2 RAW 平均值；两者分布不同，导致黑位没有压到实际输出暗部。当前改为按最终输出同样的 2x2 平均样本建立直方图，再用 1%/99% 分位做拉伸。

2026-07-06 直出清晰度试验：为了确认“糊”来自软件缩放/2x2 平均还是镜头/曝光，当前调试版改为直接输出 `820x616 RGB565`，不再缩放到 `640x480`，也不再做 2x2 平均。软件转换只把每个 RAW10 样点按本帧 1%/99% 分位拉伸成灰度 RGB565，原地写回同一像素位置。预期 UART 输出为 `RGB565_UART_DUMP_BEGIN width=820 height=616 stride=1640 bytes=1010240`。

2026-07-06 直出条纹修正：逐点 RAW10 直出会把 Bayer CFA 的 R/G/B 响应差异直接显示成条纹/网格。当前保持 `820x616` 不缩放，但改为每个 2x2 RGGB block 求平均亮度并写回同一个 2x2 RGB565 灰度块。这样牺牲一点局部锐度，换掉明显 CFA 条纹；后续如果需要更锐，可改成 Bayer-aware luma 或真正 demosaic。

### 缩放方式

当前默认程序已经绕开 PIPE1，不再使用 DCMIPP PIPE1 downsize。缩放分成两级：

当前有效尺寸分两步变化：

```text
IMX219 RAW10 Bayer:           1640 x 1232
PIPE0 crop + line 1/2 decim:   820 x 616 RAW16
CPU 软件直出:                  820 x 616 RGB565
```

第一级是 PIPE0 硬件 crop + line decimation：横向 `HAL_DCMIPP_PIPE_SetBytesDecimationConfig(..., DCMIPP_BSM_ALL)`，不做 byte 抽取；纵向 `HAL_DCMIPP_PIPE_SetLinesDecimationConfig(..., DCMIPP_LSM_ALTERNATE_2)`，每 2 行保留 1 行。这一级纵向本质是抽行，不做均值滤波。

第二级在 `CameraPipeline_ConvertPipe0Raw16ToRgb565()` 中完成。当前版本不做软件缩放；每个 2x2 RGGB block 求平均亮度，按本帧 1%/99% 分位自动拉伸后写回同一个 2x2 RGB565 灰度块。这样可以消除逐点 RAW 直出带来的 Bayer/CFA 条纹。

核心代码在 `Appli/Core/Src/camera_pipeline.c`：

```c
raw = (raw00 + raw01 + raw10 + raw11 + 2U) / 4U;
```

按当前宏计算，比例关系是：

```text
X: 820 -> 820
Y: 616 -> 616
RAW 工作行字节: 820 * 2 = 1640
PIPE0 RAW 工作 pitch: 1640
RAW 工作帧大小: 1640 * 616 = 1010240
UART 输出帧大小: 1640 * 616 = 1010240
```

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

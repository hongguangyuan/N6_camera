# STM32N6 IMX219 DCMIPP RGB565 相机示例

这个分支是 STM32N657 + IMX219 MIPI CSI-2 相机调试工程。默认从 IMX219 采集
640x480 RAW10 图像，经 STM32N6 的 CSI 接收和 DCMIPP PIPE1 处理成 RGB565，
写入片内 RAM 帧缓冲区，然后用 OpenOCD/GDB 从 PC 端把缓冲区 dump 出来并转成
PNG 查看。

默认行为：

- 开发板：STM32N6570-DK 类目标板，使用 ST-LINK。
- 传感器：Sony IMX219，I2C 配置，MIPI CSI-2 两条 data lane。
- 输入格式：RAW10，640x480。
- 传感器采样：默认使用已验证的 IMX219 端 4x4 输出模式，中心 2560x1920 窗口输出为 640x480。
- DCMIPP 通路：PIPE1。
- 输出格式：RGB565 little-endian。
- 目标帧率：约 24 fps。
- 采集模式：continuous。
- dump 模式：固件采到 30 帧后停止采集并冻结帧缓冲，方便 PC 读取稳定图像。

工程保留两级启动链：

```text
FSBL -> Appli
```

## 目录结构

关键文件：

- `Appli/Core/Src/main.c`：应用入口，初始化 GPIO/UART/I2C，然后启动相机流程。
- `Appli/Core/Src/camera_debug.c`：IMX219、CSI、DCMIPP 配置和采集逻辑。
- `Appli/Core/Inc/camera_debug.h`：相机调试模块对外 API 和状态结构体。
- `Drivers/BSP/Components/imx219/imx219.c`：简化版 IMX219 I2C 驱动。
- `Appli/Core/Src/stm32n6xx_hal_msp.c`：DCMIPP/CSI 时钟、RIF、IRQ 配置。
- `Appli/Core/Src/stm32n6xx_it.c`：DCMIPP 和 CSI 中断入口。
- `Makefile/FSBL`：一级启动程序构建目录。
- `Makefile/Appli`：相机应用构建目录。
- `tools/flash_run.ps1`：把 Appli ELF 临时加载到片内 RAM 并运行。
- `tools/dump_dcmipp_rgb565_buffer.ps1`：从 RAM dump RGB565，并自动转 PNG。
- `tools/convert_rgb565_to_png.py`：PC 端 RGB565LE 转 PNG 脚本。

## 工具链

`.vscode/settings.json` 里默认使用这些本机路径：

- GNU Arm 工具链：`D:/arm/bin`
- Make：`D:/gunwin32/gnuwin32/bin/make.exe`
- STM32CubeProgrammer：`D:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin`
- STM32CubeIDE OpenOCD/GDB 插件：`D:/ST/STM32CubeIDE_2.1.0`
- Python + Pillow：用于把 RGB565 转 PNG

如果你的安装路径不同，改 `.vscode/settings.json`，或者运行 PowerShell 脚本时显式传入路径参数。

## 编译

PowerShell 中执行：

```powershell
Set-Location 'E:\VSCODE\n6\camera4'
$env:GCC_PATH='D:/arm/bin'
& 'D:\gunwin32\gnuwin32\bin\make.exe' -C Makefile/FSBL
& 'D:\gunwin32\gnuwin32\bin\make.exe' -C Makefile/Appli
```

主要产物：

- `Makefile/FSBL/build/fsbl_appli_led_usart_baseline_FSBL.elf`
- `Makefile/FSBL/build/fsbl_appli_led_usart_baseline_FSBL.bin`
- `Makefile/Appli/build/fsbl_appli_led_usart_baseline_Appli.elf`
- `Makefile/Appli/build/fsbl_appli_led_usart_baseline_Appli.bin`

注意：Appli 链接脚本把 1 MiB 帧缓冲区放在 `.noncacheable (NOLOAD)`，这样缓冲区位于 RAM 中，但不会被打包进加载镜像。

## 从 RAM 临时运行

开发调试时最快的方式是直接加载 Appli ELF 到片内 RAM：

```powershell
Set-Location 'E:\VSCODE\n6\camera4'
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\flash_run.ps1
```

运行后等待固件完成相机初始化、采集并冻结帧缓冲。然后 dump RGB565：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\dump_dcmipp_rgb565_buffer.ps1
```

脚本会打印类似：

```text
PNG=E:\VSCODE\n6\camera4\captures\dcmipp_pipe1_YYYYMMDD_HHMMSS_rgb565.png
```

打开最新 PNG：

```powershell
$latest = Get-ChildItem .\captures\*_rgb565.png | Sort-Object LastWriteTime -Descending | Select-Object -First 1
ii $latest.FullName
```

有效图像的 GDB 状态通常类似：

```text
DCMIPP_GDB_STATUS frozen=1 frames=30 stream=0 status=3 ...
```

如果看到 `frames=0 stream=0 status=1`，说明 dump 太早，程序还没开始采图。按 Reset 后多等几秒，再执行 dump。

## 烧录到外部 Flash

先给 FSBL 和 Appli 生成 trusted 镜像：

```powershell
& 'D:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_SigningTool_CLI.exe' `
  -bin '.\Makefile\FSBL\build\fsbl_appli_led_usart_baseline_FSBL.bin' `
  -nk -of 0x80000000 -t fsbl `
  -o '.\Makefile\FSBL\build\fsbl_appli_led_usart_baseline_FSBL-trusted.bin' `
  -hv 2.3 -dump '.\Makefile\FSBL\build\fsbl_appli_led_usart_baseline_FSBL-trusted.bin' -align -s

& 'D:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_SigningTool_CLI.exe' `
  -bin '.\Makefile\Appli\build\fsbl_appli_led_usart_baseline_Appli.bin' `
  -nk -of 0x80000000 -t fsbl `
  -o '.\Makefile\Appli\build\fsbl_appli_led_usart_baseline_Appli-trusted.bin' `
  -hv 2.3 -dump '.\Makefile\Appli\build\fsbl_appli_led_usart_baseline_Appli-trusted.bin' -align -s
```

再烧录到外部 Flash：

```powershell
& 'D:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe' `
  -c port=SWD mode=HOTPLUG freq=400 `
  -el 'D:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\ExternalLoader\MX66UW1G45G_STM32N6570-DK.stldr' `
  -w '.\Makefile\FSBL\build\fsbl_appli_led_usart_baseline_FSBL-trusted.bin' 0x70000000 -v `
  -w '.\Makefile\Appli\build\fsbl_appli_led_usart_baseline_Appli-trusted.bin' 0x70100000 -v `
  -rst
```

烧到外部 Flash 后，不要再运行 `tools/flash_run.ps1`。按板子 Reset，等固件采集并冻结一帧后执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\dump_dcmipp_rgb565_buffer.ps1
```

## 串口日志

USART3 参数：

```text
115200 8N1
```

可以用 VS Code Serial Monitor、PuTTY、MobaXterm 等串口工具查看日志。重点看这些信息：

- `CSI IMX219 bring-up start`
- `IMX219 ID OK`
- `DCMIPP capture armed`
- `CSI IMX219 bring-up PASS`
- `heartbeat=... frames=... rgb565=...`

## 图像传输链路

完整图像链路：

```text
IMX219 RAW10
  -> MIPI CSI-2，VC0，两条 data lane
  -> STM32N6 CSI 接收器
  -> DCMIPP PIPE1
  -> ISP 黑电平校正
  -> ISP 曝光/白平衡增益
  -> Raw Bayer 转 RGB
  -> Gamma 转换
  -> RGB565 像素打包
  -> dcmipp_frame_buffer 非缓存 RAM 帧缓冲
  -> OpenOCD/GDB dump
  -> tools/convert_rgb565_to_png.py
  -> PC 上的 PNG 图片
```

固件启动顺序：

1. `main()` 初始化 GPIO、USART3、I2C1 和系统隔离/RIF。
2. `CameraDebug_InitAndStart(&hi2c1)` 启动相机调试流程。
3. `MX_DCMIPP_Init()` 配置 CSI、PIPE1、ISP、frame counter 和中断。
4. `DCMIPP_StartCapture()` 调用 `HAL_DCMIPP_CSI_PIPE_Start()`，把 `dcmipp_frame_buffer` 作为输出地址。
5. `IMX219_Start()` 让传感器从 standby 进入 streaming。
6. DCMIPP 回调统计 frame、vsync、sof、eof 和错误状态。
7. 达到 `DCMIPP_VERIFY_FREEZE_AFTER_FRAMES` 后，`DCMIPP_FreezeCaptureForDump()` 停止 pipe 和 sensor。
8. PC 端 dump 脚本通过 ELF 符号名读取稳定帧缓冲。

默认图像参数在 `camera_debug.c` 中：

```c
#define IMX219_DEBUG_RESOLUTION IMX219_R640_480_BIN4
#define DCMIPP_VERIFY_WIDTH 640U
#define DCMIPP_VERIFY_HEIGHT 480U
#define DCMIPP_VERIFY_CAPTURE_PIPE DCMIPP_PIPE1
#define DCMIPP_VERIFY_CAPTURE_MODE DCMIPP_MODE_CONTINUOUS
#define DCMIPP_VERIFY_PIXEL_PACKER DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1
#define DCMIPP_VERIFY_FREEZE_AFTER_FRAMES 30U
```

默认帧率计算：

```text
line_length = 3560
frame_length = 1067
fps ~= 91.2 MHz / (3560 * 1067) = 24.009 fps
```

## IMX219 端 4x4 输出模式

本工程默认切到 `IMX219_R640_480_BIN4`。这个模式的目的不是在 DCMIPP 里缩放，
而是在 IMX219 输出前就减少像素量：

```text
IMX219 中心 2560x1920 窗口
  -> 传感器端已验证 4x4 采样/合并配置
  -> 输出 640x480 RAW10
  -> DCMIPP PIPE1 转 RGB565
```

相关代码在 `Drivers/BSP/Components/imx219/imx219.c`：

```c
{ IMX219_R640_480_BIN4, 640U, 480U, 1067U, 3560U, 4U, 0x01U, 0x02U },
```

字段含义：

- `640U, 480U`：传感器输出尺寸，后级 DCMIPP 仍按 640x480 接收。
- `1067U, 3560U`：frame length 和 line length，维持约 24 fps。
- `4U`：用 `width * 4` 和 `height * 4` 计算传感器裁剪窗口，即 2560x1920。
- `0x01U`：写入 `X_ODD_INC/Y_ODD_INC`，保持常规逐像素读出，避免额外跳采样造成画面比例异常。
- `0x02U`：写入 `BINNING_MODE_H/V`。

这组配置已经在当前工程中验证过，成功 dump 状态示例：

```text
DCMIPP_GDB_STATUS frozen=1 frames=33 stream=0 status=3 done_tick=1230 freeze_tick=1290
RGB565_REPEAT_MATCH=true
wrote ...rgb565.png (640x480)
```

其中：

- `frozen=1`：固件已经冻结帧缓冲，可以稳定读取。
- `frames=33`：DCMIPP 已收到有效帧。
- `stream=0`：冻结后 IMX219 已停流，属于预期状态。
- `RGB565_REPEAT_MATCH=true`：连续两次读取同一缓冲区 hash 一致，说明 dump 期间图像稳定。

回退到普通 640x480 中心裁剪模式，只需要把 `camera_debug.c` 改回：

```c
#define IMX219_DEBUG_RESOLUTION IMX219_R640_480
```

如果 4x4 模式没有图，优先看串口日志中的寄存器回读：

```text
IMX219 regs ... mode=... lane=... fmt=... line=... frame=...
```

也可以临时在 `Camera_DumpRegisters()` 中加读这些寄存器来确认：

```c
0x0170  X_ODD_INC
0x0171  Y_ODD_INC
0x0174  BINNING_MODE_H
0x0175  BINNING_MODE_V
```

注意：早期实验曾把 `X_ODD_INC/Y_ODD_INC` 设为 `0x07`，会让画面看起来被横向拉长或纵向压扁。
当前正确配置是 `OddInc=0x01`、`BinningMode=0x02`。如果画面比例异常，先确认已经重新编译、
重新加载或重新烧录了新的 Appli。

如果你的模组输出异常，先回退到 `IMX219_R640_480` 或 `IMX219_R320_240` 验证链路，再调
4x4 寄存器组合。

## 移植到其他工程

建议把这个工程当成一个已验证的相机 bring-up 模块，逐层移植、逐层验证，不要一开始就和复杂业务逻辑混在一起。

需要复制或参考的文件/逻辑：

- `Drivers/BSP/Components/imx219/imx219.h`
- `Drivers/BSP/Components/imx219/imx219.c`
- `Appli/Core/Inc/camera_debug.h`
- `Appli/Core/Src/camera_debug.c`
- `Appli/Core/Src/stm32n6xx_hal_msp.c` 中 DCMIPP/CSI 时钟、RIF、IRQ 配置部分
- `stm32n6xx_it.c` 中 `DCMIPP_IRQHandler()` 和 `CSI_IRQHandler()` 的 HAL 分发
- `Makefile/Appli/STM32N657XX_LRUN.ld` 中 `.noncacheable (NOLOAD)` 段

目标工程需要做这些事：

1. 启用 DCMIPP、I2C、UART、GPIO、RIF、RAMCFG 等 HAL 模块。
2. 配置 IMX219 模块电源使能 GPIO 和 I2C 总线。
3. 在 `HAL_DCMIPP_MspInit()` 中配置 DCMIPP/CSI 时钟。
4. 配置 RIF/RIMC，允许 DCMIPP 访问帧缓冲所在内存。
5. 使能 `DCMIPP_IRQn` 和 `CSI_IRQn`，并在中断函数里调用 HAL handler。
6. 把 `Drivers/BSP/Components/imx219` 和 `Appli/Core/Inc` 加到 include path。
7. 把 `camera_debug.c`、`imx219.c` 以及所需 HAL 源文件加到构建系统。
8. 大帧缓冲放到非缓存 RAM，或者保留正确的 cache clean/invalidate。
9. 在 `main()` 中完成 GPIO、UART、I2C、系统隔离/RIF 初始化后调用：

```c
CameraDebug_InitAndStart(&hi2c1);

while (1)
{
  CameraDebug_Task();
}
```

如果目标工程已有 RTOS 或自己的调度器，可以周期性调用 `CameraDebug_Task()`，或者把其中的日志循环替换为自己的 frame-ready 处理逻辑。

这个模块默认依赖的符号/外设：

- `hi2c1`，或者传给 `CameraDebug_InitAndStart()` 的其他 I2C handle。
- `EN_MODULE_GPIO_Port` 和 `EN_MODULE_Pin`，用于 IMX219 模块上电。
- `LED_GPIO_Port` / `LED_Pin`，或 `LED1_GPIO_Port` / `LED1_Pin`。
- `USART3` 仅用于 `printf` 调试，可以换成你自己的日志后端。
- DCMIPP 和 CSI 中断必须启用。

## 缩放、裁剪和输出尺寸

当前默认不缩放：

```c
#define DCMIPP_VERIFY_OUTPUT_WIDTH DCMIPP_VERIFY_WIDTH
#define DCMIPP_VERIFY_OUTPUT_HEIGHT DCMIPP_VERIFY_HEIGHT
#define DCMIPP_VERIFY_ENABLE_DOWNSIZE 0U
```

如果要用 PIPE1 硬件 downsize：

1. 把 `DCMIPP_VERIFY_ENABLE_DOWNSIZE` 改成 `1U`。
2. 把 `DCMIPP_VERIFY_OUTPUT_WIDTH` 和 `DCMIPP_VERIFY_OUTPUT_HEIGHT` 改成目标输出尺寸。
3. 把 RGB565 帧大小相关宏改成使用输出尺寸：

```c
#define DCMIPP_VERIFY_RGB565_LINE_BYTES (DCMIPP_VERIFY_OUTPUT_WIDTH * 2U)
#define DCMIPP_VERIFY_RGB565_FRAME_BYTES (DCMIPP_VERIFY_OUTPUT_WIDTH * DCMIPP_VERIFY_OUTPUT_HEIGHT * 2U)
#define DCMIPP_VERIFY_FRAME_BYTES DCMIPP_VERIFY_RGB565_FRAME_BYTES
```

4. 保持 `DCMIPP_VERIFY_LINE_PITCH` 等于输出行字节数。
5. PC dump 时同步修改字节数：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\dump_dcmipp_rgb565_buffer.ps1 -ByteCount <宽*高*2>
```

6. 如果不是默认 640x480，转换 PNG 时也要传入匹配尺寸：

```powershell
python .\tools\convert_rgb565_to_png.py <input.bin> <output.png> --width <宽> --height <高>
```

320x240 输出示例：

```c
#define DCMIPP_VERIFY_OUTPUT_WIDTH 320U
#define DCMIPP_VERIFY_OUTPUT_HEIGHT 240U
#define DCMIPP_VERIFY_ENABLE_DOWNSIZE 1U
#define DCMIPP_VERIFY_RGB565_LINE_BYTES (DCMIPP_VERIFY_OUTPUT_WIDTH * 2U)
#define DCMIPP_VERIFY_RGB565_FRAME_BYTES (DCMIPP_VERIFY_OUTPUT_WIDTH * DCMIPP_VERIFY_OUTPUT_HEIGHT * 2U)
```

dump 字节数：

```text
320 * 240 * 2 = 153600 bytes
```

运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\dump_dcmipp_rgb565_buffer.ps1 -ByteCount 153600
```

当前这些裁剪/抽取宏主要用于 PIPE0 RAW dump 实验：

```c
#define DCMIPP_VERIFY_PIPE0_CROP 0U
#define DCMIPP_VERIFY_PIPE0_DECIMATE 0U
```

这个分支的主路径是 PIPE1 RGB565，所以优先使用 PIPE1 downsize。若需要 RGB565 裁剪输出，可以仿照 PIPE0 crop 配置的位置增加 PIPE1 crop 配置，然后同步重新计算输出宽高、pitch、frame bytes、dump 字节数和 PNG 转换尺寸。

## 调试检查

OpenOCD 连不上时：

- 检查 USB/ST-LINK 连接。
- 检查板子供电和 target voltage。
- 关闭其他 OpenOCD、CubeIDE debug、CubeProgrammer 会话。
- 尝试降低 SWD 频率。

dump 成功但 PNG 不对时：

- 先看脚本打印的 `DCMIPP_GDB_STATUS`。
- `frames=0` 表示还没有采到帧，Reset 后多等几秒再 dump。
- `frozen=1 frames=30` 是预期的冻结快照状态。
- 看串口日志里 IMX219 ID、CSI、DCMIPP、pipe error 是否异常。
- 确认 `ByteCount`、输出宽度、输出高度和固件配置一致。

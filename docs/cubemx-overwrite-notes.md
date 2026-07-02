# CubeMX 再生成注意事项

这个工程由 STM32CubeMX / STM32 Project Generator 生成后手工加入了 IMX219 摄像头链路。再次从 `fsbl_appli_led_usart_baseline.ioc` 生成代码后，请重点检查下面内容。

## 通常会保留的代码

CubeMX 一般会保留 `USER CODE BEGIN ...` 和 `USER CODE END ...` 之间的代码。

当前重要用户代码：

- `FSBL/Core/Src/main.c`
  - 包含 `extmem.h`。
  - 调用 `MX_EXTMEM_Init()`。
  - 调用 `BOOT_Application()` 跳转到 Appli。
- `FSBL/Core/Src/extmem.c`
  - 定义 SigningTool header 常量。
  - 实现 `MX_EXTMEM_Init()`。
  - 实现 `BOOT_GetApplicationSize()`。
- `Appli/Core/Src/main.c`
  - 初始化 GPIO、USART3、I2C1。
  - 调用 `CameraDriver_InitAndStart(&hi2c1)`。
  - 在主循环调用 `CameraDriver_Task()`。
  - 在 `Error_Handler()` 中调用 `CameraDriver_PrintErrorContext()`。

## 高风险文件

这些文件可能被 CubeMX 覆盖、还原或漏掉手工新增项。

- `Makefile/Appli/Makefile`
  - 必须包含：
    - `../../Appli/Core/Src/camera_driver.c`
    - `../../Appli/Core/Src/camera_pipeline.c`
    - `../../Drivers/BSP/Components/imx219/imx219.c`
  - 必须包含路径：
    - `-I../../Drivers/BSP/Components/imx219`
  - 如果启用 ST ISP 中间件，确认 STM32Cube N6 ISP Library 的 `Inc/Lib/Src` 路径仍然正确。
- `Makefile/FSBL/Makefile`
  - EXTMEM 路径应指向工程内的：
    - `../../Middlewares/ST/STM32_ExtMem_Manager`
- `Appli/Core/Src/main.c`
  - CubeMX 可能恢复成只初始化外设的模板，需要重新确认 `camera_driver.h` 和驱动调用。
- `Appli/Core/Inc/main.h`
  - 需要保留 `EN_MODULE_GPIO_Port` / `EN_MODULE_Pin`，IMX219 上电会用到。
- `.vscode/*.json`
  - 本机工具链、OpenOCD、GDB、make 路径可能是手工配置。

## 再生成后的检查

先看差异：

```powershell
git diff
```

再编译：

```powershell
D:/gunwin32/gnuwin32/bin/make.exe -C Makefile/FSBL
D:/gunwin32/gnuwin32/bin/make.exe -C Makefile/Appli
```

如果 Appli 编译失败，优先检查：

- `camera_driver.c`、`camera_pipeline.c`、`imx219.c` 是否仍在 Appli 的 `C_SOURCES`。
- `Drivers/BSP/Components/imx219` 是否仍在 `C_INCLUDES`。
- `main.c` 是否包含 `camera_driver.h`。
- `stm32n6xx_hal_conf.h` 是否启用了 DCMIPP、I2C、UART、GPIO、RIF、GPDMA 等 HAL 模块。

## 运行前检查

- 帧缓冲地址：`CAMERA_PIPELINE_BUFFER_ADDRESS = 0x34082000`。
- 帧缓冲大小：默认 `640x480 RGB565 = 614400` 字节。
- RISAF/GPDMA 安全属性必须允许 DCMIPP 写帧缓冲。
- USART3 需要可用，因为调试日志和 RGB565 转储都走 `printf`/`__io_putchar`。

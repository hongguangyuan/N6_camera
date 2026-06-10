param(
  [string]$OutDir = "captures",
  [string]$Elf = "Makefile/Appli/build/fsbl_appli_led_usart_baseline_Appli.elf",
  [string]$OpenOcd = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.openocd.win32_2.4.400.202601091506/tools/bin/openocd.exe",
  [string]$Gdb = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin/arm-none-eabi-gdb.exe",
  [string]$OpenOcdScripts = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.debug.openocd_2.3.300.202602021527/resources/openocd/st_scripts",
  [UInt32]$ByteCount = 614400,
  [bool]$RepeatDumpCheck = $true
)

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
$out = Join-Path $repo $OutDir
New-Item -ItemType Directory -Force -Path $out | Out-Null

$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$rgb565 = Join-Path $out "dcmipp_pipe1_${stamp}_rgb565le.bin"
$rgb565Repeat = Join-Path $out "dcmipp_pipe1_${stamp}_rgb565le_repeat.bin"
$png = Join-Path $out "dcmipp_pipe1_${stamp}_rgb565.png"
$log = Join-Path $out "openocd_${stamp}.log"
$err = Join-Path $out "openocd_${stamp}.err.log"
$gdbCmd = Join-Path $out "dump_rgb565_${stamp}.gdb"

$gdbLines = @(
  "set confirm off",
  "set pagination off",
  "target extended-remote localhost:3333",
  "monitor halt",
  ("dump binary memory {0} &dcmipp_frame_buffer ((char *)&dcmipp_frame_buffer)+{1}" -f ($rgb565 -replace "\\", "/"), $ByteCount)
)
if ($RepeatDumpCheck) {
  $gdbLines += ("dump binary memory {0} &dcmipp_frame_buffer ((char *)&dcmipp_frame_buffer)+{1}" -f ($rgb565Repeat -replace "\\", "/"), $ByteCount)
}
$gdbLines += @(
  "monitor resume",
  "detach",
  "quit"
)
$gdbLines | Set-Content -Encoding ASCII -Path $gdbCmd

$openOcdArgs = @(
  "-s", $OpenOcdScripts,
  "-f", "interface/stlink-dap.cfg",
  "-f", "target/stm32n6x.cfg"
)

$p = Start-Process -FilePath $OpenOcd `
                   -ArgumentList $openOcdArgs `
                   -PassThru `
                   -WindowStyle Hidden `
                   -RedirectStandardOutput $log `
                   -RedirectStandardError $err

try {
  Start-Sleep -Seconds 3
  & $Gdb -q $Elf -x $gdbCmd
  if ($LASTEXITCODE -ne 0) {
    throw "gdb failed with exit code $LASTEXITCODE"
  }
}
finally {
  $openOcdProcess = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
  if ($openOcdProcess) {
    Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
  }
}

Write-Host "RGB565_DUMP=$rgb565"
Write-Host "BYTES=$ByteCount"
if ($RepeatDumpCheck) {
  $hash0 = (Get-FileHash -Algorithm SHA256 -Path $rgb565).Hash
  $hash1 = (Get-FileHash -Algorithm SHA256 -Path $rgb565Repeat).Hash
  Write-Host "RGB565_REPEAT=$rgb565Repeat"
  Write-Host "RGB565_SHA256=$hash0"
  Write-Host "RGB565_REPEAT_SHA256=$hash1"
  Write-Host ("RGB565_REPEAT_MATCH={0}" -f (($hash0 -eq $hash1).ToString().ToLowerInvariant()))
}

python (Join-Path $repo "tools/convert_rgb565_to_png.py") $rgb565 $png
Write-Host "PNG=$png"

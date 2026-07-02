param(
  [string]$OutDir = "captures",
  [string]$Elf = "Makefile/Appli/build/fsbl_appli_led_usart_baseline_Appli.elf",
  [string]$OpenOcd = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.openocd.win32_2.4.400.202601091506/tools/bin/openocd.exe",
  [string]$Gdb = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin/arm-none-eabi-gdb.exe",
  [string]$OpenOcdScripts = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.debug.openocd_2.3.300.202602021527/resources/openocd/st_scripts",
  [UInt32]$Width = 640,
  [UInt32]$Height = 480,
  [UInt32]$Stride = 0,
  [UInt32]$ByteCount = 0,
  [bool]$RepeatDumpCheck = $true
)

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
$out = Join-Path $repo $OutDir
New-Item -ItemType Directory -Force -Path $out | Out-Null

if ($ByteCount -eq 0) {
  if ($Stride -eq 0) {
    $Stride = [UInt32]($Width * 2)
  }
  $ByteCount = $Stride * $Height
}
elseif ($Stride -eq 0) {
  $Stride = [UInt32]($Width * 2)
}

$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$raw16 = Join-Path $out "dcmipp_pipe0_${stamp}_raw16le.bin"
$raw16Repeat = Join-Path $out "dcmipp_pipe0_${stamp}_raw16le_repeat.bin"
$log = Join-Path $out "openocd_${stamp}.log"
$err = Join-Path $out "openocd_${stamp}.err.log"
$gdbCmd = Join-Path $out "dump_raw16_${stamp}.gdb"

$gdbLines = @(
  "set confirm off",
  "set pagination off",
  "target extended-remote localhost:3333",
  "monitor halt",
  "printf `"DCMIPP_GDB_STATUS frozen=%u frames=%u stream=%u status=%u done_tick=%u freeze_tick=%u\n`", dcmipp_capture_frozen, dcmipp_frame_count, imx219_streaming, dcmipp_status, dcmipp_frame_done_tick, dcmipp_freeze_tick",
  "printf `"DCMIPP_GDB_PIPE0 p0dcc=%u expect=%u limit_irq=%u limit_tick=%u limit_dcc=%u p0scstr=0x%08X p0scszr=0x%08X p0cscstr=0x%08X p0cscszr=0x%08X p0m0ar1=0x%08X p0stm0ar=0x%08X p0sr=0x%08X\n`", dcmipp_p0_dccntr, 614400, dcmipp_p0_limit_irq_count, dcmipp_p0_limit_tick, dcmipp_p0_limit_dccntr, dcmipp_p0_scstr, dcmipp_p0_scszr, dcmipp_p0_cscstr, dcmipp_p0_cscszr, dcmipp_p0_ppm0ar1, dcmipp_p0_stm0ar, dcmipp_p0_sr",
  ("dump binary memory {0} &dcmipp_frame_buffer ((char *)&dcmipp_frame_buffer)+{1}" -f ($raw16 -replace "\\", "/"), $ByteCount)
)
if ($RepeatDumpCheck) {
  $gdbLines += ("dump binary memory {0} &dcmipp_frame_buffer ((char *)&dcmipp_frame_buffer)+{1}" -f ($raw16Repeat -replace "\\", "/"), $ByteCount)
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

Write-Host "RAW16_DUMP=$raw16"
Write-Host "BYTES=$ByteCount"
Write-Host "STRIDE=$Stride"
if ($RepeatDumpCheck) {
  $hash0 = (Get-FileHash -Algorithm SHA256 -Path $raw16).Hash
  $hash1 = (Get-FileHash -Algorithm SHA256 -Path $raw16Repeat).Hash
  Write-Host "RAW16_REPEAT=$raw16Repeat"
  Write-Host "RAW16_SHA256=$hash0"
  Write-Host "RAW16_REPEAT_SHA256=$hash1"
  Write-Host ("RAW16_REPEAT_MATCH={0}" -f (($hash0 -eq $hash1).ToString().ToLowerInvariant()))
}

python (Join-Path $repo "tools/convert_dcmipp_raw16_to_pgm.py") $raw16 --width $Width --height $Height --stride-bytes $Stride
Write-Host "NEXT=python tools/convert_raw10_to_rgb.py `"$raw16`" --bayer RGGB"

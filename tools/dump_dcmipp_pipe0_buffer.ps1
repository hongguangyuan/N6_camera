param(
  [string]$OutDir = "captures",
  [string]$Elf = "Makefile/Appli/build/fsbl_appli_led_usart_baseline_Appli.elf",
  [string]$OpenOcd = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.openocd.win32_2.4.400.202601091506/tools/bin/openocd.exe",
  [string]$Gdb = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin/arm-none-eabi-gdb.exe",
  [string]$OpenOcdScripts = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.debug.openocd_2.3.300.202602021527/resources/openocd/st_scripts",
  [UInt32]$BufferAddress = 0x34080080,
  [UInt32]$ByteCount = 614400,
  [bool]$RepeatDumpCheck = $true
)

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
$out = Join-Path $repo $OutDir
New-Item -ItemType Directory -Force -Path $out | Out-Null

$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$raw16 = Join-Path $out "dcmipp_pipe0_${stamp}_raw16le.bin"
$raw16Repeat = Join-Path $out "dcmipp_pipe0_${stamp}_raw16le_repeat.bin"
$log = Join-Path $out "openocd_${stamp}.log"
$err = Join-Path $out "openocd_${stamp}.err.log"
$gdbCmd = Join-Path $out "dump_${stamp}.gdb"
$end = $BufferAddress + $ByteCount

$gdbLines = @(
  "set confirm off",
  "set pagination off",
  "target extended-remote localhost:3333",
  "monitor halt",
  ("dump binary memory {0} 0x{1:X8} 0x{2:X8}" -f ($raw16 -replace "\\", "/"), $BufferAddress, $end)
)
if ($RepeatDumpCheck) {
  $gdbLines += ("dump binary memory {0} 0x{1:X8} 0x{2:X8}" -f ($raw16Repeat -replace "\\", "/"), $BufferAddress, $end)
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
  if (Get-Process -Id $p.Id -ErrorAction SilentlyContinue) {
    Stop-Process -Id $p.Id -Force
  }
}

Write-Host "RAW16_DUMP=$raw16"
Write-Host "BYTES=$ByteCount"
if ($RepeatDumpCheck) {
  $hash0 = (Get-FileHash -Algorithm SHA256 -Path $raw16).Hash
  $hash1 = (Get-FileHash -Algorithm SHA256 -Path $raw16Repeat).Hash
  Write-Host "RAW16_REPEAT=$raw16Repeat"
  Write-Host "RAW16_SHA256=$hash0"
  Write-Host "RAW16_REPEAT_SHA256=$hash1"
  Write-Host ("RAW16_REPEAT_MATCH={0}" -f (($hash0 -eq $hash1).ToString().ToLowerInvariant()))
}
Write-Host "NEXT=python tools/convert_dcmipp_raw16_to_pgm.py `"$raw16`""

param(
  [string]$OutDir = "captures",
  [string]$Elf = "Makefile/Appli/build/fsbl_appli_led_usart_baseline_Appli.elf",
  [string]$OpenOcd = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.openocd.win32_2.4.400.202601091506/tools/bin/openocd.exe",
  [string]$Gdb = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin/arm-none-eabi-gdb.exe",
  [string]$Nm = "arm-none-eabi-nm",
  [string]$OpenOcdScripts = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.debug.openocd_2.3.300.202602021527/resources/openocd/st_scripts",
  [UInt32]$VectorTable = 0x34000400,
  [UInt32]$BufferAddress = 0,
  [UInt32]$ByteCount = 614400,
  [UInt32]$RunSeconds = 8,
  [bool]$RepeatDumpCheck = $true
)

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
$out = Join-Path $repo $OutDir
New-Item -ItemType Directory -Force -Path $out | Out-Null

if ($BufferAddress -eq 0) {
  $nmOutput = & $Nm -n $Elf
  if ($LASTEXITCODE -ne 0) {
    throw "nm failed with exit code $LASTEXITCODE"
  }

  $symbolAddress = $null
  foreach ($line in $nmOutput) {
    if ($line -match "^\s*([0-9a-fA-F]+)\s+\w\s+dcmipp_frame_buffer\s*$") {
      $symbolAddress = $Matches[1]
      break
    }
  }
  if (-not $symbolAddress) {
    throw "could not find dcmipp_frame_buffer in ELF symbols"
  }

  $BufferAddress = [Convert]::ToUInt32($symbolAddress, 16)
}

$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$raw16 = Join-Path $out "dcmipp_pipe0_${stamp}_raw16le.bin"
$raw16Repeat = Join-Path $out "dcmipp_pipe0_${stamp}_raw16le_repeat.bin"
$state = Join-Path $out "dcmipp_pipe0_${stamp}_state.txt"
$log = Join-Path $out "openocd_flash_dump_${stamp}.log"
$err = Join-Path $out "openocd_flash_dump_${stamp}.err.log"
$gdbCmd = Join-Path $out "flash_run_dump_${stamp}.gdb"
$end = $BufferAddress + $ByteCount

$gdbLines = @(
  "set confirm off",
  "set pagination off",
  "target extended-remote localhost:3333",
  "monitor reset halt",
  "load",
  ("monitor mww 0xE000ED08 0x{0:X8}" -f $VectorTable),
  ("set `$sp = *(unsigned int*)0x{0:X8}" -f $VectorTable),
  ("set `$pc = *(unsigned int*)0x{0:X8}" -f ($VectorTable + 4)),
  "monitor resume",
  ("shell powershell -NoProfile -Command Start-Sleep -Seconds {0}" -f $RunSeconds),
  "monitor halt",
  ("dump binary memory {0} 0x{1:X8} 0x{2:X8}" -f ($raw16 -replace "\\", "/"), $BufferAddress, $end)
)
if ($RepeatDumpCheck) {
  $gdbLines += ("dump binary memory {0} 0x{1:X8} 0x{2:X8}" -f ($raw16Repeat -replace "\\", "/"), $BufferAddress, $end)
}
$gdbLines += @(
  ("set logging file {0}" -f ($state -replace "\\", "/")),
  "set logging overwrite on",
  "set logging enabled on",
  "p/x dcmipp_capture_frozen",
  "p/x dcmipp_frame_done_tick",
  "p/x dcmipp_freeze_tick",
  "p/x dcmipp_frame_count",
  "p/x dcmipp_p0_dccntr",
  "p/x dcmipp_p0_fscr",
  "p/x dcmipp_p0_fctcr",
  "p/x dcmipp_p0_sr",
  "p/x dcmipp_raw16_high6_nonzero",
  "p/x dcmipp_raw16_max",
  "p/x dcmipp_raw16_sat10_count",
  "p/x dcmipp_csi_err1",
  "p/x dcmipp_csi_err2",
  "p/x dcmipp_ipgr1",
  "p/x dcmipp_ipc1r1",
  "p/x dcmipp_ipc1r2",
  "p/x dcmipp_ipc1r3",
  "set logging enabled off"
)
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
  Write-Host "FLASH_RUN_DUMP_GDB=$gdbCmd"
  & $Gdb -q $Elf -x $gdbCmd
  if ($LASTEXITCODE -ne 0) {
    throw "gdb flash/run/dump failed with exit code $LASTEXITCODE"
  }
}
finally {
  if (Get-Process -Id $p.Id -ErrorAction SilentlyContinue) {
    Stop-Process -Id $p.Id -Force
  }
}

Write-Host "RAW16_DUMP=$raw16"
Write-Host ("BUFFER_ADDRESS=0x{0:X8}" -f $BufferAddress)
Write-Host "BYTES=$ByteCount"
Write-Host "STATE=$state"
if ($RepeatDumpCheck) {
  $hash0 = (Get-FileHash -Algorithm SHA256 -Path $raw16).Hash
  $hash1 = (Get-FileHash -Algorithm SHA256 -Path $raw16Repeat).Hash
  Write-Host "RAW16_REPEAT=$raw16Repeat"
  Write-Host "RAW16_SHA256=$hash0"
  Write-Host "RAW16_REPEAT_SHA256=$hash1"
  Write-Host ("RAW16_REPEAT_MATCH={0}" -f (($hash0 -eq $hash1).ToString().ToLowerInvariant()))
}
Write-Host "NEXT=python tools/convert_dcmipp_raw16_to_pgm.py `"$raw16`" --mode right"
Write-Host "NEXT_RGB=python tools/convert_raw10_to_rgb.py `"$raw16`" --format raw16le --raw16-mode right --bayer RGGB"

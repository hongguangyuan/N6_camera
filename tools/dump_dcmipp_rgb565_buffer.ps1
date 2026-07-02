param(
  [string]$OutDir = "captures",
  [string]$Elf = "Makefile/Appli/build/fsbl_appli_led_usart_baseline_Appli.elf",
  [string]$OpenOcd = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.openocd.win32_2.4.400.202601091506/tools/bin/openocd.exe",
  [string]$Gdb = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin/arm-none-eabi-gdb.exe",
  [string]$Programmer = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.400.202601091506/tools/bin/STM32_Programmer_CLI.exe",
  [string]$OpenOcdScripts = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.debug.openocd_2.3.300.202602021527/resources/openocd/st_scripts",
  [UInt32]$Width = 640,
  [UInt32]$Height = 240,
  [UInt32]$Stride = 0,
  [UInt32]$ByteCount = 0,
  [ValidateSet("rgb565", "gray8", "raw16le", "raw10p")]
  [string]$Format = "rgb565",
  [bool]$RepeatDumpCheck = $true,
  [switch]$UseMonitorHalt,
  [ValidateSet("programmer", "memap", "gdb")]
  [string]$Method = "programmer",
  [UInt32]$ProgrammerAp = 0,
  [UInt32]$BufferAddress = 0x34082000,
  [UInt32]$OpenOcdSpeedKhz = 1000
)

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
$out = Join-Path $repo $OutDir
New-Item -ItemType Directory -Force -Path $out | Out-Null

if ($ByteCount -eq 0) {
  if ($Stride -eq 0) {
    $bytesPerPixel = if ($Format -eq "gray8") { 1 } else { 2 }
    $Stride = [UInt32]([Math]::Ceiling(($Width * $bytesPerPixel) / 16.0) * 16)
  }
  $ByteCount = $Stride * $Height
}
elseif ($Stride -eq 0) {
  $bytesPerPixel = if ($Format -eq "gray8") { 1 } else { 2 }
  $Stride = [UInt32]([Math]::Ceiling(($Width * $bytesPerPixel) / 16.0) * 16)
}

$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$dump = Join-Path $out "dcmipp_pipe1_${stamp}_${Format}.bin"
$dumpRepeat = Join-Path $out "dcmipp_pipe1_${stamp}_${Format}_repeat.bin"
$png = Join-Path $out "dcmipp_pipe1_${stamp}_${Format}.png"
$log = Join-Path $out "openocd_${stamp}.log"
$err = Join-Path $out "openocd_${stamp}.err.log"
$programmerLog = Join-Path $out "programmer_${stamp}.log"
$programmerErr = Join-Path $out "programmer_${stamp}.err.log"
$gdbCmd = Join-Path $out "dump_rgb565_${stamp}.gdb"
$gdbRescueCmd = Join-Path $out "resume_target_${stamp}.gdb"
$openOcdCmd = Join-Path $out "dump_rgb565_${stamp}.openocd.cfg"
$openOcdSpeedCmd = Join-Path $out "openocd_speed_${stamp}.cfg"

$gdbLines = @(
  "set confirm off",
  "set pagination off",
  "set remotetimeout 10",
  "target extended-remote localhost:3333",
  "set scheduler-locking off",
  "set mem inaccessible-by-default off",
  "printf `"DCMIPP_GDB_CONNECTED pc=0x%08X use_monitor_halt=%u\n`", `$pc, $([UInt32]$UseMonitorHalt.IsPresent)"
)
if ($UseMonitorHalt.IsPresent) {
  $gdbLines += @(
    "monitor halt"
  )
}
$gdbLines += @(
  ("dump binary memory {0} 0x{1:X8} 0x{2:X8}" -f ($dump -replace "\\", "/"), $BufferAddress, ($BufferAddress + $ByteCount))
)
if ($RepeatDumpCheck) {
  $gdbLines += ("dump binary memory {0} 0x{1:X8} 0x{2:X8}" -f ($dumpRepeat -replace "\\", "/"), $BufferAddress, ($BufferAddress + $ByteCount))
}
$gdbLines += @(
  "detach",
  "quit"
)
if ($UseMonitorHalt.IsPresent) {
  $gdbLines = $gdbLines[0..($gdbLines.Count - 3)] + @("monitor resume") + $gdbLines[($gdbLines.Count - 2)..($gdbLines.Count - 1)]
}
$gdbLines | Set-Content -Encoding ASCII -Path $gdbCmd

$gdbRescueLines = @(
  "set confirm off",
  "set pagination off",
  "set remotetimeout 10",
  "target extended-remote localhost:3333",
  "detach",
  "quit"
)
if ($UseMonitorHalt) {
  $gdbRescueLines = $gdbRescueLines[0..3] + @("monitor resume") + $gdbRescueLines[4..5]
}
$gdbRescueLines | Set-Content -Encoding ASCII -Path $gdbRescueCmd

"adapter speed $OpenOcdSpeedKhz" | Set-Content -Encoding ASCII -Path $openOcdSpeedCmd

$openOcdArgs = @(
  "-s", $OpenOcdScripts,
  "-f", "interface/stlink-dap.cfg",
  "-f", "target/stm32n6x.cfg",
  "-f", $openOcdSpeedCmd
)

if ($Method -eq "programmer") {
  $connectArgs = @("-c", "port=SWD", "mode=HOTPLUG", "ap=$ProgrammerAp")
  $uploadArgs = $connectArgs + @("-u", ("0x{0:X8}" -f $BufferAddress), "$ByteCount", $dump)
  $programmerProcess = Start-Process -FilePath $Programmer `
                                     -ArgumentList $uploadArgs `
                                     -Wait `
                                     -PassThru `
                                     -WindowStyle Hidden `
                                     -RedirectStandardOutput $programmerLog `
                                     -RedirectStandardError $programmerErr
  if ($programmerProcess.ExitCode -ne 0) {
    throw "STM32_Programmer_CLI upload failed with exit code $($programmerProcess.ExitCode); see $programmerLog and $programmerErr"
  }
  if ($RepeatDumpCheck) {
    $repeatArgs = $connectArgs + @("-u", ("0x{0:X8}" -f $BufferAddress), "$ByteCount", $dumpRepeat)
    $programmerRepeatProcess = Start-Process -FilePath $Programmer `
                                             -ArgumentList $repeatArgs `
                                             -Wait `
                                             -PassThru `
                                             -WindowStyle Hidden `
                                             -RedirectStandardOutput $programmerLog `
                                             -RedirectStandardError $programmerErr
    if ($programmerRepeatProcess.ExitCode -ne 0) {
      throw "STM32_Programmer_CLI repeat upload failed with exit code $($programmerRepeatProcess.ExitCode); see $programmerLog and $programmerErr"
    }
  }
  if (-not (Test-Path -LiteralPath $dump)) {
    throw "STM32_Programmer_CLI did not create $dump"
  }
  if ($RepeatDumpCheck -and -not (Test-Path -LiteralPath $dumpRepeat)) {
    throw "STM32_Programmer_CLI did not create $dumpRepeat"
  }
}
elseif ($Method -eq "memap") {
  $openOcdLines = @(
    "init",
    "targets stm32n6.ap0",
    "halt",
    ("dump_image {0} 0x{1:X8} {2}" -f ($dump -replace "\\", "/"), $BufferAddress, $ByteCount)
  )
  if ($RepeatDumpCheck) {
    $openOcdLines += ("dump_image {0} 0x{1:X8} {2}" -f ($dumpRepeat -replace "\\", "/"), $BufferAddress, $ByteCount)
  }
  $openOcdLines += @(
    "shutdown"
  )
  $openOcdLines | Set-Content -Encoding ASCII -Path $openOcdCmd

  $openOcdMemApArgs = $openOcdArgs + @("-f", $openOcdCmd)
  $memApProcess = Start-Process -FilePath $OpenOcd `
                                -ArgumentList $openOcdMemApArgs `
                                -Wait `
                                -PassThru `
                                -WindowStyle Hidden `
                                -RedirectStandardOutput $log `
                                -RedirectStandardError $err
  if ($memApProcess.ExitCode -ne 0) {
    throw "openocd mem_ap dump failed with exit code $($memApProcess.ExitCode); see $err"
  }
  if (-not (Test-Path -LiteralPath $dump)) {
    throw "openocd did not create $dump"
  }
  if ($RepeatDumpCheck -and -not (Test-Path -LiteralPath $dumpRepeat)) {
    throw "openocd did not create $dumpRepeat"
  }
}
else {
  $p = Start-Process -FilePath $OpenOcd `
                     -ArgumentList $openOcdArgs `
                     -PassThru `
                     -WindowStyle Hidden `
                     -RedirectStandardOutput $log `
                     -RedirectStandardError $err

  $gdbOk = $false
  try {
    $portReady = $false
    for ($i = 0; $i -lt 30; $i++) {
      Start-Sleep -Milliseconds 250
      $conn = Test-NetConnection -ComputerName 127.0.0.1 -Port 3333 -InformationLevel Quiet -WarningAction SilentlyContinue
      if ($conn) {
        $portReady = $true
        break
      }
      if ($p.HasExited) {
        break
      }
    }
    if (-not $portReady) {
      throw "OpenOCD gdb server did not become ready on port 3333; see $err"
    }
    & $Gdb -q $Elf -x $gdbCmd
    if ($LASTEXITCODE -ne 0) {
      throw "gdb failed with exit code $LASTEXITCODE"
    }
    if (-not (Test-Path -LiteralPath $dump)) {
      throw "gdb did not create $dump"
    }
    if ($RepeatDumpCheck -and -not (Test-Path -LiteralPath $dumpRepeat)) {
      throw "gdb did not create $dumpRepeat"
    }
    $gdbOk = $true
  }
  finally {
    if (-not $gdbOk) {
      try {
        & $Gdb -q $Elf -x $gdbRescueCmd *> $null
      }
      catch {
      }
    }
    $openOcdProcess = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
    if ($openOcdProcess) {
      Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
    }
  }
}

Write-Host "$($Format.ToUpperInvariant())_DUMP=$dump"
Write-Host "BYTES=$ByteCount"
Write-Host "STRIDE=$Stride"
if ($RepeatDumpCheck) {
  $hash0 = (Get-FileHash -Algorithm SHA256 -Path $dump).Hash
  $hash1 = (Get-FileHash -Algorithm SHA256 -Path $dumpRepeat).Hash
  Write-Host "$($Format.ToUpperInvariant())_REPEAT=$dumpRepeat"
  Write-Host "$($Format.ToUpperInvariant())_SHA256=$hash0"
  Write-Host "$($Format.ToUpperInvariant())_REPEAT_SHA256=$hash1"
  Write-Host ("$($Format.ToUpperInvariant())_REPEAT_MATCH={0}" -f (($hash0 -eq $hash1).ToString().ToLowerInvariant()))
}

python (Join-Path $repo "tools/convert_rgb565_to_png.py") $dump $png --width $Width --height $Height --stride $Stride --format $Format
Write-Host "PNG=$png"

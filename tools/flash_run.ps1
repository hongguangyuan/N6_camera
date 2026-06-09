param(
  [string]$Elf = "Makefile/Appli/build/fsbl_appli_led_usart_baseline_Appli.elf",
  [string]$OpenOcd = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.openocd.win32_2.4.400.202601091506/tools/bin/openocd.exe",
  [string]$Gdb = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin/arm-none-eabi-gdb.exe",
  [string]$OpenOcdScripts = "D:/ST/STM32CubeIDE_2.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.debug.openocd_2.3.300.202602021527/resources/openocd/st_scripts",
  [UInt32]$VectorTable = 0x34000400,
  [switch]$HaltAtMain
)

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
if (-not [System.IO.Path]::IsPathRooted($Elf)) {
  $Elf = Join-Path $repo $Elf
}
$Elf = (Resolve-Path $Elf).Path

$out = Join-Path $repo "captures"
New-Item -ItemType Directory -Force -Path $out | Out-Null

$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$log = Join-Path $out "openocd_flash_run_${stamp}.log"
$err = Join-Path $out "openocd_flash_run_${stamp}.err.log"
$gdbCmd = Join-Path $out "flash_run_${stamp}.gdb"

$gdbLines = @(
  "set confirm off",
  "set pagination off",
  "target extended-remote localhost:3333",
  "monitor reset halt",
  "load",
  ("monitor mww 0xE000ED08 0x{0:X8}" -f $VectorTable),
  ("set `$sp = *(unsigned int*)0x{0:X8}" -f $VectorTable),
  ("set `$pc = *(unsigned int*)0x{0:X8}" -f ($VectorTable + 4))
)

if ($HaltAtMain) {
  $gdbLines += @(
    "tbreak main",
    "continue"
  )
} else {
  $gdbLines += "monitor resume"
}

$gdbLines += @(
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
  if ($p.HasExited) {
    $openOcdErr = ""
    if (Test-Path $err) {
      $openOcdErr = (Get-Content $err -Tail 80) -join [Environment]::NewLine
    }
    throw "openocd exited before gdb could connect. stderr tail:$([Environment]::NewLine)$openOcdErr"
  }

  Write-Host "FLASH_RUN_GDB=$gdbCmd"
  $gdbOutput = & $Gdb -q $Elf -x $gdbCmd 2>&1
  $gdbExit = $LASTEXITCODE
  $gdbOutput | ForEach-Object { Write-Host $_ }
  $gdbText = ($gdbOutput | Out-String)
  if (($gdbExit -ne 0) -or ($gdbText -match "(?i)(error in sourced command file|could not connect|connection timed out|remote communication error|no connection|failed)")) {
    if ($gdbExit -eq 0) {
      $global:LASTEXITCODE = 1
    }
    throw "gdb flash/run failed with exit code $LASTEXITCODE"
  }
}
finally {
  if (Get-Process -Id $p.Id -ErrorAction SilentlyContinue) {
    Stop-Process -Id $p.Id -Force
  }
}

Write-Host "FLASH_RUN_ELF=$Elf"
Write-Host ("VECTOR_TABLE=0x{0:X8}" -f $VectorTable)
Write-Host "OPENOCD_LOG=$log"
Write-Host "OPENOCD_ERR=$err"

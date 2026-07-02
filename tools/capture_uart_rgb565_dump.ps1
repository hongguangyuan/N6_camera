param(
  [Parameter(Mandatory = $true)]
  [string]$Port,
  [UInt32]$BaudRate = 115200,
  [string]$OutDir = "captures",
  [UInt32]$ReadTimeoutMs = 10000,
  [UInt32]$OverallTimeoutSec = 420
)

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
$out = Join-Path $repo $OutDir
New-Item -ItemType Directory -Force -Path $out | Out-Null

$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$rawLog = Join-Path $out "uart_frame_${stamp}.log"
$bin = $null
$png = $null

function Convert-HexToUInt32([string]$Text) {
  $clean = $Text.Trim()
  if ($clean.StartsWith("0x")) {
    $clean = $clean.Substring(2)
  }
  return [Convert]::ToUInt32($clean, 16)
}

function Parse-BeginLine([string]$Line) {
  $info = @{}
  foreach ($part in ($Line -split "\s+")) {
    if ($part -match "^([^=]+)=(.+)$") {
      $info[$matches[1]] = $matches[2]
    }
  }
  return $info
}

$serial = [System.IO.Ports.SerialPort]::new($Port, [int]$BaudRate)
$serial.ReadTimeout = [int]$ReadTimeoutMs
$serial.NewLine = "`n"
$serial.DtrEnable = $false
$serial.RtsEnable = $false

$writer = [System.IO.StreamWriter]::new($rawLog, $false, [System.Text.Encoding]::ASCII)
$deadline = [DateTime]::UtcNow.AddSeconds($OverallTimeoutSec)
$started = $false
$expectedBytes = 0
$width = 0
$height = 0
$stride = 0
$buffer = $null
$received = 0
$checksum = 0
$lastProgress = 0
$format = ""
$beginPrefix = ""
$dataPrefix = ""
$endPrefix = ""

try {
  $serial.Open()
  Write-Host "UART_CAPTURE_PORT=$Port"
  Write-Host "UART_CAPTURE_WAITING_FOR=RGB565_UART_DUMP_BEGIN|GRAY8_UART_DUMP_BEGIN|RAW16_UART_DUMP_BEGIN|RAW10P_UART_DUMP_BEGIN"
  Write-Host "UART_CAPTURE_HINT=reset or start the board now if the dump already happened"

  while ([DateTime]::UtcNow -lt $deadline) {
    try {
      $line = $serial.ReadLine().TrimEnd("`r")
    }
    catch [TimeoutException] {
      continue
    }

    $writer.WriteLine($line)

    if (-not $started) {
      if ($line.StartsWith("RGB565_UART_DUMP_BEGIN") -or $line.StartsWith("GRAY8_UART_DUMP_BEGIN") -or $line.StartsWith("RAW16_UART_DUMP_BEGIN") -or $line.StartsWith("RAW10P_UART_DUMP_BEGIN")) {
        if ($line.StartsWith("RAW10P_UART_DUMP_BEGIN")) {
          $format = "raw10p"
          $beginPrefix = "RAW10P"
          $dataPrefix = "RAW10P_UART_DUMP_DATA "
          $endPrefix = "RAW10P_UART_DUMP_END"
          $bin = Join-Path $out "uart_raw10p_${stamp}.bin"
          $png = Join-Path $out "uart_raw10p_${stamp}.png"
        }
        elseif ($line.StartsWith("RAW16_UART_DUMP_BEGIN")) {
          $format = "raw16le"
          $beginPrefix = "RAW16"
          $dataPrefix = "RAW16_UART_DUMP_DATA "
          $endPrefix = "RAW16_UART_DUMP_END"
          $bin = Join-Path $out "uart_raw16_${stamp}.bin"
          $png = Join-Path $out "uart_raw16_${stamp}.png"
        }
        elseif ($line.StartsWith("GRAY8_UART_DUMP_BEGIN")) {
          $format = "gray8"
          $beginPrefix = "GRAY8"
          $dataPrefix = "GRAY8_UART_DUMP_DATA "
          $endPrefix = "GRAY8_UART_DUMP_END"
          $bin = Join-Path $out "uart_gray8_${stamp}.bin"
          $png = Join-Path $out "uart_gray8_${stamp}.png"
        }
        else {
          $format = "rgb565"
          $beginPrefix = "RGB565"
          $dataPrefix = "RGB565_UART_DUMP_DATA "
          $endPrefix = "RGB565_UART_DUMP_END"
          $bin = Join-Path $out "uart_rgb565_${stamp}.bin"
          $png = Join-Path $out "uart_rgb565_${stamp}.png"
        }
        $begin = Parse-BeginLine $line
        $width = [UInt32]$begin["width"]
        $height = [UInt32]$begin["height"]
        $stride = [UInt32]$begin["stride"]
        $expectedBytes = [UInt32]$begin["bytes"]
        $buffer = [byte[]]::new($expectedBytes)
        $started = $true
        Write-Host "$($beginPrefix)_UART_BEGIN=$line"
      }
      continue
    }

    if ($line.StartsWith($dataPrefix)) {
      $parts = $line.Split(" ", 3, [System.StringSplitOptions]::RemoveEmptyEntries)
      if ($parts.Count -ne 3) {
        throw "Malformed data line: $line"
      }
      $offset = Convert-HexToUInt32 $parts[1]
      $hex = $parts[2].Trim()
      if (($hex.Length % 2) -ne 0) {
        throw "Odd hex length at offset 0x$($parts[1])"
      }
      $count = [UInt32]($hex.Length / 2)
      if (($offset + $count) -gt $expectedBytes) {
        throw "Data line exceeds expected buffer at offset 0x$($parts[1])"
      }
      for ($i = 0; $i -lt $count; $i++) {
        $v = [Convert]::ToByte($hex.Substring($i * 2, 2), 16)
        $buffer[$offset + $i] = $v
        $checksum = ($checksum + $v) -band 0xFFFFFFFF
      }
      $received += $count
      if (($received - $lastProgress) -ge 65536) {
        $lastProgress = $received
        Write-Host "UART_CAPTURE_PROGRESS=$received/$expectedBytes"
      }
      continue
    }

    if ($line.StartsWith($endPrefix)) {
      $end = Parse-BeginLine $line
      $reportedChecksum = Convert-HexToUInt32 $end["checksum"]
      if ($received -ne $expectedBytes) {
        throw "Received $received bytes, expected $expectedBytes"
      }
      if ($checksum -ne $reportedChecksum) {
        throw ("Checksum mismatch: host=0x{0:X8} target=0x{1:X8}" -f $checksum, $reportedChecksum)
      }
      [System.IO.File]::WriteAllBytes($bin, $buffer)
      Write-Host "$($beginPrefix)_UART_END=$line"
      Write-Host "$($beginPrefix)_DUMP=$bin"
      Write-Host "BYTES=$expectedBytes"
      Write-Host "STRIDE=$stride"
      if ($format -eq "raw16le") {
        python (Join-Path $repo "tools/convert_dcmipp_raw16_to_pgm.py") $bin --width $width --height $height --stride-bytes $stride
      }
      else {
        python (Join-Path $repo "tools/convert_rgb565_to_png.py") $bin $png --width $width --height $height --stride $stride --format $format
        Write-Host "PNG=$png"
      }
      return
    }
  }

  throw "Timed out waiting for UART frame dump; raw log: $rawLog"
}
finally {
  $writer.Dispose()
  if ($serial.IsOpen) {
    $serial.Close()
  }
  $serial.Dispose()
}

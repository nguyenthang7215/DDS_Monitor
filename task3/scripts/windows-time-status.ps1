# Read-only, language-independent CIM property names. No service/configuration/clock changes.
$ErrorActionPreference = 'Stop'
$result = @{ service_running = $null; source = ''; time_sources = $null; offset_us = $null }
try {
    $svc = Get-CimInstance -ClassName Win32_Service -Filter "Name='W32Time'"
    if ($null -ne $svc) { $result.service_running = ($svc.State -eq 'Running') }
    $source = & w32tm.exe /query /source 2>$null
    if ($LASTEXITCODE -eq 0) { $result.source = ($source -join ' ').Trim() }
    try {
        $perf = Get-CimInstance -ClassName Win32_PerfRawData_Counters_WindowsTimeService | Select-Object -First 1
        if ($null -ne $perf.NTPClientTimeSourceCount) { $result.time_sources = [double]$perf.NTPClientTimeSourceCount }
        if ($null -ne $perf.ComputedTimeOffset) { $result.offset_us = [double]$perf.ComputedTimeOffset }
    } catch { $result.counter_error = $_.Exception.Message }
} catch { $result.error = $_.Exception.Message }
$result | ConvertTo-Json -Compress

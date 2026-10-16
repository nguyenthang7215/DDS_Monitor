param(
    [Parameter(Mandatory=$true)][string]$QtPrefix,
    [Parameter(Mandatory=$true)][string]$DdsPrefix,
    [string]$BuildDirectory = 'build-agent-windows',
    [ValidateSet('Debug','Release','RelWithDebInfo')][string]$Configuration = 'Release',
    [string]$Architecture = 'x64'
)
# Run in a Developer PowerShell for an MSVC toolset compatible with both dependencies.
$ErrorActionPreference = 'Stop'
$source = (Resolve-Path (Join-Path $PSScriptRoot '../agent')).Path
$prefixes = "$((Resolve-Path $QtPrefix).Path);$((Resolve-Path $DdsPrefix).Path)"
& cmake -S $source -B $BuildDirectory -A $Architecture "-DCMAKE_PREFIX_PATH=$prefixes"
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
& cmake --build $BuildDirectory --config $Configuration --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Agent build failed' }
Write-Host 'Agent built. Add Qt/Fast DDS/Fast CDR DLL directories to PATH before running.'
Write-Host 'Use --config task3/config/windows-agent.json. Build/runtime still require validation on this Windows host.'

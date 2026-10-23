# Kế hoạch kiểm thử chấp nhận E2E — Master Linux, Slave Windows

**Phạm vi:** kiểm thử hệ thống thật trên hai máy, đối chiếu FR-01–FR-15 trong `docs/task3/TASK3_REQUIREMENTS.md`. Hướng dẫn được viết theo source tại commit `6c804319c08430f3056f3cf243cc0747ca0f050f` (short SHA `6c80431`). Đây là kế hoạch để người vận hành thực hiện; chưa có kết quả kiểm thử Windows hoặc Linux–Windows trong tài liệu này.

**Nguyên tắc:** telemetry Agent và DDS Statistics là hai luồng riêng. Agent gửi heartbeat/metrics trên `Task3/HostTelemetry/v1`; Statistics Module của từng ứng dụng DDS công bố thống kê cho Statistics Backend. Agent Online, ping thành công, hoặc phát hiện Participant không chứng minh Writer–Reader đang trao đổi dữ liệu. Mỗi bước phải ghi bằng chứng và trạng thái thực tế. Không tắt toàn bộ firewall, không dừng dịch vụ Windows quan trọng, không tự đổi cấu hình giờ/NTP.

## 1. Checklist và thông tin phiên kiểm thử

### 1.1 Checklist trước khi bắt đầu

- [ ] Master chạy Linux 64-bit; Slave là máy Windows 10/11 64-bit trên cùng LAN có định tuyến/multicast phù hợp. Windows 11 được ưu tiên cho bộ MSVC v143 tương thích.
- [ ] Ghi địa chỉ IP tĩnh hoặc DHCP reservation của hai máy; xác nhận cùng subnet/VLAN. Không dùng địa chỉ ví dụ trong file cấu hình Chrony.
- [ ] Đồng bộ source Windows với đúng commit. Không checkout branch mới nhất thay cho commit chỉ định.
- [ ] Linux đã build được Master với script `task3/scripts/build_task3_patched.sh`; prefix Statistics Backend vá tồn tại và runtime wrapper xác nhận đúng thư viện.
- [ ] Windows đã cài Qt 6, Fast DDS/Fast CDR và toolchain cùng kiến trúc/compiler; CMake tìm được các package CMake.
- [ ] Tạo hai thư mục evidence riêng trên mỗi máy: `01-preflight` cho source/toolchain/config/approval/build; `02-acceptance` cho log/screenshot/result theo Test ID. Không trộn môi trường chuẩn bị với kết quả nghiệm thu.
- [ ] Chuẩn bị một ứng dụng DDS Publisher/Subscriber Windows thật, hoặc ứng dụng nghiệp vụ mục tiêu, có thể bật Fast DDS Statistics. Repository hiện không cung cấp binary Publisher/Subscriber Windows.
- [ ] Nếu muốn xác minh ánh xạ Application/GUID, ứng dụng mục tiêu phải xuất RuntimeIdentity metadata hoặc có GUID được cấu hình rõ. Chỉ thấy process không đủ chứng minh mapping.
- [ ] Có phê duyệt của quản trị viên trước mọi thay đổi firewall/NTP/service. Ghi lại cấu hình cũ và kế hoạch hoàn tác.

**Thông tin phiên**

| Trường | Ghi thực tế |
|---|---|
| Ngày, người thực hiện | ______________________________ |
| Commit source | `6c80431` / thực tế: __________________ |
| Linux distro/kernel, IP | ______________________________ |
| Windows edition/build, IP | ______________________________ |
| Master/Agent Fast DDS, Fast CDR | ______________________________ |
| Qt, MSVC, CMake | ______________________________ |
| DDS telemetry Domain/Topic | `42` / `Task3/HostTelemetry/v1` hoặc cấu hình thực tế: __________ |
| DDS application Domain | __________ (độc lập telemetry Domain) |
| Host ID Windows | `windows-slave-a` hoặc duy nhất: __________ |
| TASK3_TIMEOUT_MS | `5000` mặc định hoặc cấu hình thực tế: __________ |
| Evidence directory Linux/Windows | ______________________________ |

### 1.2 Nền tảng/phần mềm và nguồn tải chính thức

Windows Agent build script yêu cầu Developer PowerShell/MSVC, Qt 6 (Core, Network, Concurrent), Fast DDS major version 3, Fast CDR và CMake tối thiểu 3.22. Sample HelloWorld v3.6.1 có CMake tối thiểu 3.28; nếu build cả Agent lẫn sample trên cùng máy, cần CMake ≥3.28. CMake của Agent không ghim minor version Qt/Fast DDS. Để giảm sai khác wire/runtime, dùng Fast DDS 3.6.1 và Fast CDR 2.3.5 x64/MSVC tương thích. Linux workflow hiện dùng Qt 6.7.2 và Fast DDS 3.6.1. Chọn Qt 6.7.2 MSVC 2022 x64 nếu installer chính thức cung cấp; nếu dùng Qt khác, ghi version và coi Windows build/runtime là cần xác minh mới.

- Visual Studio Community / MSVC: [visualstudio.microsoft.com/downloads](https://visualstudio.microsoft.com/downloads/) — cài workload **Desktop development with C++**, MSVC x64/x86 build tools và Windows SDK.
- CMake: [cmake.org/download](https://cmake.org/download/) — cài bản Windows x64; kiểm tra `cmake --version` đạt tối thiểu 3.22.
- Git for Windows: [git-scm.com/download/win](https://git-scm.com/download/win).
- Qt: [Qt online installer](https://doc.qt.io/qt-6/qt-online-installation.html), [Qt downloads](https://www.qt.io/download). Chọn Qt 6 x64 cho đúng MSVC; component cần có gồm Core, Network, Concurrent và CMake package.
- Fast DDS Windows binaries: [Fast DDS 3.6.1 Windows installation](https://fast-dds.docs.eprosima.com/en/v3.6.1/installation/binaries/binaries_windows.html). Trang chính thức nêu yêu cầu Visual Studio C++; xác nhận đúng MSVC toolset trước khi cài.
- Fast DDS/Fast CDR compatibility: [eProsima versions](https://fast-dds.docs.eprosima.com/en/v3.6.1/notes/versions.html). Với Fast DDS 3.6.x, bảng tương thích chỉ định Fast CDR 2.3.5.
- Nếu cần MSVC runtime riêng: [Microsoft Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist). Chỉ cài đúng x64 runtime cần thiết, qua IT/admin.

Không cần cài Statistics Backend trên Windows Agent. Backend vá là dependency runtime của Master Linux; script Windows chỉ build Agent. Không tải DLL rời từ website không chính thức. Dùng thư viện và DLL đi cùng installer/prefix Fast DDS đã chọn.

#### Cài và xác minh Fast DDS trên máy Windows mới

Trang eProsima nêu quy trình tải binary/installer chính thức; không có direct-download URL cố định trong repository. Tải Fast DDS **3.6.1** cho Windows x64 và Visual Studio toolset tương thích, cài bằng installer chính thức và ghi lại đường dẫn cài đặt thực tế. Không lấy DLL rời từ nguồn khác. Các lệnh sau tìm package và DLL thay vì giả định installer dùng một đường dẫn cụ thể:

```powershell
$DdsPrefix = 'C:\Program Files\eProsima\<thư-mục-thực-tế>' # thay theo installer
Test-Path $DdsPrefix
Get-ChildItem $DdsPrefix -Recurse -Include fastdds.dll,fastcdr.dll -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName
Get-ChildItem $DdsPrefix -Recurse -Include fastdds-config.cmake,fastddsConfig.cmake,fastcdr-config.cmake,fastcdrConfig.cmake -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName
```

`Test-Path` phải là `True`; phải tìm thấy DLL và CMake package của cả Fast DDS lẫn Fast CDR. Tương tự, trong QtPrefix cần có `lib\cmake\Qt6\Qt6Config.cmake`, các package Core/Network/Concurrent và DLL tương ứng; lệnh xác minh cụ thể ở mục 2. Ghi phiên bản thực tế. Nếu thiếu package thì dừng và xử lý theo tài liệu installer chính thức; không đoán prefix hoặc trộn DLL.

#### Build công cụ Publisher/Subscriber có thật

Repository DDS Monitor không có binary Publisher/Subscriber Windows. Có thể dùng sample `hello_world` trong tag chính thức [Fast DDS v3.6.1](https://github.com/eProsima/Fast-DDS/tree/v3.6.1/examples/cpp/hello_world), xem [README của sample](https://github.com/eProsima/Fast-DDS/blob/v3.6.1/examples/cpp/hello_world/README.md). CMake sample yêu cầu 3.28+ (Agent standalone chỉ yêu cầu 3.22+). Trong Developer PowerShell x64, sau khi đã xác minh `$DdsPrefix`:

```powershell
New-Item -ItemType Directory -Force C:\src | Out-Null
git clone --branch v3.6.1 --depth 1 https://github.com/eProsima/Fast-DDS.git C:\src\Fast-DDS-3.6.1
cmake --version
cmake -S C:\src\Fast-DDS-3.6.1\examples\cpp\hello_world -B C:\src\Fast-DDS-3.6.1\build-hello-world -A x64 "-DCMAKE_PREFIX_PATH=$DdsPrefix" -DBUILD_SHARED_LIBS=ON
if ($LASTEXITCODE -ne 0) { throw 'HelloWorld configure failed' }
cmake --build C:\src\Fast-DDS-3.6.1\build-hello-world --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'HelloWorld build failed' }
$HelloWorld = 'C:\src\Fast-DDS-3.6.1\build-hello-world\Release\hello_world.exe'
if (-not (Test-Path $HelloWorld)) { Get-ChildItem C:\src\Fast-DDS-3.6.1\build-hello-world -Recurse -Filter hello_world.exe }
```

Nếu CMake tạo executable ở vị trí khác, dùng đường dẫn tìm được thay cho `$HelloWorld`. Trước khi chạy, thêm vào PATH **chỉ phiên PowerShell hiện tại** các thư mục đã tìm được chứa `fastdds.dll`, `fastcdr.dll` và DLL Qt cần thiết. Nếu thiếu DLL, `dumpbin /DEPENDENTS $HelloWorld` trong Developer PowerShell giúp xác định dependency; chỉ dùng DLL cùng installation/toolchain.

Sample có lệnh `hello_world.exe subscriber` và `hello_world.exe publisher`. Chạy Subscriber trước rồi Publisher; README mô tả discovery/matching, SENT/RECEIVED và unmatched khi peer dừng. Mỗi cửa sổ PowerShell là process environment riêng, vì vậy đặt biến và đường dẫn trong cả hai terminal:

```powershell
$HelloWorld = 'C:\src\Fast-DDS-3.6.1\build-hello-world\Release\hello_world.exe'
$DdsPrefix = 'điền prefix đã xác minh ở mục 2'
$DdsBins = Get-ChildItem $DdsPrefix -Recurse -Include fastdds.dll,fastcdr.dll -ErrorAction SilentlyContinue | Select-Object -ExpandProperty DirectoryName -Unique
if (-not $DdsBins) { throw 'Fast DDS/Fast CDR DLL directories were not found' }
$env:PATH = (($DdsBins + $env:PATH) -join ';')
$env:FASTDDS_STATISTICS='HISTORY_LATENCY_TOPIC;NETWORK_LATENCY_TOPIC;PUBLICATION_THROUGHPUT_TOPIC;SUBSCRIPTION_THROUGHPUT_TOPIC;PHYSICAL_DATA_TOPIC;DISCOVERY_TOPIC'
& $HelloWorld subscriber
```

Terminal PowerShell thứ hai:

```powershell
$HelloWorld = 'C:\src\Fast-DDS-3.6.1\build-hello-world\Release\hello_world.exe'
$DdsPrefix = 'điền prefix đã xác minh ở mục 2'
$DdsBins = Get-ChildItem $DdsPrefix -Recurse -Include fastdds.dll,fastcdr.dll -ErrorAction SilentlyContinue | Select-Object -ExpandProperty DirectoryName -Unique
if (-not $DdsBins) { throw 'Fast DDS/Fast CDR DLL directories were not found' }
$env:PATH = (($DdsBins + $env:PATH) -join ';')
$env:FASTDDS_STATISTICS='HISTORY_LATENCY_TOPIC;NETWORK_LATENCY_TOPIC;PUBLICATION_THROUGHPUT_TOPIC;SUBSCRIPTION_THROUGHPUT_TOPIC;PHYSICAL_DATA_TOPIC;DISCOVERY_TOPIC'
& $HelloWorld publisher
```

Khi sample đang chạy, PowerShell thứ ba có thể xác nhận đúng DLL đang được nạp:

```powershell
$HelloPid = (Get-Process -Name hello_world | Select-Object -First 1).Id
(Get-Process -Id $HelloPid).Modules |
    Where-Object { $_.ModuleName -match '^(fastdds|fastcdr)\.dll$' } |
    Select-Object ModuleName,FileName
```

`FileName` phải thuộc DdsPrefix đã chọn. Nếu module không xuất hiện hoặc đường dẫn trỏ đến bản khác, dừng ca và sửa PATH/prefix trước khi ghi Discovery/Statistics evidence.

Chọn đúng application Domain trên Master; không nhầm với telemetry Domain 42. Sample mặc định của nó không tự chứng minh RuntimeIdentity và không phải benchmark tốc độ cố định. Nó phù hợp để xác nhận discovery/matching, trao đổi sample và sự xuất hiện của statistics/latency nếu backend nhận được chúng. Kiểm tra QoS profile trong E2E-12, không giả định file profile được nạp nếu chưa xác minh log và kết quả.

Trên Linux có thể build cùng tag sample ngoài repository, dùng prefix Fast DDS/Fast CDR đã biết của Master:

```bash
export DDS_PREFIX=/home/dev/dds-deps/install
git clone --branch v3.6.1 --depth 1 https://github.com/eProsima/Fast-DDS.git /tmp/Fast-DDS-3.6.1-e2e
cmake -S /tmp/Fast-DDS-3.6.1-e2e/examples/cpp/hello_world -B /tmp/Fast-DDS-3.6.1-e2e/build-hello-world -DCMAKE_PREFIX_PATH="$DDS_PREFIX" -DBUILD_SHARED_LIBS=ON
cmake --build /tmp/Fast-DDS-3.6.1-e2e/build-hello-world --parallel 4
find /tmp/Fast-DDS-3.6.1-e2e/build-hello-world -type f -name hello_world -executable -print
```

Chạy Publisher Linux trong một terminal (Subscriber chạy ở Windows theo lệnh trên); đường dẫn được lấy từ kết quả `find`:

```bash
HELLO_WORLD="$(find /tmp/Fast-DDS-3.6.1-e2e/build-hello-world -type f -name hello_world -executable -print -quit)"
test -x "$HELLO_WORLD"
export LD_LIBRARY_PATH="$DDS_PREFIX/lib:$LD_LIBRARY_PATH"
ldd "$HELLO_WORLD" | rg 'fastdds|fastcdr'
export FASTDDS_STATISTICS='HISTORY_LATENCY_TOPIC;NETWORK_LATENCY_TOPIC;PUBLICATION_THROUGHPUT_TOPIC;SUBSCRIPTION_THROUGHPUT_TOPIC;PHYSICAL_DATA_TOPIC;DISCOVERY_TOPIC'
"$HELLO_WORLD" publisher
```

Đọc toàn bộ kết quả `ldd`: nếu có `not found`, hoặc `libfastdds`/`libfastcdr` không được nạp từ prefix dự kiến, dừng bước này tại terminal hiện tại và đánh dấu ca DDS `BLOCKED`; không chạy Publisher/Subscriber cho tới khi dependency được xử lý. Không dùng `exit` để dừng shell. Đây là công cụ test tạm, không được chép vào repository. Dùng cùng application Domain và profile ở hai máy.

### 1.3 Mạng DDS và firewall

Đặt hai máy trên LAN có thể trao đổi UDP trực tiếp; tránh Guest Wi-Fi, client isolation, VLAN chặn multicast hoặc VPN làm đổi interface. Fast DDS mặc định dùng Simple Discovery với multicast discovery và các locator unicast; XML profile/transport tùy chỉnh có thể làm thay đổi cổng/locator. Cùng Domain là điều kiện cần nhưng không đủ.

Với RTPS port mapping mặc định, `PB = 7400 + 250 × DomainId`; Domain 42 có PB 17900. Các locator mặc định liên quan gồm discovery multicast `PB`, user multicast `PB+1`, unicast participant metadata `PB+10+2×participantId` và user data `PB+11+2×participantId`. Đây chỉ là phép tính cho cấu hình mặc định; xác nhận locator thực tế, participant ID, profile và firewall trước khi tạo rule. Xem [Fast DDS discovery](https://fast-dds.docs.eprosima.com/en/v3.6.1/fastdds/discovery/discovery.html) và [listening locators/port mapping](https://fast-dds.docs.eprosima.com/en/v3.6.1/fastdds/transport/listening_locators.html).

Trên Linux kiểm tra firewall mà chưa thay đổi:

```bash
ip -br address
ip route
ip maddr
sudo ufw status verbose
```

Nếu không dùng UFW, kiểm tra công cụ firewall distro tương ứng ở chế độ chỉ đọc. Trên Windows (PowerShell):

```powershell
Get-NetIPConfiguration
Get-NetAdapter | Format-Table Name, Status, LinkSpeed
Get-NetConnectionProfile
Get-NetFirewallProfile | Format-Table Name, Enabled, DefaultInboundAction, DefaultOutboundAction
```

Chỉ sau khi admin/network owner duyệt, tạo rule inbound/outbound hẹp theo profile Private, executable/peer IP và UDP locator/cổng đã xác nhận. Không dùng `netsh advfirewall set allprofiles state off`, không mở toàn bộ UDP, không suy luận DDS hoạt động từ `ping`/`Test-NetConnection` (lệnh này kiểm tra TCP, không chứng minh multicast DDS).

## 2. Chuẩn bị và build Windows Slave

Mở **Developer PowerShell for VS 2022** (x64), không phải PowerShell thường nếu môi trường chưa nạp MSVC. Clone đúng repository đã được phép sử dụng và checkout đúng commit:

```powershell
git clone https://github.com/nguyenthang7215/DDS_Monitor.git C:\src\DDS-Monitor
Set-Location C:\src\DDS-Monitor
git fetch --all --tags
git checkout 6c804319c08430f3056f3cf243cc0747ca0f050f
git rev-parse HEAD
git status --short
```

Nếu remote không có commit SHA này, dừng và lấy source chính xác từ máy/repository đã lưu commit; không thay bằng HEAD mới. Kết quả `git rev-parse HEAD` phải in đúng SHA ở trên.

Tạo hai thư mục evidence riêng: `01-preflight` chỉ lưu cấu hình/phiên bản/build/approval; `02-acceptance` chỉ lưu kết quả test chức năng, screenshot và logs runtime. Không trộn ảnh nghiệm thu với bằng chứng chuẩn bị môi trường.

```powershell
$EvidenceRoot = Join-Path $env:USERPROFILE ("DDS-Monitor-Evidence\" + (Get-Date -Format 'yyyyMMdd-HHmmss'))
$PreflightEvidence = Join-Path $EvidenceRoot '01-preflight'
$AcceptanceEvidence = Join-Path $EvidenceRoot '02-acceptance'
New-Item -ItemType Directory -Force -Path $PreflightEvidence,$AcceptanceEvidence | Out-Null
Start-Transcript -Path (Join-Path $PreflightEvidence 'windows-session.txt')
git --version
cmake --version
Get-Command cl
```

Trong File Explorer/Qt Maintenance Tool xác nhận Qt x64/MSVC kit có `Qt6CoreConfig.cmake`, `Qt6NetworkConfig.cmake`, `Qt6ConcurrentConfig.cmake`; ghi lại QtPrefix (thường có dạng `C:\Qt\6.x.y\msvc2022_64`). Xác nhận DdsPrefix trỏ đến prefix chứa Fast DDS/Fast CDR CMake package và DLL tương ứng. Không đoán thư mục từ ví dụ; tìm package/DLL trong chính nơi đã cài:

```powershell
$QtPrefix = 'C:\Qt\6.7.2\msvc2022_64' # thay bằng prefix thực tế đã xác nhận
$DdsPrefix = 'C:\Program Files\eProsima\Fast-DDS' # thay bằng prefix thực tế đã xác nhận
Test-Path (Join-Path $QtPrefix 'lib\cmake\Qt6\Qt6Config.cmake')
Get-ChildItem (Join-Path $QtPrefix 'lib\cmake') -Recurse -Include Qt6CoreConfig.cmake,Qt6NetworkConfig.cmake,Qt6ConcurrentConfig.cmake -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName
Get-ChildItem $QtPrefix -Recurse -Filter Qt6Core.dll -ErrorAction SilentlyContinue | Select-Object -ExpandProperty DirectoryName -Unique
Get-ChildItem $QtPrefix -Recurse -Include Qt6Network.dll,Qt6Concurrent.dll -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName
Get-ChildItem $DdsPrefix -Recurse -Include fastdds.dll,fastcdr.dll -ErrorAction SilentlyContinue | Select-Object FullName
Get-ChildItem $DdsPrefix -Recurse -Include fastdds-config.cmake,fastddsConfig.cmake,fastcdr-config.cmake,fastcdrConfig.cmake -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName
```

Nếu prefix/package không khớp, dừng và sửa lựa chọn kit/prefix; không thêm thư viện ngẫu nhiên vào PATH. Build bằng script repository:

```powershell
Set-Location C:\src\DDS-Monitor
try {
    & .\task3\scripts\build-windows.ps1 -QtPrefix $QtPrefix -DdsPrefix $DdsPrefix -BuildDirectory .\build-agent-windows -Configuration Release -Architecture x64
} catch {
    throw "Windows Agent build failed: $($_.Exception.Message)"
}
$Agent = Get-ChildItem .\build-agent-windows -Recurse -Filter task3_agent.exe -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $Agent) { throw 'Build script returned but task3_agent.exe was not found' }
$AgentPath = $Agent.FullName
Test-Path $AgentPath
```

Script chạy CMake configure/build, parallel 4. Nó **không** cài Qt/Fast DDS, tìm DLL runtime, cài service, sửa firewall, tạo Publisher/Subscriber, hay cấu hình NTP. Binary dự kiến nằm trong `build-agent-windows\Release\task3_agent.exe`; xác nhận bằng `Get-ChildItem .\build-agent-windows -Recurse -Filter task3_agent.exe`.

Tìm thư mục chứa `Qt6Core.dll`, `Qt6Network.dll`, `Qt6Concurrent.dll`, `fastdds.dll`, `fastcdr.dll` và dependency runtime trong prefix tương ứng. Chúng có thể ở các thư mục khác nhau; nạp tất cả thư mục **đã xác minh** vào PATH của phiên PowerShell hiện tại (không sửa PATH hệ thống):

```powershell
$QtBin = 'điền thư mục thực tế chứa Qt6Core.dll'
$FastDdsBin = 'điền thư mục thực tế chứa fastdds.dll'
$FastCdrBin = 'điền thư mục thực tế chứa fastcdr.dll'
$RuntimeDirs = @($QtBin,$FastDdsBin,$FastCdrBin) | Select-Object -Unique
$env:PATH = (($RuntimeDirs + $env:PATH) -join ';')
Test-Path $AgentPath
& $AgentPath --once
if ($LASTEXITCODE -ne 0) { throw "Agent one-shot check failed: $LASTEXITCODE" }
```

Nếu khi chạy báo thiếu DLL, dùng `dumpbin /DEPENDENTS` trong Developer PowerShell để tìm dependency thiếu, bổ sung thư mục runtime chính thức tương ứng rồi thử lại. Không tải DLL từ nguồn không rõ.

## 3. Khởi chạy Linux Master

### 3.1 Build sạch theo workflow Backend đã vá

Từ Terminal trên Linux Master, trong repository tại đúng commit:

```bash
cd /home/dev/DDS-Monitor
git rev-parse HEAD
git status --short
bash task3/scripts/build_task3_patched.sh --jobs 4
```

Lệnh build Statistics Backend 2.5.0 từ source/patch đã ghim, rồi build Debug/Release và chạy CTest cho cả hai. Build script xác minh linkage. Không dùng `/tmp/dds-p0`, không cài đè `/home/dev/dds-deps/install`. Nếu cần chỉ build dependency: `bash task3/scripts/build_statistics_backend_patched.sh --jobs 4`.

Lưu log build vào evidence (chạy khi cần lưu lại toàn bộ đầu ra):

```bash
EVIDENCE_ROOT="$HOME/DDS-Monitor-Evidence/$(date -u +%Y%m%dT%H%M%SZ)"
PREFLIGHT_EVIDENCE="$EVIDENCE_ROOT/01-preflight"
ACCEPTANCE_EVIDENCE="$EVIDENCE_ROOT/02-acceptance"
mkdir -p "$PREFLIGHT_EVIDENCE" "$ACCEPTANCE_EVIDENCE"
bash task3/scripts/build_task3_patched.sh --jobs 4 2>&1 | tee "$PREFLIGHT_EVIDENCE/build-$(date -u +%Y%m%dT%H%M%SZ).log"
```

Build thành công không thay cho test vật lý Windows. Nếu muốn chạy CTest thủ công theo build vừa tạo:

```bash
build_root="$(<.task3-deps/task3-builds/LATEST)"
ctest --test-dir "$build_root/debug" --output-on-failure
ctest --test-dir "$build_root/release" --output-on-failure
```

### 3.2 Chạy Master và thu log

Đặt cùng telemetry Domain/Topic với Windows config. `TASK3_AUTO_DOMAIN=0` mặc định nghĩa là không tự khởi tạo Statistics Monitor. DDS application Domain được khởi tạo riêng trong giao diện DDS gốc, bằng thao tác **Initialize Monitor**; chọn Domain của ứng dụng Publisher/Subscriber, không mặc định chọn 42.

```bash
cd /home/dev/DDS-Monitor
export TASK3_TELEMETRY_DOMAIN=42
export TASK3_TELEMETRY_TOPIC='Task3/HostTelemetry/v1'
export TASK3_TIMEOUT_MS=5000
export TASK3_DATA_DIR="$PWD/.task3-runtime/e2e-master"
export TASK3_FLOW_RULES="$PWD/task3/config/flow-rules.json"
mkdir -p "$TASK3_DATA_DIR" "$HOME/DDS-Monitor-Evidence"
bash task3/scripts/run_master_patched.sh 2>&1 | tee "$ACCEPTANCE_EVIDENCE/master-$(date -u +%Y%m%dT%H%M%SZ).log"
```

Runner chọn build Release gần nhất, xác minh Backend/Fast DDS/Fast CDR qua `ldd` và xác nhận Master đã map thư viện Backend vá. Trong log cần thấy dòng khởi chạy với Backend 2.5.0; nếu wrapper báo prefix/manifest/hash/linkage sai, dừng. Không khởi chạy đồng thời nhiều Statistics Monitor trên cùng application Domain.

Kiểm tra library thực tế khi Master đang chạy, ở Terminal Linux thứ hai:

```bash
pgrep -af 'dds_monitor'
pid='<PID của Master>'
grep -E 'fastdds_statistics_backend|libfastdds\.so|libfastcdr\.so' "/proc/$pid/maps" | sort -u
```

Đường dẫn `libfastdds_statistics_backend.so.2.5.0` phải ở `.task3-deps/fastdds-statistics-backend-2.5.0-patched/`; Fast DDS phải là prefix 3.6.1 được wrapper cấu hình. Không tiếp tục nghiệm thu DDS Statistics nếu không khớp.

## 4. Cấu hình và chạy Windows Agent

Mẫu source là `task3/config/windows-agent.json`: Host ID `windows-slave-a`, telemetry Domain 42, Topic `Task3/HostTelemetry/v1`, chu kỳ 1000 ms, ổ `C:/`, process mẫu `hello_world.exe`. Host ID phải duy nhất. Thay `process_name` bằng tên executable thực tế cần theo dõi; tên service (nếu cần) phải là **Service Name**, không phải display name. Chỉ cấu hình phần mềm thử nghiệm không quan trọng; Agent chỉ quan sát, không điều khiển service.

Tạo config riêng để không sửa source/repository:

```powershell
Set-Location C:\src\DDS-Monitor
Copy-Item .\task3\config\windows-agent.json .\windows-agent.local.json
notepad .\windows-agent.local.json
```

Giữ Domain/Topic đúng Master; đặt `host_id` duy nhất, `disk_paths` theo ổ cần giám sát, `applications` theo process/service thực tế. Với Windows, `argument_token` chưa hỗ trợ và khiến trạng thái Process Unknown; không đặt trường này. `participant_guids` rỗng nghĩa là không tạo mapping tĩnh. Để kiểm tra RuntimeIdentity, cần ứng dụng instrumented cùng manifest; cấu hình `identity_directory` chỉ sau khi ứng dụng đó thật sự ghi manifest.

Chạy collector một lần để kiểm tra JSON/schema và giá trị thô mà không tạo DDS writer:

```powershell
$Agent = $AgentPath
& $Agent --config .\windows-agent.local.json --once 2>&1 | Tee-Object -FilePath (Join-Path $PreflightEvidence 'agent-once.txt')
if ($LASTEXITCODE -ne 0) { throw "Agent --once failed: $LASTEXITCODE" }
```

JSON phải có `host_id`, `session_id`, `sequence`, `timestamp_ms`, `metrics`, `applications`, `time_sync`; kiểm tra process/service đúng mục tiêu và không có key bí mật. Nếu không mở được config hoặc JSON không hợp lệ, không chạy Agent liên tục.

Chạy liên tục, ghi stdout/stderr:

```powershell
& $Agent --config .\windows-agent.local.json 2>&1 | Tee-Object -FilePath (Join-Path $AcceptanceEvidence 'agent.log')
if ($LASTEXITCODE -ne 0) { throw "Agent exited with code $LASTEXITCODE" }
```

Khi Agent đang chạy liên tục, từ PowerShell thứ hai xác nhận Fast DDS/Fast CDR DLL đã được nạp từ prefix dự kiến (khác với `--once`, vốn không tạo DDS transport):

```powershell
$AgentPid = (Get-Process -Name task3_agent | Select-Object -First 1).Id
(Get-Process -Id $AgentPid).Modules |
    Where-Object { $_.ModuleName -match '^(fastdds|fastcdr|Qt6Core|Qt6Network|Qt6Concurrent)\.dll$' } |
    Select-Object ModuleName,FileName
```

`FileName` phải trỏ tới các thư mục DLL đã xác minh ở trên. Nếu không thấy Fast DDS/Fast CDR hoặc module nạp từ bản khác, ghi BLOCKED/FAIL runtime và chưa chạy nghiệm thu DDS.

Kỳ vọng log có `Real collectors active; DDS domain 42 topic Task3/HostTelemetry/v1`. Dừng bằng `Ctrl+C`, không đóng cưỡng bức trừ khi đang chẩn đoán hang. Không cần `TASK3_KEY`; source hiện tại không sử dụng HMAC/key.

### 4.1 Phương pháp đo và phân biệt tiêu chí

`TASK3_REQUIREMENTS.md` không đặt SLA số cho CPU/RAM/Disk/Network, throughput, latency hoặc offset NTP. Các ngưỡng dưới đây là **đề xuất cho phiên kiểm thử**, không phải ngưỡng Requirements; người nghiệm thu cần thống nhất trước và ghi lại. Dù chưa thống nhất ngưỡng, vẫn FAIL nếu schema/đơn vị sai, giá trị ngoài miền hợp lệ, hoặc stale/Unknown bị trình bày như fresh/đã đồng bộ.

| Đại lượng | Yêu cầu định tính | Đối chiếu khách quan và ngưỡng đề xuất (cần thống nhất) |
|---|---|---|
| CPU | Thu thập/hiển thị hợp lệ theo Host | So với `Get-Counter '\Processor(_Total)\% Processor Time'`, cùng cửa sổ 30 s. Miền hợp lệ 0–100%; đề xuất chênh lệch trung bình không quá 10 điểm phần trăm và xu hướng tương ứng. Counter name có thể phụ thuộc locale Windows. |
| RAM | Used, Total, Percentage nhất quán | `Used = Total - Available`; `Percentage = 100 × Used / Total`. Đề xuất sai lệch so với CIM không quá 2% tổng RAM, lấy gần cùng thời điểm. |
| Disk | Total/Used/Free theo từng volume | Mỗi volume phải có `Used + Free ≈ Total`; đối chiếu `Get-Volume`. Đề xuất sai lệch không quá 1% Total, ghi nhận reservation/metadata và thời điểm lấy mẫu. |
| Network | RX/TX rate/interface/unit đúng | Cùng NIC vật lý, lấy byte counter tại `t0`, `t1`; rate tham chiếu `(counter1-counter0)/(t1-t0)` byte/s. Đề xuất sai lệch tương đối ≤10% trong tải test ổn định; loại loopback/virtual NIC và ghi rõ nếu UI hiển thị tổng interface. |
| Throughput | Thống kê thật, fresh, unit/timestamp đúng; không có rate SLA | Cần mẫu fresh ở phía Writer/Reader trong ít nhất 3 lần cập nhật liên tiếp và đối chiếu counter ứng dụng. Đề xuất benchmark cố định sai lệch ≤20% chỉ sau khi thống nhất đo payload bytes hay wire bytes. HelloWorld tương tác không phải công cụ tốc độ cố định. |
| Latency | Giá trị đo có unit/timestamp; không có SLA | Ghi tối thiểu 60 s mẫu và freshness. Đề xuất p95 ≤50 ms trên LAN dây chỉ khi clock error hai máy đã đo; phải được thống nhất trước. Nếu clock unsynchronized/offset chưa rõ, không dùng số p95 liên máy để chứng minh ngưỡng. |
| NTP | Source/status/offset đúng evidence; không Synced giả | Requirements không đặt giới hạn offset/hội tụ. Đề xuất sau hội tụ: Windows source là Master dự kiến và 5 mẫu `w32tm /stripchart` nằm trong ±100 ms, sau khi admin/người nghiệm thu đồng ý. W32Time Running riêng lẻ không chứng minh sync. |

Ví dụ đo tốc độ NIC bằng counter Windows trong 30 giây thực đo:

```powershell
$Nic = '<tên NIC vật lý>'
$a = Get-NetAdapterStatistics -Name $Nic
$t0 = Get-Date
Start-Sleep -Seconds 30
$b = Get-NetAdapterStatistics -Name $Nic
$t1 = Get-Date
$seconds = ($t1 - $t0).TotalSeconds
[pscustomobject]@{
  RxBytesPerSecond = ($b.ReceivedBytes - $a.ReceivedBytes) / $seconds
  TxBytesPerSecond = ($b.SentBytes - $a.SentBytes) / $seconds
  ElapsedSeconds = $seconds
}
```

Chạy khi workload test đang gửi, hạn chế tải nền lớn nếu được phép, lưu timestamps và đối chiếu với telemetry gần cùng thời điểm. Không tạo tải cao chỉ để đạt một ngưỡng đề xuất.

### 4.2 Preflight Gate — chỉ bắt đầu nghiệm thu khi điều kiện đã rõ

Giá trị ban đầu của từng dòng là `NOT VERIFIED`; người thực hiện đổi thành `READY` hoặc `BLOCKED` kèm đường dẫn evidence. `READY` chỉ xác nhận điều kiện đầu vào của test đã đạt, **không** phải kết quả PASS của FR. Các ca phụ thuộc môi trường có thể BLOCKED riêng mà vẫn chạy được ca độc lập; toàn bộ acceptance chưa được `PASS_ACCEPTANCE` khi còn FR bắt buộc BLOCKED.

| Gate | Điều kiện cụ thể để đánh dấu READY | Trạng thái hiện tại |
|---|---|---|
| Source và toolchain Windows | `git rev-parse HEAD` đúng `6c804319c08430f3056f3cf243cc0747ca0f050f`; MSVC x64, Qt6 Core/Network/Concurrent, CMake ≥3.22; Fast DDS 3.6.1/Fast CDR tương thích; CMake ≥3.28 nếu build HelloWorld; CMake package/DLL đã tìm thấy và ghi phiên bản | NOT VERIFIED |
| Windows Agent | Script build kết thúc không exception; binary `task3_agent.exe` tìm được; `--once` trả exit code 0; Agent chạy liên tục và module Fast DDS/Fast CDR/Qt nạp từ prefix dự kiến; config local đúng schema. Không dùng `--help` làm tiêu chí bắt buộc | NOT VERIFIED |
| Linux Master/Backend | workflow patched build/CTest đạt; wrapper/runtime maps xác nhận Backend vá đúng prefix; Master mở được cùng application Domain cần thử | NOT VERIFIED |
| DDS sample và LAN | HelloWorld v3.6.1 build trên hai máy; cùng application Domain/topic/profile; sample logs và UI chứng minh discovery/matching/receive; locator/firewall đã được kiểm tra mà không tắt firewall | NOT VERIFIED |
| FR-10 RuntimeIdentity | Có executable Windows thật được build từ test target đã xác minh hoặc ứng dụng instrumented; nó tạo manifest trong lúc chạy, PID/instance/GUID khớp telemetry và DDS entity. Thiếu điều này thì gate mapped-case = BLOCKED, không dùng JSON viết tay | NOT VERIFIED |
| QoS mismatch | Profile XML riêng ngoài repo; log chứng minh đúng file/profile đã nạp; baseline compatible PASS để có thể khôi phục sau mismatch | NOT VERIFIED |
| NTP E2E | Chỉ READY sau phê duyệt admin/network, upstream Master tin cậy, source Windows trỏ Master và ngưỡng offset đã thống nhất; nếu chưa, E2E-23/24 = BLOCKED | NOT VERIFIED |
| SQLite/test data | Master dùng `TASK3_DATA_DIR` riêng cho phiên nghiệm thu; paging fixture 501 dòng ở thư mục tạm riêng; không trỏ vào database người dùng; event thật sẽ được tạo riêng bởi E2E alert | NOT VERIFIED |
| Evidence | Có hai thư mục `01-preflight` và `02-acceptance`; version/config/firewall approvals ở preflight, ảnh/log/result theo E2E ID ở acceptance; không trộn fixture evidence với event thật | NOT VERIFIED |

## 5. Test cases

Với mỗi ca, ghi kết quả ở trường cuối cùng. `BLOCKED` có nghĩa điều kiện tiên quyết chưa có; không được tính là PASS. Evidence phải gồm thời gian, log/screenshot/query liên quan tới ca.

### E2E-01 — Xác minh baseline hai máy

- **FR liên quan:** FR-01, FR-02, FR-09; điều kiện chung.
- **Mục đích:** xác nhận đúng source, địa chỉ, Domain/Topic và LAN trước khi diễn giải lỗi DDS.
- **Điều kiện chuẩn bị:** hai máy và evidence folder đã sẵn sàng.
- **Máy thực hiện:** [LINUX MASTER] và [WINDOWS SLAVE].
- **Các bước:** (1) Trên Linux chạy `git rev-parse HEAD`, `ip -br address`, `ip route`; (2) trên Windows chạy `git rev-parse HEAD`, `Get-NetIPConfiguration`; (3) ghi IP, subnet/VLAN, Domain/Topic và version; (4) xác nhận cả hai repo ở SHA chỉ định.
- **Lệnh:** xem lệnh ở mục 2 và 3; Windows dùng `git rev-parse HEAD` và `Get-NetIPConfiguration`.
- **Kết quả mong đợi:** cả hai source đúng SHA; hai máy có địa chỉ trong mạng dự kiến; telemetry domain/topic trùng nhau.
- **Kiểm chứng/evidence:** transcript hai máy, `git rev-parse`, cấu hình Agent đã che thông tin cá nhân.
- **PASS/FAIL:** PASS nếu các điều kiện trên đúng; FAIL nếu SHA/cấu hình/mạng không nhất quán.
- **Nếu FAIL:** dừng; sửa đúng source/route/config qua quy trình quản trị, không suy ra DDS từ ping.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-02 — Kiểm tra multicast/firewall không phá chính sách

- **FR liên quan:** FR-02, FR-09–FR-13.
- **Mục đích:** xác nhận các locator/UDP cần dùng được phép qua LAN, không tắt firewall.
- **Điều kiện chuẩn bị:** đã xác nhận Domain và default/custom transport; quyền đọc firewall.
- **Máy thực hiện:** [LINUX MASTER] và [WINDOWS SLAVE].
- **Các bước:** (1) chạy lệnh chỉ đọc tại mục 1.3; (2) xác định Fast DDS profile/locator thực tế và firewall profile; (3) nếu bị chặn, mở yêu cầu cho admin/network owner nêu rõ peer IP, UDP locator/cổng, Private profile và thời hạn; (4) chỉ admin mới áp dụng rule hẹp sau phê duyệt; (5) lưu trạng thái trước/sau.
- **Lệnh:** các lệnh `ip`, `ufw status`, `Get-Net*` ở mục 1.3. Không có lệnh mở firewall chung.
- **Kết quả mong đợi:** policy vẫn bật; rule giới hạn đúng peer/profile/ports; không có xung đột multicast/VLAN.
- **Kiểm chứng/evidence:** output firewall/locator trước-sau và ticket/phê duyệt nếu đổi.
- **PASS/FAIL:** PASS khi chính sách cho phép luồng cần thiết và test DDS sau đó thực sự discovery; ping riêng không đủ. FAIL nếu cần tắt firewall hoặc mở rộng không được phê duyệt.
- **Nếu FAIL:** BLOCKED chờ admin/network điều chỉnh rule hẹp; không tiếp tục gán lỗi cho source.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-03 — Cài dependencies và build Windows Agent

- **FR liên quan:** FR-01–FR-08, NFR-05.
- **Mục đích:** chứng minh Agent build trên Windows với ABI/toolchain đã ghi.
- **Điều kiện chuẩn bị:** commit đúng; VS C++ x64, CMake ≥3.22, Qt6 Core/Network/Concurrent, Fast DDS 3.x, Fast CDR đã cài; package CMake tìm được.
- **Máy thực hiện:** [WINDOWS SLAVE].
- **Các bước:** (1) mở Developer PowerShell; (2) chạy version/check package tại mục 2; (3) chạy lệnh `try/catch` ở mục 2; script CMake kiểm tra `$LASTEXITCODE` của configure/build và ném lỗi khi khác 0; (4) chỉ coi build thành công nếu script trả bình thường **và** `task3_agent.exe` tìm thấy; (5) ghi compiler/Qt/Fast DDS/Fast CDR versions.
- **Lệnh:** dùng `build-windows.ps1 -QtPrefix ...` với tham số đầy đủ ở mục 2; không dùng riêng `$LASTEXITCODE` của lệnh gọi `.ps1` để kết luận vì lỗi build được ném dưới dạng exception.
- **Kết quả mong đợi:** configure/build thành công, x64 Release executable tồn tại; script copy `windows-time-status.ps1` cạnh executable.
- **Kiểm chứng/evidence:** transcript, CMake configure/build output, file binary/script, version.
- **PASS/FAIL:** PASS nếu build target thành công và dependencies cùng compiler/arch; FAIL nếu thiếu package, ABI mismatch hoặc unresolved symbol.
- **Nếu FAIL:** đối chiếu Qt/MSVC/Fast DDS prefix; không chỉnh source hay trộn DLL khác phiên bản.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-04 — Windows collector một lần và cấu hình Host

- **FR liên quan:** FR-01, FR-03–FR-08.
- **Mục đích:** xác nhận Agent đọc đúng config và Windows collector trả schema hợp lệ trước khi gửi DDS.
- **Điều kiện chuẩn bị:** E2E-03 PASS; config riêng có Host ID duy nhất, đúng domain/topic, disk path và ứng dụng thử.
- **Máy thực hiện:** [WINDOWS SLAVE].
- **Các bước:** (1) tạo `windows-agent.local.json`; (2) chạy `task3_agent.exe --config ... --once`; (3) parse JSON; (4) so sánh tên process/service/ổ đĩa với Windows thực; (5) kiểm tra field Unknown/Unavailable không bị biến thành số 0 giả.
- **Lệnh:** `& $Agent --config .\windows-agent.local.json --once`.
- **Kết quả mong đợi:** một JSON telemetry hợp lệ; config lỗi trả exit code khác 0; process/service không cấu hình hoặc không đủ quyền thể hiện Unknown/Unavailable.
- **Kiểm chứng/evidence:** `agent-once.txt`, `Get-Process`, `Get-Service -Name <ServiceName>`, `Get-CimInstance Win32_OperatingSystem`, `Get-Volume` chỉ đọc.
- **PASS/FAIL:** PASS khi Host ID và các trường có giá trị hợp lý; FAIL nếu schema sai hoặc collector khai báo trạng thái không có bằng chứng.
- **Nếu FAIL:** xác minh tên process/service, access và drive letter; không dừng dịch vụ để ép test.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-05 — Master build, CTest và patched Backend runtime

- **FR liên quan:** FR-09–FR-13; regression FR-01–FR-08, FR-14–FR-15.
- **Mục đích:** xác nhận đúng workflow Linux và dependency mà Master thực sự nạp.
- **Điều kiện chuẩn bị:** source đúng SHA; Linux build dependencies/prefixes đã cài.
- **Máy thực hiện:** [LINUX MASTER].
- **Các bước:** (1) chạy build script mục 3; (2) lưu cả Debug/Release CTest; (3) chạy Master; (4) kiểm tra `/proc/<pid>/maps`; (5) nếu library không ở prefix patched thì dừng DDS acceptance.
- **Lệnh:** `bash task3/scripts/build_task3_patched.sh --jobs 4`; CTest và `grep` ở mục 3.
- **Kết quả mong đợi:** Debug/Release build và CTest exit 0; Master map Backend 2.5.0 từ `.task3-deps/...-patched` và Fast DDS prefix đã ghim.
- **Kiểm chứng/evidence:** build/CTest logs, manifest, `ldd`, maps và Master log.
- **PASS/FAIL:** PASS chỉ cho Linux build/unit/dependency; không phải PASS cho Windows/cross-machine.
- **Nếu FAIL:** giữ log, kiểm tra script/manifest/prefix; không dùng binary cũ hoặc backend `/tmp`.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-06 — Host Online, Last Seen và telemetry đầu tiên

- **FR liên quan:** FR-01, FR-02.
- **Mục đích:** chứng minh DDS telemetry từ Agent Windows tới Master Linux.
- **Điều kiện chuẩn bị:** E2E-01–05; Master đang chạy; Agent config domain/topic giống Master.
- **Máy thực hiện:** [LINUX MASTER] và [WINDOWS SLAVE].
- **Các bước:** (1) khởi động Master; (2) khởi động Agent liên tục; (3) đợi vài chu kỳ 1 s; (4) tìm `windows-slave-a` trong danh sách Host; (5) chọn Host và quan sát Online/Last Seen cập nhật; (6) ghi thời gian đầu/cuối.
- **Lệnh:** trên Windows chạy Agent mục 4; trên Linux theo dõi Master log và UI.
- **Kết quả mong đợi:** đúng Host ID; Online khi telemetry mới đến; Last Seen tiến triển; không tạo Host khác do đổi Host ID.
- **Kiểm chứng/evidence:** screenshot Host list/detail, `agent.log`, Master log, đồng hồ thời điểm thu evidence.
- **PASS/FAIL:** PASS khi nhận telemetry từ máy Windows thật và Last Seen thay đổi; FAIL nếu chỉ có ping hoặc Host chưa từng nhận sample.
- **Nếu FAIL:** kiểm tra domain/topic/type/QoS, interface, firewall và locator; thử DDS discovery riêng sau khi xác nhận mạng.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-07 — CPU Windows (FR-03)

- **FR liên quan:** FR-03.
- **Mục đích:** đối chiếu CPU Agent với Windows tại cùng khoảng lấy mẫu.
- **Điều kiện chuẩn bị:** E2E-06 PASS; Task Manager hoặc Performance Counter sẵn sàng.
- **Máy thực hiện:** [WINDOWS SLAVE] và [LINUX MASTER].
- **Các bước:** (1) ghi CPU baseline 30 s; (2) ghi CPU Agent/UI cùng thời điểm; (3) tạo tải nhẹ, ngắn trên workload không quan trọng nếu được phép; (4) đối chiếu xu hướng và sau đó chờ hồi về baseline.
- **Lệnh:** trên Windows đọc counter: `Get-Counter '\Processor(_Total)\% Processor Time' -SampleInterval 1 -MaxSamples 10` (locale counter có thể khác); Master xem Host detail.
- **Kết quả mong đợi:** phần trăm hợp lệ trong 0–100 và xu hướng phù hợp; không yêu cầu hai phép đo cùng chính xác từng mẫu.
- **Kiểm chứng/evidence:** counter output, screenshot, thời điểm và sample interval.
- **PASS/FAIL:** PASS nếu số liệu thay đổi hợp lý và đủ mới; FAIL nếu giá trị không hợp lệ, treo/stale mà UI báo fresh.
- **Nếu FAIL:** xác minh collector health, freshness, thời gian lấy mẫu và counter locale.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-08 — RAM, Disk, Network RX/TX (FR-04–FR-06)

- **FR liên quan:** FR-04, FR-05, FR-06.
- **Mục đích:** so sánh metrics Windows với số liệu OS, gồm đơn vị, interface và N/A.
- **Điều kiện chuẩn bị:** E2E-06 PASS; ghi nhận interface đang dùng; không tạo tải lớn.
- **Máy thực hiện:** [WINDOWS SLAVE] và [LINUX MASTER].
- **Các bước:** (1) đọc RAM total/available; (2) đọc `C:` total/free; (3) chọn physical interface active; (4) ghi RX/TX counters hai lần cách nhau 10 s và tính delta/10; (5) so sánh với Agent/UI; (6) kiểm tra interface/metric unavailable được thể hiện N/A/Unknown, không phải tốc độ 0 giả.
- **Lệnh:** `Get-CimInstance Win32_OperatingSystem | Select-Object TotalVisibleMemorySize,FreePhysicalMemory`; `Get-Volume -DriveLetter C | Select-Object Size,SizeRemaining`; `Get-NetAdapterStatistics -Name '<Interface>' | Select-Object ReceivedBytes,SentBytes`; chạy lệnh interface hai lần cách 10 s.
- **Kết quả mong đợi:** RAM Used/Total/Percentage nhất quán; Disk Total/Used/Free theo `C:`; RX/TX byte/s hợp lý theo delta và interface; UI ghi đúng tổng/interface, freshness và đơn vị. Ngưỡng đối chiếu định lượng đề xuất nằm ở mục 4.1, không phải SLA Requirements.
- **Kiểm chứng/evidence:** lệnh outputs, config `disk_paths`, screenshot.
- **PASS/FAIL:** PASS khi số liệu xấp xỉ cùng mốc thời gian/đơn vị; FAIL nếu disk path sai, RX/TX đảo/không cập nhật hoặc unavailable hiển thị như 0 hiện hành.
- **Nếu FAIL:** kiểm tra interface index/counters, NIC ảo/loopback, permissions và sample period; không giả định tổng là traffic vật lý.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-09 — Process và Service (FR-07)

- **FR liên quan:** FR-07, FR-14.
- **Mục đích:** đối chiếu process/service được cấu hình với Windows API.
- **Điều kiện chuẩn bị:** E2E-04 và E2E-06 PASS; ứng dụng thử an toàn đang chạy; config dùng `process_name`/Service Name đúng.
- **Máy thực hiện:** [WINDOWS SLAVE] và [LINUX MASTER].
- **Các bước:** (1) xác nhận process/service Running qua UI; (2) xác nhận bằng `Get-Process`/`Get-Service`; (3) dừng riêng ứng dụng thử không quan trọng; (4) xác nhận Stopped/alert ở các ca alert bên dưới; (5) khởi động lại ứng dụng thử và xác nhận Running.
- **Lệnh:** `Get-Process -Name '<name-without-.exe>' -ErrorAction SilentlyContinue`; `Get-Service -Name '<ServiceName>'`; chỉ `Stop-Process`/`Start-Service` cho ứng dụng/service do người kiểm thử sở hữu và được phép.
- **Kết quả mong đợi:** trạng thái khớp OS; thiếu quyền/không truy vấn được thể hiện Unknown; không dùng danh sách process suy đoán mapping.
- **Kiểm chứng/evidence:** config JSON, OS query, Host/Application detail.
- **PASS/FAIL:** PASS nếu đối tượng được theo dõi chính xác và trạng thái nguồn phù hợp; FAIL nếu service/process khác bị gán nhầm.
- **Nếu FAIL:** xác minh Service Name, quyền, trường `argument_token` (không hỗ trợ trên Windows) và application id.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-10 — Agent Offline, restart/session và Recovery

- **FR liên quan:** FR-02, FR-14, FR-15.
- **Mục đích:** xác nhận heartbeat timeout, session mới sau restart và trạng thái phục hồi.
- **Điều kiện chuẩn bị:** E2E-06 PASS; có quyền dừng/chạy Agent; không đổi đồng hồ hệ thống.
- **Máy thực hiện:** [WINDOWS SLAVE] và [LINUX MASTER].
- **Các bước:** (1) ghi Last Seen; (2) dừng Agent bằng `Ctrl+C`; (3) giữ Master chạy lâu hơn `TASK3_TIMEOUT_MS` (mặc định 5000 ms cộng biên quan sát); (4) xác nhận Offline/alert Active; (5) chạy lại Agent; (6) xác nhận Online/Last Seen mới/alert Recovered; (7) ghi session_id/sequence nếu có thể quan sát từ log/diagnostic; (8) lặp lại restart Agent thêm hai lần.
- **Lệnh:** `Ctrl+C`; khởi động lại bằng lệnh ở mục 4. Không cần key và không chỉnh đồng hồ để mô phỏng rollback.
- **Kết quả mong đợi:** Offline chỉ sau timeout; restart session mới hợp lệ; sequence mới được chấp nhận; cảnh báo recovery nối đúng sự kiện, không tạo Offline dai dẳng.
- **Kiểm chứng/evidence:** video/screenshots theo thời gian, event history, logs, sample timestamps/session identifiers nếu diagnostic cung cấp.
- **PASS/FAIL:** PASS nếu Online→Offline→Online thực tế và không crash/duplicate Active; FAIL nếu Master chỉ thể hiện ping hoặc Agent restart không phục hồi.
- **Nếu FAIL:** xác minh timeout/domain/topic/session và database status; không sửa sequence bằng cách xóa DB.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-11 — DDS Domain/Participant/Topic/Writer/Reader Discovery

- **FR liên quan:** FR-09.
- **Mục đích:** chứng minh DDS Monitor/Backend thấy các entity DDS của publisher/subscriber thật.
- **Điều kiện chuẩn bị:** E2E-05 PASS; build sample `hello_world.exe` theo mục 1.2; Statistics Module được bật trước khi chạy; application Domain của sample đã xác định (mặc định sample dùng Domain 0 nếu không cấu hình khác; xác nhận bằng sample/help và chọn cùng Domain trên Monitor).
- **Máy thực hiện:** [LINUX MASTER] và [WINDOWS SLAVE].
- **Các bước:** (1) trên Master mở workspace DDS gốc, chọn **Initialize Monitor** và nhập application Domain; (2) trên Windows đặt `FASTDDS_STATISTICS` như mục 1.2, chạy `hello_world.exe subscriber`; (3) trên Linux, build/chạy sample cùng tag/domain, đặt biến Statistics trước khi chạy `hello_world publisher`; (4) chờ sample xác nhận discovery/matching và SENT/RECEIVED; (5) kiểm tra Domain/Participant/Topic/Writer/Reader trên Monitor.
- **Lệnh:** `$env:FASTDDS_STATISTICS='HISTORY_LATENCY_TOPIC;NETWORK_LATENCY_TOPIC;PUBLICATION_THROUGHPUT_TOPIC;SUBSCRIPTION_THROUGHPUT_TOPIC;PHYSICAL_DATA_TOPIC;DISCOVERY_TOPIC'`; dùng đúng lệnh `& $HelloWorld subscriber` / `& $HelloWorld publisher` và Linux sample ở mục 1.2. Cú pháp này thuộc sample Fast DDS v3.6.1, không phải executable do repository DDS Monitor cung cấp.
- **Kết quả mong đợi:** Domain, Participant, Topic, Writer, Reader thật hiện diện; sample logs chứng minh peer discovery và sample send/receive; Statistics Backend vẫn là backend đã vá trên Linux. Discovery/match/traffic là ba bằng chứng khác nhau.
- **Kiểm chứng/evidence:** app command/version, variable, Domain ID, graph/entity screenshot, Master maps.
- **PASS/FAIL:** PASS nếu entities và sample logs xuất hiện trên hai máy thật; FAIL nếu chỉ có telemetry Host/ping hoặc entity bị gán cho Domain sai. Nếu binary sample chưa build/chạy được: BLOCKED, không thay bằng giả định.
- **Nếu FAIL:** xác minh Statistics Module bật trong đúng tiến trình, Domain, multicast/unicast/firewall, discovery locator, Fast DDS compatibility.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-12 — QoS Matching và topology không suy diễn

- **FR liên quan:** FR-11, FR-12.
- **Mục đích:** phân biệt entity discovery, endpoint matching và dữ liệu thực truyền.
- **Điều kiện chuẩn bị:** E2E-11 PASS; HelloWorld hai máy trao đổi được; lưu bản XML profile nguyên gốc trong evidence. Sample có thể tải profile qua `FASTDDS_DEFAULT_PROFILES_FILE`; trước khi dùng QoS mismatch phải xác nhận từ README/log rằng đúng profile được nạp.
- **Máy thực hiện:** [LINUX MASTER] và [WINDOWS SLAVE].
- **Các bước:** (1) với profile mặc định tương thích, lưu Writer→Topic→Reader, match state và SENT/RECEIVED; (2) sao chép `hello_world_profile.xml` từ Fast DDS source checkout sang file `hello_world_profile.test.xml` trong thư mục test ngoài repo, ghi hash bản gốc/bản copy; (3) theo tài liệu XML Profiles Fast DDS v3.6.1, cấu hình một phía Writer `BEST_EFFORT` và phía Reader `RELIABLE` (Reader yêu cầu Reliability cao hơn khả năng Writer); giữ Domain/Topic/Type giống nhau; (4) nạp profile đó cho đúng tiến trình qua `FASTDDS_DEFAULT_PROFILES_FILE`, chạy lại và xác nhận endpoints còn discover nhưng không matched/không có receive; (5) bỏ biến môi trường/khởi chạy lại với profile gốc và xác nhận match/data trở lại. Không sửa profile cài đặt hay source repository.
- **Lệnh:** trên Windows tạo thư mục QoS test ngoài repository, sao chép và hash profile:

  ```powershell
  $QosTestDir = Join-Path $env:TEMP ("DDS-Monitor-QoS-" + (Get-Date -Format 'yyyyMMdd-HHmmss'))
  New-Item -ItemType Directory -Force -Path $QosTestDir | Out-Null
  $ProfileSource = 'C:\src\Fast-DDS-3.6.1\examples\cpp\hello_world\hello_world_profile.xml'
  $ProfileTest = Join-Path $QosTestDir 'hello_world_profile.test.xml'
  Copy-Item $ProfileSource $ProfileTest
  Get-FileHash $ProfileSource,$ProfileTest
  notepad $ProfileTest
  $env:FASTDDS_DEFAULT_PROFILES_FILE = $ProfileTest
  ```

  Linux đặt biến trỏ tới bản copy tương tự ngoài source repository: `export FASTDDS_DEFAULT_PROFILES_FILE=/tmp/<qos-test-dir>/hello_world_profile.test.xml`. XML profile syntax/profile names phải lấy từ sample v3.6.1 và [Fast DDS XML configuration docs](https://fast-dds.docs.eprosima.com/en/v3.6.1/fastdds/xml_configuration/xml_configuration.html), không chép XML tự đoán. Để bỏ profile: `Remove-Item Env:FASTDDS_DEFAULT_PROFILES_FILE` (PowerShell) / `unset FASTDDS_DEFAULT_PROFILES_FILE` (Bash), rồi khởi chạy tiến trình mới.
- **Kết quả mong đợi:** cùng tên Topic không tự xác nhận match; match cần connection GUID/backend evidence; data flow cần bằng chứng receive/counter; no-match không được gọi là truyền thành công.
- **Kiểm chứng/evidence:** GUID Writer/Reader, topic/type/QoS, connection status/timestamp, application receive counter/log.
- **PASS/FAIL:** PASS nếu mặc định tương thích tạo match+receive, mismatch giữ discovery nhưng bỏ match/receive và khôi phục profile phục hồi hoạt động; FAIL nếu UI suy luận traffic từ entity/topic name hoặc hiển thị match sai. Nếu sample XML profile không cho cấu hình endpoint QoS riêng/không xác nhận đã nạp, ca mismatch là BLOCKED và cần app test hỗ trợ QoS, không tự nhận PASS.
- **Nếu FAIL:** lưu entity và connection evidence; kiểm tra XML parse/profile name, type, policy và log Fast DDS trước khi kết luận lỗi ứng dụng.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-13 — RuntimeIdentity và Host/Application/GUID mapping

- **FR liên quan:** FR-10.
- **Mục đích:** kiểm tra ánh xạ có bằng chứng và hành vi Unmapped khi thiếu metadata.
- **Điều kiện chuẩn bị:** E2E-11; mapping động cần một process Windows thật tạo DDS Participant GUID và RuntimeIdentity manifest khi đang chạy. `task3/tests/RuntimeProbe.cpp` là test fixture thật: tạo `DdsTransport`, lấy GUID và gọi `writeRuntimeIdentity`; target `task3_probe` được khai báo trong root `task3/CMakeLists.txt` chỉ khi `TASK3_BUILD_TESTS=ON`. `task3/scripts/build-windows.ps1` dùng standalone `task3/agent/CMakeLists.txt` nên **không build target này**. Chỉ khi full-project Windows CMake configure đã phân giải được toàn bộ dependency, `task3_probe.exe` build thành công và chạy được trên Windows mới có fixture instrumented; việc này chưa được xác minh tại commit này. Fixture chứng minh cơ chế mapping cho test application, không chứng minh ứng dụng nghiệp vụ khác.
- **Máy thực hiện:** [WINDOWS SLAVE] và [LINUX MASTER].
- **Các bước:** (1) nếu full-project build có `task3_probe.exe`, chạy fixture trên Windows bằng `--identity-dir <path> --app <application_id> --mode writer --domain <app-domain> --participants 1`; (2) kiểm tra manifest được fixture cập nhật khi process sống, có PID/instance/GUID thật; (3) cấu hình Agent `identity_directory` trỏ đúng thư mục và application entry có `application_id=windows-runtime-probe`, `process_name=task3_probe.exe`; chạy Agent liên tục, xác nhận mapping GUID trên Master; (4) restart fixture, kiểm tra PID/instance/GUID mới chỉ map khi manifest mới hợp lệ; (5) chạy HelloWorld không có manifest để kiểm tra Unmapped. Nếu không có executable đã build hoặc nếu cần nghiệm thu ứng dụng nghiệp vụ nhưng chỉ có fixture, ghi BLOCKED/limited scope; không tuyên bố FR-10 đầy đủ PASS.
- **Lệnh:** sau khi full-project target đã build thành công, tìm executable trong đúng build directory đã chọn rồi chạy tham số thực tế của `RuntimeProbe.cpp`:

  ```powershell
  Set-Location C:\src\DDS-Monitor
  $Probe = Get-ChildItem .\build-runtime-probe -Recurse -Filter task3_probe.exe -ErrorAction SilentlyContinue | Select-Object -First 1
  if (-not $Probe) { throw 'task3_probe.exe was not built; FR-10 mapped test is BLOCKED' }
  & $Probe.FullName --identity-dir 'C:\task3-runtime\identities' --app windows-runtime-probe --mode writer --domain 0 --participants 1
  ```

  `build-runtime-probe` là tên build directory do người kiểm thử chọn cho full-project CMake, không phải output đã xác minh. Repository không có lệnh standalone `build-windows.ps1` để build fixture; full-project configure/build phụ thuộc toàn bộ CMake packages của root, phải xác minh khả dụng trước. Không dùng JSON viết tay làm bằng chứng mapping runtime.
- **Kết quả mong đợi:** mapping đúng ứng dụng/run khi metadata hiện hành; app thiếu metadata là Unmapped; stale/conflict không bị map.
- **Kiểm chứng/evidence:** manifest đã lọc trường nhạy cảm, PID/instance/GUID, UI mapping, logs thời gian.
- **PASS/FAIL:** PASS riêng cho nhánh Unmapped nếu HelloWorld được phát hiện nhưng không bị gán sai; mapped case chỉ PASS với manifest được app Windows cập nhật và GUID so khớp. Không có app instrumented = BLOCKED cho mapped case và FR-10 chưa đủ E2E evidence.
- **Nếu FAIL:** kiểm tra thời hạn manifest, PID/instance identity và GUID, không dùng tên Topic thay metadata.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-14 — Throughput, Latency, timestamp, cửa sổ và biểu đồ

- **FR liên quan:** FR-13.
- **Mục đích:** đo thống kê có nguồn, đơn vị và thời điểm cập nhật, không suy ra từ Discovery.
- **Điều kiện chuẩn bị:** E2E-11/12 PASS; ứng dụng có Statistics Module phát `PUBLICATION_THROUGHPUT_TOPIC`, `SUBSCRIPTION_THROUGHPUT_TOPIC`, `HISTORY_LATENCY_TOPIC` và/hoặc `NETWORK_LATENCY_TOPIC` phù hợp.
- **Máy thực hiện:** [LINUX MASTER] và [WINDOWS SLAVE].
- **Các bước:** (1) ghi payload size/rate/receive counter từ ứng dụng; (2) chạy ổn định ít nhất 60 s; (3) chụp throughput writer/reader, latency, unit, sample/window timestamp và biểu đồ động; (4) thay đổi publish rate ở mức an toàn; (5) kiểm tra throughput thay đổi cùng xu hướng; (6) dừng Publisher, chờ metric stale/no data; (7) khởi động lại và xác nhận mẫu mới/timestamp mới.
- **Lệnh:** bật biến `FASTDDS_STATISTICS` trước khi mở ứng dụng như E2E-11; command line chạy ứng dụng theo docs của chính binary. Không có CLI Publisher Windows do repo cung cấp.
- **Kết quả mong đợi:** Throughput dương/fresh khi có traffic, đơn vị byte/s ở metrics tương ứng, timestamp mới, biểu đồ nhận ít nhất 3 mẫu liên tiếp và thay đổi cùng xu hướng sample send rate. Ghi throughput/counter HelloWorld thực tế; vì sample tương tác không có tốc độ cố định, không dùng nó để tuyên bố đạt benchmark/SLA. Latency là thống kê Fast DDS ở điểm đo, không mặc định là end-to-end application latency. Requirements không quy định ngưỡng số tuyệt đối.
- **Kiểm chứng/evidence:** screenshot trước/sau rate change, app rate/receive counter, entity metric value/unit/timestamps, log Statistics Backend.
- **PASS/FAIL:** PASS nếu metric/graph fresh và biến thiên hợp lý với counter/rate; FAIL nếu chỉ hiển thị entity nhưng không có statistic, timestamp không cập nhật, sai unit, hoặc biểu đồ không nhận mẫu. Chỉ có Linux sample không chứng minh Windows.
- **Nếu FAIL:** kiểm tra đúng Statistic Topic, Backend version, app instrumentation, matching và chart query window; không chỉnh `delay_time` nếu chưa có lỗi tái hiện.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-15 — Debounce Process/Service: Stopped → Active → Running → Recovered

- **FR liên quan:** FR-07, FR-14, FR-15.
- **Mục đích:** xác nhận cảnh báo process/service dựa trên bằng chứng và trigger delay.
- **Điều kiện chuẩn bị:** ứng dụng thử không quan trọng do người kiểm thử sở hữu; biết `TASK3_ALERT_TRIGGER_MS` thực tế (mặc định 0 tức không debounce); Event History đang hoạt động.
- **Máy thực hiện:** [WINDOWS SLAVE] và [LINUX MASTER].
- **Các bước:** (1) ghi trạng thái Running; (2) nếu cần kiểm tra debounce khác mặc định, dừng Master sạch rồi khởi chạy trong test session với `TASK3_ALERT_TRIGGER_MS=300` (lệnh bên dưới); (3) dừng ứng dụng thử; (4) đo thời gian tới Active so với trigger config; (5) chạy lại app; (6) xác nhận cùng alert chuyển Recovered, ghi start/end; (7) tạo Stopped ngắn hơn ngưỡng, sau đó Missing/Unknown rồi Stopped lại nếu Agent thực tế cung cấp các trạng thái đó; (8) để Stopped liên tục đủ ngưỡng và xác nhận Active; (9) nếu không thể tạo Missing/Unknown bằng Agent Windows thật, chạy regression Linux hiện có và ghi riêng là unit evidence, không gọi là Windows E2E.
- **Lệnh:** dùng app/service test đã phê duyệt; Windows OS query `Get-Process`/`Get-Service`; tuyệt đối không dừng service Windows quan trọng. Trigger env chỉ áp dụng khi Master được chạy với biến đó trước startup, ví dụ Terminal Linux riêng: `TASK3_ALERT_TRIGGER_MS=300 TASK3_DATA_DIR="$PWD/.task3-runtime/e2e-master" bash task3/scripts/run_master_patched.sh`.
- **Kết quả mong đợi:** Stopped liên tục đủ điều kiện tạo một alert; Unknown/Missing không tự tạo hoặc hồi phục; sau khi có bằng chứng Running thì Recovered cùng alert id.
- **Kiểm chứng/evidence:** timeline Agent samples, config/trigger, Alert/Event History rows, `alert_id`, OS state. Regression local có thể đối chiếu bằng `ctest --test-dir "$(<.task3-deps/task3-builds/LATEST)/release" -R task3_core --output-on-failure`; trong `CoreTests.cpp` có `missingApplicationResetsPendingDebounce` và `unknownEvidenceResetsPendingDebounce`.
- **PASS/FAIL:** PASS nếu debounce đúng và Active→Recovered không duplicate; FAIL nếu Missing/Unknown cộng dồn thời gian hoặc sinh alert không có evidence.
- **Nếu FAIL:** giữ timeline/capture; không dừng tiến trình hệ thống để tái hiện.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-16 — Unknown, Agent Offline và “Not verified” (không Recover giả)

- **FR liên quan:** FR-02, FR-07, FR-14, FR-15.
- **Mục đích:** xác nhận Active Process/Service alert vẫn Active nhưng chưa thể kiểm chứng khi Agent mất/thiếu evidence.
- **Điều kiện chuẩn bị:** một ứng dụng test đã tạo Process/Service alert Active theo E2E-15.
- **Máy thực hiện:** [WINDOWS SLAVE] và [LINUX MASTER].
- **Các bước:** (1) xác nhận alert Active; (2) tạm ngắt Agent bằng Ctrl+C hoặc mất kết nối riêng, không dừng app mục tiêu; (3) chờ Host Offline; (4) mở event; (5) xác nhận cảnh báo process/service vẫn Active và nhãn thể hiện chưa xác minh/Not verified; (6) phục hồi Agent khi app vẫn stopped/unknown; (7) chỉ khi Agent cung cấp Running thực tế mới chấp nhận Recovered.
- **Lệnh:** dừng/restart Agent theo E2E-10; không sửa database.
- **Kết quả mong đợi:** Host Offline không tự khôi phục process/service alert; Unknown/missing không gắn Verified; Recovery chỉ sau evidence Running.
- **Kiểm chứng/evidence:** Host state, alert id/state/verification, application samples và SQLite history.
- **PASS/FAIL:** PASS nếu trạng thái Active được giữ tới khi có evidence; FAIL nếu Offline/Unknown sinh Recovered hoặc Verified giả.
- **Nếu FAIL:** lưu logs/evidence, dừng nghiệm thu FR-14/15.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-17 — Host Offline alert và phục hồi

- **FR liên quan:** FR-02, FR-14, FR-15.
- **Mục đích:** kiểm tra timeout Host tạo đúng một sự kiện và recovery khi heartbeat quay lại.
- **Điều kiện chuẩn bị:** E2E-06 PASS; Host ban đầu Online; ghi timeout config.
- **Máy thực hiện:** [LINUX MASTER] và [WINDOWS SLAVE].
- **Các bước:** (1) dừng Agent; (2) đợi vượt timeout; (3) xác nhận Host Offline + Active alert; (4) khởi động Agent; (5) xác nhận Host Online + cùng sự kiện Recovered; (6) kiểm tra start/recovery timestamps và duplicate.
- **Lệnh:** Agent dừng/chạy theo mục 4.
- **Kết quả mong đợi:** Offline sau timeout, không ngay khi mất 1 packet; recovery sau sample hợp lệ; lịch sử giữ cùng alert lifecycle.
- **Kiểm chứng/evidence:** Host details, event history, Agent/Master timestamps.
- **PASS/FAIL:** PASS nếu state/timestamp phù hợp timeout và không có Active trùng; FAIL nếu mất dữ liệu bị báo Online hoặc Recovery không có heartbeat.
- **Nếu FAIL:** đối chiếu timeout, Host ID, sequence/session và clock; không xóa database.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-18 — DDS No Data/vi phạm rule và Recovery

- **FR liên quan:** FR-12, FR-14, FR-15.
- **Mục đích:** kiểm tra rule DDS có cấu hình, freshness và evidence thực, không cảnh báo chỉ vì thiếu entity.
- **Điều kiện chuẩn bị:** publisher app thật và flow rule phù hợp Topic/mode/deadline. Mẫu repo chỉ có `Task3/RuntimeProbe` periodic 1500 ms và `BusinessEvents` event 15000 ms; không tự áp dụng rule cho Topic khác.
- **Máy thực hiện:** [LINUX MASTER] và [WINDOWS SLAVE].
- **Các bước:** (1) xác nhận writer/match và sample mới trong rule; (2) giữ publisher hoạt động, xác nhận không alert; (3) dừng Publisher có chủ ý; (4) chờ deadline/freshness policy; (5) xác nhận DDS No Data Active với đúng flow; (6) khởi động lại Publisher; (7) xác nhận sample mới, chart/flow recovery và event Recovered.
- **Lệnh:** dùng lệnh của publisher đã cung cấp; không có executable Windows trong repo. Nếu cấu hình rule runtime cho application topic thì dùng file cấu hình test riêng ngoài source, đúng schema `topic`, `mode`, `deadline_ms` (100–3600000 ms), không chỉnh source.
- **Kết quả mong đợi:** alert chỉ khi rule có đủ evidence; No Data và Statistics Stale phân biệt; recovery sau metric/sample mới phù hợp rule.
- **Kiểm chứng/evidence:** flow rule, writer GUID, connection, metric timestamps, event rows, application receive counter.
- **PASS/FAIL:** PASS nếu vòng Active/Recovered đúng; FAIL nếu entity presence được hiểu là data activity, hoặc Unknown/Offline tạo false alert/recovery. Không có publisher/rule evidence = BLOCKED.
- **Nếu FAIL:** kiểm tra matching GUID, deadline, statistics source, stale window; không thay timeout tùy tiện.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-19 — SQLite Event History, restart và Load Older Events (>500)

- **FR liên quan:** FR-15.
- **Mục đích:** xác nhận persistence Active/Recovered, khôi phục sau Master restart, phân trang và integrity.
- **Điều kiện chuẩn bị:** E2E-17 tạo event thật; biết `TASK3_DATA_DIR` của Master. Paging >500 có thể dùng 501 event thật trong database test hoặc fixture 501 dòng trong `TASK3_DATA_DIR` mới/tạm. Không chèn/xóa dữ liệu sản phẩm hoặc database đang dùng.
- **Máy thực hiện:** [LINUX MASTER].
- **Các bước:** (1) với event thật, kiểm tra Active/Recovered, restart Master và integrity; (2) nếu cần paging mà chưa có >500 bản ghi, dừng Master và seed 501 fixture vào thư mục dữ liệu test mới bằng lệnh dưới đây; (3) khởi động Master với `TASK3_DATA_DIR` fixture; (4) xác nhận 500 dòng đầu, bấm **Tải lịch sử cũ hơn**, thấy dòng 501 và không có duplicate `alert_id`; (5) ghi rõ fixture chỉ kiểm tra pagination UI, không chứng minh alert lifecycle/persistence; (6) regression unit hiện có `pagedHistoryIncludesRecoveryForOlderAlert` và `pagedHistoryDoesNotReintroducePendingRecovery` là bằng chứng hỗ trợ riêng.
- **Tình trạng xác minh ban đầu:** Fixture 501 dòng chưa được xác minh bằng GUI thật. Giữ kết quả pagination là `NOT RUN` cho tới khi Master thực sự mở DB fixture, hiển thị trang đầu, bấm **Tải lịch sử cũ hơn**, tải đủ 501 sự kiện và không có `alert_id` trùng. Không suy kết quả UI từ việc seed thành công, SQL count, hoặc unit/integration test.
- **Lệnh tạo fixture an toàn:**

  Schema trong `AlertStore.cpp` là `events(id INTEGER PRIMARY KEY, payload TEXT NOT NULL)`; `alert_state` và `metadata` được Master tạo khi mở DB. Fixture chỉ tạo bảng `events` với 501 JSON payload gồm `alert_id`, `state`, `source`, `message`, `severity`, `kind`, `triggered_ms`, `recovered_ms`, khớp cách cache/history đọc dữ liệu.

  ```bash
  E2E_DATA_DIR="$(mktemp -d "$HOME/DDS-Monitor-history-e2e.XXXXXX")"
  python3 - "$E2E_DATA_DIR/events.sqlite" <<'PY'
  import json, sqlite3, sys, time
  db = sqlite3.connect(sys.argv[1])
  db.execute("CREATE TABLE events (id INTEGER PRIMARY KEY, payload TEXT NOT NULL)")
  now = int(time.time() * 1000)
  rows = []
  for i in range(501):
      started = now - (501 - i) * 2000
      row = {"rule_id": f"fixture-{i}", "alert_id": f"fixture-alert-{i}",
             "kind": "Fixture", "source": f"fixture-host-{i}", "severity": "warning",
             "message": f"Synthetic pagination row {i}", "state": "Recovered",
             "triggered_ms": started, "recovered_ms": started + 1000}
      rows.append((json.dumps(row),))
  db.executemany("INSERT INTO events(payload) VALUES (?)", rows)
  db.commit()
  print("rows", db.execute("SELECT COUNT(*) FROM events").fetchone()[0])
  print("integrity", db.execute("PRAGMA integrity_check").fetchone()[0])
  db.close()
  PY
  export TASK3_DATA_DIR="$E2E_DATA_DIR"
  bash /home/dev/DDS-Monitor/task3/scripts/run_master_patched.sh
  ```

  Chỉ dùng thư mục mới vừa tạo; tuyệt đối không trỏ `TASK3_DATA_DIR` tới dữ liệu hiện hữu. Sau test thoát Master sạch và lưu evidence. Fixture tự tạo không thay thế event thật.
- **Lệnh kiểm tra DB thật:** `python3 -c 'import sqlite3; db=sqlite3.connect("/đường/dẫn/thực/events.sqlite"); print(db.execute("PRAGMA integrity_check").fetchone()[0]); print(db.execute("select count(*) from events").fetchone()[0]); db.close()'`. Linux regression: `ctest --test-dir "$(<.task3-deps/task3-builds/LATEST)/release" -R task3_core --output-on-failure`.
- **Kết quả mong đợi:** integrity `ok`; event thật giữ state/timestamp sau restart; fixture UI có 501 alert_id và Load Older tải đủ, không duplicate; không dùng fixture làm bằng chứng event lifecycle. Unit PASS không biến thành Windows E2E PASS.
- **Kiểm chứng/evidence:** SQL integrity/count, screenshots trước/sau restart và Load Older, CTest output.
- **PASS/FAIL:** PASS cho persistence khi event thật còn nguyên; PASS cho pagination UI khi DB fixture/test riêng >500 dòng và UI tải đủ không duplicate; PASS cho lifecycle chỉ với event thật. Unit PASS không biến thành Windows E2E PASS. FAIL nếu mất/duplicate/sai trạng thái.
- **Nếu FAIL:** giữ bản sao read-only DB/evidence, dừng thao tác ghi, không xóa/rebuild database.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-20 — Ba tab, chọn Host/Application/Flow và giữ trạng thái

- **FR liên quan:** FR-01, FR-09–FR-15; NFR-04.
- **Mục đích:** xác nhận ba workspace, selection, điều hướng và thông tin đọc được trên cửa sổ thật.
- **Điều kiện chuẩn bị:** Master, Agent và DDS app đang chạy; màn hình 1280×800 hoặc 1600×900.
- **Máy thực hiện:** [LINUX MASTER].
- **Các bước:** (1) mở Giám sát hệ thống; (2) chọn Windows Host; (3) chọn Application; (4) chuyển qua DDS chuyên sâu rồi quay lại; (5) chọn DDS Flow; (6) mở Cảnh báo & lịch sử; (7) quay lại các tab; (8) resize cửa sổ; (9) chụp mỗi tab và màn chi tiết.
- **Lệnh:** không cần lệnh; chụp screenshot và lưu evidence. Không dùng screenshot headless thay cho UI thật.
- **Kết quả mong đợi:** đúng ba tab; selection Host/Application còn phù hợp sau điều hướng; không tràn chữ/cột; trạng thái severity có cả chữ và màu; charts có timestamp; không thấy nhãn Task 3 không cần thiết.
- **Kiểm chứng/evidence:** ảnh ba tab và Host/Application/Flow details ở hai kích thước; QML stdout/stderr.
- **PASS/FAIL:** PASS nếu điều hướng/selection hoạt động và không có lỗi QML nghiêm trọng; FAIL nếu tab tái tạo workspace mất lựa chọn, nội dung không đọc được hoặc crash.
- **Nếu FAIL:** ghi viewport, tab sequence và screenshot; không sửa backend trong test.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-21 — Fresh/Stale/N/A và Time Sync Unknown

- **FR liên quan:** FR-02–FR-08, FR-13.
- **Mục đích:** đảm bảo thiếu dữ liệu không bị trình bày thành 0 hoặc Synced.
- **Điều kiện chuẩn bị:** Host Online; ghi một metric/time status; có thể tạm dừng Agent cho ca stale.
- **Máy thực hiện:** [LINUX MASTER] và [WINDOWS SLAVE].
- **Các bước:** (1) ghi metrics đang fresh; (2) dừng Agent và đợi telemetry vượt freshness/timeout; (3) kiểm tra metric stale/Unknown và N/A cho rate chưa đủ delta; (4) xem Time Sync service/source/offset; (5) xác nhận Windows không bị gắn Synced chỉ vì W32Time chạy; (6) bật Agent lại và kiểm tra timestamp fresh.
- **Lệnh:** Windows read-only: `Get-Service W32Time`; `w32tm /query /source`; `w32tm /query /status`; Master UI xem Time Sync. Các lệnh này đọc trạng thái, không chỉnh service.
- **Kết quả mong đợi:** khi không có evidence mới thì Unknown/Stale/N/A; W32Time Running không đủ để khẳng định synchronized; Windows code trả Unknown cho clock_synced/master_source_verified hiện tại.
- **Kiểm chứng/evidence:** raw commands, telemetry timestamps, screenshots.
- **PASS/FAIL:** PASS nếu UI không bịa 0/Synced/traffic; FAIL nếu stale data hiển thị current hoặc Source/offset không có thời điểm.
- **Nếu FAIL:** lưu snapshot/time_sync evidence; đừng reset đồng hồ.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-22 — Kiểm tra NTP baseline, không thay đổi hệ thống

- **FR liên quan:** FR-08.
- **Mục đích:** ghi trạng thái ban đầu và phân biệt service chạy với đồng bộ thực.
- **Điều kiện chuẩn bị:** trước khi thay đổi cấu hình; không cần admin nếu lệnh đọc được.
- **Máy thực hiện:** [LINUX MASTER] và [WINDOWS SLAVE].
- **Các bước:** (1) Linux ghi `chronyc tracking` và `chronyc sources -v`; (2) Windows ghi `Get-Service W32Time`, `w32tm /query /source`, `w32tm /query /status`; (3) kiểm tra Master Time Sync card; (4) ghi offset, nguồn và state nếu có; (5) không chạy `w32tm /resync`, `Set-Service`, `chronyc makestep` hoặc lệnh cấu hình.
- **Lệnh:** `chronyc tracking; chronyc sources -v`; Windows commands trên.
- **Kết quả mong đợi:** source/service/offset được ghi theo đúng hệ điều hành; Agent Windows giữ Unknown nếu chưa thể xác minh; Master source chưa được khẳng định là Windows peer.
- **Kiểm chứng/evidence:** command outputs và UI screenshot.
- **PASS/FAIL:** PASS cho quan sát nếu dữ liệu không bị diễn giải quá mức; FR-08 chưa PASS chỉ vì service Running. Nếu Master không cung cấp NTP hoặc policy chặn, cấu hình test tiếp theo BLOCKED.
- **Nếu FAIL:** dừng, xác minh quyền/policy/network; không thay đổi cấu hình.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-23 — Chrony Master → Windows Time Client (chỉ sau phê duyệt admin)

- **FR liên quan:** FR-08.
- **Mục đích:** kiểm tra đồng bộ liên máy, nguồn Master, tự chạy và offset.
- **Điều kiện chuẩn bị:** admin Linux/Windows và network owner duyệt; backup config; Master có nguồn upstream tin cậy; không bị Group Policy/domain NTP override. Không dùng cấu hình `local stratum` giả.
- **Máy thực hiện:** [LINUX MASTER] và [WINDOWS SLAVE].
- **Các bước:** (1) xác định subnet/IP thật; (2) trên Linux, admin review `task3/config/time-sync/master.conf.example`, thay subnet cho `allow`, giữ upstream thật; backup file Chrony; thêm rule, restart service đúng tên distro; (3) kiểm tra `chronyc tracking`/`sources -v` và `systemctl is-enabled chrony` (hoặc `chronyd`); (4) trên Windows ghi cấu hình cũ, admin cấu hình W32Time source thành Master theo policy, restart/resync; (5) xác minh `w32tm /query /source` đúng Master và `w32tm /query /status`, đồng thời đọc StartMode bằng `Get-CimInstance Win32_Service -Filter "Name='W32Time'" | Select-Object State,StartMode`; (6) đợi hội tụ, ghi stripchart samples và offset; (7) xem Time Sync telemetry/UI; (8) ghi trạng thái startup/service.
- **Lệnh:** Ubuntu/Debian thường dùng `/etc/chrony/chrony.conf`, service `chrony`; RHEL-like thường `/etc/chrony.conf`, service `chronyd`. Cấu hình mẫu đã có `allow 192.0.2.0/24` và `server 192.0.2.10 iburst` — **đây là địa chỉ documentation, phải thay bằng subnet/IP/upstream đã duyệt**. Ví dụ Windows admin sau khi lưu source cũ: `$MasterIP='địa chỉ IPv4 thật'; w32tm /config /manualpeerlist:"$MasterIP,0x8" /syncfromflags:manual /update; Restart-Service W32Time; w32tm /resync /rediscover`. Chạy `w32tm /stripchart /computer:$MasterIP /samples:5 /dataonly` để đo chênh lệch riêng; lệnh này không chứng minh service đã chọn source đó. Không chạy nếu domain policy/IT cấm. Trước khi đổi, lưu `w32tm /query /configuration` và làm theo quy trình rollback của quản trị viên; không có rollback tổng quát an toàn cho mọi máy domain-joined.
- **Kết quả mong đợi:** service khởi động tự động; Windows reported source trỏ Master sau cấu hình; chrony cho thấy client source; offset đo được hữu hạn và giảm/ổn định trong ngưỡng đã thống nhất trước. UI Windows vẫn có thể báo Unknown cho `clock_synced`/`master_source_verified` vì parser không xác minh độc lập; không ép thành Synced.
- **Kiểm chứng/evidence:** backup/config diff đã che secrets, source/status/stripchart output, service startup setting, UI timestamp/offset. Requirements không quy định ngưỡng offset; ghi ngưỡng nghiệm thu được hai bên thống nhất trước.
- **PASS/FAIL:** PASS cho network synchronization chỉ khi source/offset thực tế đúng tiêu chí được phê duyệt; phần UI được đánh giá theo mức evidence mà code cung cấp. FAIL nếu service chạy nhưng source sai/unsynchronized. Không có admin/upstream = BLOCKED.
- **Nếu FAIL:** admin hoàn tác từ backup/policy; không đổi system clock bằng tay.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-24 — Mất nguồn NTP/mạng và đồng bộ trở lại

- **FR liên quan:** FR-08, FR-02, NFR-02.
- **Mục đích:** xác nhận client tiếp tục thử đồng bộ khi nguồn khả dụng và UI không báo Synced giả.
- **Điều kiện chuẩn bị:** E2E-23 đã PASS; admin/network owner cho phép ngắt riêng kết nối NTP thử nghiệm; không ngắt kết nối quản trị/production.
- **Máy thực hiện:** [WINDOWS SLAVE], [LINUX MASTER].
- **Các bước:** (1) ghi source/offset và telemetry baseline; (2) sau phê duyệt admin tạo block outbound chỉ từ Windows test tới IP Master, UDP remote port 123; (3) xác nhận Host telemetry vẫn tới Master; (4) quan sát W32Time status/source và UI; (5) xóa đúng firewall rule test; (6) chờ policy resync và xác nhận source/offset mới; (7) lưu state trước/sau, đảm bảo rule đã được gỡ.
- **Lệnh:** chỉ chạy trong PowerShell Administrator trên máy test, sau khi network owner duyệt và thay IP thật:

  ```powershell
  $MasterIP = '<IPv4 Master đã xác nhận>'
  New-NetFirewallRule -DisplayName 'DDSMon-E2E-NTP-Block' -Direction Outbound -Action Block -Protocol UDP -RemotePort 123 -RemoteAddress $MasterIP -Profile Private
  # Chạy ngắn gọn phép thử, xác nhận telemetry Agent vẫn tới Master.
  Remove-NetFirewallRule -DisplayName 'DDSMon-E2E-NTP-Block'
  Get-NetFirewallRule -DisplayName 'DDSMon-E2E-NTP-Block' -ErrorAction SilentlyContinue
  ```

  Không chạy trên máy domain-joined/managed nếu IT chưa duyệt. Rule chỉ chặn UDP remote port 123 tới IP chỉ định trên profile Private, nhưng vẫn có thể ảnh hưởng đồng bộ giờ của máy test; không tắt firewall. Nếu không có quyền/phê duyệt, dùng test VLAN hoặc ghi BLOCKED. Chỉ đọc trạng thái bằng `w32tm /query /status`, `w32tm /query /source` và Master UI.
- **Kết quả mong đợi:** NTP mất không làm Agent heartbeat bị block; status chuyển Unknown/Unavailable hoặc trạng thái có evidence; sau mạng hồi phục, source/offset được cập nhật. W32Time Running không đủ để PASS.
- **Kiểm chứng/evidence:** ticket/rule before-after, heartbeat timestamps, Windows status, UI timestamp/source.
- **PASS/FAIL:** PASS nếu telemetry độc lập tiếp tục và thời gian tự hội tụ theo policy; FAIL nếu query blocking làm mất heartbeat hoặc source sai. Chưa có phê duyệt = BLOCKED.
- **Nếu FAIL:** hoàn tác rule và trao cho admin/network; không chỉnh đồng hồ.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-25 — Master restart, SQLite continuity và Agent reconnect

- **FR liên quan:** FR-01, FR-02, FR-14, FR-15.
- **Mục đích:** xác nhận shutdown/restart Master sạch, DB còn nguyên và Agent tiếp tục gửi.
- **Điều kiện chuẩn bị:** đã có ít nhất một event Active/Recovered; `TASK3_DATA_DIR` cố định; Agent đang chạy.
- **Máy thực hiện:** [LINUX MASTER] và [WINDOWS SLAVE].
- **Các bước:** (1) dừng Master bằng Ctrl+C; (2) chạy `PRAGMA integrity_check`; (3) khởi động Master cùng env/Data dir cũ; (4) xác nhận Agent Online lại; (5) xác nhận history/events vẫn có đúng state; (6) kiểm tra không tạo duplicate Active cùng alert id; (7) dừng bình thường và chạy integrity lần nữa.
- **Lệnh:** dùng runner mục 3 với cùng `TASK3_DATA_DIR`; integrity command ở E2E-19.
- **Kết quả mong đợi:** Master thoát sạch; SQLite `ok`; events cũ còn nguyên; Agent heartbeat/recovery và UI trở lại.
- **Kiểm chứng/evidence:** logs trước/sau, database path/count/integrity, event screenshots.
- **PASS/FAIL:** PASS nếu DB và state được giữ; FAIL nếu lỗi database, duplicate hoặc Agent không tự phục hồi khi Master lên.
- **Nếu FAIL:** không xóa database; copy DB/log và ghi rõ exit code.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

### E2E-26 — Windows reboot, network recovery và shutdown cuối phiên

- **FR liên quan:** FR-02, FR-07, FR-08, NFR-02.
- **Mục đích:** xác nhận Agent/OS service khởi động lại và phục hồi sau reboot/mất mạng.
- **Điều kiện chuẩn bị:** lưu công việc và có quyền reboot Windows test. Repository không cung cấp Windows service installer/auto-start registration cho Agent. Recovery sau reboot có thể kiểm tra bằng cách khởi động Agent thủ công; auto-start là deployment ngoài repo và tiêu chí riêng. Kiểm tra W32Time startup/service policy riêng theo FR-08.
- **Máy thực hiện:** [WINDOWS SLAVE] và [LINUX MASTER].
- **Các bước:** (1) ghi Host Online; (2) lưu công việc và reboot Windows theo quyền vận hành; (3) sau đăng nhập, xác nhận W32Time state/start mode theo policy; (4) khởi động Agent thủ công bằng mục 4, trừ khi có startup deployment ngoài repo đã được duyệt; (5) xác nhận Host Online lại và Last Seen mới; (6) nếu được phép, mất/kết nối lại LAN và kiểm tra Offline/Online; (7) dừng Publisher/Subscriber/Agent/Master sạch; (8) integrity-check SQLite.
- **Lệnh:** người vận hành dùng Start menu hoặc `Restart-Computer` sau khi lưu công việc. Không có lệnh service registration cho Windows Agent trong repository. Auto-start nếu muốn kiểm thử phải do IT triển khai/quản lý/rollback.
- **Kết quả mong đợi:** sau reboot, Agent khởi động thủ công và reconnect được; đây không phải bằng chứng Agent tự chạy khi boot. W32Time startup policy được ghi riêng; Master DB hợp lệ.
- **Kiểm chứng/evidence:** Windows startup/service config được phê duyệt, timestamps trước/sau reboot, process list, Master host events, SQLite integrity.
- **PASS/FAIL:** PASS cho reconnect nếu reboot thật, Agent chạy thủ công và Host Online/Last Seen mới; auto-start chỉ PASS khi deployment riêng tồn tại và được test; BLOCKED nếu reboot chưa được phép. FAIL nếu crash/mất dữ liệu trong phạm vi đã chạy.
- **Nếu FAIL:** thu logs, dừng test và xử lý deployment qua IT; không tự cài service.
- **Kết quả thực tế:** `NOT RUN / PASS / FAIL / BLOCKED` — ______; ghi chú ______.

## 6. Troubleshooting nhanh

| Triệu chứng | Kiểm tra trước | Hướng xử lý an toàn |
|---|---|---|
| Windows CMake không tìm thấy Qt/Fast DDS | QtPrefix/DdsPrefix, CMake config, kiến trúc x64, Developer PowerShell | Dùng đúng prefix từ installer; cùng MSVC toolset. Không đổi source/dependency để lách lỗi. |
| Agent báo thiếu DLL | `Get-ChildItem` tìm DLL; `dumpbin /DEPENDENTS`; PATH của phiên hiện tại | Bổ sung folder `bin` chính thức tương ứng; không tải DLL lẻ. |
| Agent không tạo Host | log Agent, telemetry domain/topic, Host ID, Fast DDS version, NIC/route/firewall | Đối chiếu hai đầu; xác nhận UDP multicast/unicast và locator thực tế. Ping không chứng minh DDS. |
| Host Online nhưng metrics Unknown/N/A | `--once`, collector health, process/service name, path/interface và freshness | Sửa config local đúng đối tượng; Unknown có thể là kết quả chính xác khi thiếu bằng chứng. |
| Entity discovery không có | app Domain khác telemetry domain? Monitor đã Initialize Domain? `FASTDDS_STATISTICS` đặt trước khi chạy app? | Khởi tạo đúng app Domain; xác nhận multicast/firewall/profile và process thật. |
| Participant/Writer/Reader có nhưng không có match | Topic type, Reliability/Durability/QoS, connection GUID | Cùng Topic name không đủ; đối chiếu matching evidence. |
| Match nhưng không có throughput/latency | Statistics Module/topic chưa bật, Backend prefix/runtime, writer không gửi, statistic stale | Bật đúng statistic trong app, tạo traffic test an toàn; phân biệt No Data với stale. |
| Mapping hiện Unmapped | app chưa ghi runtime manifest; `application_id`, PID/instance/GUID, manifest hết hạn | Dùng instrumented app và xác minh identity; không map chỉ theo executable/topic. |
| Event DB lỗi | status bar/log, writable `TASK3_DATA_DIR`, SQLite path | Không xóa DB; dừng sạch, sao lưu file và WAL, kiểm tra quyền/dung lượng. |
| Time Sync Running nhưng UI Unknown | W32Time service chỉ chứng minh service; source/offset độc lập | Đọc `w32tm /query`; chỉ admin cấu hình source. Không ép UI thành Synced. |
| NTP source không phải Master | domain policy/AD hierarchy, W32Time configuration, Chrony `allow`/route | BLOCKED chờ admin; không thay policy/cấu hình hệ thống ngoài phê duyệt. |

## 7. Ma trận kết quả và nghiệm thu

### 7.1 Nhật ký tổng hợp

| Test ID | Kết quả thực tế | Evidence path/ID | Ghi chú/issue |
|---|---|---|---|
| E2E-01 | NOT RUN | | |
| E2E-02 | NOT RUN | | |
| E2E-03 | NOT RUN | | |
| E2E-04 | NOT RUN | | |
| E2E-05 | NOT RUN | | |
| E2E-06 | NOT RUN | | |
| E2E-07 | NOT RUN | | |
| E2E-08 | NOT RUN | | |
| E2E-09 | NOT RUN | | |
| E2E-10 | NOT RUN | | |
| E2E-11 | NOT RUN | | |
| E2E-12 | NOT RUN | | |
| E2E-13 | NOT RUN | | |
| E2E-14 | NOT RUN | | |
| E2E-15 | NOT RUN | | |
| E2E-16 | NOT RUN | | |
| E2E-17 | NOT RUN | | |
| E2E-18 | NOT RUN | | |
| E2E-19 | NOT RUN | | |
| E2E-20 | NOT RUN | | |
| E2E-21 | NOT RUN | | |
| E2E-22 | NOT RUN | | |
| E2E-23 | NOT RUN | | |
| E2E-24 | NOT RUN | | |
| E2E-25 | NOT RUN | | |
| E2E-26 | NOT RUN | | |

### 7.2 Đối chiếu FR-01–FR-15

Chỉ đánh dấu `PASS` khi có test/evidence nêu trong bảng; Linux unit/integration cũ không thay cho Windows physical test.

| FR | Nội dung nghiệm thu | Test case/evidence tối thiểu | Kết quả |
|---|---|---|---|
| FR-01 | Host list theo ID và chọn xem detail | E2E-06, E2E-20: Host Windows thật, screenshot | NOT RUN |
| FR-02 | Unknown ban đầu, Online, timeout Offline, Last Seen, Recovery | E2E-06, E2E-10, E2E-17, E2E-21 | NOT RUN |
| FR-03 | CPU theo khoảng lấy mẫu | E2E-07: Windows counter + Master UI/time | NOT RUN |
| FR-04 | RAM Used/Total/Percentage | E2E-08: CIM + Master UI | NOT RUN |
| FR-05 | Disk total/used/free từng volume được cấu hình | E2E-08: `Get-Volume` + UI | NOT RUN |
| FR-06 | Network RX/TX/interface/rate và N/A hợp lệ | E2E-08, E2E-21: counter delta + UI | NOT RUN |
| FR-07 | Process/service trong danh sách cấu hình | E2E-04, E2E-09, E2E-15 | NOT RUN |
| FR-08 | Dịch vụ OS, source và offset; liên máy khi được cấu hình | E2E-21–E2E-24; E2E-23 mới là test sync thật | NOT RUN |
| FR-09 | Domain/Participant/Topic/Writer/Reader | E2E-11: app DDS Windows thật + Backend Linux | NOT RUN |
| FR-10 | Host/Application/run mapping; Unmapped khi thiếu evidence | E2E-13: app instrumented + no-metadata case | NOT RUN |
| FR-11 | Writer→Topic→Reader giữa hai máy | E2E-12: GUID/route evidence | NOT RUN |
| FR-12 | Matching, sending/receiving, QoS/status/freshness | E2E-12, E2E-18: app receive counter + metadata | NOT RUN |
| FR-13 | Throughput/Latency, unit, window, timestamp, dynamic chart | E2E-14: publisher sample thật; đo tốc độ/counter | NOT RUN |
| FR-14 | Host/process/service/DDS alert, debounce, Unknown, recovery | E2E-15–E2E-18 | NOT RUN |
| FR-15 | Event object/condition/start/recovery, SQLite, paging | E2E-19/25: real Windows-triggered event + SQLite persistence; >500 UI paging có thể dùng 501-row isolated fixture, nhưng không thay event lifecycle evidence | NOT RUN |

### 7.3 Cleanup và rollback sau phiên test

Chỉ hoàn tác các mục được người kiểm thử tạo trong phiên này. Với OS/NTP/firewall, admin phải dùng change record/backup ban đầu; không có lệnh rollback chung an toàn cho mọi máy managed/domain-joined.

| Hạng mục | Cách cleanup/rollback | Bằng chứng đã hoàn tác |
|---|---|---|
| QoS/XML profile | Giữ profile test trong `$QosTestDir` ngoài repo; sau test bỏ `FASTDDS_DEFAULT_PROFILES_FILE`, khởi chạy lại Publisher/Subscriber với cấu hình gốc; chỉ xóa đúng thư mục test sau khi process đã dừng và evidence đã lưu; không ghi đè `hello_world_profile.xml` nguồn | Env var đã bỏ; default profile tương thích; sample lại báo match và SENT/RECEIVED; source profile hash không đổi |
| NTP/Chrony/W32Time | Chỉ admin hoàn tác từ bản sao cấu hình/policy đã lưu trước E2E-23; phục hồi source/policy cũ rồi kiểm tra `chronyc tracking`, `w32tm /query /configuration`, `/source`, `/status`; không tự chạy một lệnh `w32tm /config` ngược chung cho máy domain-joined | Source trước/sau, service state và approval/change record; không đổi đồng hồ thủ công |
| Firewall test rule | Gỡ đúng rule do E2E-24 tạo: `Remove-NetFirewallRule -DisplayName 'DDSMon-E2E-NTP-Block'`; kiểm tra `Get-NetFirewallRule -DisplayName 'DDSMon-E2E-NTP-Block' -ErrorAction SilentlyContinue` không trả rule. Khôi phục policy ban đầu theo IT nếu có thay đổi khác | Output kiểm tra rule đã mất; firewall profile vẫn enabled; telemetry/NTP baseline sau rollback |
| Environment variables | Variables được đặt trong terminal/process test. PowerShell: `Remove-Item Env:FASTDDS_DEFAULT_PROFILES_FILE,Env:FASTDDS_STATISTICS,Env:TASK3_DATA_DIR,Env:TASK3_TIMEOUT_MS -ErrorAction SilentlyContinue`; Bash: `unset FASTDDS_DEFAULT_PROFILES_FILE FASTDDS_STATISTICS TASK3_DATA_DIR TASK3_TIMEOUT_MS TASK3_FLOW_RULES`. Đóng terminal/process cũ để không truyền setting sang tiến trình kế tiếp; nếu trước test có giá trị cũ thì khôi phục giá trị đã ghi thay vì unset | `Get-ChildItem Env:FASTDDS*` / `Get-ChildItem Env:TASK3*` hoặc `env | rg '^(FASTDDS|TASK3)_'` không còn giá trị test ngoài dự kiến |
| SQLite fixture 501 dòng | Dừng Master sạch trước. Chỉ xóa thư mục do `mktemp` tạo trong E2E-19 sau khi evidence đã lưu; không đụng `TASK3_DATA_DIR` thật hoặc DB người dùng. Bash an toàn: `case "$E2E_DATA_DIR" in "$HOME"/DDS-Monitor-history-e2e.*) rm -ri -- "$E2E_DATA_DIR" ;; *) echo 'Refusing: path is not the fixture directory';; esac` | Xác nhận đường dẫn canonical/name đúng fixture, Master đã dừng và chỉ fixture bị xóa; DB thật vẫn còn nguyên |
| Evidence | Không xóa evidence nghiệm thu cùng fixture. Lưu riêng `01-preflight` và `02-acceptance`; lọc secrets/IP nhạy cảm theo policy trước khi chia sẻ | Kiểm tra file/manifest evidence và quyền truy cập; không commit dữ liệu runtime |

### 7.4 Checklist bắt buộc trước nghiệm thu

Đánh dấu từng mục và lưu evidence; unit/integration trên Linux hỗ trợ kiểm thử nhưng không thay bằng chứng phần cứng Windows/LAN.

- [ ] **Windows vật lý bắt buộc — FR-01–FR-08:** Agent build và chạy trên Windows thật; Host/heartbeat/timeout/recovery; CPU/RAM/Disk/Network đối chiếu Windows counters; Process/Service và Time Sync status/source. Đối với FR-08, chỉ ghi đồng bộ Master–Slave PASS sau E2E-23/24 có admin approval, nguồn Master và offset đo được.
- [ ] **Linux–Windows DDS vật lý bắt buộc — FR-09, FR-11, FR-12:** HelloWorld/application chạy hai máy; evidence Domain/entity discovery, Writer–Topic–Reader, match và sample receive độc lập. Unit test hoặc local same-host discovery không đủ.
- [ ] **Instrumented application bắt buộc — FR-10 mapping dương:** manifest do app Windows thật sinh khi chạy, PID/instance/GUID khớp. HelloWorld không có RuntimeIdentity; nếu chưa có test app Windows instrumented thì FR-10 mapped case BLOCKED, chỉ có thể chứng minh nhánh Unmapped.
- [ ] **Statistics/biểu đồ — FR-13:** publisher/subscriber cross-machine tạo mẫu thật; statistics fresh, unit/timestamp/chart evidence. Nếu cần đánh giá throughput định lượng thì phải có publisher tốc độ cố định đã build/xác minh; HelloWorld tương tác không đủ để PASS benchmark/SLA. Requirements hiện không quy định ngưỡng rate/latency.
- [ ] **Alerts — FR-14:** Windows process/service/Agent/DDS event thật được kích hoạt và khôi phục; có evidence Active/Unknown/Not verified/Recovered, không dựa riêng unit test.
- [ ] **SQLite — FR-15:** alert thật từ phiên Windows được ghi, Master restart giữ lịch sử và integrity `ok`; paging >500 có thể dùng fixture trong DB tạm để kiểm thử UI, đồng thời phải ghi rõ fixture không chứng minh lifecycle event thật.
- [ ] **Unit/integration hỗ trợ, không thay E2E:** Protocol/schema/sequence/session, clock rollback, debounce Missing/Unknown, cache merge/paging logic, parser Time Sync/non-blocking, UI integration và local/continuation integration. Ghi SHA/build/test output, nhưng không dùng chúng làm bằng chứng Windows Agent, NTP liên máy hay DDS discovery qua LAN.
- [ ] **Ngưỡng:** phân biệt Requirements định tính với ngưỡng đề xuất ở mục 4.1; ghi người duyệt/ngày/ngưỡng trước đo. Nếu chưa được thống nhất, báo kết quả số đo và giới hạn, không tuyên bố đạt SLA.

### 7.5 Kết luận chấp nhận

- **`PASS_ACCEPTANCE`** — mọi FR-01–FR-15 có evidence PASS phù hợp, bao gồm các mục vật lý checklist 7.4; không có P0/P1; Debug/Release/CTest và shutdown/database checks đạt. SQLite paging >500 có thể kiểm bằng fixture riêng; event lifecycle vẫn phải có event thật.
- **`FAIL_ACCEPTANCE`** — có lỗi tái hiện được trong chức năng đã thực hiện, sai dữ liệu/trạng thái, crash/mất event, hoặc P0/P1 chưa khắc phục. Ghi test, bước tái hiện, evidence, máy/source version và tác động.
- **`INCONCLUSIVE`** — chưa đủ môi trường, app Publisher/Subscriber Windows, RuntimeIdentity instrumentation, benchmark tốc độ cố định (nếu cần số throughput SLA), quyền admin/upstream NTP, firewall/network approval hoặc test data; không đánh đồng BLOCKED/PENDING với PASS.

Khi một ca bị `BLOCKED`, ghi chính xác điều kiện còn thiếu và chủ thể cần cung cấp. Giới hạn đã xác minh từ source commit: chưa có Windows build/runtime acceptance; repo không kèm Windows publisher/subscriber (có thể build Fast DDS HelloWorld chính thức như mục 1.2); chưa có Windows RuntimeIdentity app; Windows Agent không có service installer/auto-start; Windows Time parser cố ý không khẳng định `clock_synced` hoặc `master_source_verified`; Requirements không định lượng SLA offset/throughput/latency. Ghi rõ các giới hạn này hoặc hoàn thiện bằng chứng trước kết luận hệ thống.

## 8. Nguồn repository đã đối chiếu

- `docs/task3/TASK3_REQUIREMENTS.md` — FR-01–FR-15 và NFR.
- `docs/task3/BUILD_AND_RUN.md`, `docs/task3/CONFIGURATION.md`, `docs/task3/TEST_PLAN.md`, `docs/task3/PRE_TEST_READINESS.md`.
- `task3/scripts/build-windows.ps1`, `task3/agent/CMakeLists.txt`, `task3/scripts/build_task3_patched.sh`, `task3/scripts/run_master_patched.sh`, `task3/scripts/patched_runtime_common.sh`.
- `task3/agent_main.cpp`, `task3/Collectors.cpp`, `task3/DdsTransport.cpp`, `task3/Protocol.cpp`, `task3/RuntimeIdentity.cpp`, `task3/MonitoringCore.cpp`, `task3/AlertStore.cpp`, `task3/Dashboard.qml`, `task3/HostWorkspace.qml`, `task3/FlowDetails.qml`, `task3/EventHistoryView.qml`.
- Tests already present (not executed while preparing this guide): `task3/tests/CoreTests.cpp`, `local_integration.py`, `continuation_integration.py`, `ui_integration.py`, `runtime_library_verification.py`.
- NTP examples: `task3/config/time-sync/master.conf.example`, `task3/config/time-sync/slave.conf.example`; Windows read-only probe: `task3/scripts/windows-time-status.ps1`.

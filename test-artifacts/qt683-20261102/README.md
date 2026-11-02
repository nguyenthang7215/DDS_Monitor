# Build DDS Monitor bằng Qt 6.8.3 (02/11/2026)

Build Release vào `.task3-deps/qt683-builds/release` (không đổi `task3-builds/LATEST`), Backend Patched 2.5.0. Lệnh chạy: `run_all.sh`.

| Bước | Kết quả |
| :--- | :--- |
| Build | PASS, không lỗi |
| CTest | PASS |
| `tst_DomainGraphLoading.qml` (code QML **gốc**) | **6/6 PASS** trên Qt 6.8.3 (Qt 6.7.2: 3/6) → GRAPH-01 phụ thuộc phiên bản Qt |
| local / continuation / subscriber / publisher | Không chạy được: Master thoát ngay vì thiếu module `Qt5Compat.GraphicalEffects` (`MainStartView.qml:22`) |

Cần cài *Qt 5 Compatibility Module* cho Qt 6.8.3 rồi chạy lại `run_all.sh`.

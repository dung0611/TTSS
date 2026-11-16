# FACO cho môn Tính toán song song

Dự án C++17 tự triển khai Focused Ant Colony Optimization để giải TSP đối xứng có tọa độ 2D. Có bản tuần tự thực sự (không liên kết OpenMP) và bản OpenMP dùng chung toàn bộ thuật toán. FACO là heuristic, không bảo đảm nghiệm tối ưu.

Đã đọc bài *Improving Ant Colony Optimization Efficiency for Solving Large TSP Instances*, R. Skinderowicz, DOI [10.1016/j.asoc.2022.108653](https://doi.org/10.1016/j.asoc.2022.108653), bản đính kèm 28 trang, và [FocusedACO tại commit 05b7c1b8d52b27d121b6138222f986fcbf196863](https://github.com/RSkinderowicz/FocusedACO/tree/05b7c1b8d52b27d121b6138222f986fcbf196863). Đây là triển khai phục vụ học tập có những lựa chọn khác mã tác giả; xem [bảng đối chiếu](docs/paper_mapping.md). Không phải bản tái hiện thực nghiệm của bài báo.

## Xây dựng trên Ubuntu

Yêu cầu GCC hỗ trợ C++17/OpenMP, CMake >=3.16 và Python 3; cài thêm Pillow (`python3-pil`) để dựng biểu đồ.

```bash
sudo apt update
sudo apt install build-essential cmake python3 python3-pil
cmake -S . -B build-seq -DFACO_OPENMP=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-seq -j
ctest --test-dir build-seq --output-on-failure

cmake -S . -B build-omp -DFACO_OPENMP=ON -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-omp -j
ctest --test-dir build-omp --output-on-failure
```

Môi trường thực hiện lần này là Windows 10, GCC 16.2.0 portable; cả hai cấu hình đã được biên dịch/chạy tại đó. Chưa chạy trực tiếp trên Ubuntu. CMake và mã không phụ thuộc API Windows; cần xác nhận lại trên máy Ubuntu đích. Lệnh Windows tương đương thêm `.exe` vào tên chương trình; dùng generator phù hợp bộ công cụ của bạn.

Gói bàn giao có hai executable Windows x64 đã biên dịch trong `bin/windows/`. Sau khi giải nén, có thể chạy ngay từ thư mục gốc bằng PowerShell:

```powershell
.\bin\windows\faco-seq.exe --input data/uniform2048.tsp --ants 128 --iterations 200 --threads 1 --seed 11 --initial-solutions 4 --output-dir results/uniform2048-seq
.\bin\windows\faco-omp.exe --input data/uniform2048.tsp --ants 128 --iterations 200 --threads 4 --seed 11 --initial-solutions 4 --output-dir results/uniform2048-omp
```

## Chạy nhanh

```bash
./build-seq/faco --input data/uniform2048.tsp --ants 128 --iterations 200 \
  --threads 1 --seed 11 --initial-solutions 4 --verify on --output-dir results/uniform2048-seq
./build-omp/faco --input data/uniform2048.tsp --ants 128 --iterations 200 \
  --threads 4 --seed 11 --initial-solutions 4 --schedule dynamic --output-dir results/uniform2048-omp
./build-omp/faco --help
```

`square4` có tối ưu 40 đã kiểm tra bằng vét cạn trong tests. Các điểm dùng cho kiểm thử local search được khai báo ngay trong mã test, không cần tệp dữ liệu demo riêng.

Đầu vào chỉ hỗ trợ `TYPE=TSP`, `EDGE_WEIGHT_TYPE=EUC_2D`, ID 1..n (có thể nhập không theo thứ tự), ít nhất 2 đỉnh. Khoảng cách `floor(hypot(dx,dy)+0.5)`. Các loại ATT/GEO/CEIL_2D/EXPLICIT/ATSP bị từ chối. Tọa độ phải hữu hạn, trị tuyệt đối <=10^12; parser giới hạn n<=10^6 để bảo đảm tổng số nguyên 64 bit. Giới hạn parser không phải cam kết hiệu năng ở kích thước đó.

## Tham số

| Cờ | Mặc định | Ý nghĩa |
|---|---:|---|
| `--input` | bắt buộc | Tệp TSPLIB |
| `--ants` | 0 | 0 dùng `64*ceil(4*sqrt(n)/64)`, tối thiểu 64 |
| `--iterations` | 5000 | Số vòng FACO |
| `--threads` | 1 | Số luồng yêu cầu; bản tuần tự chỉ chấp nhận 1 |
| `--seed` | 1 | Số nguyên 64 bit, seed 0 vẫn tái lập |
| `--repeat` | 1 | Lặp với seed, seed+1, ... |
| `--rho`, `--beta` | 0.5, 1 | Tỷ lệ pheromone còn lại; số mũ heuristic |
| `--candidate-size`, `--backup-size` | 16, 64 | Hai danh sách rời nhau; tự giảm khi n nhỏ |
| `--min-new-edges` | 8 | Ngưỡng chuyển sang sao chép, không phải số khác biệt cuối |
| `--local-search` | on | `on/off` hoặc `1/0`, áp dụng cả khởi tạo và vòng lặp |
| `--ls-size` | 20 | Số láng giềng xét trong LS, dùng cả khởi tạo |
| `--initial-solutions` | 0 | 0 dùng số CPU logic phát hiện bằng C++ |
| `--schedule` | dynamic | `dynamic,1` hoặc `static,1` cho vòng kiến |
| `--bounds` | paper | `paper` giữ căn bậc n; `reference` dùng căn bậc cl khi bật LS |
| `--p-best`, `--gbest-probability` | 0.1, 0.01 | Cận pheromone; xác suất chọn global best làm nguồn |
| `--verify` | off | Tính lại chi phí sau từng move và kiểm tra cận mỗi vòng |
| `--log-every` | 4 | In tiến độ sau mỗi N vòng; 0 để tắt; luôn in vòng đầu/cuối khi bật |
| `--profile` | off | Đo từng pha và thống kê số kiến mỗi luồng |
| `--output-dir` | results | Thư mục kết quả |

Không có tham số alpha: số mũ pheromone cố định 1 như phép nhân trong mã tác giả. `0<rho<1`, `beta>=0` hữu hạn, `0<p_best<1`. Khi tắt LS, rho vẫn giữ cấu hình đã chọn; nên khảo sát rho riêng, không suy rộng hiệu quả mặc định bật LS.

Mỗi lần chạy ghi CSV thời gian/chất lượng, JSON đầy đủ cấu hình và tour, tệp TSPLIB TOUR và CSV lịch sử (gồm vòng 0). ID trong kết quả là 1-based; nội bộ là 0-based. Tên `run-sSEED-tTHREADS-SCHEDULE-rREPEAT` ổn định: chạy lại cùng tên trong cùng thư mục sẽ ghi đè. Dùng thư mục riêng cho từng instance/cấu hình.

Không có mốc hợp lệ thì CSV để trống reference/error, JSON dùng `null`. Muốn tính sai số, cung cấp cùng lúc `--reference-length`, `--reference-source`, `--reference-date`. Người chạy phải xác thực giá trị và loại mốc; chương trình không tự chứng minh một best-known là tối ưu. Không tự nạp mọi giá trị từ `best-known.json` của tác giả.

## Kiểm thử và đo hiệu năng

```bash
python3 scripts/integration.py --binary build-omp/faco
python3 scripts/benchmark.py --binary build-omp/faco --sequential-binary build-seq/faco \
  --input data/uniform2048.tsp --threads 1 2 4 6 --seeds 11 22 33 \
  --ants 128 --iterations 200 --initial-solutions 4 --output-dir results/benchmark-new

# Ubuntu: kiểm tra bộ nhớ; tắt OpenMP giúp cô lập lỗi bộ nhớ
cmake -S . -B build-asan -DFACO_OPENMP=OFF -DFACO_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-asan -j
ctest --test-dir build-asan --output-on-failure
```

Chỉ chọn 6 luồng nếu phần cứng cho phép. Dữ liệu `uniform2048` được sinh bằng script gốc, seed 2026, chưa biết tối ưu. Hai instance `d15112.tsp` và `Tnm30001.tsp` lấy từ repository FocusedACO; xem nguồn trong `data/SOURCES.md`. Có thể tái sinh bằng `python3 scripts/generate_data.py --output data/uniform2048.tsp --dimension 2048 --seed 2026`.

Kết quả benchmark đủ 1/2/4/8 luồng cho hai instance lớn được lưu tại [d15112](results/d15112/report.md) và [Tnm30001](results/Tnm30001/report.md), kèm raw/summary CSV, metadata, tour/lịch sử từng lần đo và bốn biểu đồ PNG/SVG. Do workload 128 kiến × 200 vòng cùng local search của benchmark 2048 mất nhiều phút mỗi lượt ở các instance này, hai benchmark lớn dùng workload nhẹ hơn: 32 kiến, 40 vòng, 1 nghiệm khởi tạo, local search tắt; mỗi seed có 3 lần đo và 3 seed (11/22/33). Mọi mức luồng dùng cùng workload, và kết quả đã qua kiểm chứng độc lập. Không so sánh trực tiếp thời gian/chất lượng giữa các instance với cấu hình khác nhau.

## Các tệp để học

- [algorithm.md](docs/algorithm.md): từng bước, công thức, bộ nhớ và ví dụ 6 đỉnh.
- [parallel.md](docs/parallel.md): shared/private, barrier, RNG và giới hạn speedup.
- [paper_mapping.md](docs/paper_mapping.md): ánh xạ mục/hình và khác biệt với GitHub.
- [experiments.md](docs/experiments.md): số đo thực, môi trường và kiểm thử đã chạy.
- `include/faco/`, `src/`: các thành phần dữ liệu, tour, pheromone, LS, FACO, CLI và xuất kết quả.
- `tests/tests.cpp`: tám nhóm kiểm thử cơ chế; có vét cạn rất nhỏ chỉ làm mốc kiểm chứng.
- `docs/giai_thich_ma_nguon.md`: giải thích các file và hàm/lớp chính trong dự án.
- `scripts/benchmark.py`: chạy các cấu hình có công việc cố định, lưu số thô và tổng hợp theo seed.

Hạn chế: chưa kiểm chứng hiệu quả 100k-200k đỉnh; khởi tạo tuần tự và 3-opt có giới hạn; không dùng tối ưu SIMD, pool vùng nhớ hoặc cấu trúc tour nhiều tầng. Xem [NOTICE](NOTICE.md) về nguồn và điều kiện tái sử dụng.

## VS Code trên máy Windows hiện tại

Đã cấu hình IntelliSense cho cả cách mở `D:\TTSS` và mở trực tiếp `D:\TTSS\FACO-lab`. Các tệp `.vscode/c_cpp_properties.json`, `settings.json` và `tasks.json` chỉ định GCC, C++17, OpenMP và include của dự án. IntelliSense dùng thêm `build-omp/compile_commands.json` do CMake tạo để lấy đúng tham số của từng tệp.

Bộ công cụ được đặt cố định ngoài thư mục dự án:

- GCC: `D:\TTSS\tools\w64devkit\bin\g++.exe`
- CMake: `D:\TTSS\tools\cmake\bin\cmake.exe`

Sau khi thay cấu hình, nhấn **Ctrl+Shift+P**, chạy **Developer: Reload Window**. Nhấn **Ctrl+Shift+B** để configure/build toàn bộ dự án. Để chạy tests, dùng **Terminal: Run Task** rồi chọn **FACO: Test**. Mở terminal mới để nhận PATH của workspace; terminal cũ không tự cập nhật môi trường.

Cấu hình này không tắt cảnh báo IntelliSense. Header `pheromone.hpp` đã được kiểm tra độc lập bằng GCC, và bản OpenMP xây dựng ở vị trí mới đạt 3/3 CTest. Thư viện chuẩn được lấy từ compilerPath theo [hướng dẫn IntelliSense của VS Code](https://code.visualstudio.com/docs/cpp/configure-intellisense).

Toolchain không nằm trong ZIP để giữ gói nhỏ. Nếu chuyển sang máy khác hoặc Ubuntu, cần chỉnh compilerPath/đường dẫn công cụ trong `.vscode` cho máy đó; các lệnh CMake Ubuntu ở trên vẫn áp dụng.

Các tệp `.tsp` trong dự án là dữ liệu TSPLIB. Extension TypeSpec cũng dùng đuôi `.tsp`, nên có thể báo lỗi cú pháp khi mở dữ liệu này. Workspace đã đặt `files.associations` để mở chúng dưới dạng **Plain Text**, giữ nguyên nội dung dữ liệu và chức năng đọc của FACO. Nếu tab đang mở còn hiển thị lỗi cũ, đóng rồi mở lại tệp, hoặc chọn **Plain Text** ở mục Language Mode.


## Tự chạy và xem tiến độ mỗi 4 vòng

Mở **Terminal → New Terminal** trong VS Code rồi chạy:

```powershell
cd D:\TTSS\FACO-lab
.\build-omp\faco.exe --input data/uniform2048.tsp --ants 128 --iterations 200 --threads 4 --seed 11 --initial-solutions 4 --log-every 4 --output-dir results/uniform2048-live
```

Mặc định `--log-every 4`: in vòng khởi tạo 0, rồi 4, 8, 12, ... và vòng cuối dù chưa đủ một nhóm 4. `iter_best` là độ dài tốt nhất của riêng vòng vừa xong, `best` là độ dài tốt nhất từ đầu đến hiện tại, `elapsed` là số giây tìm kiếm kể từ sau khởi tạo. Mỗi dòng được flush ngay để terminal hiện tiến độ trong lúc chương trình chạy. Toàn bộ lịch sử trong CSV vẫn được ghi cho **mọi vòng**, không chỉ các vòng đã in.

`--threads 4` chọn 4 luồng tính toán; `--log-every 4` chọn chu kỳ in 4 vòng. Có thể dùng `--log-every 1` để in mỗi vòng hoặc `--log-every 0` để tắt tiến độ. Việc in không thay đổi RNG hoặc tour, nhưng thời gian console được tính trong thời gian tìm kiếm. Script benchmark tự truyền `--log-every 0` để giữ phép đo không có I/O tiến độ. Số liệu benchmark cũ được thu trước khi bổ sung tính năng in, nên giữ nguyên như bằng chứng lịch sử.

## Phân tích OpenMP

Phần song song chính nằm trong `src/faco.cpp::solve`: một team OpenMP duy trì suốt vòng lặp, `omp for` chia các kiến, bay hơi và chặn cận; `omp single` xử lý best, nguồn, deposit và tiến độ. Bật `--profile on` để xem thời gian từng pha và số kiến thực tế mỗi luồng xử lý. Dòng tiến độ có `block_ms` là thời gian thực của nhóm vòng vừa chạy.

```powershell
.\build-omp\faco.exe --input data/uniform2048.tsp --ants 128 --iterations 200 --threads 4 --seed 11 --initial-solutions 4 --schedule dynamic --log-every 4 --profile on --output-dir results/openmp-live
```

Xem [giải thích shared/private, barrier và profiler](docs/parallel.md). Khi đo speedup, tắt profiler và log (`--profile off --log-every 0`); script benchmark đã tắt log mặc định. Profiler ghi số liệu chẩn đoán, không dùng làm thay thế cho các phép đo thời gian không instrument.

## Chạy và benchmark OpenMP 1/2/4/8 luồng trên Ubuntu/Linux

Build một binary Release duy nhất có OpenMP, sau đó chạy kiểm thử:

```bash
cmake -S . -B build-omp -DFACO_OPENMP=ON -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-omp -j
ctest --test-dir build-omp --output-on-failure
```

Chạy riêng một mức luồng trên `uniform2048.tsp` (128 kiến, 200 vòng, seed 11, 4 nghiệm khởi tạo):

```bash
./scripts/run_openmp.sh 1
./scripts/run_openmp.sh 2
./scripts/run_openmp.sh 4
./scripts/run_openmp.sh 8
```

Script gọi cùng `build-omp/faco`, truyền rõ `--threads`, và tạo thư mục kết quả có timestamp. Có thể gọi solver trực tiếp nếu muốn tự chọn tham số:

```bash
./build-omp/faco --input data/uniform2048.tsp --ants 128 --iterations 200 \
  --threads 8 --seed 11 --initial-solutions 4 --schedule dynamic \
  --local-search on --log-every 4 --profile off --output-dir results/manual-8
```

Chạy benchmark đầy đủ bằng một lệnh:

```bash
./scripts/run_openmp.sh benchmark
```

Benchmark giữ nguyên workload ở mọi luồng: seed 11/22/33; một warm-up cho mỗi luồng; năm phép đo lặp cho từng cặp seed/luồng; thứ tự luồng được xoay và các tiến trình chạy lần lượt. Mỗi lần đo có thư mục riêng. Script lưu `raw.csv`, `summary.csv`, `metadata.json`, tự gọi kiểm chứng độc lập, rồi dựng bốn hình từ CSV/lịch sử. Mặc định tạo `results/openmp-benchmark-YYYYMMDD-HHMMSS`; có thể chọn vị trí bằng `./scripts/run_openmp.sh benchmark --output-dir results/my-openmp-study`.

Báo cáo thực nghiệm Ubuntu đã chạy: [report.md](results/openmp-benchmark-20261114-094522/report.md). Tệp kết quả nằm dưới `results/` (được `.gitignore` loại khỏi Git), gồm CSV, JSON, tour, lịch sử và hình.

Để kiểm chứng lại hoặc dựng lại hình mà không chạy solver:

```bash
python3 scripts/verify_results.py --input data/uniform2048.tsp \
  --results results/openmp-benchmark-YYYYMMDD-HHMMSS
python3 scripts/plot_openmp_benchmark.py \
  --results results/openmp-benchmark-YYYYMMDD-HHMMSS
```

Các PNG được xuất ở 220 dpi cùng SVG trong thư mục `figures/`. `execution_time` vẽ trung vị và khoảng Q1–Q3; speedup lấy tỷ số thời gian trung vị ghép theo từng seed rồi lấy trung vị ba tỷ số. Efficiency chia search speedup cho số luồng thực tế, không phải mức sử dụng CPU. Khởi tạo vẫn tuần tự nên total speedup có thể thấp hơn search speedup. Số luồng yêu cầu và số luồng OpenMP thực tế đều được lưu trong raw CSV, từng JSON và metadata.

Script cũ `scripts/benchmark.py` vẫn dùng cho thí nghiệm ngắn/tuỳ chọn. Kết quả trước/sau persistent region và so static/dynamic được lưu riêng trong `openmp_study.md`; các số đo cũ trong `experiments.md` được giữ nguyên. Chưa chạy TSan/Helgrind; kiểm thử kết quả và Debug assertions không thay thế công cụ phát hiện race.


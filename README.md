# TTSS — Song song hóa thuật toán FACO bằng OpenMP

## 1. Giới thiệu dự án

TTSS là dự án học tập C++17 triển khai **Focused Ant Colony Optimization (FACO)** để giải bài toán người du lịch (**TSP**): tìm chu trình ngắn nhất đi qua mỗi thành phố đúng một lần rồi trở về điểm xuất phát. FACO xây dựng tour dựa trên pheromone và khoảng cách, nhưng tập trung thay đổi một phần nghiệm nguồn, sao chép phần còn lại và tìm kiếm cục bộ quanh vùng thay đổi. Đây là heuristic, **không bảo đảm nghiệm tối ưu**.

Đề tài hướng đến giảm thời gian tính toán bằng OpenMP trên máy bộ nhớ dùng chung, so sánh bản tuần tự thực sự với bản OpenMP và đánh giá khả năng mở rộng theo số luồng. Hai bản dùng chung thuật toán; bản tuần tự không liên kết thư viện OpenMP. Triển khai phục vụ học tập, có khác biệt với mã tác giả, không phải bản tái hiện toàn bộ thực nghiệm của bài báo.

**Phạm vi dữ liệu:** TSPLIB với `TYPE=TSP`, `EDGE_WEIGHT_TYPE=EUC_2D`, tọa độ 2D trong `NODE_COORD_SECTION`, ID đủ từ 1 đến n (không bắt buộc nhập theo thứ tự). Khoảng cách là `floor(hypot(dx,dy)+0.5)`; tổng độ dài dùng số nguyên 64 bit. Parser chấp nhận 2–1.000.000 đỉnh, tọa độ hữu hạn có trị tuyệt đối không quá 10^12. Đây chỉ là giới hạn kiểm tra đầu vào, không phải cam kết hiệu năng. Không hỗ trợ ATSP, ATT, GEO, CEIL_2D hay ma trận EXPLICIT.

## 2. Cách thực hiện và song song hóa

Luồng xử lý trong [mã nguồn](FACO-lab/src/) và [tài liệu thuật toán](FACO-lab/docs/algorithm.md):

1. **Đọc dữ liệu:** kiểm tra header, số đỉnh và tọa độ; chuyển ID sang chỉ số nội bộ 0-based.
2. **Tiền xử lý:** dựng k-d tree và các danh sách láng giềng candidate, backup, local search; tính heuristic khoảng cách. Pheromone chỉ lưu trên các cạnh candidate, không tạo ma trận n × n.
3. **Khởi tạo:** tạo các tour nearest-neighbor từ điểm bắt đầu ngẫu nhiên; nếu bật local search, cải thiện bằng 2-opt và 3-opt giới hạn. Chọn tour tốt nhất làm nghiệm nguồn và global best, khởi tạo pheromone. Bước này vẫn tuần tự; cố định `--initial-solutions` để workload không thay đổi theo máy.
4. **Xây dựng tour cho từng kiến:** chọn đỉnh chưa thăm theo trọng số pheromone × heuristic^beta trên candidate; hết candidate thì dùng backup, sau đó tìm đỉnh gần nhất còn lại. Khi đạt `--min-new-edges` cạnh không thuộc nguồn, thử sao chép phần tour nguồn chưa thăm. Ngưỡng này không phải số cạnh khác biệt cuối cùng.
5. **Tìm kiếm cục bộ:** nếu bật, mỗi kiến chạy 2-opt với checklist tập trung quanh các thay đổi, rồi tính độ dài tour. 3-opt giới hạn chỉ dùng ở khởi tạo.
6. **Chọn nghiệm và cập nhật pheromone:** chọn iteration best, cập nhật global best và cận; chọn nguồn là global best với xác suất mặc định 0,01, còn lại là iteration best. Bay hơi, bồi đắp theo nguồn rồi chặn pheromone trong cận. `rho` là tỷ lệ pheromone **còn lại**, không phải tỷ lệ mất đi.
7. **Xuất kết quả:** lưu tour tốt nhất, cấu hình, thời gian và lịch sử từ vòng 0 đến vòng cuối.

### OpenMP chia việc gì?

Trong [src/faco.cpp](FACO-lab/src/faco.cpp), một team `omp parallel` được duy trì xuyên suốt các vòng FACO:

| Phần việc | Bản tuần tự | Bản OpenMP |
|---|---|---|
| Đọc dữ liệu, tiền xử lý, khởi tạo | Tuần tự | Vẫn tuần tự |
| Xây tour + 2-opt + tính độ dài từng kiến | Vòng lặp thường | `omp for schedule(runtime)` chia các kiến |
| Chọn best, nguồn và cận | Tuần tự | `omp single` |
| Bay hơi và chặn cận pheromone | Vòng lặp thường | Hai `omp for schedule(static)` chia các slot |
| Bồi đắp, ghi lịch sử và tiến độ | Tuần tự | `omp single` |

Tổng số kiến không tăng khi tăng số luồng: 128 kiến × 200 vòng luôn là 25.600 tour trong pha tìm kiếm. Dữ liệu bài toán, nguồn và pheromone được đọc chung trong pha kiến; RNG, visited và checklist riêng cho từng kiến; mỗi kiến ghi vào ô tour/chi phí riêng. Barrier ngầm bảo đảm kết thúc pha kiến trước chọn best và hoàn tất cập nhật pheromone trước vòng tiếp theo.

`--schedule` chỉ chọn lịch cho **vòng kiến**:

- `static`: `static,1`, phân kiến theo chỉ số cố định theo vòng tròn, ít chi phí điều phối hơn nhưng có thể mất cân bằng nếu local search của các kiến tốn thời gian khác nhau.
- `dynamic` (mặc định): `dynamic,1`, luồng rảnh nhận tiếp một kiến, hỗ trợ cân bằng tải nhưng có thêm chi phí lập lịch. Không có lịch luôn nhanh hơn trong mọi trường hợp.

RNG dựa trên seed/phase/iteration/ant, không dựa trên ID luồng. Nhờ vậy đổi số luồng hoặc lịch không tự đổi chuỗi ngẫu nhiên của kiến; không cam kết kết quả bit-for-bit trên mọi compiler/CPU. Chi tiết đồng bộ và shared/private: [Thiết kế OpenMP](FACO-lab/docs/parallel.md).

## 3. Cấu trúc repository

Các đường dẫn dưới đây tính từ gốc Git, nơi có README này, `FACO-lab/`, `tools/` và `package.json`.

| Đường dẫn | Nội dung |
|---|---|
| [FACO-lab/src/](FACO-lab/src/), [include/faco/](FACO-lab/include/faco/) | Mã C++ đọc TSP, tour, láng giềng, pheromone, local search, solver, CLI và xuất kết quả |
| [FACO-lab/CMakeLists.txt](FACO-lab/CMakeLists.txt) | Build C++17, tùy chọn OpenMP/sanitizers và đăng ký CTest |
| [FACO-lab/data/](FACO-lab/data/) | Bốn instance TSP và [nguồn dữ liệu](FACO-lab/data/SOURCES.md) |
| [FACO-lab/tests/tests.cpp](FACO-lab/tests/tests.cpp) | Kiểm thử cơ chế, vét cạn ca nhỏ và đối chiếu song song |
| [FACO-lab/scripts/](FACO-lab/scripts/) | Sinh dữ liệu, integration, benchmark, kiểm chứng tour, vẽ hình và đóng gói |
| [FACO-lab/docs/](FACO-lab/docs/) | Thuật toán, thiết kế song song, đối chiếu bài báo, thực nghiệm, PDF và bằng chứng validation |
| [FACO-lab/results/](FACO-lab/results/) | Báo cáo, raw/summary CSV, metadata, tour, lịch sử, hình; `archive/` giữ các đợt cũ |
| [FACO-lab/README.md](FACO-lab/README.md) | Hướng dẫn kỹ thuật đầy đủ, tham số và ghi chú Windows/VS Code |
| [tools/](tools/) | Công cụ và executable Windows có sẵn trong repo; không cần cho build Ubuntu |
| [package.json](package.json) | Phụ thuộc TypeSpec của workspace; solver C++ không cần `npm install` |

## 4. Hướng dẫn chạy trên Ubuntu

### Lấy mã và cài phụ thuộc

Từ thư mục cha nơi muốn đặt repository:

```bash
git clone https://github.com/dung0611/TTSS.git
cd TTSS
```

Tại **gốc repository `TTSS/`**, cài Git, GCC/G++ hỗ trợ C++17/OpenMP, CMake >= 3.16, Python 3 và Pillow (dựng hình benchmark):

```bash
sudo apt update
sudo apt install git build-essential cmake python3 python3-pil
cd FACO-lab
```

Nếu máy chưa có Git, cài `git` trước bước clone. Từ đây, **mọi lệnh build, test, chạy solver và benchmark bên dưới đều chạy trong `TTSS/FACO-lab/`**, trừ khi ghi khác. Đường dẫn tương đối trong CLI được tính từ thư mục làm việc này.

### Build hai cấu hình và chạy CTest

```bash
# Thư mục làm việc: TTSS/FACO-lab/
cmake -S . -B build-seq -DFACO_OPENMP=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-seq -j
ctest --test-dir build-seq --output-on-failure

cmake -S . -B build-omp -DFACO_OPENMP=ON -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-omp -j
ctest --test-dir build-omp --output-on-failure
```

Cả hai cấu hình tạo executable `faco` trong thư mục build tương ứng. CTest có ba mục: `faco_mechanisms`, `cli_help`, `cli_reject_unknown` (mục cuối phải từ chối tham số lạ). Không dùng lại CMake cache được tạo trên máy hoặc đường dẫn cũ.

### Chạy thử `square4.tsp`

```bash
# Thư mục làm việc: TTSS/FACO-lab/
./build-seq/faco --input data/square4.tsp --ants 16 --iterations 10 \
  --threads 1 --seed 11 --initial-solutions 4 --verify on \
  --output-dir results/demo-square4-seq

./build-omp/faco --input data/square4.tsp --ants 16 --iterations 10 \
  --threads 4 --seed 11 --initial-solutions 4 --schedule dynamic --verify on \
  --output-dir results/demo-square4-omp

./build-omp/faco --help
```

`square4` có độ dài tối ưu 40, được kiểm tra bằng vét cạn trong tests. Bản tuần tự chỉ chấp nhận `--threads 1`. Muốn thử lịch còn lại, thay `--schedule dynamic` bằng `--schedule static` và dùng thư mục đầu ra khác.

### Tham số chính và đầu ra

| Tham số | Ý nghĩa |
|---|---|
| `--input PATH` | Bắt buộc, tệp TSPLIB được hỗ trợ |
| `--ants N`, `--iterations N` | Số kiến mỗi vòng và số vòng; mặc định 0 (tự tính theo n) và 5000 |
| `--threads N`, `--schedule static\|dynamic` | Số luồng yêu cầu và lịch chia kiến; mặc định 1 và dynamic |
| `--seed N`, `--repeat N` | Seed 64 bit và số lần chạy với seed, seed+1, …; mặc định 1 và 1 |
| `--initial-solutions N` | Số tour khởi tạo; 0 mặc định dùng số CPU logic phát hiện, nên đặt cố định khi đo |
| `--candidate-size N`, `--backup-size N` | Candidate/backup, mặc định 16/64; tự giảm khi bài toán nhỏ |
| `--min-new-edges N` | Ngưỡng chuyển sang sao chép nguồn, mặc định 8 |
| `--local-search on\|off`, `--ls-size N` | Bật/tắt LS ở cả khởi tạo và tìm kiếm; mặc định on và 20 láng giềng |
| `--rho R`, `--beta B` | Phần pheromone còn lại (0 < R < 1) và số mũ heuristic (B >= 0); mặc định 0,5 và 1; alpha cố định 1 |
| `--verify on\|off` | Kiểm tra sau move và cận pheromone, mặc định off; dùng kiểm tra, không dùng đo hiệu năng |
| `--log-every N`, `--profile on\|off` | Chu kỳ in tiến độ (0 tắt, mặc định 4) và đo pha/số kiến mỗi luồng (mặc định off) |
| `--output-dir PATH` | Thư mục đầu ra; mặc định `results` |

Mỗi lần chạy tạo bốn tệp với tiền tố `run-sSEED-tTHREADS-SCHEDULE-rREPEAT` (`THREADS` là số luồng thực tế, `REPEAT` bắt đầu từ 0):

- `.csv`: thời gian các pha, độ dài tốt nhất và sai số nếu có mốc tham chiếu.
- `.json`: cấu hình, seed, luồng yêu cầu/thực tế, thời gian, tour và profile nếu bật.
- `.tour`: tour định dạng TSPLIB TOUR, ID 1-based.
- `-history.csv`: `iteration`, `best_length`, `iteration_best`, gồm cả vòng khởi tạo 0.

Chạy lại cùng tiền tố trong cùng thư mục sẽ **ghi đè**; dùng thư mục riêng cho từng dữ liệu/cấu hình. Không có mốc thì reference/error để trống trong CSV, là `null` trong JSON. Muốn tính sai số phải cung cấp đồng thời `--reference-length`, `--reference-source`, `--reference-date`; người chạy chịu trách nhiệm xác thực mốc, không coi best-known mặc nhiên là tối ưu. Danh sách cờ đầy đủ: [Hướng dẫn kỹ thuật](FACO-lab/README.md).

## 5. Thực nghiệm

### Dữ liệu và báo cáo

| Instance trong `FACO-lab/data/` | Số đỉnh | Vai trò và nguồn |
|---|---:|---|
| [square4.tsp](FACO-lab/data/square4.tsp) | 4 | Ca kiểm thử tự sinh, tối ưu 40 |
| [uniform2048.tsp](FACO-lab/data/uniform2048.tsp) | 2.048 | Dữ liệu tổng hợp bằng script, seed 2026; chưa biết tối ưu; [báo cáo](FACO-lab/results/uniform2048/report.md) |
| [d15112.tsp](FACO-lab/data/d15112.tsp) | 15.112 | Lấy từ bộ instance FocusedACO; [báo cáo](FACO-lab/results/d15112/report.md) |
| [Tnm30001.tsp](FACO-lab/data/Tnm30001.tsp) | 30.001 | Lấy từ bộ instance FocusedACO; [báo cáo](FACO-lab/results/Tnm30001/report.md) |

Xem [nguồn dữ liệu](FACO-lab/data/SOURCES.md) và [chỉ mục kết quả](FACO-lab/results/README.md). Không trộn số liệu trong `results/archive/` với đợt đo hiện tại vì workload có thể khác.

### Benchmark 1/2/4/8 luồng

Sau khi build OpenMP Release, chạy tại **`TTSS/FACO-lab/`**:

```bash
# Chạy riêng từng mức, mỗi lệnh tạo thư mục kết quả có timestamp
./scripts/run_openmp.sh 1
./scripts/run_openmp.sh 2
./scripts/run_openmp.sh 4
./scripts/run_openmp.sh 8

# Benchmark đầy đủ trên uniform2048; dùng thư mục mới, không ghi đè báo cáo có sẵn
./scripts/run_openmp.sh benchmark --output-dir results/bench-uniform2048-new

# Cùng quy trình cho hai instance lớn hơn
./scripts/run_openmp.sh benchmark --input data/d15112.tsp \
  --output-dir results/bench-d15112-new
./scripts/run_openmp.sh benchmark --input data/Tnm30001.tsp \
  --output-dir results/bench-Tnm30001-new
```

Các lệnh chạy riêng dùng seed 11 và có log; **không thay thế benchmark lặp**. Benchmark dùng cùng binary OpenMP ở 1/2/4/8 luồng, 128 kiến, 200 vòng, 4 nghiệm khởi tạo, LS bật, lịch dynamic, seed 11/22/33. Mỗi mức có một warm-up bị loại khỏi thống kê, mỗi cặp seed/luồng có năm phép đo: tổng 4 warm-up + 60 phép đo. Các tiến trình chạy lần lượt, thứ tự mức luồng được xoay. Log, profiler và verify trong solver đều tắt khi đo.

Script lưu `raw.csv`, `summary.csv`, `metadata.json`, các lần chạy trong `runs/`, tự kiểm chứng tour/độ dài/lịch sử và dựng bốn biểu đồ trong `figures/`. Thư mục đầu ra phải mới hoặc rỗng. Có thể kiểm chứng hoặc dựng hình lại tại **`TTSS/FACO-lab/`**:

```bash
python3 scripts/verify_results.py --input data/uniform2048.tsp \
  --results results/bench-uniform2048-new
python3 scripts/plot_openmp_benchmark.py --results results/bench-uniform2048-new
```

Để tùy chọn lịch và so với binary tuần tự, dùng script benchmark ngắn hiện có (quy trình khác benchmark đầy đủ ở trên). Script này từ chối số luồng vượt số CPU logic phát hiện; ví dụ dưới đây cần ít nhất 4 CPU logic, có thể thêm 8 chỉ khi máy có ít nhất 8 CPU logic:

```bash
# Thư mục làm việc: TTSS/FACO-lab/
python3 scripts/benchmark.py --binary build-omp/faco --sequential-binary build-seq/faco \
  --input data/uniform2048.tsp --threads 1 2 4 --seeds 11 22 33 \
  --ants 128 --iterations 200 --initial-solutions 4 --schedule static \
  --output-dir results/bench-static-new
```

### Đọc thời gian, speedup và efficiency

- `preprocessing_seconds`: dựng láng giềng và heuristic; `initialization_seconds`: tạo nghiệm ban đầu và trạng thái khởi tạo; `search_seconds`: các vòng FACO; `total_seconds`: tổng các pha trong solver. Total không phải thời gian toàn tiến trình: không gồm đọc input và ghi tệp kết quả.
- Benchmark đầy đủ dùng **OpenMP một luồng làm baseline**, không phải binary tuần tự. Với từng seed, lấy trung vị năm thời gian ở một luồng chia cho trung vị ở p luồng; sau đó lấy trung vị ba tỷ số để có speedup. Tính riêng search speedup và total speedup.
- Search efficiency (%) = 100 × search speedup / số luồng thực tế. Đây không phải CPU utilization. Kiểm tra `requested_threads` và `actual_threads` trong dữ liệu đo.
- Trung vị và Q1–Q3 của thời gian được tổng hợp từ 15 phép đo mỗi mức; Q1–Q3 là độ phân tán, không phải khoảng tin cậy. Speedup ghép seed không nhất thiết bằng tỷ số hai trung vị gộp. Khởi tạo tuần tự có thể làm total speedup thấp hơn search speedup.
- Đọc chất lượng từng lần chạy từ `raw.csv`/JSON; không diễn giải `summary.csv::best_length` là nghiệm ngắn nhất của mọi seed (script hiện lấy giá trị từ dòng đầu nhóm).

Các báo cáo liên kết ở trên ghi cấu hình đo ngày 14–15/11/2026: Ubuntu 24.04.5, Intel Core i3-4030U (2 lõi vật lý/4 CPU logic), RAM 7,65 GiB, GCC 13.3.0, CMake 3.28.3, OpenMP Release; workload như mô tả trên. **8 luồng trên máy đó là oversubscription**, không phải phép đo trên 8 lõi. Không suy rộng kết quả thành khuyến nghị chung; xem báo cáo nguồn để đọc số đo, hash binary/input và giới hạn kiểm soát môi trường.

## 6. Hạn chế và tài liệu tham khảo

- FACO là heuristic; tour hợp lệ và độ dài tính đúng không chứng minh tối ưu. Chưa kiểm chứng hiệu năng với 100.000–200.000 đỉnh.
- Khởi tạo tuần tự, chọn best, bồi đắp và đồng bộ giới hạn speedup. 3-opt bị giới hạn; chưa dùng SIMD hay pool vùng nhớ. Pheromone thưa không có nghĩa toàn bộ bộ nhớ chỉ O(n): lưu tour các kiến cần O(ants × n), trạng thái riêng các luồng cần O(threads × n).
- Đợt đo có ba seed, một budget cố định và một máy; không cố định affinity hay kiểm soát nhiệt độ. Chưa chạy TSan/Helgrind; kiểm thử kết quả không thay thế công cụ phát hiện race.

Tài liệu đọc tiếp:

- **Rafał Skinderowicz (2022), _Improving Ant Colony Optimization Efficiency for Solving Large TSP Instances_**, Applied Soft Computing: [DOI 10.1016/j.asoc.2022.108653](https://doi.org/10.1016/j.asoc.2022.108653), [arXiv:2203.02228](https://arxiv.org/abs/2203.02228).
- [FocusedACO của tác giả](https://github.com/RSkinderowicz/FocusedACO/tree/05b7c1b8d52b27d121b6138222f986fcbf196863), [đối chiếu và khác biệt triển khai](FACO-lab/docs/paper_mapping.md).
- [Hướng dẫn kỹ thuật](FACO-lab/README.md), [thuật toán](FACO-lab/docs/algorithm.md), [thiết kế OpenMP](FACO-lab/docs/parallel.md), [giải thích mã nguồn](FACO-lab/docs/giai_thich_ma_nguon.md).
- [Thực nghiệm](FACO-lab/docs/experiments.md), [nghiên cứu OpenMP/static–dynamic](FACO-lab/docs/openmp_study.md), [chỉ mục kết quả](FACO-lab/results/README.md).
- [Trích dẫn, PDF và điều kiện sử dụng](FACO-lab/docs/references.md); giấy phép manuscript không tự động áp dụng cho mã nguồn dự án.

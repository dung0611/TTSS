# Thực nghiệm và bằng chứng kiểm thử

> Đây là các số đo của bản triển khai ban đầu. Kết quả phát triển vùng OpenMP duy trì và profiler được ghi riêng trong [openmp_study.md](openmp_study.md); giữ nguyên số cũ để đối chiếu nguồn gốc.

Ngày đo: **2026-11-10, Asia/Saigon**. Máy thực: Intel Core i7-8700 @3.20GHz, 6 lõi vật lý / 12 CPU logic, RAM 7,94 GiB; Windows 10 build 19045. GCC 16.2.0 (w64devkit 2.11.0), CMake 4.4.4, OpenMP 5.2 do CMake báo. Release dùng `-O3 -DNDEBUG`, cảnh báo `-Wall -Wextra -Wpedantic -Wshadow`; không bắt buộc AVX2 hoặc `-march=native`. Chưa chạy trực tiếp trên Ubuntu. Chi tiết máy và hash nguồn: [environment.json](validation/environment.json).

## Phương pháp

Mỗi instance chạy ba seed 11,22,33; 128 kiến; 200 vòng; 4 nghiệm NN khởi tạo; bật 2-opt+3-opt khởi tạo và 2-opt checklist trong vòng lặp. Candidate=16, backup=64, LS=20, min_new_edges=8, beta=1, rho=0.5, p_best=0.1, xác suất dùng global best=0.01; bounds=paper; RNG theo kiến/vòng. OpenMP dùng dynamic,1. Tất cả cấu hình cùng seed có tour và toàn bộ lịch sử giống nhau khi kiểm tra độc lập. Dừng theo số vòng cố định để giữ cùng lượng công việc. Budget này nhỏ hơn 5000/10000 vòng và 30 lần lặp trong các thực nghiệm của bài.

Script luân phiên thứ tự các cấu hình theo seed. Không chạy benchmark cùng lúc với biên dịch. Chưa kiểm soát nhiệt độ hoặc CPU affinity. Có ba lần chạy theo ba seed, chưa lặp nhiều lần cùng một seed để tách biến thiên hệ thống khỏi biến thiên đường tìm kiếm. Khoảng min-max dưới đây gồm cả hai loại biến thiên.

Các thời gian là wall clock từ `steady_clock`, tính bằng giây:

- Preprocess: k-d tree, danh sách và heuristic.
- Init: NN+LS, pheromone và trạng thái đầu.
- Search: toàn bộ vòng lặp, cập nhật pheromone, đồng bộ và lịch sử trong RAM.
- Total: từ đầu preprocess đến hết search; không gồm parser, ghi tệp và bootstrap runtime đo số luồng trước timer.

Median của các thành phần không nhất thiết cộng bằng median total. T1 là search của binary OpenMP ở 1 luồng, Tp cùng binary ở p luồng. Tính `S(seed,p)=T(seed,1)/T(seed,p)` từng cặp seed rồi lấy median; efficiency=median_speedup/p. Vì vậy speedup có thể khác tỷ lệ của hai median thời gian. Hàng tuần tự không OpenMP chỉ để xem overhead, không dùng làm T1 của các hàng OpenMP.

## Dữ liệu tự sinh uniform2048

2.048 đỉnh tọa độ nguyên do `scripts/generate_data.py` tạo, seed 2026; tệp [uniform2048.tsp](../data/uniform2048.tsp). Chưa có mốc tối ưu hoặc best-known độc lập nên reference/error để trống. Không diễn giải độ dài thành phần trăm sai số.

SHA256 input: `cc84eb0e7b49fbf95b8a932c42a68fe71a36856417545d23beff3b648f360b45`.

| Build | Luồng | Preprocess median | Init median | Search median [min,max] | Total median | Speedup ghép seed | Efficiency |
|---|---:|---:|---:|---:|---:|---:|---:|
| Tuần tự | 1 | 0.0277 | 1.4791 | 8.1983 [7.7637,8.2177] | 9.7053 | - | - |
| OpenMP | 1 | 0.0233 | 1.7232 | 8.0414 [7.7663,8.1034] | 9.5858 | 1.000x | 100.00% |
| OpenMP | 2 | 0.0228 | 1.4640 | 4.1636 [3.9644,4.2069] | 5.5416 | 1.946x | 97.31% |
| OpenMP | 4 | 0.0245 | 1.4417 | 2.2549 [2.2518,2.2672] | 3.7318 | 3.566x | 89.15% |
| OpenMP | 6 | 0.0255 | 1.6355 | 1.6288 [1.6142,1.6601] | 3.2807 | 4.844x | 80.73% |

| Seed | Độ dài tốt nhất, giống nhau ở mọi luồng |
|---:|---:|
| 11 | 3317356 |
| 22 | 3314560 |
| 33 | 3312998 |

Qua ba seed: min=3312998, mean=3314971.333, max=3317356. Có 15 lần chạy gồm bản tuần tự và OpenMP 1/2/4/6 luồng.

Dữ liệu thô: [raw.csv](../results/uniform2048/raw.csv), [summary.csv](../results/uniform2048/summary.csv), [metadata.json](../results/uniform2048/metadata.json). Các thư mục con chứa lệnh, stdout, JSON cấu hình/tour, TOUR và lịch sử cho từng seed/luồng.

## TSPLIB pr2392

2.392 đỉnh EUC_2D, nguồn: [instances/pr2392.tsp tại commit tác giả](https://github.com/RSkinderowicz/FocusedACO/blob/05b7c1b8d52b27d121b6138222f986fcbf196863/instances/pr2392.tsp). Chỉ đọc bản tải riêng ngoài dự án, không đóng gói lại dữ liệu nguồn. Mốc **378032** lấy từ [best-known.json cùng commit](https://github.com/RSkinderowicz/FocusedACO/blob/05b7c1b8d52b27d121b6138222f986fcbf196863/best-known.json), cũng xuất hiện trong Bảng 2 của bài; kiểm tra ngày 2026-11-10. Báo cáo coi đây là best-known, không xác minh chứng minh tối ưu. Nguồn/ngày có trong từng JSON.

SHA256 input: `c04538e9227798c771939b096b2e98c466284075f6e4b0e4e046204b42fa8d95`.

| Build | Luồng | Preprocess median | Init median | Search median [min,max] | Total median | Speedup ghép seed | Efficiency |
|---|---:|---:|---:|---:|---:|---:|---:|
| OpenMP | 1 | 0.0324 | 2.3817 | 8.7740 [8.4736,9.1241] | 11.1881 | 1.000x | 100.00% |
| OpenMP | 2 | 0.0283 | 2.2644 | 4.4608 [4.2628,4.5884] | 6.7561 | 1.988x | 99.39% |
| OpenMP | 4 | 0.0285 | 2.2263 | 2.3648 [2.2519,2.3970] | 4.6206 | 3.763x | 94.07% |
| OpenMP | 6 | 0.0300 | 2.3039 | 1.7492 [1.6890,1.7953] | 4.0831 | 5.017x | 83.62% |

| Seed | Độ dài tốt nhất, giống nhau ở mọi luồng | Sai số so mốc |
|---:|---:|---:|
| 11 | 382054 | 1.0639% |
| 22 | 381641 | 0.9547% |
| 33 | 381648 | 0.9565% |

Qua ba seed: min=381641, mean=381781.000, max=382054. Có 12 lần chạy OpenMP 1/2/4/6 luồng.

Dữ liệu thô: [raw.csv](../results/pr2392/raw.csv), [summary.csv](../results/pr2392/summary.csv), [metadata.json](../results/pr2392/metadata.json). Các thư mục con chứa toàn bộ tour và lịch sử. Mốc có nguồn nhưng giá trị best-known không tự biến thành chứng minh tối ưu.

## Chạy lại

```bash
python3 scripts/benchmark.py --binary build-omp/faco --sequential-binary build-seq/faco \
  --input data/uniform2048.tsp --threads 1 2 4 6 --seeds 11 22 33 \
  --ants 128 --iterations 200 --initial-solutions 4 --output-dir results/new-uniform
python3 scripts/verify_results.py --input data/uniform2048.tsp --results results/new-uniform

# Tải pr2392 có nguồn riêng trước và dùng đường dẫn thực trên máy của bạn
python3 scripts/benchmark.py --binary build-omp/faco --input /path/pr2392.tsp \
  --threads 1 2 4 6 --seeds 11 22 33 --ants 128 --iterations 200 --initial-solutions 4 \
  --reference-length 378032 --reference-source "FocusedACO commit 05b7c1b8d52b27d121b6138222f986fcbf196863 best-known.json" \
  --reference-date 2026-11-10 --output-dir results/new-pr2392
```

Đổi số luồng theo phần cứng. Thay schedule hoặc bounds cần lưu thành thí nghiệm riêng; không trộn các workload khác vào một bảng speedup. Metadata gốc chứa đường dẫn Windows của lần đo; trên Ubuntu thay bằng đường dẫn máy mới. Hash tệp có thể thay nếu sinh lại dữ liệu với quy ước xuống dòng khác, dù tọa độ không đổi.

## Kiểm thử đã chạy

| Kiểm tra | Kết quả |
|---|---|
| Release FACO_OPENMP=OFF | 3/3 CTest pass, gồm tám nhóm cơ chế |
| Release FACO_OPENMP=ON | 3/3 CTest pass; RNG 1/2/4 luồng static/dynamic |
| Debug OpenMP, `_GLIBCXX_DEBUG`, `_GLIBCXX_ASSERTIONS` | 3/3 CTest pass |
| Build sạch hai bản Release với cảnh báo | Không cảnh báo |
| CLI/export Python cho mỗi binary | Pass; repeat, reference, JSON/CSV/TOUR và 13 nhóm tham số không hợp lệ |
| Kiểm tra độc lập uniform2048 | 15 tour hợp lệ, tổng độ dài và lịch sử đúng/tương đương |
| Kiểm tra độc lập pr2392 | 12 tour hợp lệ, tổng độ dài và lịch sử đúng/tương đương |

Tests C++ bao gồm parser/rounding, cạnh đóng tour, tour sai, láng giềng so vét cạn, đối xứng cạnh nguồn, sao chép thuận/ngược và ngưỡng không đạt, fallback backup/global, tổng trọng số 0, 2-opt 48->40 và demo74->60, ca 3-opt thoát cực tiểu 2-opt, pheromone hữu hạn/trong cận qua 500 cập nhật, phân biệt công thức cận và chặn, 2 đỉnh, tọa độ trùng/tour 0 và global best không tăng.

Logs thực: [validation/](validation/). Kiểm tra container/iterator ở Debug không thay thế AddressSanitizer hoặc công cụ phát hiện race. Chưa chạy ASan/UBSan/TSan/Helgrind trong môi trường Windows này; không tuyên bố đã kiểm chứng bằng các công cụ đó. Có tùy chọn CMake và lệnh Ubuntu trong README cho ASan/UBSan.

## Giới hạn

Search giảm tốt khi tăng luồng trong hai instance đã đo; khởi tạo tuần tự làm total speedup nhỏ hơn. Chất lượng bằng nhau qua số luồng vì RNG theo kiến/vòng, chỉ được xác nhận trong tập chạy/môi trường công bố. FACO vẫn là heuristic, ba seed cho chất lượng khác nhau.

Chưa chạy dữ liệu 100k-200k; chưa khảo sát nhiều instance, nhiều budget hay độ nhạy min_new_edges/rho/bounds; chưa đo phân bố số lần chọn ACO. Không suy ra đã tái hiện hiệu năng/chất lượng của bài báo. K-d tree giúp tiền xử lý, nhưng NN fallback, cấp phát tạm theo kiến, m tour và 3-opt vector vẫn giới hạn khả năng mở rộng.

Lệnh tham khảo cho dữ liệu lớn khi đã có dữ liệu và tài nguyên phù hợp, **chưa thực thi**:

```bash
./build-omp/faco --input /path/mona-lisa100K.tsp --ants 512 --iterations 10000 \
  --threads 6 --initial-solutions 4 --seed 11 --output-dir results/mona100k
```

Dự toán riêng bộ nhớ tour m*n*4 byte, cộng hàng xóm/pheromone/heuristic và O(p*n) bộ nhớ tạm. Lệnh mẫu không phải cam kết hoàn tất trong thời gian của bài báo.


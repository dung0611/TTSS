# Nghiên cứu OpenMP sau khi thêm team duy trì và profiler

Ngày thực hiện: 2026-11-10, Asia/Saigon. Máy, GCC và dữ liệu như experiments.md: i7-8700 6 lõi/12 CPU logic, RAM khoảng 8 GiB, Windows 10, GCC 16.2.0, OpenMP theo CMake 5.2. Đây là phép đo bổ sung; số cũ được giữ nguyên.

## Cấu hình và phạm vi

uniform2048, 128 kiến, 200 vòng, 4 nghiệm khởi tạo, seed 11/22/33, rho=.5, beta=1, candidate16/backup64, LS20, min_new_edges8, bounds=paper. Mỗi cấu hình có cùng dữ liệu và công việc; tổng 25600 tour main-loop. Benchmark tắt log và profiler. Có ba nhóm: trước thay đổi (dynamic), team duy trì (dynamic), team duy trì (static). Mỗi nhóm đo 1/2/4/6 luồng và ba seed, tổng 36 lần chạy. Hai lần chạy profiler bổ sung được để riêng, không trộn vào bảng speedup.

T1/Tp là search wall-clock cùng binary và cùng schedule, ghép theo seed rồi lấy median. Efficiency=S/p. Các nhóm binary/schedule chạy nối tiếp theo nhóm; thứ tự số luồng được xoay theo seed. Không kiểm soát CPU affinity, nhiệt độ hoặc các tiến trình khác. Các con số phản ánh đợt chạy này, không phải thử nghiệm cô lập sai số hệ thống hay bằng chứng nhân quả cho chênh lệch vài phần trăm. Không lấy tỷ lệ thời gian giữa hai đợt cũ/mới ở experiments.md để kết luận speedup.

Binary cũ được giữ ở ngoài gói dự án trong D:\TTSS\tools\faco-before-persistent.exe; gói nguồn/executable cũ được bảo toàn tại D:\TTSS\tools\FACO-before-persistent.zip; hash binary và các nguồn trước thay đổi ở [openmp-baseline.json](validation/openmp-baseline.json). Không commit/push hoặc sửa lại các kết quả cũ.

## Kết quả không bật profiler

| Phiên bản / lịch | Luồng | Search median [min,max] s | Total median s | Paired speedup | Efficiency |
|---|---:|---:|---:|---:|---:|
| Trước / dynamic | 1 | 10.9540 [10.7185,10.9775] | 13.2271 | 1.000x | 100.00% |
| Trước / dynamic | 2 | 5.7705 [5.7019,5.8819] | 8.0456 | 1.866x | 93.32% |
| Trước / dynamic | 4 | 3.2526 [3.0275,3.2874] | 5.5704 | 3.368x | 84.19% |
| Trước / dynamic | 6 | 2.5325 [2.4009,2.5375] | 4.8561 | 4.326x | 72.10% |
| Team duy trì / dynamic | 1 | 10.9226 [10.7944,11.1490] | 13.3945 | 1.000x | 100.00% |
| Team duy trì / dynamic | 2 | 5.9035 [5.7381,5.9951] | 8.1579 | 1.860x | 92.98% |
| Team duy trì / dynamic | 4 | 3.3364 [3.2504,3.3415] | 5.6625 | 3.337x | 83.41% |
| Team duy trì / dynamic | 6 | 2.6491 [2.6488,2.8681] | 5.1911 | 4.123x | 68.72% |
| Team duy trì / static | 1 | 10.8353 [9.3908,10.8794] | 13.1235 | 1.000x | 100.00% |
| Team duy trì / static | 2 | 5.8780 [5.8421,6.3988] | 8.5461 | 1.843x | 92.17% |
| Team duy trì / static | 4 | 3.2531 [2.9094,3.4533] | 5.8474 | 3.228x | 80.69% |
| Team duy trì / static | 6 | 3.0588 [2.9683,3.2569] | 5.5010 | 3.542x | 59.04% |

Mỗi hàng có ba lần chạy. Search không gồm tiền xử lý/khởi tạo; Total gồm các phần đó. Tỷ lệ ghép seed không bắt buộc bằng tỷ lệ hai median. Dữ liệu thô, lệnh, stdout, tour và lịch sử nằm trong [results/openmp-study-20261110](../results/openmp-study-20261110/).

| Luồng | Median paired T_before / T_persistent_dynamic |
|---:|---:|
| 1 | 0.9846 |
| 2 | 0.9811 |
| 4 | 0.9749 |
| 6 | 0.9063 |

Cấu trúc team duy trì chưa cho thấy tăng tốc rõ so với bản cũ ở bộ dữ liệu này; một số mức luồng chậm hơn trong đợt đo. Giảm số parallel region không tự bảo đảm thời gian thấp hơn: vẫn có barrier, single, phân công công việc, kiểm tra lỗi và chi phí runtime. Không gán chắc chắn mức chênh lệch cho riêng một thành phần khi chưa có profiler mức hệ thống hoặc thí nghiệm cô lập. Hai phiên bản cho cùng tour và toàn bộ lịch sử với cùng seed.

## Số đo profiler: 4 luồng, seed 11

| Schedule | Search s | Ant phase s | Selection s | Evaporation s | Deposit s | Clamp s | Bookkeeping s |
|---|---:|---:|---:|---:|---:|---:|---:|
| dynamic | 3.330086 | 3.234195 | 0.004862 | 0.046348 | 0.025334 | 0.014595 | 0.003667 |
| static | 3.395630 | 3.326067 | 0.004858 | 0.020492 | 0.025526 | 0.014208 | 0.003804 |

Thời gian các pha gồm phần đồng bộ; không phải CPU time và không phải phép đo riêng barrier. Profiler gọi đồng hồ cho từng ant, nên dùng để phân tích tỷ trọng/công việc, không thay bảng thời gian tắt profiler. Tổng phase có thể lệch search_seconds một ít bởi overhead đo và vùng không nằm trong khoảng phase.

| Schedule | Thread | Tour đã xử lý | Ant work s | Lựa chọn ACO | Đỉnh sao chép | Move 2-opt |
|---|---:|---:|---:|---:|---:|---:|
| dynamic | 0 | 6450 | 3.169546 | 11761635 | 1441515 | 75234 |
| dynamic | 1 | 6338 | 3.179751 | 11596171 | 1377715 | 73861 |
| dynamic | 2 | 6436 | 3.174047 | 11753146 | 1421346 | 74789 |
| dynamic | 3 | 6376 | 3.171135 | 11637287 | 1414385 | 75901 |
| static | 0 | 6400 | 3.112732 | 11678325 | 1422475 | 75106 |
| static | 1 | 6400 | 3.090703 | 11698883 | 1401917 | 75659 |
| static | 2 | 6400 | 3.064511 | 11691018 | 1409782 | 75286 |
| static | 3 | 6400 | 3.111036 | 11680013 | 1420787 | 73734 |

Tổng số tour = **25600**; lựa chọn ACO = **46748239**; đỉnh sao chép = **5654961**; tổng bước nối trước cạnh đóng = **52403200 = 25600 × 2047**. Tỷ lệ ACO = **89.21%**, sao chép = **10.79%**. Ngưỡng 8 đếm cạnh khác nguồn, nên số lựa chọn ACO có thể lớn hơn 8 rất nhiều. Đây là số đo của cấu hình này; tỷ lệ thay đổi theo tham số và instance.

Số kiến theo dynamic có thể khác giữa các lần chạy, trong khi tour/lịch sử vẫn giống vì RNG theo ant/iteration. Static,1 chia đúng 6400 tour cho mỗi worker trong ca này. Thời gian worker là tổng các khoảng wall-clock xử lý ant trên luồng đó; có thể chồng nhau giữa các luồng, không cộng thành wall time toàn chương trình.

## Kết luận để phát triển tiếp

Pha kiến chiếm **97.12%** thời gian tìm kiếm thực trong mẫu dynamic có bật profiler. Khởi tạo mất **2.2974 giây** trong tổng **5.6763 giây**. Độ dài tốt nhất **3317356**, không đổi so với phiên bản trước.

1. Ưu tiên vùng construct + 2-opt + tính lại độ dài: tái sử dụng bộ nhớ visited/position/checklist theo luồng và đo lại trước khi kết luận.
2. Khởi tạo NN/2-opt/3-opt vẫn tuần tự. Các nghiệm khởi tạo độc lập có thể song song hóa, giữ số nghiệm và seed theo chỉ số để không đổi workload hoặc tính tái lập.
3. Chọn static/dynamic dựa trên thời gian thực và mất cân bằng LS; không chỉ nhìn số tour mỗi luồng.
4. Selection/deposit có tỷ trọng nhỏ trong ca này; chia nhỏ thêm phần đó cần cân nhắc chi phí đồng bộ. Có thể khảo sát cập nhật pheromone theo slot trong thí nghiệm riêng.

Không thêm thời gian chờ giả để làm vòng lặp chậm. Muốn khảo sát workload dài hơn, tăng số kiến/vòng hoặc kích thước instance, giữ cùng workload khi so các số luồng. Cách bật quan sát hiện tại:

```powershell
cd D:\TTSS\FACO-lab
.\build-omp\faco.exe --input data/uniform2048.tsp --ants 128 --iterations 200 --threads 4 --seed 11 --initial-solutions 4 --schedule dynamic --log-every 4 --profile on --output-dir results/openmp-live
```

Có thể đổi schedule thành static để so phân bố công việc. Khi đo speedup dùng profile off và log-every 0, hoặc scripts/benchmark.py. Các trường block_ms/rounds giải thích vì sao bốn vòng có thể chỉ mất vài chục mili giây.

## Kiểm thử

Release tuần tự, Release OpenMP và Debug OpenMP với _GLIBCXX_DEBUG/_GLIBCXX_ASSERTIONS đều đạt 3/3 CTest. Kiểm thử có đối chiếu tour/lịch sử 1/2/4 luồng, static/dynamic, profiler on/off, tổng công việc mỗi worker, phân bố static, bất biến số cạnh được chọn/sao chép và ngoại lệ callback giữa vòng. Sau khi callback ném lỗi, solver được chạy lại thành công, nhằm kiểm tra đường thoát team không kẹt barrier.

CLI integration đạt kiểm tra log chu kỳ 4/3, vòng cuối không đủ chu kỳ, tắt log, metadata profiler, bộ đếm JSON/stdout, repeat và 16 nhóm tham số không hợp lệ. Kiểm tra độc lập 38 tour trong nghiên cứu này xác nhận đủ đỉnh, độ dài đúng và toàn bộ lịch sử bằng nhau theo seed giữa các phiên bản/lịch/số luồng.

Log CTest của lần sửa nằm trong validation/openmp-refactor-*-ctest.txt. Chưa chạy công cụ phát hiện race TSan/Helgrind hoặc profiler phần cứng; không suy ra đã chứng minh không có race chỉ từ các phép đo này.


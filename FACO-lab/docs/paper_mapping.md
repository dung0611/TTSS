# Đối chiếu bài báo và mã tham khảo

Nguồn ưu tiên là [manuscript PDF](FACO-Skinderowicz-2022.pdf) 28 trang do người dùng cung cấp, tiêu đề thực tế *Improving Ant Colony Optimization Efficiency for Solving Large TSP Instances*. Xem [bài báo tham khảo](references.md) để lấy liên kết arXiv, DOI, trích dẫn và giấy phép. Số trang dưới đây là số trang in trong manuscript. Đã xem trực quan trang 11 (Hình 2), 13 (cận), 14 (Hình 3/lịch OpenMP), 20 (số kiến) để tránh lỗi trích công thức từ PDF.

Mã đối chiếu cố định tại [commit 05b7c1b8d52b27d121b6138222f986fcbf196863](https://github.com/RSkinderowicz/FocusedACO/tree/05b7c1b8d52b27d121b6138222f986fcbf196863), ngày đọc 2026-11-10. Không mặc định mã và bài báo giống nhau.

| Thành phần | Mục/hình bài báo | Tệp/hàm tham khảo | Tệp/hàm dự án | Khác biệt và lý do |
|---|---|---|---|---|
| TSP, khoảng cách | 2.1 | problem_instance.h/.cpp, euc2d_distance/load_tsplib_instance | problem.cpp, read_tsplib/Problem::distance | Chỉ EUC_2D/TSP; kiểm tra ID, trường thiếu, tọa độ không hữu hạn; tổng int64; không ma trận khoảng cách |
| Candidate/backup | 4.1, 4.3 | problem_instance.h, compute_nn_lists/get_backup_neighbors | neighbors.cpp, Neighbors | K-d tree bất biến viết mới; thứ tự `(khoảng cách làm tròn, ID)` xử lý hòa nhất quán; tự giảm kích thước khi n nhỏ |
| K-d tree | triển khai để hỗ trợ 4.1 | kd_tree.h, nn_bottom_up/delete_point | neighbors.cpp, KDTree | Truy vấn top-k bằng heap và pruning có xét làm tròn; không delete/undelete; NN tour dùng danh sách rồi quét phần còn lại |
| Pheromone một phần | 4.2 | pheromone.h, CandListPheromone | pheromone.cpp, Pheromone | Chỉ n*cl ô; xử lý hai chiều tồn tại như mã tác giả; không tự thêm chiều thiếu |
| Heuristic và alpha | 2.1 Eq.1, 4.3 | faco.cpp, HeuristicData/select_next_node | faco.cpp, select_next/solve | Alpha cố định 1; heuristic cho d=0 bằng 1; tổng trọng số 0 chọn ứng viên đầu tiên |
| Khởi tạo | 4.3, trang 12 | faco.cpp, par_build_initial_routes | faco.cpp, solve; neighbors.cpp, nearest_neighbor_tour | Bài mô tả NN+2-opt+3-opt, không ấn định số nghiệm. Mã tạo omp_get_num_procs() nghiệm, chọn ngắn nhất; dự án dùng hardware_concurrency() hoặc cờ cố định, xử lý tuần tự để dễ đối chiếu |
| LS khởi tạo | 4.3 | two_opt_nn + three_opt_nn; 16 NN; DLB | local_search.cpp, two_opt/three_opt | Dùng cùng --ls-size (mặc định 20) cho khởi tạo và vòng lặp. Không DLB, giới hạn mỗi thủ tục n move. 3-opt xét 7 tái nối không đồng nhất, gồm 4 trường hợp 3-opt thực; không sao chép tối ưu segment của tác giả |
| Nguồn và cạnh mới | Hình 2 dòng 7-19 | faco.cpp, run_focused_aco; ant.h, contains_edge | faco.cpp, construct; tour.cpp, Source | Tính cạnh vô hướng; đếm cạnh ACO trước đóng tour, không ép tổng khác biệt bằng threshold |
| Sao chép nguồn | Hình 2 dòng 20-29 | run_focused_aco, RouteIterator | tour.cpp, copy_source | Theo mã: đi thuận đến đỉnh bị chặn, lùi iterator một bước để hủy bước thất bại, thử đi ngược. Nếu vẫn thiếu, quay lại chọn ACO; không nhảy qua đỉnh đã thăm bằng cách quét source |
| Checklist ban đầu | Hình 2 dòng 18 | run_focused_aco | faco.cpp, construct | Hình 2 thêm đỉnh đến của cạnh mới. Mã tác giả còn thêm đỉnh xuất phát; dự án theo chi tiết này. Không thêm mọi đỉnh của phần sao chép |
| 2-opt checklist | 4.4, Hình 3 | local_search.cpp, overload two_opt_nn(checklist) | local_search.cpp, two_opt | Xét hai nhóm cạnh outgoing/predecessor, chỉ gain>0; cập nhật position, enqueue 4 đầu mút; không trùng trong phần queue đang chờ; tối đa n move như bài/mã |
| Cận khi bật LS | 4.5 trang 13 | faco.cpp, calc_trail_limits_cl | pheromone.cpp, trail_limits | **Bài: avg=cl nhưng căn p_best vẫn bậc n. Mã: avg=cl và căn bậc cl.** Mặc định --bounds paper; --bounds reference chỉ đổi số mũ để khảo sát, không phải toàn bộ chế độ tương thích mã tác giả |
| Cận khi tắt LS | 4.5 | calc_trail_limits | trail_limits | Cả hai dùng avg=n/2, căn bậc n; chặn tau_min<=tau_max; thêm guard avg<=1 |
| Bay hơi/bồi đắp | Eq.2, Hình 2 dòng 35-37 | ACOModel::evaporate_pheromone/deposit_pheromone; CandListPheromone | Pheromone::evaporate_slot/deposit/clamp_slot | **Bài: clamp(rho*tau+delta). Mã: max(min,rho*tau) trước, rồi cộng delta và chặn max.** Dự án trì hoãn chặn dưới đến sau bồi đắp để thực hiện đúng Eq.2; có test phân biệt hai công thức |
| Chọn nguồn | Hình 2 dòng 36, 5.3 | run_focused_aco, gbest_as_source_prob_=0.01 | faco.cpp, solve | Global best xác suất .01, còn lại iteration best; nguồn cũng dùng bồi đắp. Chọn nguồn trước bay hơi là tương đương vì hai việc không phụ thuộc nhau |
| Số kiến tự động | 5.6 trang 20 | faco.cpp, main | faco.cpp, default_ant_count | **Bài: 64*ceil(4*sqrt(n)/64). Mã: lround(4*sqrt(n)/64)*64**, có thể thành 0 khi n nhỏ. Dự án làm tròn lên, tối thiểu 64; nên ghi --ants nhỏ cho ví dụ nhỏ |
| Lịch OpenMP | 4.6 trang 14 | run_focused_aco, schedule(static,1) | faco.cpp, schedule(runtime) | **Bài ghi dynamic,1; commit kiểm tra dùng static,1.** Dự án mặc định dynamic,1, hỗ trợ static,1; bay hơi/clamp static |
| RNG | chi tiết triển khai | rand.h/.cpp, xoroshiro+, threadprivate, jump | faco.cpp, ant_rng/uniform01/uniform_index | RNG theo kiến/vòng là bổ sung của dự án, giúp so cùng đường tìm kiếm qua số luồng; không tái hiện chuỗi RNG tác giả |
| Tham số | Bảng 1, 5.2, 5.6 | progargs.h/.cpp | faco.hpp, options.cpp | cl16, backup64, min8, beta1, rho.5, 5000 vòng; --ls-size20 theo mặc định vòng lặp của mã; thí nghiệm bàn giao dùng budget thấp hơn và công khai |
| Ghi kết quả | 5.1, 5.6 | logging.h, utils.h/.cpp, best-known.json | output.cpp; scripts/benchmark.py | Lưu đủ tour/lịch sử/cấu hình, tách bốn thời gian; không auto coi mọi best-known là tối ưu; yêu cầu metadata mốc |
| Build và dữ liệu | 5.1 | README.md/Makefile, instances/ | CMakeLists.txt, data/, scripts/ | CMake thay Makefile; không bắt buộc AVX2/march=native; dữ liệu gốc tự sinh để không tái phân phối dữ liệu chưa rõ điều kiện |

## Phạm vi tuân theo

Đã triển khai các cơ chế cốt lõi Hình 2/Hình 3, candidate/backup, pheromone một phần, MMAS bounds, NN+2-opt+3-opt và phân chia công việc OpenMP. Không thiếu 3-opt; tuy nhiên bản 3-opt và khởi tạo được đơn giản hóa như bảng trên. Không khẳng định cùng hành vi bit-for-bit, cùng chất lượng hay cùng thời gian với chương trình tác giả.

Các trường hợp ngoài giả thiết chi phí dương được mở rộng để tránh lỗi: `eta=1` khi d=0, dùng `max(length,1)` trong cận/bồi đắp khi tour dài 0, `tau_min=tau_max` khi avg<=1, n=2 bồi đắp một lần cho mỗi chiều lưu của cạnh vô hướng duy nhất. Đây là quy tắc của dự án cho ca suy biến, không phải kết quả thực nghiệm của bài.

## Bổ sung triển khai OpenMP sau bản đầu

`src/faco.cpp::solve` hiện giữ một `omp parallel default(none)` xuyên suốt main loop; `omp for` chia kiến, bay hơi và clamp; `omp single` chọn best/nguồn, deposit và ghi tiến độ. RNG vẫn theo kiến/vòng. `--profile on` là phần bổ sung để đo các pha và công việc từng luồng; không phải tham số thuật toán trong bài báo. Các kiểm thử mới xác nhận tour/lịch sử không thay đổi khi đổi lịch, số luồng hoặc bật profiler; có xử lý ngoại lệ qua barrier và kiểm thử chạy lại sau lỗi callback. Xem parallel.md và openmp_study.md cho kết quả đo mới; những số trong experiments.md thuộc bản đo ban đầu.

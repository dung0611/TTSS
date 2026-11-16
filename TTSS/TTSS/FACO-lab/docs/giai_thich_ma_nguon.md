# Giải thích cấu trúc và hàm của FACO-lab

Tài liệu này mô tả mã nguồn hiện có trong `FACO-lab`, tập trung vào từng tệp do dự án quản lý và các hàm/lớp chính. Thư viện bên thứ ba, compiler, file sinh tự động và sản phẩm build không phải mã nguồn của thuật toán.

## 1. Dự án làm gì?

FACO-lab là chương trình C++17 dùng Focused Ant Colony Optimization (FACO) kết hợp song song hóa với OpenMP để tìm tour ngắn cho bài toán người du lịch (TSP) đối xứng trên tọa độ 2D. Chương trình đọc TSPLIB `TSP/EUC_2D`, tạo tour ban đầu, cải thiện bằng local search, lặp cập nhật pheromone và ghi kết quả. Đây là thuật toán heuristic, không cam kết nghiệm tối ưu.

Luồng chính: `src/main.cpp` → `options.cpp` → `problem.cpp` → `solve()` trong `faco.cpp` → `output.cpp`. Các thành phần `neighbors`, `tour`, `pheromone`, `local_search` được solver sử dụng.

## 2. Cây thư mục

| Đường dẫn | Vai trò |
|---|---|
| `include/faco/` | Khai báo kiểu dữ liệu và giao diện các thành phần C++. |
| `src/` | Cài đặt parser, thuật toán, CLI và xuất kết quả. |
| `tests/tests.cpp` | Kiểm tra parser, tour, láng giềng, chọn cạnh, local search, pheromone và solver. |
| `scripts/` | Tạo dữ liệu, benchmark, kiểm chứng và vẽ biểu đồ. |
| `data/` | Instance TSP mẫu. |
| `docs/` | Tài liệu thuật toán, song song, thí nghiệm, đối chiếu bài báo và bàn giao. |
| `.vscode/` | Cấu hình VS Code cho build, IntelliSense và task. |
| `CMakeLists.txt` | Khai báo target, chuẩn C++, OpenMP, sanitizer và CTest. |
| `README.md` | Hướng dẫn build, chạy, tham số, benchmark và giới hạn. |
| `NOTICE.md` | Nguồn tham khảo và lưu ý tái sử dụng. |
| `.clang-format`, `.gitignore` | Quy tắc định dạng và các đường dẫn Git bỏ qua. |
| `MANIFEST.sha256` | Hash kiểm tra tính toàn vẹn các file bàn giao. |

### Các thư mục không cần đọc để hiểu thuật toán

`node_modules/` là phụ thuộc TypeSpec cài bằng npm, không được solver FACO gọi. `tools/w64devkit/` là toolchain Windows gồm compiler/header/runtime. `build-*` chứa cache CMake, thư viện và executable được tạo từ mã nguồn. `__pycache__/` là bytecode Python tự sinh. Không sửa các file này để thay đổi thuật toán.

## 3. Giao diện và kiểu dữ liệu (`include/faco/`)

### `problem.hpp`

- `Cost`: chi phí kiểu số nguyên 64-bit; `Tour`: thứ tự đỉnh dùng chỉ số nội bộ 0-based; `Point`: tọa độ `x,y`.
- `Problem`: tên instance và danh sách tọa độ. `size()` trả số đỉnh; `distance(a,b)` trả khoảng cách Euclidean làm tròn.
- `read_tsplib(in)`: đọc và xác thực instance từ stream.
- `load_tsplib(path)`: mở file rồi gọi parser.
- `valid_tour(t,n)`: xác nhận tour là hoán vị hợp lệ gồm n đỉnh.
- `tour_length(p,t)`: tính tổng chu trình, gồm cạnh cuối về đầu.

### `tour.hpp`

- `Source`: tour nguồn cùng bảng `position` tra vị trí. `successor(v)`/`predecessor(v)` tra đỉnh kế/trước trong vòng; `contains_edge(a,b)` kiểm tra cạnh nguồn không phụ thuộc chiều.
- `copy_source(source,from,route,visited)`: nối các đỉnh theo nguồn nếu chưa thăm, dừng trước đỉnh bị chặn và thử hướng ngược theo quy tắc thuật toán.

### `neighbors.hpp`

- `Neighbors`: mỗi đỉnh có một hàng láng giềng đã sắp xếp; prefix candidate, phần tiếp theo backup và prefix riêng cho local search.
- `Neighbors(p,candidates,backup,local_search)`: dựng các danh sách láng giềng.
- `nearest_neighbor_tour(p,nn,start)`: tạo tour tham lam từ `start`, fallback quét đỉnh chưa thăm nếu cần.

### `pheromone.hpp`

- `Limits`: cận pheromone nhỏ nhất/lớn nhất.
- `trail_limits(...)`: tính các cận theo kích thước, candidate, `p_best`, `rho`, độ dài tour và cấu hình local search.
- `Pheromone`: lưu pheromone trên candidate, không cấp ma trận n×n. `get(from,to)` tra theo cạnh; `at(from,slot)` tra theo slot; `evaporate_slot` bay hơi; `deposit` bồi đắp theo tour nguồn; `clamp_slot` chặn cận; `in_bounds()` xác minh toàn bộ giá trị.

### `local_search.hpp`

- `two_opt(p,nn,tour,checklist,verify)`: tìm và áp dụng move 2-opt có cải thiện trong vùng được xét; trả số move.
- `three_opt(p,nn,tour,verify)`: tìm kiếm 3-cut giới hạn bởi láng giềng, dùng trong khởi tạo; trả số move.

### `faco.hpp`

- `Config`: tham số CLI và mặc định: số kiến/vòng/luồng, kích thước danh sách, seed, rho/beta, local search, profile, input/output và reference.
- `Construction`: tour do một kiến dựng, checklist và bộ đếm cạnh/đỉnh liên quan.
- `History`: best toàn cục và best của vòng tại iteration.
- `WorkerProfile`, `ParallelProfile`: số công việc và thời gian theo luồng/các pha OpenMP.
- `Result`: tour tốt nhất, độ dài, cấu hình thực dùng, thời gian, lịch sử và profile.
- `ProgressCallback`: kiểu callback báo tiến độ.
- `validate_config(c)`: kiểm tra cấu hình.
- `default_ant_count(n)`: tính số kiến mặc định theo số đỉnh.
- `ant_rng(seed,phase,iteration,ant)`: tạo RNG riêng cho kiến/vòng/pha, độc lập lịch chạy luồng.
- `uniform01(rng)`, `uniform_index(rng,n)`: lấy số thực [0,1) và chỉ số [0,n).
- `select_next(...)`: chọn candidate chưa thăm theo pheromone và heuristic.
- `construct(...)`: dựng một tour, chuyển từ chọn cạnh mới sang sao chép chuỗi từ nguồn sau ngưỡng.
- `solve(p,c,progress)`: chạy tiền xử lý, khởi tạo, các vòng FACO, cập nhật nghiệm và trả `Result`.
- `parse_options(argc,argv)`, `print_help()`: đọc CLI và in trợ giúp.
- `save_result(p,r,repeat_index)`: ghi kết quả cho một lần chạy.

## 4. Cài đặt trong `src/`

### `problem.cpp`

- `trim(s)`: bỏ khoảng trắng hai đầu chuỗi.
- `read_tsplib(in)`: đọc header và `NODE_COORD_SECTION`; xác nhận TSP/EUC_2D, ID và tọa độ; sắp tọa độ theo ID.
- `load_tsplib(path)`: mở file và chuyển nội dung cho parser.
- `valid_tour(t,n)`: kiểm tra kích thước, chỉ số và trùng lặp.
- `tour_length(p,t)`: cộng khoảng cách vòng kín bằng `Cost`.
- `Problem::distance(a,b)`: tính `floor(hypot(dx,dy)+0.5)`.

### `neighbors.cpp`

- `KDTree::build(lo,hi,depth)`: chia điểm luân phiên theo trục để dựng cây cân bằng.
- `KDTree::query(index,target,k,heap)`: truy vấn k điểm gần nhất, cắt nhánh theo cận khoảng cách và xử lý khoảng cách hòa.
- `KDTree(problem)`: chuẩn bị ID và dựng k-d tree.
- `KDTree::nearest(v,k)`: trả k láng giềng theo khoảng cách rồi ID.
- `Neighbors::Neighbors(...)`: dựng hàng láng giềng và giới hạn kích thước theo n.
- `nearest_neighbor_tour(...)`: tạo tour tham lam, dùng fallback toàn bộ đỉnh khi danh sách cục bộ hết lựa chọn.

### `tour.cpp`

- `Source::Source(t)`: lưu tour nguồn và lập bảng vị trí.
- `successor(v)` / `predecessor(v)`: tìm đỉnh kế/trước theo chu trình.
- `contains_edge(a,b)`: kiểm tra hai đỉnh kề nhau trong nguồn.
- `copy_source(...)`: chép chuỗi đỉnh nguồn vào tour đang dựng, tránh đỉnh đã thăm và hoàn tác bước bị chặn.

### `pheromone.cpp`

- `trail_limits(...)`: suy ra `tau_min` và `tau_max`.
- `Pheromone::Pheromone(...)`: cấp phát pheromone cho candidate rows và đặt giá trị ngoài danh sách.
- `get(from,to)` / `at(from,slot)`: truy cập pheromone theo cạnh hoặc slot.
- `evaporate_slot(slot,rho)`: nhân pheromone với phần còn lại `rho`.
- `deposit(source,length,rho)`: cộng lượng tỷ lệ nghịch độ dài cho các cạnh nguồn có trong candidate rows.
- `clamp_slot(slot)`: kẹp giá trị trong giới hạn.
- `in_bounds()`: kiểm tra mọi pheromone có nằm trong cận không.

### `local_search.cpp`

- `two_opt(...)`: dùng checklist và láng giềng tìm cặp cạnh thay thế ngắn hơn; đảo đoạn, cập nhật vị trí/checklist; khi bật `verify`, kiểm tra lại tour và chi phí. Dừng khi hết việc, không cải thiện hoặc đạt giới hạn move.
- `three_opt(...)`: thử các cách tái nối bị giới hạn bởi láng giềng, áp dụng cải thiện rồi tiếp tục tìm. Không duyệt toàn bộ mọi 3-opt có thể.

### `faco.cpp`

- `seconds(start)`: đổi thời gian từ steady clock sang giây.
- `validate_config(c)`: kiểm tra các giá trị tham số và tổ hợp cấu hình.
- `default_ant_count(n)`: quy tắc số kiến mặc định.
- `ant_rng(...)`: seed bộ sinh giả ngẫu nhiên theo seed/pha/vòng/kiến.
- `uniform01`, `uniform_index`: tiện ích sinh số ngẫu nhiên.
- `select_next(...)`: roulette theo pheromone × heuristic mũ beta; có fallback khi tổng trọng số bằng 0.
- `construct(...)`: chọn đỉnh candidate, rồi backup/toàn bộ đỉnh khi cần; đếm cạnh mới so với nguồn; sau ngưỡng thì chèn chuỗi từ nguồn và khép tour.
- `solve(...)`: chuẩn bị láng giềng/heuristic; tạo nghiệm ban đầu; thiết lập pheromone; lặp dựng kiến, local search, chọn best, chọn nguồn, deposit; đồng thời quản lý OpenMP, tiến độ và profile.

### `options.cpp`, `main.cpp`, `output.cpp`

- `integer(s)`: parse số nguyên không dấu 64-bit.
- `small(s)`: parse số nguyên nhỏ và kiểm tra miền biểu diễn.
- `real(s)`: parse số thực hữu hạn.
- `boolean(s)`: parse giá trị bật/tắt.
- `print_help()`: in hướng dẫn cờ CLI.
- `parse_options(argc,argv)`: chuyển cờ và giá trị thành `Config`, từ chối tham số không hợp lệ.
- `main(argc,argv)`: nhận cấu hình, nạp instance, chạy solver/lặp chạy, in tiến độ và xử lý lỗi cấp ứng dụng.
- `json_string(s)`: escape chuỗi JSON.
- `csv_string(s)`: escape trường CSV.
- `file(path)`: mở output stream và báo lỗi nếu thất bại.
- `save_result(...)`: tạo thư mục và ghi số liệu, cấu hình, tour và lịch sử theo các định dạng của chương trình.

## 5. Script Python (`scripts/`)

### `generate_data.py`

- `generate(path,n,seed)`: sinh tọa độ ngẫu nhiên tái lập được và ghi TSPLIB EUC_2D.
- Khối `__main__`: parse đường dẫn/kích thước/seed rồi gọi hàm trên.

### `benchmark.py`

- `main()`: chạy binary với các mức luồng và seed, thu kết quả, tổng hợp thống kê và ghi báo cáo benchmark.
- Khối `__main__`: điểm vào CLI.

### `run_openmp_benchmark.py`

- `sha256(path)`: tính hash file.
- `read_release()`: lấy thông tin phát hành môi trường nếu có.
- `cpu_topology()`: thu thập thông tin CPU và cấu trúc luồng.
- `q(value,probs)`: tính phân vị thống kê.
- `build_metadata(...)`: tạo metadata về binary, input, cấu hình và máy chạy.
- `main()`: thực hiện warm-up và lần đo benchmark OpenMP, lưu raw/summary CSV cùng metadata và chạy các bước kiểm chứng/đồ thị được cấu hình.
- Khối `__main__`: điểm vào CLI.

### `verify_results.py`

- `coordinates(path)`: đọc tọa độ instance để kiểm tra độc lập.
- `verify(input_path,output_dir)`: xác minh tour, tính lại chiều dài và đối chiếu output.
- Khối `__main__`: đọc tham số và gọi `verify`.

### `integration.py`

- `main()`: chọn binary/dữ liệu và gọi các kiểm tra tích hợp CLI.
- `progress_checks(binary,data)`: xác minh tiến độ, tần suất log và lịch sử.
- `profile_checks(binary,data)`: xác minh profile, output và trạng thái tắt.
- Khối `__main__`: chạy bộ kiểm tra tích hợp.

### `package.py`

- `main()`: chọn file bàn giao, đóng gói, tính và in hash/kích thước.
- Khối `__main__`: chạy đóng gói.

### `run_openmp.sh`

Chọn số luồng hoặc chế độ benchmark, gọi binary OpenMP với instance và tham số mặc định, rồi chuyển các đối số bổ sung. Script điều phối chạy, không cài đặt thuật toán.

### `plot_openmp_benchmark.py`

- `font(size,bold)`: chọn font chữ.
- `Figure.__init__()`: khởi tạo canvas.
- `Figure.line`, `rect`, `circle`, `text`: thêm đường, hình chữ nhật, hình tròn và chữ vào canvas.
- `Figure.png(path)`: raster hóa và ghi PNG (cần Pillow).
- `Figure.svg(path)`: ghi SVG.
- `Figure.save(stem,output)`: lưu các định dạng hình.
- `csv_rows(path)`: đọc CSV thành danh sách từ điển.
- `axes(...)`: vẽ trục, nhãn, ticks, tiêu đề.
- `actual_labels(summary)`: tạo nhãn luồng thực tế từ summary.
- `draw_legend(fig,entries,y)`: vẽ chú giải.
- `execution_chart(...)`: biểu đồ thời gian thực thi.
- `speedup_chart(...)`: biểu đồ speedup.
- `efficiency_chart(...)`: biểu đồ efficiency.
- `convergence_chart(...)`: biểu đồ hội tụ theo lịch sử.
- `main()`: đọc dữ liệu kết quả và tạo bốn biểu đồ PNG/SVG.
- Khối `__main__`: điểm vào CLI.

## 6. Dữ liệu, test và tài liệu

- `data/square4.tsp`: instance hình vuông nhỏ để kiểm tra.
- `data/uniform2048.tsp`: tọa độ phân bố đều dùng trong thí nghiệm; có thể tái tạo bằng `generate_data.py`.
- `data/d15112.tsp` và `data/Tnm3001.tsp1`: instance với kích cỡ lớn để kiểm chứng khả năng mở rộng.
- `tests/tests.cpp`: `rejects` kiểm tra trường hợp phải báo lỗi; `square` và `random_problem` tạo fixture. Các nhóm `parser`, `tours`, `neighbors`, `copying`, `selection`, `local_search`, `pheromone`, `solver` kiểm tra từng thành phần. `check_profile` xác minh profile; `main` gọi các nhóm và tổng hợp trạng thái.
- `docs/algorithm.md`: quy trình, công thức, ví dụ và độ phức tạp.
- `docs/parallel.md`: dữ liệu chia sẻ/riêng, barrier và OpenMP profile.
- `docs/paper_mapping.md`: ánh xạ và khác biệt so với bài báo/mã tham khảo.
- `docs/experiments.md`, `docs/openmp_study.md`: cấu hình và kết quả thử nghiệm.
- `docs/delivery.md`: ghi chú đóng gói/bàn giao.
- `docs/giai_thich_ma_nguon.md`: tài liệu hiện tại.

## 7. Cấu hình và file đầu ra

- `CMakeLists.txt`: tạo `faco_core`, executable `faco`, `faco_tests`; cấu hình C++17, OpenMP, sanitizer và CTest.
- `.vscode/settings.json`, `c_cpp_properties.json`, `tasks.json`: cấu hình editor, compiler/include và task; các file `.bak` là cấu hình cũ.
- `package.json`/`package-lock.json` ở workspace cha khai báo và khóa phụ thuộc TypeSpec; solver C++ không gọi package này.
- `MANIFEST.sha256`, `FACO-lab.zip.sha256`: checksum, không tham gia runtime.
- `build-debug/`, `build-seq/`, `build-omp/`, `build-linux/`: cache và sản phẩm build, có thể tái tạo.
- CSV/JSON/TOUR/lịch sử/log/ảnh được ghi trong thư mục output, mặc định `results/`.

## 8. Thứ tự đọc gợi ý

1. `README.md` để build và chạy.
2. `include/faco/problem.hpp`, `tour.hpp`, `neighbors.hpp` để hiểu dữ liệu.
3. `src/problem.cpp`, `neighbors.cpp`, `tour.cpp` để xem tiền xử lý.
4. `src/faco.cpp` và `src/pheromone.cpp` để theo thuật toán chính.
5. `src/local_search.cpp` để hiểu cách cải thiện nghiệm.
6. `options.cpp`, `main.cpp`, `output.cpp` để theo vòng đời CLI.
7. `tests/tests.cpp` và `scripts/` để xem kiểm chứng/thí nghiệm.

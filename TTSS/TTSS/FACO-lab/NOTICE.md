# Ghi nhận nguồn

Thuật toán: Rafał Skinderowicz, *Improving Ant Colony Optimization Efficiency for Solving Large TSP Instances*, Applied Soft Computing (2022), 108653, DOI https://doi.org/10.1016/j.asoc.2022.108653. Bản manuscript do người dùng cung cấp được lưu nguyên vẹn tại [docs/FACO-Skinderowicz-2022.pdf](docs/FACO-Skinderowicz-2022.pdf), © 2022 Rafał Skinderowicz, theo giấy phép [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/). Không chỉnh sửa PDF; việc chia sẻ phải giữ ghi nhận nguồn, không thương mại và tuân thủ điều kiện không phân phối bản phái sinh. Xem [bài báo tham khảo](docs/references.md) để biết nguồn và cách trích dẫn.

Mã tham khảo: https://github.com/RSkinderowicz/FocusedACO tại commit `05b7c1b8d52b27d121b6138222f986fcbf196863`, kiểm tra ngày 2026-11-10 (Asia/Saigon). Đã xem README, Makefile, faco.cpp, ant.h, pheromone.h, problem_instance.h/.cpp, local_search.h/.cpp, kd_tree.h, progargs.h/.cpp, rand.h/.cpp, logging.h, utils.h/.cpp, best-known.json và instances/readme.md.

Không tìm thấy giấy phép cấp quyền chung ở gốc repository tại commit trên. Các thư viện đi kèm có giấy phép riêng (ví dụ cxxopts), không thể suy rộng sang toàn bộ mã FACO. Vì vậy dự án này tự viết mã C++/Python từ mô tả thuật toán, không phân phối mã tác giả, thư viện đi kèm hoặc bộ instances từ repository đó. Các cấu trúc, cách tổ chức, RNG và local search được viết độc lập; các lựa chọn thuật toán lấy từ bài báo/mã đã được ghi nhận trong docs/paper_mapping.md.

Các tệp data/square4.tsp, uniform2048.tsp và script sinh dữ liệu là dữ liệu dạy học được tạo cho dự án này. Nếu tải thêm TSPLIB hoặc dữ liệu khác, cần giữ ghi nhận và tuân thủ điều kiện phân phối của nguồn tương ứng. Kết quả pr2392 (nếu có trong experiments) là kết quả đo, không bao gồm tệp dữ liệu nguồn.

Công cụ xây dựng dùng ở lần đo: GNU GCC 16.2.0 / w64devkit 2.11.0, CMake 4.4.4. Đây là công cụ bên ngoài, không thuộc nguồn thuật toán được tái sử dụng. Bộ công cụ portable nằm ngoài thư mục mã dự án. Dự án không tự gán giấy phép của tác giả FACO cho mã mới; chủ sở hữu có thể bổ sung giấy phép phân phối phù hợp sau.

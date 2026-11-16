# Ghi chú bàn giao

Dự án đã được chép thành công vào workspace sau khi người dùng cấp quyền Modify ngày 2026-11-10:

- Mã nguồn, tài liệu và kết quả: `D:\TTSS\FACO-lab`
- Gói dự án: `D:\TTSS\FACO-lab.zip`
- Checksum gói: `D:\TTSS\FACO-lab.zip.sha256`

Gói chứa mã nguồn C++17, CMake, dữ liệu tự sinh, tài liệu tiếng Việt, kết quả đo thô, logs và hai executable Windows x64. Không chứa toolchain, bài báo, mã tác giả, instances tham khảo hoặc cây build CMake phụ thuộc đường dẫn cũ. MANIFEST.sha256 ghi hash các tệp trong dự án.

Chương trình đã được biên dịch và đo thực nghiệm tại thư mục tạm trước khi chép sang workspace. Các đường dẫn trong logs/metadata thực nghiệm giữ nguyên để phản ánh đúng nơi chạy ban đầu. Không cần dùng bản tạm để chạy hai executable Windows trong bin/windows; thao tác từ thư mục dự án mới theo README.

Khi xây dựng từ nguồn ở vị trí mới hoặc trên Ubuntu, dùng lệnh CMake trong README để tạo cây build mới. Binary Windows chỉ phụ thuộc DLL hệ thống KERNEL32 và msvcrt; không chạy trực tiếp trên Ubuntu.

Đã kiểm chứng bản tuần tự không OpenMP, bản OpenMP, Debug kiểm tra container/iterator, CLI/export, 27 lần benchmark và đối chiếu tour/lịch sử giữa các số luồng. Chưa kiểm chứng chạy trực tiếp trên Ubuntu, sanitizer/race detector hoặc hiệu quả 100k-200k đỉnh. Chi tiết trong experiments.md và paper_mapping.md.

Không commit, push, tạo PR hoặc tự sửa ACL. Trước khi chép, đã kiểm tra gói ZIP và manifest, đồng thời xác nhận không ghi đè tệp dự án có sẵn trong đích.

Sau khi xử lý cấu hình VS Code, GCC/CMake đã được chuyển sang vị trí ổn định `D:\TTSS\tools` (ngoài thư mục dự án và ngoài ZIP). Workspace và dự án có cấu hình IntelliSense, compile_commands và tác vụ build/test. Bản OpenMP đã được biên dịch lại tại D:\TTSS\FACO-lab và đạt 3/3 CTest; header pheromone.hpp qua kiểm tra cú pháp độc lập. Bộ công cụ không còn phụ thuộc bản trong thư mục tạm khi dùng cấu hình workspace này.

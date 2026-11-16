# Kết quả benchmark chính

| Instance | Số đỉnh | Báo cáo | Mức luồng | Ghi chú |
|---|---:|---|---|---|
| `uniform2048` | 2.048 | [Báo cáo benchmark OpenMP](openmp-benchmark-20261114-094522/report.md) | 1/2/4/8 | Workload chuẩn: 128 kiến, 200 vòng, local search bật; 3 seed × 5 lần đo. |
| `d15112` | 15.112 | [Báo cáo d15112](d15112/report.md) | 1/2/4/8 | 32 kiến, 40 vòng, 1 nghiệm khởi tạo, local search tắt; 3 seed × 3 lần đo. |
| `Tnm30001` | 30.001 | [Báo cáo Tnm30001](Tnm30001/report.md) | 1/2/4/8 | 32 kiến, 40 vòng, 1 nghiệm khởi tạo, local search tắt; 3 seed × 3 lần đo. |

Mỗi thư mục benchmark có `raw.csv`, `summary.csv`, `metadata.json`, thư mục `runs/` giữ kết quả từng lần chạy, và `figures/` gồm bốn biểu đồ PNG/SVG. `openmp-study-20261110/` được giữ lại vì là dữ liệu đối chiếu chiến lược OpenMP được trích dẫn trong `docs/openmp_study.md`.

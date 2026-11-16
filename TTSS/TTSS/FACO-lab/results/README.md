# Kết quả benchmark chính

| Instance | Số đỉnh | Báo cáo | Mức luồng | Ghi chú |
|---|---:|---|---|---|
| `uniform2048` | 2.048 | [Báo cáo benchmark OpenMP](uniform2048/report.md) | 1/2/4/8 | 128 kiến, 200 vòng, 4 nghiệm khởi tạo, local search bật; 3 seed × 5 lần đo. |
| `d15112` | 15.112 | [Báo cáo d15112](d15112/report.md) | 1/2/4/8 | 128 kiến, 200 vòng, 4 nghiệm khởi tạo, local search bật; 3 seed × 5 lần đo. |
| `Tnm30001` | 30.001 | [Báo cáo Tnm30001](Tnm30001/report.md) | 1/2/4/8 | 128 kiến, 200 vòng, 4 nghiệm khởi tạo, local search bật; 3 seed × 5 lần đo. |

Mỗi thư mục benchmark có `raw.csv`, `summary.csv`, `metadata.json`, thư mục `runs/` giữ kết quả từng lần chạy, và `figures/` gồm bốn biểu đồ PNG/SVG. `openmp-study-20261110/` được giữ lại vì là dữ liệu đối chiếu chiến lược OpenMP được trích dẫn trong `docs/openmp_study.md`.


# Bài báo tham khảo

## Bài báo gốc về FACO

**Rafał Skinderowicz (2022). _Improving Ant Colony Optimization Efficiency for Solving Large TSP Instances_.**

- arXiv: [2203.02228](https://arxiv.org/abs/2203.02228), công bố lần đầu ngày 04-03-2022.
- DOI của bài báo được chấp nhận trên *Applied Soft Computing*: [10.1016/j.asoc.2022.108653](https://doi.org/10.1016/j.asoc.2022.108653).
- [Đọc bản PDF trong dự án](FACO-Skinderowicz-2022.pdf): bản manuscript do người dùng cung cấp, trang đầu ghi `arXiv:2203.02228v1` và DOI trên. Tên tệp tải về có cụm “The Focused Ant Colony Optimization (FACO) for solving large TSP instances”; tiêu đề trích dẫn chính xác là tiêu đề in trong bài báo ở trên.
- [Mã nguồn tham khảo của tác giả](https://github.com/RSkinderowicz/FocusedACO), đối chiếu tại [commit 05b7c1b8d52b27d121b6138222f986fcbf196863](https://github.com/RSkinderowicz/FocusedACO/tree/05b7c1b8d52b27d121b6138222f986fcbf196863).

## Nội dung được tham khảo trong dự án

| Phần trong bài báo | Nội dung | Tài liệu dự án |
|---|---|---|
| Mục 4.1–4.2 | Candidate/backup lists, bộ nhớ pheromone một phần | [Thuật toán](algorithm.md) |
| Mục 4.3, Hình 2 | Xây dựng tour với ngưỡng cạnh mới và sao chép nghiệm nguồn | [Đối chiếu bài báo và mã](paper_mapping.md) |
| Mục 4.4, Hình 3 | Local search 2-opt sử dụng checklist | [Thuật toán](algorithm.md) |
| Mục 4.5 | Giới hạn pheromone | [Đối chiếu công thức và khác biệt triển khai](paper_mapping.md) |
| Mục 4.6 | Song song hóa bằng OpenMP trên bộ nhớ dùng chung | [Thiết kế song song](parallel.md) |
| Mục 5 | Phân tích tham số và thực nghiệm của tác giả | [Thực nghiệm của dự án](experiments.md), [kết quả benchmark](../results/README.md) |

Bài báo giới thiệu cơ chế giới hạn mức thay đổi so với nghiệm nguồn và dùng các thay đổi đó để tập trung local search. Những kết quả về instance 100.000–200.000 đỉnh trong bài là kết quả của tác giả, không phải kết quả của dự án này. Dự án triển khai phục vụ học tập, có khác biệt được ghi rõ trong `paper_mapping.md`; không khẳng định tái hiện toàn bộ thực nghiệm hoặc hành vi của mã tác giả.

## Trích dẫn BibTeX

```bibtex
@article{skinderowicz2022faco,
  author  = {Rafa{\l} Skinderowicz},
  title   = {Improving Ant Colony Optimization Efficiency for Solving Large {TSP} Instances},
  journal = {Applied Soft Computing},
  year    = {2022},
  doi     = {10.1016/j.asoc.2022.108653},
  eprint  = {2203.02228},
  archivePrefix = {arXiv},
  primaryClass  = {cs.NE},
  url     = {https://arxiv.org/abs/2203.02228}
}
```

## Nguồn và điều kiện sử dụng PDF

© 2022, Rafał Skinderowicz. Bản manuscript ghi giấy phép [Creative Commons Attribution–NonCommercial–NoDerivatives 4.0 International (CC BY-NC-ND 4.0)](https://creativecommons.org/licenses/by-nc-nd/4.0/). PDF trong thư mục này là bản sao nguyên vẹn của tệp người dùng cung cấp, không biên tập hoặc chuyển đổi nội dung. Khi chia sẻ, phải giữ ghi nhận tác giả, thông báo bản quyền và giấy phép; không dùng cho mục đích thương mại hoặc phân phối bản phái sinh trái điều kiện giấy phép. Giấy phép này áp dụng cho manuscript, không tự động áp dụng cho mã nguồn dự án hay repository tham khảo.

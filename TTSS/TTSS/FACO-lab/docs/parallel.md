# OpenMP: vị trí mã, đồng bộ và cách phân tích

## Vì sao mỗi vòng chạy nhanh?

Một vòng FACO xây dựng m tour, chạy 2-opt checklist cho từng tour, chọn nghiệm tốt nhất và cập nhật pheromone. Đây không phải một lần tìm kiếm đầy đủ mọi tour TSP. Với min_new_edges=8, kiến dùng quy tắc ACO cho đến khi đủ cạnh mới rồi tận dụng nghiệm nguồn. Nhiều đỉnh còn lại được sao chép; LS tập trung vào vùng thay đổi. Với n=2048, m=128, 200 vòng và 4 luồng, vài giây tìm kiếm là kết quả có thể xảy ra. In mỗi 4 vòng chỉ quyết định tần suất hiển thị.

Dòng tiến độ thêm `block_ms` và `rounds`: thời gian thực của nhóm vòng vừa chạy và số vòng trong nhóm. Vòng 0 là nghiệm sau khởi tạo. Hai trường này giúp đọc tốc độ trực tiếp; không có sleep hoặc thời gian chờ nhân tạo.

## Các đoạn OpenMP trong src/faco.cpp

`solve` dùng chung cho bản tuần tự và OpenMP. Khi FACO_OPENMP=OFF, tiền xử lý loại các pragma và omp.h; không liên kết OpenMP. Khi ON:

1. `omp_set_num_threads`, `omp_set_dynamic(0)`, `omp_set_schedule` cấu hình runtime. Một vùng ngắn ban đầu đo số luồng thực, giữ phạm vi timing giống phiên bản trước.
2. **Một `omp parallel default(none)` duy trì team xuyên suốt vòng lặp FACO.** Không tạo ba parallel region trong từng vòng nữa. Runtime OpenMP vốn có thể tái sử dụng thread pool; thay đổi này giảm số lần vào/ra vùng và đồng bộ fork/join, không đồng nghĩa phiên bản cũ tạo mới OS thread mỗi lần.
3. `omp for schedule(runtime)` chia các kiến. Mỗi kiến gồm construct, 2-opt và tính độ dài; hỗ trợ dynamic,1 hoặc static,1.
4. `omp single` chọn iteration/global best, cập nhật cận và nguồn.
5. `omp for schedule(static)` chia các slot bay hơi pheromone.
6. `omp single` bồi đắp pheromone.
7. `omp for schedule(static)` chia các slot chặn cận.
8. `omp single` kiểm tra bất biến, ghi lịch sử và gọi callback in tiến độ.

Các luồng đều đi qua vòng lặp iteration để gặp cùng các điểm đồng bộ. `omp for` chia **tổng m kiến**, không phải mỗi luồng tự làm thêm m kiến. Với 128 kiến và 200 vòng, tổng số tour chính phải là **25600** ở mọi số luồng.

```cpp
#pragma omp parallel default(none) shared(/* dữ liệu đã khai báo tường minh */)
{
    // RNG, ant, visited, checklist, bộ đếm cục bộ: riêng mỗi luồng/kiến
    for (int iteration = 0; iteration < iterations; ++iteration) {
        #pragma omp for schedule(runtime)
        for (int ant = 0; ant < ants; ++ant) { /* construct + 2-opt */ }
        #pragma omp single
        { /* chọn best, nguồn, cận */ }
        #pragma omp for schedule(static)
        for (int slot = 0; slot < slots; ++slot) { /* bay hơi */ }
        #pragma omp single
        { /* bồi đắp */ }
        #pragma omp for schedule(static)
        for (int slot = 0; slot < slots; ++slot) { /* chặn cận */ }
        #pragma omp single
        { /* lịch sử và tiến độ */ }
    }
}
```

Đoạn trên minh họa cấu trúc; mã đầy đủ còn chứa timing, ownership và truyền ngoại lệ. Không dùng nowait: các barrier ngầm ở cuối `for` và `single` bảo đảm mọi tour đã xong trước chọn best, bay hơi trước deposit, deposit trước clamp và clamp trước kiến vòng sau. Theo [đặc tả worksharing của OpenMP 5.2](https://www.openmp.org/spec-html/5.2/openmpch11.html), các worksharing region có barrier cuối khi không có nowait; mọi luồng trong team phải gặp cùng các construct tương ứng.

## Shared/private và xử lý lỗi

| Dữ liệu | Trong pha kiến | Cập nhật |
|---|---|---|
| Problem, Neighbors, heuristic | Shared, chỉ đọc | Tuần tự trước tìm kiếm |
| Source và Pheromone | Shared, chỉ đọc | Single/for ở các pha sau barrier |
| tours[ant], costs[ant] | Mỗi ant có phần tử riêng | Không resize vector ngoài trong vùng kiến |
| RNG, Construction, visited, checklist, position, queue | Riêng từng ant | Chỉ ant đó sửa |
| Thống kê worker | Bộ đếm cục bộ từng luồng | Ghi một lần vào workers[thread] khi kết thúc team |
| Global best, lịch sử, callback tiến độ | Không sửa trong pha kiến | Single |

`default(none)` buộc khai báo biến dùng chung. Từng thread tích lũy bộ đếm và thời gian xử lý kiến ở biến cục bộ, giảm tranh chấp/false sharing; không dùng atomic cho mỗi lần tăng bộ đếm kiến. Atomic chỉ là cờ báo lỗi hiếm gặp.

Ngoại lệ từ ant, cập nhật nguồn hoặc callback được lưu trong exception_ptr qua critical. Tất cả luồng vẫn đi qua các barrier, nhận quyết định dừng chung ở single cuối vòng, rồi rời team. Chỉ sau join mới rethrow trên luồng gọi. Quyết định dừng không đọc trực tiếp cờ lỗi ở điểm một luồng có thể đã bắt đầu vòng tiếp theo, tránh các luồng rẽ nhánh khác nhau gây kẹt barrier. Tests có ca callback ném lỗi từ vòng 4 rồi chạy lại solver thành công.

Callback tiến độ được thực thi bởi một luồng ở single, có thể là worker khác nhau giữa các vòng; các lần gọi không đồng thời. Không gọi API giao diện đòi hỏi main-thread từ callback nếu không tự chuyển sự kiện sang main-thread.

## Lịch phân công và RNG

Bài báo 4.6 ghi dynamic,1; commit tác giả đã đối chiếu dùng static,1. Dynamic chia tiếp kiến cho luồng rảnh, hữu ích khi LS từng kiến tốn thời gian khác nhau; static có ít phí phân công hơn và gán chỉ số kiến theo lịch cố định. Không mặc định một lịch luôn nhanh hơn.

RNG dự án theo seed/phase/iteration/ant bằng mt19937_64 và seed_seq, không dùng RNG chung. Phase 0 khởi tạo, 1 xử lý kiến, 2 chọn nguồn. Vì vậy thay lịch hoặc số luồng không tự đổi luồng số ngẫu nhiên của từng ant. Tests đối chiếu cả tour và toàn bộ lịch sử ở 1/2/4 luồng, static/dynamic, profiler on/off. Điều này là bằng chứng trên môi trường hiện có; không cam kết bit-for-bit cho mọi compiler/CPU. Mã tác giả dùng RNG theo luồng, nên kết quả không bit-for-bit với dự án này.

## Đo từng pha và khối lượng mỗi luồng

Bật bằng `--profile on`; mặc định off. Ví dụ trong PowerShell:

```powershell
cd D:\TTSS\FACO-lab
.\build-omp\faco.exe --input data/uniform2048.tsp --ants 128 --iterations 200 --threads 4 --seed 11 --initial-solutions 4 --schedule dynamic --log-every 4 --profile on --output-dir results/openmp-live
```

Console và JSON ghi:

- `ant_phase_seconds`: construct + LS + tính độ dài, gồm chờ kết thúc pha.
- `selection_seconds`: chọn best, cận và source trong single.
- `evaporation_seconds`, `deposit_seconds`, `clamp_seconds`: cập nhật pheromone theo pha.
- `bookkeeping_seconds`: verify, lịch sử và callback/console.
- Mỗi worker: số kiến đã hoàn thành, thời gian thực trong phần xử lý kiến, số cạnh chọn bằng ACO, số đỉnh sao chép, số move 2-opt.

Các khoảng pha được đo bởi một luồng ở ranh giới đã đồng bộ. Chúng gồm chi phí barrier/lập lịch; chi phí cuối single được quy vào pha tiếp theo. Đây không phải đo barrier riêng, CPU time hay thời gian kernel. Tổng pha có thể lệch search_seconds do overhead đo, khởi tạo kết thúc team và các lệnh nằm ngoài khoảng pha. Không cộng thời gian worker rồi so trực tiếp với wall clock vì nhiều worker chạy chồng thời gian.

Hai bất biến kiểm chứng công việc thật:

```text
sum(workers.ants_completed) = ants * iterations
sum(workers.aco_selected_edges + workers.copied_vertices)
    = ants * iterations * (dimension - 1)
```

Cạnh đóng tour không nằm trong bộ đếm này. Static,1 có phân bố số kiến xác định; dynamic có thể cho số kiến khác nhau giữa worker mà tour/best vẫn giữ nguyên. Thời gian worker và số kiến giúp nhận diện mất cân bằng nhưng không đủ để khẳng định mức sử dụng CPU của hệ điều hành.

## Benchmark và hướng phát triển

Đo speedup với log/profiler tắt; profiler có gọi đồng hồ cho từng ant và làm thay đổi thời gian đo. Script benchmark tự truyền log-every=0; chỉ thêm profile khi có cờ --profile.

```powershell
python scripts/benchmark.py --binary build-omp/faco.exe --input data/uniform2048.tsp --threads 1 2 4 6 --seeds 11 22 33 --ants 128 --iterations 200 --initial-solutions 4 --schedule dynamic --output-dir results/openmp-bench-new
```

T1 là search_seconds của binary OpenMP 1 luồng; Tp cùng binary/schedule ở p luồng, ghép cùng seed rồi lấy median T1/Tp. Efficiency=S/p. Không trộn timing bật profiler với tắt profiler hoặc log khác nhau. Initialization vẫn tuần tự, nên total speedup nhỏ hơn search speedup; muốn giảm total cần khảo sát parallel initialization riêng.

Kết quả trước/sau persistent region và so static/dynamic được lưu riêng trong openmp_study.md; các số đo cũ trong experiments.md được giữ nguyên. Hướng tiếp theo dựa trên bằng chứng: nếu ant phase chiếm ưu thế thì tối ưu vùng nhớ tạm và cân bằng LS; nếu selection/deposit chiếm đáng kể thì tối ưu phần tuần tự; nếu init lớn thì song song các nghiệm NN/LS độc lập. Chưa chạy TSan/Helgrind; kiểm thử kết quả và Debug assertions không thay thế công cụ phát hiện race.

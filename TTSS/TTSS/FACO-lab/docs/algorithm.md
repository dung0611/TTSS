# Thuật toán và cấu trúc dữ liệu

## Đầu vào, đầu ra, bất biến

Đầu vào là TSPLIB TSP/EUC_2D, n tọa độ 2D. ID được ánh xạ về 0..n-1. Khoảng cách `d(i,j)=floor(hypot(xi-xj,yi-yj)+0.5)` là số nguyên; bằng 0 được phép cho tọa độ trùng hoặc rất gần. Tổng tour dùng int64, gồm cạnh từ đỉnh cuối về đỉnh đầu.

Mọi tour trả về là hoán vị n đỉnh. `visited` ngăn kiến thêm lại một đỉnh, nên không có chu trình con trong quá trình xây dựng. Chỉ đóng chu trình sau khi đã có đủ n đỉnh. FACO tối thiểu hóa độ dài nhưng không có bảo đảm tối ưu toàn cục.

## Tiền xử lý và khởi tạo

1. Đọc/kiểm tra dữ liệu, từ chối định dạng ngoài phạm vi.
2. Dựng k-d tree cân bằng, lấy k láng giềng gần nhất theo `(d đã làm tròn, ID)`. Với khoảng cách hòa, truy vấn vẫn duyệt nhánh có cận bằng khoảng cách tệ nhất để không bỏ ID nhỏ hơn. Kết quả đã được so với vét cạn trên các ca ngẫu nhiên và tọa độ trùng.
3. Candidate là prefix độ dài cl, backup là nhóm tiếp theo, không trùng nhau hoặc chứa chính đỉnh. LS dùng prefix riêng. Các kích thước giảm để không vượt n-1.
4. Tạo nhiều tour NN từ điểm bắt đầu ngẫu nhiên; mặc định số tour bằng CPU logic, có thể cố định bằng `--initial-solutions`. Mỗi bước NN lấy đỉnh chưa thăm đầu tiên trong danh sách, nếu hết thì quét tất cả đỉnh còn lại. Khi bật LS, chạy 2-opt checklist ban đầu chứa mọi đỉnh, sau đó 3-opt restricted; chọn tour ngắn nhất làm nguồn và global best.

Khởi tạo dùng cùng `--ls-size` với main loop; mặc định 20, khác init16 trong mã tác giả. `three_opt` dùng hai neo gần nhau để giới hạn bộ ba cạnh cắt, loại cạnh kề, đánh giá các thứ tự/chiều của hai đoạn còn lại trong O(1) mỗi tái nối, rồi ghép lại tour trong O(n) nếu giảm chi phí. Xét 7 tái nối khác tour gốc (3 tương đương 2-opt và 4 3-opt thực), dừng khi không tìm thấy cải thiện hoặc đã có n move. Không bảo đảm cực tiểu 3-opt đầy đủ, không dùng don't-look bits. Có test tour 8 đỉnh không còn move 2-opt cải thiện nhưng 3-opt giảm được chi phí.

## Pheromone

Chỉ lưu n*cl số thực; không có ma trận n*n ở bất kỳ kích thước nào. Mỗi dòng tương ứng danh sách ứng viên riêng. Một cạnh có thể chỉ có một chiều lưu: bồi đắp các chiều có tồn tại, không tạo chiều thiếu. Nếu cả hai chiều có tồn tại thì khởi tạo/cập nhật giống nhau. `get(u,v)` ngoài dòng u trả giá trị mặc định đã bay hơi và chặn cận, kể cả khi chiều ngược được lưu. Vì chọn ACO chỉ dùng candidate của u, giá trị ngoài danh sách không được dùng để roulette; backup/fallback chỉ theo khoảng cách.

Với L là global best, `tau_max=1/(max(L,1)*(1-rho))`. Gọi q là căn của p_best:

```text
LS bật: avg=cl; q=p_best^(1/n) ở chế độ paper
LS bật: avg=cl; q=p_best^(1/cl) ở chế độ reference
LS tắt: avg=n/2; q=p_best^(1/n)
tau_min=min(tau_max, tau_max*(1-q)/((avg-1)*q))
```

Khi avg<=1 dùng tau_min=tau_max. Cận được cập nhật nếu global best cải thiện. Khởi tạo tất cả bằng tau_max. Mỗi vòng:

```text
delta(u,v) = 1/max(L_source,1) nếu cạnh nằm trong source, ngược lại 0
tau_new(u,v) = clamp(rho*tau_old(u,v) + delta(u,v), tau_min, tau_max)
```

Rho là phần **còn lại**. Triển khai bay hơi trước, cộng delta sau, cuối cùng chặn cận: giá trị tạm ở giữa hai bước có thể dưới tau_min nhưng không kiến nào đọc lúc đó. Cách này giữ đúng Eq.2; chặn dưới trước rồi mới cộng delta như mã tham khảo có thể cho số khác. `--verify on` kiểm tra toàn bộ cận sau mỗi vòng.

## Xây dựng FACO và cải thiện

Chọn điểm đầu ngẫu nhiên và đưa vào checklist. Mỗi bước ACO lấy ứng viên chưa thăm theo trọng số `tau * eta^beta`, alpha=1, eta=1/d nếu d>0 và eta=1 nếu d=0. Nếu tổng trọng số bằng 0 do underflow, chọn ứng viên đầu tiên. Nếu hết ứng viên, lấy đỉnh chưa thăm đầu tiên trong backup; nếu vẫn hết, tìm đỉnh chưa thăm gần nhất trên toàn bộ phần còn lại, hòa theo ID.

Mỗi cạnh ACO không thuộc nguồn theo cả hai chiều làm tăng new_edges và thêm đỉnh đến vào checklist. Từ khi đạt min_new_edges, thử nối tiếp theo nguồn: đi thuận cho đến đỉnh đã thăm; hủy bước thử thất bại bằng cách giữ cursor ở đỉnh trước nó; thử đi ngược từ cursor đó. Nếu vẫn thiếu đỉnh, trở lại chọn một cạnh ACO và thử sao chép tiếp. Mỗi lượt ngoài luôn thêm ít nhất một đỉnh nên kết thúc sau tối đa n-1 lựa chọn ACO. Cạnh đóng tour không được cộng vào bộ đếm; các mối nối phát sinh khi sao chép hoặc đóng tour có thể tạo thêm khác biệt.

2-opt lấy một đỉnh a trong checklist, xét hai nhóm trong Hình 3: hai cạnh đi ra từ a,b và hai cạnh đi vào a,b, với b thuộc NN(a). Bỏ cặp cạnh kề hoặc trùng. Chọn move có gain lớn nhất dương theo điều kiện cắt nhánh khoảng cách của bài. Đảo đoạn, cập nhật chỉ số vị trí và thêm bốn đầu mút vào queue nếu chưa đang chờ. Một đỉnh đã xử lý có thể được thêm lại. Dừng khi queue rỗng hoặc đã có n move; chỉ chấp nhận cải thiện nghiêm ngặt. Khi bật verify, kiểm tra lại hoán vị và đối chiếu tổng thực với gain sau từng move.

Sau khi mọi kiến xong, chọn iteration best bằng thứ tự chỉ số kiến khi hòa, cập nhật global best nếu ngắn hơn, tính lại cận nếu cần. Chọn global best làm nguồn với xác suất .01, còn lại iteration best; cùng tour nguồn dùng để bồi đắp. Lưu global best qua từng vòng và trả về nó.

## Ví dụ sáu đỉnh

Dùng ID nội bộ 0-based. Tọa độ: 0=(0,0), 1=(10,0), 2=(20,0), 3=(20,10), 4=(10,10), 5=(0,10). Nguồn `[0,1,2,3,4,5]`, chu trình dài 60. Đặt threshold=1; đây là một đường lựa chọn minh họa hợp lệ, không khẳng định seed tùy ý luôn chọn nó.

| Bước | Cạnh/thao tác | Cạnh mới theo nguồn? | new_edges | Checklist |
|---|---|---|---:|---|
| Bắt đầu | 0 | - | 0 | [0] |
| ACO | 0->2 | Có, vì nguồn chỉ kề 0 với 1,5 | 1 | [0,2] |
| Chuyển cơ chế | Đạt threshold=1 | - | 1 | [0,2] |
| Sao chép thuận | 2->3->4->5, dừng trước 0 đã thăm | Không | 1 | [0,2] |
| Thử ngược | Từ 5 gặp 4 đã thăm | Không thêm đỉnh | 1 | [0,2] |
| Quay lại ACO | 5->1 (đỉnh còn lại) | Có | 2 | [0,2,1] |
| Đóng tour | 1->0 | Có trong nguồn, không đếm đóng tour | 2 | [0,2,1] |

Tour trước LS `[0,2,3,4,5,1]` có tổng `20+10+10+10+14+10=74`. Xét a=0, b=5: bỏ cạnh (0,2) dài 20 và (5,1) dài 14; thêm (0,5) dài 10 và (2,1) dài 10. Gain=14. Đảo đoạn `[2,3,4,5]` thành `[5,4,3,2]`, được `[0,5,4,3,2,1]` dài 60. Queue sau move có thêm các đầu mút 0,2,5,1 (không lặp các đỉnh còn đang chờ). Toàn bộ trace và kết quả 74->60 được kiểm thử trong `tests/tests.cpp`.

## Độ phức tạp và giới hạn bộ nhớ

Tọa độ O(n), k-d tree O(n), hàng xóm O(n*max(cl+backup,ls)), pheromone và heuristic O(n*cl). Không có bộ đệm khoảng cách dày. Truy vấn k-d tree thường nhanh trên tọa độ 2D nhưng trường hợp xấu có thể duyệt cả cây, đặc biệt nhiều tọa độ trùng. NN khởi tạo có fallback quét nên vẫn có trường hợp O(n²). LS đảo đoạn dùng vector O(n) mỗi move, giới hạn n move nên 2-opt xấu nhất O(n²) khi ls là hằng số; 3-opt khởi tạo có thể tốn hơn do quét lại bộ ba sau mỗi cải thiện.

Các tour của m kiến được giữ để chọn nguồn: O(m*n) số nguyên, không phải O(n) cho toàn chương trình. Trạng thái construction/LS riêng của p luồng là O(p*n). Ví dụ n=100k,m=512 chỉ riêng tour các kiến cần khoảng 195 MiB, chưa kể vùng tạm/hàng xóm. Do đó pheromone tuyến tính không đồng nghĩa tổng bộ nhớ luôn tuyến tính theo n khi m tăng. Chưa benchmark 100k-200k; không cam kết khả năng xử lý hiệu quả các cỡ đó.

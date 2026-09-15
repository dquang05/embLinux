# Tối ưu hóa nâng cao & Kiến thức Kernel

Tài liệu này chứa các kiến thức đặc thù, các thuật toán sâu bên trong Kernel, và các API nâng cao dùng để tối ưu hiệu năng cực gắt hoặc xử lý các yêu cầu khắt khe (Real-time, Database, Security).

## 1. Thuật toán của Kernel (OS Internal)
- **Swapping & Memory Compression**: 
  - Hệ thống Mobile (như Android) thường không dùng Swap đĩa, mà dùng nén bộ nhớ (Memory Compression - ZRAM) để nén các trang ít dùng thẳng trên RAM đa nhân, vì tốc độ nén/giải nén nhanh hơn ghi ổ cứng Flash.
- **Thuật toán Page Replacement**:
  - Khi hết RAM, OS chọn một trang để đẩy xuống Swap. Thường dùng thuật toán xấp xỉ LRU (Least Recently Used) như *Second-Chance (Clock algorithm)*.
  - HĐH ưu tiên chọn các trang "sạch" (Clean - chưa bị sửa đổi, tức Dirty bit = 0) để loại bỏ vì không tốn thời gian ghi lại xuống đĩa.
- **Thrashing & Working-Set**:
  - Thrashing là hiện tượng hệ thống tốn nhiều thời gian để swap in/out các trang nhớ hơn là thực thi lệnh do thiếu RAM. HĐH chống lại bằng cách theo dõi Working-Set (tập các trang dùng gần đây) và tự đình chỉ bớt tiến trình nếu quá tải.
- **Kernel Memory Allocation**:
  - *Buddy System*: Cấp phát bộ nhớ cho Kernel theo lũy thừa của 2, dễ gom mảnh nhưng bị nội phân mảnh.
  - *Slab Allocation (SLAB/SLUB)*: Quản lý bộ đệm (cache) cho các object thường dùng của Kernel. Cấp phát siêu nhanh và loại bỏ hoàn toàn phân mảnh.

## 2. API Tối ưu hóa Đặc thù (Virtual Memory Operations)
- **`mlock()` / `mlockall()` (Memory Pinning)**:
  - Khóa vĩnh viễn các trang nhớ trên RAM vật lý, cấm HĐH đẩy xuống Swap.
  - *Ứng dụng*: Lập trình hệ thống Real-time (cần độ trễ thấp, không được phép dính Page Fault) hoặc bảo mật (không để lọt key mã hóa xuống ổ cứng).
- **`madvise()` / `posix_madvise()`**:
  - Gợi ý cho Kernel về cách chương trình sẽ dùng bộ nhớ sắp tới để HĐH ra quyết định tối ưu (Read-ahead hoặc dọn dẹp).
  - Ví dụ: `MADV_SEQUENTIAL` (đọc tuần tự -> nên read-ahead nhiều), `MADV_RANDOM` (đọc ngẫu nhiên -> đừng read-ahead), `MADV_WILLNEED` (sắp dùng tới -> nạp trước đi), `MADV_DONTNEED` (không dùng nữa -> dọn dẹp RAM đi).
- **`msync()`**:
  - HĐH tự động lưu dữ liệu của `MAP_SHARED` xuống đĩa nhưng không theo thời gian thực. `msync(MS_SYNC)` ép HĐH đồng bộ ngay lập tức và block cho đến khi ghi xong. Thường dùng trong Database để đảm bảo dữ liệu không bị mất khi mất điện.
- **`mincore()`**: Kiểm tra xem một trang nhớ cụ thể có đang nằm sẵn trên RAM vật lý hay đã bị đẩy xuống đĩa.
- **`mprotect()`**: Thay đổi quyền truy cập của bộ nhớ (Read/Write/Exec) lúc runtime. Hữu ích để tạo Guard Pages (set `PROT_NONE`) ngăn chặn Stack Overflow.

## 3. Các lưu ý về Cấu trúc dữ liệu
- **TLB Reach & Huge Pages**: Tăng hiệu suất TLB bằng cách dùng trang nhớ siêu lớn (2MB, 1GB thay vì 4KB) trong các ứng dụng ngốn RAM.
- **Program Structure**: Duyệt mảng đa chiều đúng theo cách bộ nhớ sắp xếp (Row-major trong C/C++) giúp giảm số lượng Page Fault đi hàng chục đến hàng trăm lần.

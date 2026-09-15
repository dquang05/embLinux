# Tổng quan: Memory Management (Linux System Engineering)

Tài liệu này hệ thống hóa các khái niệm cốt lõi về Quản lý bộ nhớ trong Linux, được chia làm 2 cấp độ: kiến thức nền tảng bắt buộc phải có cho mọi System/Embedded Engineer, và các kỹ thuật tối ưu hóa nâng cao cho các trường hợp đặc thù (Real-time, Database, High-performance).

*(Nguồn tham khảo: File OS.txt - Chương 9 & 10, File Syntax.txt - Chương 7, 49 & 50).*

---

## 1. Kiến thức Cốt lõi (Core Foundations)
Đây là những nền tảng **bắt buộc phải nắm vững** khi lập trình ứng dụng C/C++ trên Linux. Không thể thiết kế hệ thống tốt nếu không hiểu các cơ chế này.

👉 **[Xem chi tiết: core_knowledge.md](core_knowledge.md)**

**Tóm tắt các điểm chính:**
- **Kiến trúc Bộ nhớ**: 
  - CPU thao tác với **Địa chỉ ảo (Virtual Address)**. Phần cứng **MMU** chịu trách nhiệm biên dịch sang **Địa chỉ vật lý (Physical Address)** trên RAM thông qua cơ chế **Phân trang (Paging)**.
  - Cơ chế **Demand Paging** và **Copy-on-Write (COW)** giúp HĐH nạp và khởi tạo tiến trình cực kỳ nhanh chóng, tối ưu hóa lượng RAM sử dụng.
- **Memory Allocation APIs (Cấp phát bộ nhớ)**:
  - Bản chất của `malloc()` / `free()` hoạt động dựa trên việc điều chỉnh ranh giới Heap (program break - hàm `brk`/`sbrk`).
  - Lỗi ghi tràn bộ đệm (Buffer Overflow) thường phá hỏng metadata nội bộ của `malloc`, dẫn đến crash khi gọi `free()`.
  - Có thể cấp phát bộ nhớ động trên Stack siêu nhanh bằng `alloca()` (tuyệt đối không được gọi `free()`).
- **Memory Mappings (`mmap`)**:
  - Ánh xạ trực tiếp file hoặc thiết bị (`/dev/mem`) vào bộ nhớ ảo.
  - Phân chia làm 2 quyền chia sẻ: `MAP_PRIVATE` (thay đổi không ghi xuống file gốc) và `MAP_SHARED` (các tiến trình dùng chung bộ nhớ, dữ liệu được ghi xuống file gốc).
  - Đây là nền tảng cho **Memory-Mapped I/O** (tăng tốc độ đọc/ghi file khổng lồ) và là một cơ chế IPC vô cùng mạnh mẽ.

---

## 2. Tối ưu hóa Nâng cao & Specific Cases (Advanced Optimizations)
Đây là những kỹ năng chuyên sâu, chỉ nên áp dụng khi cần tối ưu hiệu năng khắt khe, lập trình Real-time, hoặc yêu cầu bảo mật cao. Việc lạm dụng không đúng chỗ có thể làm hệ thống mất ổn định.

👉 **[Xem chi tiết: advanced_optimizations.md](advanced_optimizations.md)**

**Tóm tắt các điểm chính:**
- **Kiểm soát RAM Vật lý**:
  - `mlock()` / `mlockall()` (Memory Pinning): Khóa cứng trang nhớ trên RAM, cấm HĐH đẩy xuống ổ đĩa (Swap). Bắt buộc phải dùng trong hệ thống Real-time để tránh độ trễ do Page Fault, hoặc giữ an toàn cho key mã hóa.
  - `mincore()`: Kiểm tra trạng thái hiện tại của một vùng nhớ ảo xem nó có thực sự nằm trên RAM hay không.
- **Tương tác và "Gợi ý" cho Kernel**:
  - `madvise()`: Chủ động báo cho Kernel biết thói quen truy cập bộ nhớ sắp tới (vd: truy cập tuần tự, ngẫu nhiên, hay sẽ không dùng tới nữa) để Kernel tối ưu hóa bộ nhớ đệm (Read-ahead cache).
  - `msync()`: Ép HĐH phải đồng bộ (flush) ngay lập tức vùng nhớ `MAP_SHARED` xuống ổ cứng (dùng bảo vệ tính toàn vẹn dữ liệu cho Database).
- **Tối ưu kiến trúc (Architecture Specific)**:
  - Hiểu cách cấu trúc mảng đa chiều trong code (Row-major vs Column-major) ảnh hưởng chí mạng đến số lượng Page Fault.
  - Sử dụng **Huge Pages** để tăng cường giới hạn TLB Cache của CPU.
  - Nắm được cách Kernel quản lý vùng nhớ riêng của nó qua *Buddy System* và *Slab Allocator*.

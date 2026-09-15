# Kiến thức Cốt lõi: Linux Memory Management

Tài liệu này chứa các kiến thức nền tảng bắt buộc đối với một Embedded Linux Engineer khi làm việc với bộ nhớ.

## 1. Cơ chế Bộ nhớ & Phân trang (Paging)
- **MMU (Memory Management Unit)**: Đơn vị phần cứng ánh xạ địa chỉ ảo (Logical Address) do CPU sinh ra sang địa chỉ vật lý (Physical Address) trên RAM tại runtime.
- **Paging (Phân trang)**:
  - Chia bộ nhớ vật lý thành các **Frames** (khung trang) và bộ nhớ ảo thành các **Pages** (trang) có kích thước bằng nhau (thường 4KB).
  - Không gian vật lý không cần liên tục, giúp giải quyết triệt để vấn đề phân mảnh ngoại (External Fragmentation).
- **Virtual Memory & Demand Paging**:
  - HĐH chỉ nạp các trang nhớ từ đĩa lên RAM khi có yêu cầu thực sự (tiến trình truy cập vào).
  - **Page Fault**: Xảy ra khi CPU truy cập một trang ảo chưa có trên RAM vật lý. HĐH sẽ bẫy (trap), nạp trang đó từ ổ cứng (hoặc swap) vào RAM, rồi chạy tiếp.
  - **Copy-on-Write (COW)**: Khi gọi `fork()`, tiến trình con dùng chung RAM với tiến trình cha. Chỉ khi 1 trong 2 tiến trình ghi/sửa dữ liệu, HĐH mới tạo bản sao của trang đó. Giúp khởi tạo tiến trình cực nhanh.

## 2. Các hàm cấp phát bộ nhớ (Memory Allocation APIs)
- **`brk` / `sbrk`**: Thay đổi kích thước vùng Heap bằng cách dịch chuyển ranh giới *program break*. OS chỉ cấp phát bộ nhớ ảo, RAM vật lý chỉ được cấp khi tiến trình thực sự truy cập (Lazy Allocation).
- **`malloc()` và `free()`**:
  - `malloc(size)` xin cấp phát bộ nhớ động. Luôn cấp phát dư vài byte phía trước con trỏ để lưu kích thước khối nhớ (metadata).
  - Việc ghi tràn bộ đệm (buffer overflow) phá hỏng metadata này sẽ làm chương trình crash (SIGSEGV) khi gọi `free()`.
  - `free()` thường chỉ trả vùng nhớ vào *free list* để dùng lại chứ ít khi trả RAM lại cho HĐH (trừ khi khối nhớ cực lớn và nằm trên đỉnh Heap).
- **`calloc(num, size)`**: Cấp phát và tự động khởi tạo bằng 0.
- **`realloc(ptr, size)`**: Đổi kích thước vùng nhớ. Nếu phải di chuyển sang chỗ mới lớn hơn, hàm sẽ copy dữ liệu cũ và trả về con trỏ mới.
- **`alloca(size)`**: Cấp phát trên **Stack** thay vì Heap. Cực nhanh và tự động giải phóng (auto-freed) khi hàm `return`. Rất dễ gây Stack Overflow nếu xin cấp phát quá lớn.

## 3. Memory Mappings (`mmap`) & IPC
- **`mmap()`**: Ánh xạ một file hoặc thiết bị (như `/dev/mem`) thẳng vào không gian bộ nhớ ảo của tiến trình. Thao tác với file lúc này nhanh như thao tác với mảng RAM.
- Phân loại ánh xạ:
  - **File mapping**: Ánh xạ từ 1 file thật.
  - **Anonymous mapping**: Không có file tương ứng, bộ nhớ được khởi tạo bằng 0. Dùng ngầm bởi `malloc` để xin vùng nhớ lớn.
- Quyền chia sẻ (Visibility):
  - **`MAP_PRIVATE`**: Dùng Copy-on-Write, không làm thay đổi file gốc. (Dùng để nạp Shared Library / file thực thi).
  - **`MAP_SHARED`**: Các tiến trình chia sẻ chung RAM, mọi thay đổi được lưu (flush) xuống file gốc. 
- **Memory-Mapped I/O**: Dùng `mmap` (`MAP_SHARED`) thay cho `read/write` tăng tốc độ cực lớn với các file kích thước bự hoặc truy cập ngẫu nhiên, nhờ bỏ qua được bước copy dữ liệu từ Kernel Space sang User Space.

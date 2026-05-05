##1. Bài tập 2.1: Cài đặt GDB
```
make menuconfig -> Target packages -> Debugging, profiling... -> gdb -> full debugger/gdbserver
```
2. Bài tập 2.2: Sử dụng GDB điều khiển luồng chương trình
```
gdb ./main_app
```
Trong môi trường GDB, các thao tác sau đã được thực thi:

Đặt Breakpoint: b 20 (lỗi do file chưa chuẩn) và chuyển sang b main (đặt điểm dừng thành công tại dòng 13 của hàm main).

Điều khiển luồng: Sử dụng lệnh r (run) để chạy đến điểm dừng. Tiếp tục dùng n (next) để chạy từng dòng không nhảy vào hàm con, và s (step) để nhảy sâu vào bên trong các khối lệnh.

Can thiệp biến số:

In giá trị biến: p counter (kết quả $1 = 0).

Gán giá trị trực tiếp ở runtime: set var counter = 1. Khi kiểm tra lại bằng p counter, kết quả trả về $2 = 1 chứng tỏ bộ nhớ đã được can thiệp thành công mà không cần biên dịch lại code.

Kiểm tra thanh ghi: Lệnh info registers xuất ra toàn bộ trạng thái của các thanh ghi trên kiến trúc ARM (từ r0 đến r12, sp, lr, pc, cpsr). Giá trị bộ đếm chương trình pc đang trỏ đúng đến lệnh hiện tại đang dừng.

<img width="861" height="857" alt="image" src="https://github.com/user-attachments/assets/f89e59c1-e77f-41b4-bf84-5732db85dc14" />

<img width="883" height="656" alt="image" src="https://github.com/user-attachments/assets/519ca222-6fed-4f9f-bfb6-92d3df722c26" />

3. Bài tập 2.3: Phân tích rò rỉ bộ nhớ (Memory Leak) với Valgrind

```
valgrind --leak-check=full ./leak_app
```
Phân tích kết quả:
Báo cáo HEAP SUMMARY của Valgrind chỉ ra rõ ràng:

in use at exit: 400 bytes in 1 blocks: Khi chương trình kết thúc, vẫn còn 400 bytes đang bị chiếm dụng.

definitely lost: 400 bytes in 1 blocks: Hệ thống xác nhận 400 bytes này đã bị "mất hoàn toàn" (rò rỉ).

Truy vết (Traceback): Valgrind định vị chính xác lỗi xảy ra tại quá trình gọi hàm malloc. Điều này giúp lập trình viên nhanh chóng tìm đến dòng code cấp phát mảng để bổ sung lệnh free().

<img width="863" height="438" alt="image" src="https://github.com/user-attachments/assets/579c73d7-f6cd-445b-bda0-f23e6a4a8bf8" />

4. Bài tập 2.4: Phân tích Core Dump

```
ulimit -c unlimited
./crash_app
```

Lúc này, chương trình văng lỗi Segmentation fault (core dumped) và sinh ra file core. Tiếp tục nạp file core này vào GDB:

```
gdb ./crash_app core
```
Phân tích kết quả:

GDB đọc file core và thông báo nguyên nhân kết thúc chương trình là do nhận được tín hiệu SIGSEGV (Lỗi truy cập vùng nhớ cấm).

Truy vết tự động: GDB tự động trỏ ngay đến hàm cause_crash () tại dòng số 5 của file crash_app.c. Đây chính là nơi con trỏ bị thao tác sai (dereference con trỏ NULL), giúp khoanh vùng lỗi lập tức mà không cần đặt breakpoint chạy lại từ đầu.

<img width="746" height="525" alt="image" src="https://github.com/user-attachments/assets/d65b7028-61da-4a64-b89f-cece448867c9" />

<img width="879" height="278" alt="image" src="https://github.com/user-attachments/assets/585559cc-f25c-4b5c-96c5-8fa425b53283" />

5. Bài tập 2.5: Phân tích hiệu năng với Perf
```
perf record ./main_app
```

Phân tích kết quả:

Công cụ đã ghi lại các sự kiện hiệu năng trong suốt vòng đời của chương trình (chạy 5 vòng lặp).

Output [ perf record: Captured and wrote 0.014 MB perf.data (328 samples) ] cho thấy perf đã lấy được 328 mẫu trạng thái CPU và lưu vào file perf.data.

(Lưu ý: Kernel có cảnh báo tự động giảm tỷ lệ lấy mẫu perf_event_max_sample_rate do ngắt mất quá nhiều thời gian - một hiện tượng khá bình thường trên các hệ thống nhúng tài nguyên hạn chế).

Dữ liệu trong perf.data lúc này có thể được đọc bằng lệnh perf report để phân tích hàm nào ngốn nhiều phần trăm CPU nhất.

<img width="879" height="278" alt="image" src="https://github.com/user-attachments/assets/585559cc-f25c-4b5c-96c5-8fa425b53283" />

6. Bài tập 2.6: Phân tích lời gọi hệ thống Strace
```
strace ./main_app
```
<img width="883" height="705" alt="image" src="https://github.com/user-attachments/assets/ac4e0868-d1fd-4dac-a4d9-edec949fa426" />


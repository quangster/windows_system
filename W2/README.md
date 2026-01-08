Bài tập 2.1 – Process Explorer mini
Dùng CreateToolhelp32Snapshot + Process32First/Next để liệt kê toàn bộ process đang chạy.
In ra:PID, tên tiến trình, đường dẫn, dung lượng RAM đang chiếm.
Mở rộng: Cho phép kill process (OpenProcess + TerminateProcess). Cho phép lọc process theo tên.

Bài tập 2.2 – Thread Monitor
Liệt kê các thread của 1 process nhất định (Thread32First, Thread32Next).
In ra TID, trạng thái, CPU usage.

Bài tập 2.3 – Create Process & Inject Parameter
Viết chương trình tự tạo một process con (CreateProcessW) và truyền tham số từ cha → con.
Mở rộng:Log lại thời gian chạy, exit code. Học cách redirect output (STDOUT redirection).
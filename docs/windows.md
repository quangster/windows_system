- Sự khác biệt giữa Kernel Mode và User Mode
- MiniFilter là gì, khác gì file system driver / legacy filter
- Hiểu cách hệ điều hành xử lý các yêu cầu Input/Output (Đọc/Ghi), vai trò của IRP (I/O Request Packet) trong Windows ( User API → I/O Manager → FS → MiniFilter → Disk)
- Các callback trọng tâm (IRP_MJ_CREATE, READ, WRITE, SET_INFORMATION,AcquireForSectionSynchronization), Khi nào dùng Pre vs Post
- IRQL & Pageable rules (2–3 slide) – PASSIVE_LEVEL vs APC/DISPATCH_LEVEL
- Giao tiếp usermode : DeviceIoControl, Communication port: connect / message / disconnect
- Hiệu năng & chống deadlock


1. Phân biệt Kernel Mode và User Mode
Trong Windows, CPU sử dụng các mức đặc quyền (Privilege Levels) được gọi là Rings.
- User Mode (Ring 3): Là nơi các ứng dụng thông thường (Notepad, Chrome) chạy. Các ứng dụng này bị giới hạn quyền truy cập trực tiếp vào phần cứng hoặc bộ nhớ của hệ thống để đảm bảo tính ổn định. Nếu một ứng dụng bị lỗi, chỉ ứng dụng đó "chết".
- Kernel Mode (Ring 0): Là nơi hệ điều hành và các driver hoạt động. Tại đây, code có toàn quyền truy cập vào mọi tài nguyên hệ thống. Một lỗi nhỏ ở chế độ này (như truy cập sai địa chỉ bộ nhớ) sẽ dẫn đến lỗi màn hình xanh (BSOD)
- Mode ở đây được hiểu là quyền hạn của CPU tại một thời điểm

2. Sự chuyển giao quyền lực: I/O Request Packet (IRP)
Khi bạn mở một file trong Notepad, một chuỗi các sự kiện sẽ xảy ra. Để các tầng khác nhau (từ ứng dụng đến đĩa cứng) có thể "nói chuyện" được với nhau, Windows sử dụng một cấu trúc dữ liệu chung gọi là IRP.

Hãy tưởng tượng IRP như một "đơn đặt hàng". Nó chứa thông tin: Ai yêu cầu? Làm gì (Đọc hay Ghi)? Dữ liệu nằm ở đâu? Đơn hàng này sẽ được truyền qua các tầng:

- User API: ReadFile() được gọi.
- I/O Manager: Tạo ra IRP và gửi nó đi. (Chuyển yêu cầu từ User Mode sang Kernel Mode). Tạo ra IRP và đóng gói các thông số (file nào, dữ liệu gì).
- File System (FS): Hiểu cấu trúc file (NTFS, FAT32). Chịu trách nhiệm hiểu cấu trúc file nằm ở những địa chỉ vật lý nào trên ổ đĩa.
- MiniFilter: Driver của bạn sẽ "soi" hoặc chỉnh sửa IRP này. Driver có thể soi gói hàng, thay đổi nó hoặc chặn không cho đi tiếp.
- Disk Driver: Ghi dữ liệu thực tế (bit01) xuống phần cứng.

IRP là một cấu trúc dữ liệu phức tạp gồm 2 phần chính:
- Header: Chứa thông tin chung (trạng thái hoàn thành, yêu cầu này dành cho file nào)
- I/O Stack Locations: 

3. MiniFilter vs Legacy Filter
Trước đây, các lập trình viên phải viết Legacy Filter Drivers, việc này rất phức tạp vì bạn phải tự quản lý việc chèn mình vào ngăn xếp (stack) thiết bị.
MiniFilter ra đời cùng với Filter Manager (FltMgr.sys) để đơn giản hóa việc này.
- Legacy: Giống như việc bạn phải tự xây dựng một cái thang để trèo vào giữa một dây chuyền sản xuất.
- MiniFilter: Giống như việc bạn chỉ cần đăng ký một "trạm kiểm soát" với người quản lý dây chuyền (Filter Manager). Bạn chỉ tập trung vào việc xử lý dữ liệu.


4. Các Callback trọng tâm
Trong MiniFilter, code sẽ đăng ký các hàm callback để xử lý các loại hành động khác nhau:
- IRP_MJ_CREATE: xảy ra khi có ai đó mở hoặc tạo mới một file
- IRP_MJ_READ / IRP_MJ_WRITE: Khi dữ liệu được đọc ra hoặc ghi vào
- IRP_MJ_SET_INFORMATION: Khi muốn đổi tên file, xóa file hoặc thay đổi thuộc tính
- AcquireForSectionSynchronization: Một callback đặc biệt liên quan đến việc ánh xạ file vào bộ nhớ (Memory Mapping)

5. Pre vs Post
Trong lập trình MiniFilter, chúng ta sử dụng 2 thời điểm quan trọng để xử lý IRP
- Pre-operation Callback: mục đích: kiểm tra, thay đổi dữ liệu hoặc ngăn chặn yêu cầu nếu thấy nguy hiểm
- Post-operation Callback: xảy ra khi yêu cầu I/O đã hoàn tất và đang đi ngược lên từ ổ đĩa về phía ứng dụng. mục đích là xem thao tác đó có thành công hay không, sửa đổi dữ liệu mà ổ đĩa vừa đọc lên trước khi đưa cho ứng dụng

Mỗi MiniFilter sẽ được gán một chỉ số gọi là Altitude (độ cao):
- Driver có Altitude cao hơn sẽ nhận được yêu cầu Pre-operation trước
- Nhưng khi kết quả trả về (Post-Operation), driver có Altitude cao hơn lại nhận được sau cùng

6. IRQL
- IRQL (Interrupt Request Level): mức yêu cầu ngắt của CPU. Ví dụ khi một thread yêu cầu đọc file, CPU không chờ mà chuyển sang làm việc khác. Khi ổ cứng tìm xong dữ liệu, nó cần báo cho CPU biết đã làm xong bằng cách gửi tín hiệu ngắt (Interrupt)

- IRQL càng cao thì công việc càng khẩn cấp: ví dụ các tín hiệu từ chuột, bàn phím được coi là quan trọng hơn so với một số tác vụ như render hình ảnh trên màn hình
nên thường có IRQL cao hơn

IRQL là một thuộc tính của CPU. Priotiry là một thuộc tính của thread
Priority của Thread chỉ có ý nghĩa khi IRQL < 2.


Nguyên tắc cơ bản lúc này là bộ xử lý thực thi mã có IRQL cao nhất. Ví dụ nếu IRQL của CPU = 0 tại một thời điểm và một ngắt với IRQL là 5 xuất hiện, nó sẽ lưu context vào ngăn xếp kernel của thread hiện tại và sau đó thực thi ISR liên quan đến ngắt đó. Khi IRL hoàn tất, IRQL sẽ trở lại mức trước đó, tiếp tục thực thi mã trước đó. Trong khi ISQ đang chạy, các ngắt khác <= 5 sẽ không ngắt được CPU lúc này. Ngược lại nếu IRQL của ngắt mới cao hơn 5, CPU lại lưu trạng thái của nó một lần nữa, thực hiện và lại trở về mức IRQL cũ là 5.


Thông thường IRQL thường bằng 0 và đặc biệt là bằng 0 khi code ở User Mode. Ở Kernel Mode, IRQL thường bằng 0 nhưng không phải lúc nào cũng vậy. 

Nếu IRP là gói hàng thì IRQL chính là luật giao thông quyết định mức độ ưu tiên của các công việc đang diễn ra trên CPU

- Mức 0: PASSIVE_LEVEL: mức thấp nhất. Đây là nơi các ứng dụng User Mode và hầu hết các hàm driver chạy. 
- Mức 1: APC_LEVEL: dùng cho các lời gọi thủ tục không đồng bộ
- Mức 2: DISPATCH_LEVEL: Mức cực kỳ quan trọng. Tại đây, bộ lập lịch của windows ngừng hoạt động trên CPU đó. Bạn không được phép ngủ hay gây ra lỗi page fault
- Mức >2: DIRQL: Dành cho phần cứng (bàn phím, chuột, ổ đĩa) ngắt lời CPU để báo có dữ liệu mới

Quy tắc
- Quy tắc ưu tiên: Một luồng đang chạy ở IRQL cao chỉ có thể ngắt bởi một yêu cầu có IRAL cao hơn nó
- Quy tắc Pageable: ở múc DISPATCH_LEVEL hoặc cao hơn, không bao giờ được truy cập vào vùng nhớ Pageable (vùng nhớ có thể bị đẩy ra ổ cứng). Nếu dữ liệu bạn cần không nằm sẵn trong RAM, hệ thống sẽ cố gằng gặp nạp từ đĩa - hành động này đòi hỏi IRQL thấp hơn để chờ đợi, dẫn đền xung đột và làm hệ thống treo cứng ngay lập tức

Khi bạn viết các hàm callback trong MiniFilter:
- Hầu hết các yêu cầu (như IRP_MJ_CREATE) chạy ở PASSIVE_LEVEL.
- Tuy nhiên, một số thao tác hoàn tất (Post-operation) có thể chạy ở DISPATCH_LEVEL.

Vùng bộ nhớ:
- Non-Paged Pool: Vùng nhớ luôn nằm trong RAM, an toàn để truy cập ở mọi mức IRQL.
- Paged Pool: Vùng nhớ có thể bị hệ điều hành đẩy ra ổ cứng (Page file) để tiết kiệm RAM. Nếu bạn truy cập vùng này ở DISPATCH_LEVEL mà dữ liệu không có sẵn trong RAM, hệ thống sẽ cố nạp nó từ đĩa (gây ra Page Fault) — và "Bùm", BSOD xuất hiện.

7. Cách User Mode giao tiếp với Kernel Mode
- DeviceIoControl: cách truyền thống. Ứng dụng gửi một mã điều khiển (IOCTL) và một buffer dữ liệu xuống Driver.
- Filter Communication Ports: Cách chuyên dụng cho MiniFilter. Nó thiết lập một "kênh chat" riêng giữa User và Kernel cho phép gửi tin nhắn hai chiều rất linh hoạt. Driver tạo một Port (cổng). Ứng dụng User Mode kết nối vào cổng đó. Hai bên có thể gửi tin nhắn qua lại bằng các hàm như FilterSendMessage (User) và FltSendMessage (Kernel)

8. DeadLock
Thread A giữ Khóa 1 và đợi Khóa 2
Thread B giữ Khóa 2 và đợi Khóa 1 => Cả 2 dừng lại mãi mãi, và máy tính treo cứng. Trong Kernel ko có ai nhấn End Task được cả

Lock: trong kernel, nhiều CPU có thể cùng chạy các đoạn code driver cùng một lúc. Nếu hai CPU cùng sửa đổi một danh sách các file đang bị chặn, dữ liệu sẽ bị hỏng. Lock giúp đảm bảo tại một thời điểm chỉ có một luồng duy nhất được quyền thay đổi dữ liệu quan trọng

- Spin Lock: Nếu khóa đang bị giữ, luồng đang đợi sẽ "chạy lòng vòng" tại chỗ để chwof đến khi khóa trống
- ERESOURCE: cách hoạt động: cho phép nhiều luồng cùng Đọc dữ liệu nhưng chỉ một luồng được ghi



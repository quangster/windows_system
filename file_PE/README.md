PE File
- MS-DOS Header & Stub
- Nt Headers
    - File Header
    - Optional Header
        - Data Directories
- Section Headers
- Import Directory
- Resource Diretory
- Exception Directory
- Relocation Directory
- Debug Directory


ImageBase là một thuộc tính của Optional Header, là địa chỉ lý tưởng mà một chương trình muốn được đặt vào khi nạp vào bộ nhớ RAM. Nó là một con số (địa chỉ bộ nhớ) xác định điểm bắt đầu của tệp thực thi trong không gian địa chỉ ảo của một process. Đối với tệp .exe giá trị mặc định thường là 0x00400000 (windows 32bit) hoặc 0x000140000000 (windows 64bit). Đối với tệp .dll thì thường là 0x10000000. 

RVA (Relative Virtual Address) là địa chỉ ảo tương đối, là khoảng cách từ địa chỉ gốc nơi file được nạp vào bộ nhớ (ImageBase). Để CPU có thể thực thi mã, nó cần một địa chỉ tuyệt đối (VA - Virtual Address)
VA = ImageBase + RVA

Ví dụ: Nếu Image Base là 0x400000 và địa chỉ hàm main có RVA là 0x1000, thì khi nạp vào RAM, hàm main sẽ nằm tại địa chỉ 0x401000

Trong thực tế, không phải lúc nào hệ điều hành cũng có thể nạp tệp vào đúng địa chỉ ImageBase mong muốn:
- Relocation: nếu địa chỉ ImageBase không khả dụng, Windows Loader sẽ tìm một vùng trống khác trong RAM để đặt chương trình vào. Lúc này cần một việc là Rebase để tính toán lại toàn bộ địa chỉ tuyệt đối dựa trên địa chỉ nạp mới bằng cách sử dụng bảng Base Relocation Table trong Data Directory
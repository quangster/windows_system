Xây dựng một hệ thống gồm Service và Client giao tiếp với nhau bằng Windows Named Pipe .

Client gửi yêu cầu “scan file”, Service xử lý và trả về kết quả.

Trong đó Service sẽ phải làm các việc sau:
- Service load dll động (.dll thường gọi là engine)
- Service có chức năng monitor hiệu năng 
- Service có chức năng giao tiếp với client để lấy thông tin của client gửi lên ( Đường dẫn file) sau đó check xem máy có rảnh không thì scanfile đó bằng engine dll 

Yêu cầu:

- Tạo demo 1 file dll có chức năng nếu file nằm ngoài ổ C thì cảnh báo nguy hiểm
- Tạo 1 service có chức năng như trên
- Tạo 1 client exe gửi đường dẫn lên cho service
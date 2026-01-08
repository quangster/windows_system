Bài tập 3.1 (RegistryEditor)
Tạo tool thêm, sửa, xóa key trong Registry.
Dùng API: RegOpenKeyEx, RegSetValueEx, RegDeleteValue.

Bài tập 3.2 (ServiceController)
Viết tool liệt kê tất cả Windows Services (OpenSCManager, EnumServicesStatusEx).
Cho phép start/stop service cụ thể.

Mở rộng (LogService): Tự tạo 1 service nhỏ của bạn (sử dụng CreateService), chạy nền ghi log hiệu tổng hiệu năng sử dụng của hệ điều hành ( RAM, CPU, DISK) 1 phút 1 lần. Cho phép auto-restart nếu bị kill. Thêm chế độ xoay log (xóa log cũ khi vượt 1MB hoặc quá 5 ngày).
# Kernel Prog Basic

## General

| | User Mode | Kernel Mode |
| :--- | :--- | :--- |
| **Unhandled Exceptions** | Unhandled exceptions crash the process | Unhandled exceptions crash the system |
| **Termination** | Khi process kết thúc, toàn bộ memory và resources của process đó được giải phóng tự động | Khi driver unload mà không giải phóng bộ nhớ thì sẽ gây ra rò rỉ, chỉ hết khi boot lại |
| **Return values** | API errors are sometimes ignored | Should (almost) never ignore errors |
| **IRQL** | Luôn là PASSIVE_LEVEL (0) | DISPATCH_LEVEL (2) hoặc cao hơn |
| **Bad Coding** | Thường chỉ ảnh hưởng đến cục bộ trong process đó | Có thể gây ảnh hưởng toàn hệ thống |
| **Testing and Debugging** | Thường làm ngay trên máy của LTV | Debugging phải thực hiện trên máy khác |
| **Libraries** | Có thể sử dụng hầu hết thư viện C/C++ | Hầu hết các thư viện chuẩn không thể sử dụng |
| **Exception Handling** | Có thể dùng C++ exceptions (try-catch) hoặc SEH | Chỉ có thể sử dụng SEH (Structured Exception Handling) |
| **C++ Usage** | Có đầy đủ bộ C++ Runtime | Không có C++ Runtime |

## The Kernel API

Hầu hết các hàm này được tổ chức trong chính kernel module (NtOskrnl.exe), tuy nhiên một số thì được cung cấp bởi các kernel module khác như hal.dll

| Prefix | Meaning | Example |
| :--- | :--- | :--- |
| **Ex** | General executive functions | `ExAllocatePoolWithTag` |
| **Ke** | General kernel functions | `KeAcquireSpinLock` |
| **Mm** | Memory manager | `MmProbeAndLockPages` |
| **Rtl** | General runtime library | `RtlInitUnicodeString` |
| **FsRtl** | File system runtime library | `FsRtlGetFileSize` |
| **Flt** | File system mini-filter library | `FltCreateFile` |
| **Ob** | Object manager | `ObReferenceObject` |
| **Io** | I/O manager | `IoCompleteRequest` |
| **Se** | Security | `SeAccessCheck` |
| **Ps** | Process manager | `PsLookupProcessByProcessId` |
| **Po** | Power manager | `PoSetSystemState` |
| **Wmi** | Windows management instrumentation | `WmiTraceMessage` |
| **Zw** | Native API wrappers | `ZwCreateFile` |
| **Hal** | Hardware abstraction layer | `HalExamineMBR` |
| **Cm** | Configuration manager (registry) | `CmRegisterCallbackEx` |

Trong 1 số TH, các giá trị NTSTATUS được trả về từ các hàm sẽ lên tới user mode. Trong những trường hợp này, giá trị STATUS_XXX sẽ được dịch sang một giá trị ERROR_yyy nào đó mà user-mode có thể truy cập được thông qua hàm GetLastError()


## Strings
## Linked List
## The Driver Object
## Object Attributes
## Device Objects
## 


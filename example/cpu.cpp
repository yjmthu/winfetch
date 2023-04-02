#include <stdio.h>
// 引入 Windows API 的头文件
#include <windows.h>
#include <intrin.h>

// 定义一个函数，获取 CPU 的名称和信息
void GetCPUInfo() {
    // 创建一个 SYSTEM_INFO 结构体，用于存储系统信息
    SYSTEM_INFO sysInfo;
    // 调用 GetSystemInfo 函数，获取系统信息
    GetSystemInfo(&sysInfo);

    // 打印逻辑 CPU 的数量和物理核的数量
    printf("Logical CPU Count: %lu\n", sysInfo.dwNumberOfProcessors);

    // printf("Physical Core Count: %d\n", sysInfo.dwNumberOfCores);

    // 创建一个 char 数组，用于存储 CPU 的名称
    char CPUName[0x40];
    // 创建一个 int 数组，用于存储 CPUID 指令的结果
    int CPUInfo[4] = {-1};
    // 调用 __cpuid 函数，执行 CPUID 指令，获取 CPU 的名称
    __cpuid(CPUInfo, 0x80000000);
    unsigned int nExIds = CPUInfo[0];
    for (unsigned int i = 0x80000000; i <= nExIds; ++i) {
        __cpuid(CPUInfo, i);
        if (i == 0x80000002) {
            memcpy(CPUName, CPUInfo, sizeof(CPUInfo));
        } else if (i == 0x80000003) {
            memcpy(CPUName + 16, CPUInfo, sizeof(CPUInfo));
        } else if (i == 0x80000004) {
            memcpy(CPUName + 32, CPUInfo, sizeof(CPUInfo));
        }
    }

    // 打印 CPU 的名称
    printf("CPU Name: %s\n", CPUName);
}

// 定义一个主函数
int main() {
    // 调用获取 CPU 信息的函数
    GetCPUInfo();
    return 0;
}

#include <Windows.h>
#include <Uxtheme.h>
#include <DXGI.h>
#include <iterator>
#include <lmcons.h>
#include <string_view>
#include <tlhelp32.h>
#include <cstdio>

#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <utility>
#include <vcruntime.h>
#include <vector>
#include <sstream>
#include <thread>
#include <format>
#include <filesystem>
#include <regex>
#include <chrono>

typedef std::basic_string<TCHAR> String;
typedef std::basic_string_view<TCHAR> StringView;
typedef std::basic_ostringstream<TCHAR> Ostringstream;
typedef std::basic_istringstream<TCHAR> Istringstream;
typedef std::basic_regex<TCHAR> Regex;
typedef std::match_results<String::const_iterator> Match;

using namespace std::literals;

#ifdef UNICODE
#define tcout wcout
#else
#define tcout cout
#endif

// 0     1   2     3      4    5      6    7
// Black Red Green Yellow Blue Purple Cyan White

#define CtlCmd(x, c) std::format(TEXT("\033[{}{}"), x, TCHAR(c))

#define NC(x) CtlCmd(x, 'm') 
#define TC(x) CtlCmd(x+30, 'm')
#define TLC(x) CtlCmd(x+90, 'm')
#define BC(x) CtlCmd(x+40, 'm')
#define BLC(x) CtlCmd(x+100, 'm')

#define UPWARD(x) CtlCmd(x, 'A')
#define DOWNWARD(x) CtlCmd(x, 'B')
#define RIGHTWARD(x) CtlCmd(x, 'C')
#define LEFTWARD(x) CtlCmd(x, 'D')

std::multimap<String, String> gNameMap;

void PrintLogo() {
    std::tcout << std::format(TEXT("\n"
        "{1}        ,.=:!!t3Z3z.,\n"
        "{1}       :tt:::tt333EE3\n"
        "{1}       Et:::ztt33EEEL {3}@Ee.,      ..,\n"
        "{1}      ;tt:::tt333EE7 {3};EEEEEEttttt33#\n"
        "{1}     :Et:::zt333EEQ. {3}$EEEEEttttt33QL\n"
        "{1}     it::::tt333EEF {3}@EEEEEEttttt33F\n"
        "{1}    ;3=*^```\"*4EEV {3}:EEEEEEttttt33@.\n"
        "{2}    ,.=::::!t=., {1}` {3}@EEEEEEtttz33QF\n"
        "{2}   ;::::::::zt33)   {3}\"4EEEtttji3P*\n"
        "{2}  :t::::::::tt33.{4}:Z3z..  {3}`` {4},..g.\n"
        "{2}  i::::::::zt33F {4}AEEEtttt::::ztF\n"
        "{2} ;:::::::::t33V {4};EEEttttt::::t3\n"
        "{2} E::::::::zt33L {4}@EEEtttt::::z3F\n"
        "{2}{{3=*^```\"*4E3) {4};EEEtttt:::::tZ`\n"
        "{2}             ` {4}:EEEEtttt::::z7\n"
        "{4}                 \"VEzjt:;;z>*`{0}{5}")
    , NC(0), TC(1), TC(4), TC(2), TC(3), UPWARD(17)) << std::endl;
}

String GetArchitecture() {
    SYSTEM_INFO info;
    GetNativeSystemInfo(&info);
    switch (info.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64:
            return TEXT("x86_64");
        case PROCESSOR_ARCHITECTURE_ARM:
            return TEXT("ARM");
        case PROCESSOR_ARCHITECTURE_ARM64:
            return TEXT("ARM64");
        case PROCESSOR_ARCHITECTURE_IA64:
            return TEXT("Intel Itanium-based");
        case PROCESSOR_ARCHITECTURE_INTEL:
            return TEXT("x86");
        case PROCESSOR_ARCHITECTURE_UNKNOWN:
        default:
            return TEXT("Unknown architecture");
    }
}

bool GetComputerName() {
    DWORD nSize = MAX_COMPUTERNAME_LENGTH + 1;
    String buffer(nSize, 0);
    if (!GetComputerName(buffer.data(), &nSize)) {
        return false;
    }
    buffer.resize(nSize);
    gNameMap.insert(std::make_pair(TEXT("ComputerName"), buffer));
    return true;
}

bool GetUserName() {
    DWORD cbBuffer = UNLEN + 1;
    String buffer(cbBuffer, 0);
    if (!GetUserName(buffer.data(), &cbBuffer)) {
        return false;
    }
    buffer.resize(cbBuffer - 1);
    gNameMap.insert(std::make_pair(TEXT("UserName"), buffer));
    return true;
}

String GetSoftVersion(LPCTSTR szExePath)
{
    String result;
	UINT sz = GetFileVersionInfoSize(szExePath, 0);
	if (sz != 0) {
		BYTE* pBuf = new BYTE[sz];
		VS_FIXEDFILEINFO *pVsInfo;
		if (GetFileVersionInfo(szExePath, 0, sz, pBuf)) {
			if (VerQueryValue((LPCVOID)pBuf, TEXT("\\"), (LPVOID*)&pVsInfo, &sz)) {
                result = std::format(TEXT("{}.{}.{}.{}"), HIWORD(pVsInfo->dwFileVersionMS), LOWORD(pVsInfo->dwFileVersionMS), HIWORD(pVsInfo->dwFileVersionLS), LOWORD(pVsInfo->dwFileVersionLS));
			}
		}
		delete[] pBuf;
	}
	return result;
}

bool GetRegString(HKEY dwSubKey, LPCTSTR pPath, LPCTSTR pKeyName, LPCTSTR pMapKey) {
    HKEY hKey = NULL;
    if (ERROR_SUCCESS != RegOpenKeyEx(dwSubKey, pPath, 0, KEY_READ, &hKey)) {
        return false;
    }
    DWORD dwType = REG_SZ;
    DWORD dwDataSize = 0;
    PBYTE pData = NULL;
    if (ERROR_SUCCESS != RegQueryValueEx(hKey, pKeyName, 0, &dwType, pData, &dwDataSize)) {
        return false;
    }
    pData = new BYTE[dwDataSize+sizeof(TCHAR)] { 0 };
    if (ERROR_SUCCESS != RegQueryValueEx(hKey, pKeyName, 0, &dwType, pData, &dwDataSize)) {
        return false;
    }
    gNameMap.insert(std::make_pair(pMapKey, reinterpret_cast<PTCHAR>(pData)));
    delete [] pData;
    RegCloseKey(hKey);
    return true;
}

inline bool GetOSName() {
    bool ret = GetRegString(
            HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), TEXT("ProductName"), TEXT("OS"));
    if (!ret) return false;
    gNameMap.find(TEXT("OS"))->second.append(TEXT(" ") + GetArchitecture());
    return true;
}

bool GetProcessorInfo() {
    HKEY hKey = NULL;
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), 0, KEY_READ, &hKey)) {
        return false;
    }
    DWORD dwType = REG_SZ;
    DWORD dwDataSize = 0;
    PBYTE pData = NULL;
    if (ERROR_SUCCESS != RegQueryValueEx(hKey, TEXT("ProcessorNameString"), 0, &dwType, pData, &dwDataSize)) {
        return false;
    }
    pData = new BYTE[dwDataSize+sizeof(TCHAR)] { 0 };
    if (ERROR_SUCCESS != RegQueryValueEx(hKey, TEXT("ProcessorNameString"), 0, &dwType, pData, &dwDataSize)) {
        return false;
    }

    String sCpuName(reinterpret_cast<PTCHAR>(pData));
    delete [] pData;

    Regex pattern(TEXT("^.+ @ [0-9\\.]+ GHz$"));
    Match result;
    if (!std::regex_match(sCpuName, result, pattern)) {
        dwType = REG_DWORD;
        dwDataSize = sizeof(DWORD);
        DWORD dwData;
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, TEXT("~MHz"), 0, &dwType, (PBYTE)&dwData, &dwDataSize)) {
            sCpuName += std::format(TEXT(" @ {:.2f} GHz"), dwData / 1000.0);
        }
    }

    gNameMap.insert(std::make_pair(TEXT("CPU"), sCpuName));
    RegCloseKey(hKey);
    
    gNameMap.insert(std::make_pair(TEXT("CPU Core"), std::format(TEXT("{}"), std::thread::hardware_concurrency())));

    return true;
}

bool GetBiosInfo() {
    // HARDWARE\DESCRIPTION\System\BIOS
    HKEY hKey = NULL;
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\BIOS"), 0, KEY_READ, &hKey)) {
        return false;
    }
    DWORD dwType = REG_SZ;
    DWORD dwDataSize = 0;
    PBYTE pData = NULL;
    if (ERROR_SUCCESS != RegQueryValueEx(hKey, TEXT("BaseBoardManufacturer"), 0, &dwType, pData, &dwDataSize)) {
        return false;
    }
    pData = new BYTE[dwDataSize+sizeof(TCHAR)] { 0 };
    if (ERROR_SUCCESS != RegQueryValueEx(hKey, TEXT("BaseBoardManufacturer"), 0, &dwType, pData, &dwDataSize)) {
        delete [] pData;
        return false;
    }

    Ostringstream stream;
    stream << reinterpret_cast<PTCHAR>(pData) << TEXT(' ');
    delete [] pData;

    pData = NULL;
    if (ERROR_SUCCESS != RegQueryValueEx(hKey, TEXT("SystemProductName"), 0, &dwType, pData, &dwDataSize)) {
        return false;
    }
    pData = new BYTE[dwDataSize + sizeof(DWORD)] { 0 };
    if (ERROR_SUCCESS != RegQueryValueEx(hKey, TEXT("SystemProductName"), 0, &dwType, pData, &dwDataSize)) {
        delete [] pData;
        return false;
    }
    stream << reinterpret_cast<PTCHAR>(pData);
    delete [] pData;

    gNameMap.insert(std::make_pair(TEXT("Host"), stream.str()));
    RegCloseKey(hKey);
    return true;
}

bool GetMemoryInfo() {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx (&statex);
    Ostringstream stream;
    stream << (statex.ullAvailPhys >> 20) << TEXT("MiB / ")
        << (statex.ullTotalPhys >> 20) << TEXT("MiB");
    gNameMap.insert(std::make_pair(TEXT("Memory"), stream.str()));
    return true;
}

bool GetUptime() {
  using namespace std::chrono;
  auto time = duration_cast<seconds>(milliseconds(GetTickCount()));
  gNameMap.insert(std::make_pair(L"Uptime", std::format(L"{:%H hours, %M minutes, %S seconds}", time)));
  return true;
}

bool GetScreenInfo() {
    Ostringstream stream;
    DEVMODE dm;
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
    HWND hd = GetDesktopWindow();
    stream << dm.dmPelsWidth << TEXT('x') << dm.dmPelsHeight << TEXT(" @ ") << GetDpiForWindow(hd) << TEXT("dpi");
    gNameMap.insert(std::make_pair(TEXT("Resolution"), stream.str()));
    return true;
}

bool GetStorageInfo() {
    Ostringstream stream;

    DWORD dwDsLength = GetLogicalDriveStrings(0, NULL);
    PTCHAR szDrivesName = new TCHAR[dwDsLength];
    GetLogicalDriveStrings(dwDsLength, szDrivesName);

    PTCHAR ptr = szDrivesName;
    ULARGE_INTEGER allSpace { 0 }, freeSpace { 0 };
    while (*ptr)
    {
        if (GetDriveType(ptr) == DRIVE_FIXED) {
            ULARGE_INTEGER dwFreeBytesToCaller;
            ULARGE_INTEGER dwTotalNumberOfBytes;
            ULARGE_INTEGER dwTotalNumberOfFreeBytes;
            GetDiskFreeSpaceEx(ptr, &dwFreeBytesToCaller, &dwTotalNumberOfBytes, &dwTotalNumberOfFreeBytes);
            allSpace.QuadPart += dwTotalNumberOfBytes.QuadPart;
            freeSpace.QuadPart += dwTotalNumberOfFreeBytes.QuadPart;
        }
        ptr += 4;
    }

    stream << (freeSpace.QuadPart >> 30) << TEXT("GiB / ") << (allSpace.QuadPart >> 30) << TEXT("GiB");
    gNameMap.insert(std::make_pair(TEXT("Storage"), stream.str()));
    return true;
}

bool GetGPUInfo() {
        // 参数定义  
    IDXGIFactory * pFactory;
    IDXGIAdapter * pAdapter;
    std::vector <IDXGIAdapter*> vAdapters;            // 显卡           
    int iAdapterNum = 0; // 显卡的数量  

    // 创建一个DXGI工厂  
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory));

    if (FAILED(hr))
        return false;

    // 枚举适配器  
    while (pFactory->EnumAdapters(iAdapterNum, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        vAdapters.push_back(pAdapter);
        ++iAdapterNum;
    }

    // 信息输出   
    for (size_t i = 0; i < vAdapters.size(); i++)
    {
        DXGI_ADAPTER_DESC adapterDesc;
        vAdapters[i]->GetDesc(&adapterDesc);
        gNameMap.insert(std::make_pair(TEXT("GPU"), adapterDesc.Description));
    }
    vAdapters.clear();
    return true;
}

String GetProcessOutPut(LPCTSTR cmd)
{
    String result;
	TCHAR szBuffer[128] {0};
	FILE * fp;

#ifdef UNICODE
	if ((fp = _wpopen(cmd, L"r")) == NULL)
#else
	if ((fp = _popen(cmd, "r")) == NULL)
#endif
	{
		return result;
	}
	else
	{
#ifdef UNICODE
		while (std::fgetws(szBuffer, 128, fp) != NULL)
#else
		while (fgets(szBuffer, 128, fp) != NULL)
#endif
		{
            result.append(szBuffer);
		}
        _pclose(fp);
	}
	return result;
}

DWORD GetParentPID(DWORD pid)
{
	DWORD ppid = 0;
	PROCESSENTRY32W processEntry = { 0 };
	processEntry.dwSize = sizeof(PROCESSENTRY32W);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (Process32FirstW(hSnapshot, &processEntry))
	{
		do
		{
			if (processEntry.th32ProcessID == pid)
			{
				ppid = processEntry.th32ParentProcessID;
				break;
			}
		} while (Process32NextW(hSnapshot, &processEntry));
	}
	CloseHandle(hSnapshot);
	return ppid;
}

bool GetShellName() {
    using namespace std::literals;
    DWORD parentPid = GetParentPID(GetCurrentProcessId());
	TCHAR parentName[MAX_PATH + 1];
	DWORD dwParentName = MAX_PATH;
	HANDLE hParent = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, parentPid);
	QueryFullProcessImageName(hParent, 0, parentName, &dwParentName); // another way to get process name is to use 'Toolhelp32Snapshot'
    std::filesystem::path parentPath(parentName);
#ifdef UNICODE
    String filename = parentPath.filename().wstring();
#else
    String filename = parentPath.filename().string();
#endif
    if (filename == TEXT("explorer.exe"))
        return false;
    if (filename != TEXT("pwsh.exe")) {
#ifdef UNICODE
        String filepos = parentPath.wstring();
#else
        String filepos = parentPath.string();
#endif
        gNameMap.insert(std::make_pair(TEXT("Shell"), filename + TEXT(" ") + GetSoftVersion(filepos.c_str())));
        return true;
    }
#ifdef UNICODE
    String cmd = L"\"" + parentPath.wstring() + L"\" --version";
#else
    String cmd = "\"" + parentPath.string() + "\" --version";
#endif
    String shellInfo = GetProcessOutPut(cmd.c_str());
    while (!shellInfo.empty() && TEXT("\r\n"sv).find(shellInfo.back()) != StringView::npos) {
        shellInfo.pop_back();
    }
    gNameMap.insert(std::make_pair(TEXT("Shell"), shellInfo));
    return true;
}

bool GetNtVersionNumbers()
{
    bool bRet = false;
    if (HMODULE hModNtdll = LoadLibrary(TEXT("ntdll.dll")); hModNtdll)
    {
        typedef void (WINAPI *pfRTLGETNTVERSIONNUMBERS)(DWORD*,DWORD*, DWORD*);
        pfRTLGETNTVERSIONNUMBERS pfRtlGetNtVersionNumbers;
        pfRtlGetNtVersionNumbers = (pfRTLGETNTVERSIONNUMBERS)::GetProcAddress(hModNtdll, "RtlGetNtVersionNumbers");
        if (pfRtlGetNtVersionNumbers)
        {
            DWORD dwMajorVer, dwMinorVer, dwBuildNumber;
            pfRtlGetNtVersionNumbers(&dwMajorVer, &dwMinorVer,&dwBuildNumber);
            dwBuildNumber &= 0x0ffff;
            gNameMap.insert(std::make_pair(TEXT("Kernel"), std::format(TEXT("{}.{}.{}"), dwMajorVer, dwMinorVer, dwBuildNumber)));
            bRet = true;
        }
 
        FreeLibrary(hModNtdll);
        hModNtdll = NULL;
    }
 
    return bRet;
}

bool GetWindowsManagerTheme() {
    TCHAR pszThemeFileName[MAX_PATH];
    TCHAR pszColorBuff[MAX_PATH];
    TCHAR pszSizeBuff[MAX_PATH];
    HRESULT hResult = GetCurrentThemeName(pszThemeFileName, MAX_PATH, pszColorBuff, MAX_PATH, pszSizeBuff, MAX_PATH);
    if (hResult != S_OK) return false;
#ifdef UNICODE
    std::wstring sThemeName = std::filesystem::path(pszThemeFileName).stem().wstring();
#else
    std::string sThemeName = std::filesystem::path(pszThemeFileName).stem().string();
#endif
    gNameMap.insert(std::make_pair(TEXT("WM Theme"), sThemeName));
    gNameMap.insert(std::make_pair(TEXT("Color Size"), std::format(TEXT("{} | {}"), pszColorBuff, pszSizeBuff)));
    return true;
}


inline void GetAllInfo() {
    GetUserName();
    GetComputerName();
    GetOSName();
    GetBiosInfo();
    GetNtVersionNumbers();
    GetUptime();
    GetShellName();
    GetScreenInfo();
    GetWindowsManagerTheme();
    GetProcessorInfo();
    GetGPUInfo();
    GetMemoryInfo();
    GetStorageInfo();
}

void PrintInfo() {
    auto lst = { 
        TEXT("OS"),
        TEXT("Host"),
        TEXT("Kernel"),
        TEXT("Uptime"),
        TEXT("Shell"),
        TEXT("Resolution"),
        TEXT("WM Theme"),
        TEXT("Color Size"),
        TEXT("CPU"),
        TEXT("CPU Core"),
        TEXT("GPU"),
        TEXT("Memory"),
        TEXT("Storage")
    };
    std::tcout << RIGHTWARD(39) << TC(1)
        << gNameMap.find(TEXT("UserName"))->second << NC(0) << TEXT('@')
        << TC(1) << gNameMap.find(TEXT("ComputerName"))->second
        << NC(0) << std::endl << RIGHTWARD(39)
        << std::setw(
                gNameMap.find(TEXT("UserName"))->second.size()
                + gNameMap.find(TEXT("ComputerName"))->second.size()
                + 1)
        << std::setfill(TEXT('-')) << TEXT('-');
    for (auto key: lst) {
        for (auto first = gNameMap.lower_bound(key), last = gNameMap.upper_bound(key); first != last; ++first) {
            std::tcout << std::endl << RIGHTWARD(39) << TC(2) << key << NC(0)
                << TEXT(": ") << first->second;
        }
    }
}

void PrintRect() {
    std::tcout << std::endl << std::endl
        <<  std::format(TEXT("{9}{1}   {2}   {3}   {4}   {5}   {6}   {7}   {8}   {0}"),
            NC(0), BC(0), BC(1), BC(2),
            BC(3), BC(4), BC(5),
            BC(6), BC(7), RIGHTWARD(39)) <<
    std::endl << std::format(TEXT("{9}{1}   {2}   {3}   {4}   {5}   {6}   {7}   {8}   {0}\n\n"),
            NC(0), BLC(0), BLC(1), BLC(2),
            BLC(3), BLC(4), BLC(5),
            BLC(6), BLC(7), RIGHTWARD(39));
}

int main() {
    PrintLogo();
    GetAllInfo();
    PrintInfo();
    PrintRect();
    return 0;
}

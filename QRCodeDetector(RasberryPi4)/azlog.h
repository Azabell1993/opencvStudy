#ifndef _AZLOGD_H_
#define _AZLOGD_H_

#include <stdio.h>       // 표준 입출력 헤더 포함
#include <wchar.h>       // 유니코드 관련 함수 헤더 포함
#include <iostream>      // C++ 입출력 스트림 사용
#include <string>        // 문자열 처리 사용
#include <memory>        // 스마트 포인터 사용
#include <mutex>         // 쓰레드 안전성을 위한 뮤텍스 사용
#include <cstdarg>       // 가변 인자 처리 헤더 포함
#include <syslog.h>      // 시스템 로그 사용
#include <ctime>         // 시간 관련 함수 사용
#include <unistd.h>      // 유닉스 표준 함수 사용
#include <cerrno>        // errno 값 처리
#include <fstream>       // 파일 입출력 사용
#include <sstream>       // 문자열 스트림 사용
#include <vector>        // 벡터 사용
#include <unordered_set> // 중복 제거를 위한 집합 사용
#include <sys/stat.h>    // 디렉토리 생성 및 확인용 헤더

#define MAX_ENAME 133                                               // errno 배열의 최대 크기 정의
#define RESOURCE_PATH "/home/azabell/Desktop/QRCodeDetector/output" // 리소스 경로 정의

// errno 이름 배열 정의
static const char *ename[] = {
    /*   0 */ "",
    /*   1 */ "EPERM", "ENOENT", "ESRCH", "EINTR", "EIO", "ENXIO",
    /*   7 */ "E2BIG", "ENOEXEC", "EBADF", "ECHILD",
    /*  11 */ "EAGAIN/EWOULDBLOCK", "ENOMEM", "EACCES", "EFAULT",
    /*  15 */ "ENOTBLK", "EBUSY", "EEXIST", "EXDEV", "ENODEV",
    /*  20 */ "ENOTDIR", "EISDIR", "EINVAL", "ENFILE", "EMFILE",
    /*  25 */ "ENOTTY", "ETXTBSY", "EFBIG", "ENOSPC", "ESPIPE",
    /*  30 */ "EROFS", "EMLINK", "EPIPE", "EDOM", "ERANGE",
    /*  35 */ "EDEADLK/EDEADLOCK", "ENAMETOOLONG", "ENOLCK", "ENOSYS",
    /*  39 */ "ENOTEMPTY", "ELOOP", "", "ENOMSG", "EIDRM", "ECHRNG",
    /*  45 */ "EL2NSYNC", "EL3HLT", "EL3RST", "ELNRNG", "EUNATCH",
    /*  50 */ "ENOCSI", "EL2HLT", "EBADE", "EBADR", "EXFULL", "ENOANO",
    /*  56 */ "EBADRQC", "EBADSLT", "", "EBFONT", "ENOSTR", "ENODATA",
    /*  62 */ "ETIME", "ENOSR", "ENONET", "ENOPKG", "EREMOTE",
    /*  67 */ "ENOLINK", "EADV", "ESRMNT", "ECOMM", "EPROTO",
    /*  72 */ "EMULTIHOP", "EDOTDOT", "EBADMSG", "EOVERFLOW",
    /*  76 */ "ENOTUNIQ", "EBADFD", "EREMCHG", "ELIBACC", "ELIBBAD",
    /*  81 */ "ELIBSCN", "ELIBMAX", "ELIBEXEC", "EILSEQ", "ERESTART",
    /*  86 */ "ESTRPIPE", "EUSERS", "ENOTSOCK", "EDESTADDRREQ",
    /*  90 */ "EMSGSIZE", "EPROTOTYPE", "ENOPROTOOPT",
    /*  93 */ "EPROTONOSUPPORT", "ESOCKTNOSUPPORT",
    /*  95 */ "EOPNOTSUPP/ENOTSUP", "EPFNOSUPPORT", "EAFNOSUPPORT",
    /*  98 */ "EADDRINUSE", "EADDRNOTAVAIL", "ENETDOWN", "ENETUNREACH",
    /* 102 */ "ENETRESET", "ECONNABORTED", "ECONNRESET", "ENOBUFS",
    /* 106 */ "EISCONN", "ENOTCONN", "ESHUTDOWN", "ETOOMANYREFS",
    /* 110 */ "ETIMEDOUT", "ECONNREFUSED", "EHOSTDOWN", "EHOSTUNREACH",
    /* 114 */ "EALREADY", "EINPROGRESS", "ESTALE", "EUCLEAN",
    /* 118 */ "ENOTNAM", "ENAVAIL", "EISNAM", "EREMOTEIO", "EDQUOT",
    /* 123 */ "ENOMEDIUM", "EMEDIUMTYPE", "ECANCELED", "ENOKEY",
    /* 127 */ "EKEYEXPIRED", "EKEYREVOKED", "EKEYREJECTED",
    /* 130 */ "EOWNERDEAD", "ENOTRECOVERABLE", "ERFKILL", "EHWPOISON"};

// 현재 시간 저장 구조체
typedef struct current_t
{
    int year;   // 연도
    int month;  // 월
    int day;    // 일
    int hour;   // 시
    int minute; // 분
    int second; // 초
} CURRENT_T;

// 로그 항목 정의 구조체
typedef struct Entry
{
    std::string serverId;    // 서버 ID
    int uuid;                // UUID
    std::uint64_t timestamp; // 타임스탬프
} Entry;

// 스캔 결과 정의 구조체
typedef struct ScanResult
{
    std::string hubId;          // 허브 ID
    std::vector<Entry> logList; // 로그 목록
} ScanResult;

// 현재 시간을 가져오는 함수
static inline void getCurrentTime(CURRENT_T *current)
{
    time_t current_time = time(nullptr); // 현재 시간 가져오기
    struct tm current_tm;
    localtime_r(&current_time, &current_tm); // 로컬 시간 변환

    current->year = current_tm.tm_year + 1900; // 연도 저장
    current->month = current_tm.tm_mon + 1;    // 월 저장
    current->day = current_tm.tm_mday;         // 일 저장
    current->hour = current_tm.tm_hour;        // 시 저장
    current->minute = current_tm.tm_min;       // 분 저장
    current->second = current_tm.tm_sec;       // 초 저장
}

// 로그 레벨 정의
#define AZLOGD_LEVEL_NONE 0    // 로그 없음
#define AZLOGD_LEVEL_ERROR 1   // 오류 로그
#define AZLOGD_LEVEL_WARNING 2 // 경고 로그
#define AZLOGD_LEVEL_INFO 3    // 정보 로그
#define AZLOGD_LEVEL_DEBUG 4   // 디버그 로그

// 현재 로그 레벨 설정
#ifndef AZLOGD_LEVEL

#define AZLOGD_LEVEL AZLOGD_LEVEL_DEBUG // 기본 디버그 레벨
#endif

#define AZLOGD_BUF_SIZE 4096 // 로그 버퍼 크기

// 플래그 확인 함수
static inline char az_getflag(char flag)
{
    switch (flag)
    {
    case '0':
    case '#':
    case '+':
    case '-':
    case '*':
    case '.':
        return flag;
    default:
        return '\0'; // 플래그가 없으면 null 문자 반환
    }
}

// 숫자의 자릿수를 계산하는 함수
static inline size_t az_nbrlen(int n)
{
    size_t len = (n < 0) ? 1 : 0; // 음수일 경우 부호를 포함하여 초기값 설정
    do
    {
        n /= 10;
        len++;
    } while (n != 0);
    return len;
}

// 문자열에서 특정 문자의 위치를 찾는 함수
static inline int az_chrpos(const char *str, int cnt)
{
    for (int i = 0; str[i]; i++)
    {
        if ((int)str[i] == cnt)
        {
            return i;
        }
    }
    return -1;
}

// 지정된 문자로 채우는 함수
static inline void az_fill(int fillcnt, char cnt)
{
    for (int i = 0; i <= fillcnt; i++)
    {
        putchar(cnt);
    }
}

// 숫자를 출력하는 함수
static inline void az_putnbr(int n)
{
    unsigned int abs_val = (n < 0) ? -n : n; // 절대값 계산
    if (n < 0)
    {
        putchar('-');
    }
    if (abs_val >= 10)
    {
        az_putnbr(abs_val / 10);
    }
    putchar((abs_val % 10) + '0');
}

// '+' 플래그에 따른 출력 처리 함수
static inline void az_plusflag(int d, char flag, int param, const char *format)
{
    int width = param - az_nbrlen(d);
    int flag_pos = az_chrpos(format, flag);
    char add_flag = format[flag_pos + 1];
    char add_flag_val = format[flag_pos + 2];

    if (flag == '+' && add_flag != '0')
    {
        if (width > 0)
            az_fill(width, ' '); // 빈칸 채움
        if (d > 0)
            putchar('+'); // 양수이면 '+' 출력
        az_putnbr(d);     // 숫자 출력
    }
    else if (flag == '+' && add_flag == '0')
    {
        d = -d;       // 음수로 변환
        putchar('-'); // '-' 출력
        if (add_flag_val != '1')
            putchar('0'); // '0' 출력
        az_putnbr(d);     // 숫자 출력
    }
}

// 디렉토리 확인 및 생성 함수
static inline void ensureDirectoryExists(const std::string &dirPath)
{
    struct stat info;
    std::cerr << "Directory being checked: " << dirPath << std::endl;

    // stat 함수로 디렉토리 상태 확인
    errno = 0; // stat 호출 전 초기화
    if (stat(dirPath.c_str(), &info) != 0)
    {
        std::cerr << "Stat failed for path: " << dirPath << " [errno: " << errno << " - " << ename[errno] << "]" << std::endl;

        // 디렉토리가 없으면 생성 시도
        errno = 0;
        if (mkdir(dirPath.c_str(), 0777) == -1)
        {
            // 오류 상세 출력
            std::cerr << "Failed to create directory: " << dirPath
                      << " [errno: " << errno << " - " << ename[errno] << "]" << std::endl;

            // 추가 디버깅 정보 출력
            switch (errno)
            {
            case EACCES:
                std::cerr << "Permission denied. Check your access rights to create this directory." << std::endl;
                break;
            case ENOENT:
                std::cerr << "A component of the path does not exist. Check the parent directory: "
                          << dirPath.substr(0, dirPath.find_last_of('/')) << std::endl;
                break;
            case EEXIST:
                std::cerr << "The path already exists but is not a directory. Check the file: " << dirPath << std::endl;
                break;
            default:
                std::cerr << "An unknown error occurred while creating the directory." << std::endl;
                break;
            }
        }
    }
    else if (!(info.st_mode & S_IFDIR))
    {
        std::cerr << dirPath << " exists but is not a directory." << std::endl;
    }
}

// 로그 매크로 정의// 매크로 정의
#if AZLOGD_LEVEL >= AZLOGD_LEVEL_INFO
#define AZLOGDI(format_str, locationLogUrl, scanResults, ...) \
    COUT_("INFO", __func__, __LINE__, format_str, locationLogUrl, scanResults, ##__VA_ARGS__)
#else
#define AZLOGDI(format_str, locationLogUrl, scanResults, ...) (void)0 // 로그 비활성화
#endif

#if AZLOGD_LEVEL >= AZLOGD_LEVEL_DEBUG
#define AZLOGDD(format_str, locationLogUrl, scanResults, ...) \
    COUT_("DEBUG", __func__, __LINE__, format_str, locationLogUrl, scanResults, ##__VA_ARGS__)
#else
#define AZLOGDD(format_str, locationLogUrl, scanResults, ...) (void)0
#endif

#if AZLOGD_LEVEL >= AZLOGD_LEVEL_WARNING
#define AZLOGDW(format_str, locationLogUrl, scanResults, ...) \
    COUT_("WARNING", __func__, __LINE__, format_str, locationLogUrl, scanResults, ##__VA_ARGS__)
#else
#define AZLOGDW(format_str, locationLogUrl, scanResults, ...) (void)0
#endif

#if AZLOGD_LEVEL >= AZLOGD_LEVEL_ERROR
#define AZLOGDE(format_str, locationLogUrl, scanResults, ...) \
    COUT_("ERROR", __func__, __LINE__, format_str, locationLogUrl, scanResults, ##__VA_ARGS__)
#else
#define AZLOGDE(format_str, locationLogUrl, scanResults, ...) (void)0
#endif

// COUT_ 함수 정의
static inline void COUT_(std::string log_level, std::string function, int line, const char *format_str, const std::string &locationLogUrl, std::vector<ScanResult> scanResults, ...)
{
    // 가변 인수 처리
    va_list ap;
    char buf[AZLOGD_BUF_SIZE];
    va_start(ap, scanResults); // 가변 인수 시작
    size_t size = vsnprintf(buf, AZLOGD_BUF_SIZE, format_str, ap);
    va_end(ap); // 가변 인수 종료

    std::string dst(buf, buf + size);

    CURRENT_T current;
    getCurrentTime(&current); // 현재 시간 가져오기

    static std::mutex coutWriteMutex;

    std::string logDir = std::string(RESOURCE_PATH);
    std::string filePath = logDir + "/" + locationLogUrl;

    ensureDirectoryExists(logDir);

    {
        std::lock_guard<std::mutex> lock(coutWriteMutex); // 동기화

        std::ostringstream rowBuilder;
        rowBuilder << "[" << current.year << "/" << current.month << "/" << current.day << " "
                   << current.hour << ":" << current.minute << ":" << current.second << "] "
                   << log_level << " (" << function << ":" << line << ") - " << dst;

        // errno가 설정된 경우 ename 추가
        if (errno > 0 && errno < MAX_ENAME)
        {
            rowBuilder << " [errno: " << errno << " - " << ename[errno] << "]";
        }

        // ScanResult 처리
        if (!scanResults.empty())
        {
            rowBuilder << " [ScanResults: ";
            for (const auto &result : scanResults)
            {
                rowBuilder << "HubId=" << result.hubId << ", Logs=" << result.logList.size() << "; ";
            }
            rowBuilder << "]";
        }

        rowBuilder << "\n";

        // 콘솔 출력
        std::cout << rowBuilder.str();

        // 파일에 로그 기록
        std::ofstream outFile(filePath, std::ios::app);
        errno = 0;
        if (!outFile.is_open())
        {
            std::cerr << "Failed to open log file: " << filePath << " [errno: " << errno << " - " << ename[errno] << "]" << std::endl;
        }
        else if (outFile.is_open())
        {
            outFile << rowBuilder.str();
            outFile.close();
        }
    }
}
#endif // _AZLOGD_H_

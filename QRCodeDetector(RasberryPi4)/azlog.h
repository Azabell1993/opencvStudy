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
#include "scan_result.h"

#define MAX_ENAME 133                                               // errno 배열의 최대 크기 정의
#define RESOURCE_PATH "/home/azabell/Desktop/QRCodeDetector/output" // 리소스 경로 정의

// 로그 레벨 정의
#define AZLOGD_LEVEL_NONE 0    // 로그 없음
#define AZLOGD_LEVEL_ERROR 1   // 오류 로그
#define AZLOGD_LEVEL_WARNING 2 // 경고 로그
#define AZLOGD_LEVEL_INFO 3    // 정보 로그
#define AZLOGD_LEVEL_DEBUG 4   // 디버그 로그
#define AZLOGD_BUF_SIZE 4096   // 로그 버퍼 크기

// 현재 로그 레벨 설정
#ifndef AZLOGD_LEVEL
#define AZLOGD_LEVEL AZLOGD_LEVEL_DEBUG // 기본 디버그 레벨
#endif

// errno 이름 배열 정의
static const char *ename[] = {
    /*   0 */ "",                   // 0: 에러 없음
    /*   1 */ "EPERM",              // 1: 작업 권한이 거부됨
    /*   2 */ "ENOENT",             // 2: 파일이나 디렉토리가 존재하지 않음
    /*   3 */ "ESRCH",              // 3: 프로세스를 찾을 수 없음
    /*   4 */ "EINTR",              // 4: 함수 호출이 신호로 방해받음
    /*   5 */ "EIO",                // 5: 입출력 오류
    /*   6 */ "ENXIO",              // 6: 디바이스가 없거나 주소가 유효하지 않음
    /*   7 */ "E2BIG",              // 7: 인수 목록이 너무 큼
    /*   8 */ "ENOEXEC",            // 8: 실행 파일 형식 오류
    /*   9 */ "EBADF",              // 9: 잘못된 파일 디스크립터
    /*  10 */ "ECHILD",             // 10: 자식 프로세스가 없음
    /*  11 */ "EAGAIN/EWOULDBLOCK", // 11: 자원 부족으로 작업이 일시적으로 차단됨
    /*  12 */ "ENOMEM",             // 12: 메모리 부족
    /*  13 */ "EACCES",             // 13: 접근 권한 거부
    /*  14 */ "EFAULT",             // 14: 잘못된 주소
    /*  15 */ "ENOTBLK",            // 15: 디바이스가 블록 디바이스가 아님
    /*  16 */ "EBUSY",              // 16: 자원이 사용 중임
    /*  17 */ "EEXIST",             // 17: 파일이 이미 존재함
    /*  18 */ "EXDEV",              // 18: 파일 시스템 간의 작업이 지원되지 않음
    /*  19 */ "ENODEV",             // 19: 디바이스가 없음
    /*  20 */ "ENOTDIR",            // 20: 디렉토리가 아님
    /*  21 */ "EISDIR",             // 21: 디렉토리로 작업이 요청됨
    /*  22 */ "EINVAL",             // 22: 잘못된 인수
    /*  23 */ "ENFILE",             // 23: 시스템의 열려 있는 파일 제한 초과
    /*  24 */ "EMFILE",             // 24: 프로세스의 열려 있는 파일 제한 초과
    /*  25 */ "ENOTTY",             // 25: 적절한 터미널 디바이스가 아님
    /*  26 */ "ETXTBSY",            // 26: 텍스트 파일이 사용 중임
    /*  27 */ "EFBIG",              // 27: 파일 크기 제한 초과
    /*  28 */ "ENOSPC",             // 28: 디스크 공간 부족
    /*  29 */ "ESPIPE",             // 29: 잘못된 파일 위치
    /*  30 */ "EROFS",              // 30: 읽기 전용 파일 시스템
    /*  31 */ "EMLINK",             // 31: 하드 링크 수 제한 초과
    /*  32 */ "EPIPE",              // 32: 파이프가 깨짐
    /*  33 */ "EDOM",               // 33: 수학 함수 입력 범위 초과
    /*  34 */ "ERANGE",             // 34: 수학 함수 출력 범위 초과
    /*  35 */ "EDEADLK/EDEADLOCK",  // 35: 교착 상태 감지
    /*  36 */ "ENAMETOOLONG",       // 36: 파일 이름이 너무 김
    /*  37 */ "ENOLCK",             // 37: 잠금 자원 부족
    /*  38 */ "ENOSYS",             // 38: 지원되지 않는 시스템 호출
    /*  39 */ "ENOTEMPTY",          // 39: 디렉토리가 비어 있지 않음
    /*  40 */ "ELOOP",              // 40: 심볼릭 링크가 너무 많음
    /*  41 */ "",                   // 41: 사용되지 않음
    /*  42 */ "ENOMSG",             // 42: 메시지 없음
    /*  43 */ "EIDRM",              // 43: 식별자가 제거됨
    /*  44 */ "ECHRNG",             // 44: 채널 번호가 범위를 벗어남
    /*  45 */ "EL2NSYNC",           // 45: 2단계 동기화 오류
    /*  46 */ "EL3HLT",             // 46: 3단계 중지
    /*  47 */ "EL3RST",             // 47: 3단계 재설정
    /*  48 */ "ELNRNG",             // 48: 링크 번호가 범위를 벗어남
    /*  49 */ "EUNATCH",            // 49: 프로토콜 드라이버가 연결되지 않음
    /*  50 */ "ENOCSI",             // 50: CSI가 없음
    /*  51 */ "EL2HLT",             // 51: 2단계 중지
    /*  52 */ "EBADE",              // 52: 잘못된 교환
    /*  53 */ "EBADR",              // 53: 잘못된 요청 디스크립터
    /*  54 */ "EXFULL",             // 54: 교환 테이블이 가득 참
    /*  55 */ "ENOANO",             // 55: 아니요(Anode)가 없음
    /*  56 */ "EBADRQC",            // 56: 잘못된 요청 코드
    /*  57 */ "EBADSLT",            // 57: 잘못된 슬롯
    /*  58 */ "",                   // 58: 사용되지 않음
    /*  59 */ "EBFONT",             // 59: 잘못된 폰트 파일 형식
    /*  60 */ "ENOSTR",             // 60: 스트림 장치가 아님
    /*  61 */ "ENODATA",            // 61: 데이터 없음
    /*  62 */ "ETIME",              // 62: 타이머 만료
    /*  63 */ "ENOSR",              // 63: 스트림 자원 부족
    /*  64 */ "ENONET",             // 64: 네트워크 없음
    /*  65 */ "ENOPKG",             // 65: 패키지 없음
    /*  66 */ "EREMOTE",            // 66: 원격 객체 오류
    /*  67 */ "ENOLINK",            // 67: 링크 없음
    /*  68 */ "EADV",               // 68: 광고 오류
    /*  69 */ "ESRMNT",             // 69: Srmount 오류
    /*  70 */ "ECOMM",              // 70: 통신 오류
    /*  71 */ "EPROTO",             // 71: 프로토콜 오류
    /*  72 */ "EMULTIHOP",          // 72: 다중 홉 시도 오류
    /*  73 */ "EDOTDOT",            // 73: ".." 횡단 오류
    /*  74 */ "EBADMSG",            // 74: 잘못된 메시지
    /*  75 */ "EOVERFLOW",          // 75: 값이 범위를 벗어남
    /*  76 */ "ENOTUNIQ",           // 76: 이름이 고유하지 않음
    /*  77 */ "EBADFD",             // 77: 잘못된 파일 디스크립터 상태
    /*  78 */ "EREMCHG",            // 78: 원격 주소 변경
    /*  79 */ "ELIBACC",            // 79: 라이브러리에 접근 불가
    /*  80 */ "ELIBBAD",            // 80: 라이브러리 손상
    /*  81 */ "ELIBSCN",            // 81: 라이브러리 섹션 손상
    /*  82 */ "ELIBMAX",            // 82: 라이브러리 개수 초과
    /*  83 */ "ELIBEXEC",           // 83: 라이브러리 실행 불가
    /*  84 */ "EILSEQ",             // 84: 잘못된 바이트 시퀀스
    /*  85 */ "ERESTART",           // 85: 재시작 필요
    /*  86 */ "ESTRPIPE",           // 86: 스트림 파이프 오류
    /*  87 */ "EUSERS",             // 87: 사용자 제한 초과
    /*  88 */ "ENOTSOCK",           // 88: 소켓이 아님
    /*  89 */ "EDESTADDRREQ",       // 89: 대상 주소 필요
    /*  90 */ "EMSGSIZE",           // 90: 메시지 크기 초과
    /*  91 */ "EPROTOTYPE",         // 91: 프로토콜 유형 오류
    /*  92 */ "ENOPROTOOPT",        // 92: 프로토콜 옵션 사용 불가
    /*  93 */ "EPROTONOSUPPORT",    // 93: 프로토콜 지원 안 함
    /*  94 */ "ESOCKTNOSUPPORT",    // 94: 소켓 유형 지원 안 함
    /*  95 */ "EOPNOTSUPP/ENOTSUP", // 95: 작업이 지원되지 않음
    /*  96 */ "EPFNOSUPPORT",       // 96: 프로토콜 패밀리 지원 안 함
    /*  97 */ "EAFNOSUPPORT",       // 97: 주소 패밀리 지원 안 함
    /*  98 */ "EADDRINUSE",         // 98: 주소가 이미 사용 중
    /*  99 */ "EADDRNOTAVAIL",      // 99: 사용 가능한 주소 없음
    /* 100 */ "ENETDOWN",           // 100: 네트워크 다운
    /* 101 */ "ENETUNREACH",        // 101: 네트워크 접근 불가
    /* 102 */ "ENETRESET",          // 102: 네트워크 연결 재설정
    /* 103 */ "ECONNABORTED",       // 103: 연결이 중단됨
    /* 104 */ "ECONNRESET",         // 104: 연결이 재설정됨
    /* 105 */ "ENOBUFS",            // 105: 버퍼 공간 부족
    /* 106 */ "EISCONN",            // 106: 이미 연결됨
    /* 107 */ "ENOTCONN",           // 107: 연결되지 않음
    /* 108 */ "ESHUTDOWN",          // 108: 소켓이 종료됨
    /* 109 */ "ETOOMANYREFS",       // 109: 참조 수 초과
    /* 110 */ "ETIMEDOUT",          // 110: 연결 시간 초과
    /* 111 */ "ECONNREFUSED",       // 111: 연결 거부됨
    /* 112 */ "EHOSTDOWN",          // 112: 호스트가 다운됨
    /* 113 */ "EHOSTUNREACH",       // 113: 호스트 접근 불가
    /* 114 */ "EALREADY",           // 114: 작업이 이미 진행 중
    /* 115 */ "EINPROGRESS",        // 115: 작업이 진행 중임
    /* 116 */ "ESTALE",             // 116: 스테일 파일 핸들
    /* 117 */ "EUCLEAN",            // 117: 구조체 필요
    /* 118 */ "ENOTNAM",            // 118: 파일이 네임 스페이스가 아님
    /* 119 */ "ENAVAIL",            // 119: 리소스 불가
    /* 120 */ "EISNAM",             // 120: 파일이 네임 스페이스임
    /* 121 */ "EREMOTEIO",          // 121: 원격 입출력 오류
    /* 122 */ "EDQUOT",             // 122: 디스크 할당량 초과
    /* 123 */ "ENOMEDIUM",          // 123: 매체 없음
    /* 124 */ "EMEDIUMTYPE",        // 124: 매체 유형 오류
    /* 125 */ "ECANCELED",          // 125: 작업이 취소됨
    /* 126 */ "ENOKEY",             // 126: 키 없음
    /* 127 */ "EKEYEXPIRED",        // 127: 키가 만료됨
    /* 128 */ "EKEYREVOKED",        // 128: 키가 취소됨
    /* 129 */ "EKEYREJECTED",       // 129: 키가 거부됨
    /* 130 */ "EOWNERDEAD",         // 130: 소유자가 사망 상태
    /* 131 */ "ENOTRECOVERABLE",    // 131: 상태를 복구할 수 없음
    /* 132 */ "ERFKILL",            // 132: 무선 장치가 차단됨
    /* 133 */ "EHWPOISON"           // 133: 메모리 오염
};

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
static inline std::string az_fill(int fillcnt, char cnt)
{
    return std::string(fillcnt, cnt); // 문자열로 반환
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
static inline std::string az_plusflag(int d, char flag, int param, const char *format)
{
    std::ostringstream result;
    int width = param - az_nbrlen(d); // 전체 너비에서 숫자 길이 차감
    int flag_pos = az_chrpos(format, flag);
    char add_flag = (flag_pos != -1) ? format[flag_pos + 1] : '\0';
    char add_flag_val = (flag_pos != -1 && format[flag_pos + 2]) ? format[flag_pos + 2] : '\0';

    if (flag == '+' && add_flag != '0')
    {
        if (width > 0)
            result << az_fill(width, ' '); // 빈칸으로 채움
        if (d > 0)
            result << "+"; // 양수일 경우 '+' 추가
        result << d;       // 숫자 추가
    }
    else if (flag == '+' && add_flag == '0')
    {
        if (d < 0)
        {
            result << "-";
            d = -d; // 음수 처리
        }
        if (width > 0)
            result << az_fill(width, '0'); // '0'으로 채움
        result << d;                       // 숫자 추가
    }

    return result.str();
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

// 로그 매크로 정의
#if AZLOGD_LEVEL >= AZLOGD_LEVEL_INFO
#define AZLOGDI(format_str, locationLogUrl, scanResults, ...) \
    COUT_("INFO", __func__, __LINE__, format_str, locationLogUrl, scanResults, ##__VA_ARGS__)
// AZLOGD_LEVEL이 INFO(3) 이상일 경우, COUT_ 함수 호출. 로그 레벨은 "INFO"로 지정.
// __func__는 현재 함수 이름, __LINE__은 현재 코드 줄 번호를 전달.
// format_str: 출력할 문자열 형식
// locationLogUrl: 로그 파일 경로
// scanResults: 스캔 결과 리스트
#else
#define AZLOGDI(format_str, locationLogUrl, scanResults, ...) (void)0 // 로그 비활성화
#endif

#if AZLOGD_LEVEL >= AZLOGD_LEVEL_DEBUG
#define AZLOGDD(format_str, locationLogUrl, scanResults, ...) \
    COUT_("DEBUG", __func__, __LINE__, format_str, locationLogUrl, scanResults, ##__VA_ARGS__)
// AZLOGD_LEVEL이 DEBUG(4) 이상일 경우, COUT_ 함수 호출. 로그 레벨은 "DEBUG"로 지정.
#else
#define AZLOGDD(format_str, locationLogUrl, scanResults, ...) (void)0 // 로그 비활성화
#endif

#if AZLOGD_LEVEL >= AZLOGD_LEVEL_WARNING
#define AZLOGDW(format_str, locationLogUrl, scanResults, ...) \
    COUT_("WARNING", __func__, __LINE__, format_str, locationLogUrl, scanResults, ##__VA_ARGS__)
// AZLOGD_LEVEL이 WARNING(2) 이상일 경우, COUT_ 함수 호출. 로그 레벨은 "WARNING"으로 지정.
#else
#define AZLOGDW(format_str, locationLogUrl, scanResults, ...) (void)0 // 로그 비활성화
#endif

#if AZLOGD_LEVEL >= AZLOGD_LEVEL_ERROR
#define AZLOGDE(format_str, locationLogUrl, scanResults, ...) \
    COUT_("ERROR", __func__, __LINE__, format_str, locationLogUrl, scanResults, ##__VA_ARGS__)
// AZLOGD_LEVEL이 ERROR(1) 이상일 경우, COUT_ 함수 호출. 로그 레벨은 "ERROR"로 지정.
#else
#define AZLOGDE(format_str, locationLogUrl, scanResults, ...) (void)0 // 로그 비활성화
#endif

// COUT_ 함수 정의
static inline void COUT_(std::string log_level, std::string function, int line, const char *format_str, const std::string &locationLogUrl, std::vector<ScanResult> scanResults, ...)
{
    // 가변 인수 처리
    va_list ap;                                                    // 가변 인수를 처리하기 위한 변수 선언
    char buf[AZLOGD_BUF_SIZE];                                     // 로그 메시지를 임시 저장할 버퍼 선언
    va_start(ap, scanResults);                                     // 가변 인수 초기화
    size_t size = vsnprintf(buf, AZLOGD_BUF_SIZE, format_str, ap); // 버퍼에 포맷팅된 문자열 저장
    va_end(ap);                                                    // 가변 인수 종료

    std::string dst(buf, buf + size); // 문자열로 변환

    CURRENT_T current;
    getCurrentTime(&current); // 현재 시간 가져오기

    static std::mutex coutWriteMutex; // 쓰레드 동기화를 위한 뮤텍스 선언

    std::string logDir = std::string(RESOURCE_PATH);      // 로그 디렉토리 경로 설정
    std::string filePath = logDir + "/" + locationLogUrl; // 로그 파일 경로 설정

    ensureDirectoryExists(logDir); // 로그 디렉토리 존재 확인 및 생성

    {
        std::lock_guard<std::mutex> lock(coutWriteMutex); // 동기화 블록 시작

        std::ostringstream rowBuilder; // 로그 메시지를 작성할 스트림
        rowBuilder << "[" << current.year << "/" << current.month << "/" << current.day << " "
                   << current.hour << ":" << current.minute << ":" << current.second << "] "
                   << log_level << " (" << function << ":" << line << ") - " << dst;

        // errno가 설정된 경우, 오류 정보 추가
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
                rowBuilder << "HubId=" << result.hubId << ". Logs=";
                for (const auto &entry : result.logList)
                {
                    // UUID 값을 형식에 맞게 변환 후 추가
                    rowBuilder << az_plusflag(entry.uuid, '+', 6, "%+06d") << " ";
                }
                rowBuilder << "; ";
            }
            rowBuilder << "]";
        }
        rowBuilder << "\n"; // 로그 메시지 끝에 줄바꿈 추가

        // 콘솔에 로그 출력
        std::cout << rowBuilder.str();

        // 파일에 로그 기록
        std::ofstream outFile(filePath, std::ios::app); // 로그 파일을 추가 모드로 엶
        errno = 0;                                      // errno 초기화
        if (!outFile.is_open())                         // 파일 열기에 실패한 경우
        {
            std::cerr << "Failed to open log file: " << filePath << " [errno: " << errno << " - " << ename[errno] << "]" << std::endl;
        }
        else
        {
            outFile << rowBuilder.str(); // 파일에 로그 메시지 작성
            outFile.close();             // 파일 닫기
        }
    }
}

#endif // _AZLOGD_H_
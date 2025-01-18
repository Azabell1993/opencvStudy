#ifndef SCAN_RESULT_H
#define SCAN_RESULT_H

#include <string>
#include <vector>

// 로그 항목 정의
struct Entry
{
    std::string serverId;
    int uuid;
};

// 스캔 결과 정의
struct ScanResult
{
    std::string hubId;
    std::vector<Entry> logList;
};

#endif // SCAN_RESULT_H
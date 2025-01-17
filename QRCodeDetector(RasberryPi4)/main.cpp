#include <iostream>
#include <memory>
#include <string>
#include "azlog.h"
#include "edge_ble.h"

int main()
{
    try
    {
        // 서버 IP와 포트를 설정
        std::string server_ip = "192.168.0.37"; // 맥 서버의 IP
        unsigned short server_port = 12345;     // 맥 서버의 포트

        std::vector<ScanResult> scanResults;
        ScanResult result;
        result.hubId = "Hub_1234";
        result.logList.push_back({"Server_1", 12345, 1673968254});
        scanResults.push_back(result);

        // BLE 서비스 초기화
        AZLOGDI("BLE 서비스 초기화 중: 서버 IP=%s, 포트=%d", "info_log.txt", scanResults, server_ip.c_str(), server_port);
        auto bleService = std::make_shared<EdgeBLE>(server_ip, server_port);

        // 스캔 시작
        AZLOGDI("BLE 스캔 시작.", "info_log.txt", scanResults);
        bleService->startScanning();

        std::cout << "Press Enter to stop scanning..." << std::endl;
        std::cin.get();

        // 스캔 중지
        AZLOGDI("BLE 스캔 중지.", "info_log.txt", scanResults);
        bleService->stopScanning();

        // 종료 로그 출력
        AZLOGDI("BLE 서비스 종료.", "info_log.txt", scanResults);

        return 0;
    }
    catch (const std::exception &e)
    {
        // 예외 처리 로그 출력
        AZLOGDE("예외 발생: %s", "error_log.txt", {}, e.what());
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}

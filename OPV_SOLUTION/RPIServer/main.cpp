#include <iostream>
#include <memory>
#include "edge_ble.h"

int main()
{
    try
    {
        // 서버 IP와 포트를 설정
        std::string server_ip = "192.168.0.37"; // 맥 서버의 IP
        unsigned short server_port = 12345;     // 맥 서버의 포트

        // BLE 서비스 초기화
        auto bleService = std::make_shared<EdgeBLE>(server_ip, server_port);

        // 스캔 시작 및 이미지 전송
        bleService->startScanning();

        std::cout << "Press Enter to stop scanning..." << std::endl;
        std::cin.get();

        // 스캔 중지
        bleService->stopScanning();

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}

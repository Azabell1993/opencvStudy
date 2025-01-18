#include "edge_ble.h"
#include "azlog.h"
#include <boost/asio.hpp>
#include <opencv2/opencv.hpp>
#include "scan_result.h"
#include <fstream>

using boost::asio::ip::tcp;

EdgeBLE::EdgeBLE(const std::string &server_ip, unsigned short server_port)
    : running(false), server_ip_(server_ip), server_port_(server_port)
{
    scanResults.clear();
    AZLOGDI("EdgeBLE initialized with empty scanResults.", "debug_log.txt", {});
}

void EdgeBLE::startScanning()
{
    running = true;
    scanningThread = std::make_shared<std::thread>(&EdgeBLE::scanBLEDevices, this);
}

void EdgeBLE::stopScanning()
{
    running = false;
    if (scanningThread && scanningThread->joinable())
    {
        scanningThread->join();
    }
}

void EdgeBLE::scanBLEDevices()
{
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // 5초 간격으로 전송
        sendImageToServer();
    }
}

void EdgeBLE::setScanResults(const std::vector<ScanResult> &results)
{
    std::lock_guard<std::mutex> lock(bleMutex);
    scanResults = results;

    AZLOGDI("Scan results set: Size=%d", "debug_log.txt", scanResults, results.size());
    for (const auto &result : scanResults)
    {
        AZLOGDI("HubId: %s, Logs Count: %d", "debug_log.txt", scanResults, result.hubId.c_str(), result.logList.size());
    }
}

void EdgeBLE::sendImageToServer()
{
    std::lock_guard<std::mutex> lock(bleMutex);

    if (scanResults.empty())
    {
        AZLOGDW("No scan results available to send. Check if setScanResults was called.", "warning_log.txt", scanResults);
        return;
    }

    AZLOGDI("Preparing to send scan results. Size: %d", "debug_log.txt", scanResults, scanResults.size());

    try
    {
        boost::asio::io_context io_context;

        // 서버 주소 및 포트 설정
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(server_ip_, std::to_string(server_port_));
        auto socket = std::make_shared<tcp::socket>(io_context);
        boost::asio::connect(*socket, endpoints);

        // sample.jpg 파일 읽기
        // cv::Mat image = cv::imread("sample.jpg", cv::IMREAD_COLOR);
        cv::Mat image = cv::imread("sample.jpg", cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            // std::cerr << "Failed to load image from sample.jpg" << std::endl;
            AZLOGDE("Failed to load image from sample.jpg", "error_log.txt", scanResults);
            return;
        }

        cv::Mat dst = image + 100;
        // 동기화된 scanResults 접근
        {
            std::lock_guard<std::mutex> lock(bleMutex);
            AZLOGDI("Sending image after pixel adjustment.", "info_log.txt", scanResults);
        }

        // 이미지를 png로 인코딩
        std::vector<uchar> buffer;
        cv::imencode(".png", dst, buffer);
        std::string image_data(buffer.begin(), buffer.end());

        // 이미지 데이터 크기를 먼저 전송
        uint32_t data_size = static_cast<uint32_t>(image_data.size());
        boost::asio::write(*socket, boost::asio::buffer(&data_size, sizeof(data_size)));

        // 이미지 데이터 전송
        boost::asio::write(*socket, boost::asio::buffer(image_data));
        std::cout << "Image sent to server: " << server_ip_ << ":" << server_port_ << std::endl;
    }
    catch (const std::exception &e)
    {
        AZLOGDE("Error sending image: %s", "error_log.txt", scanResults, e.what());
        std::cerr << "Error sending image: " << e.what() << std::endl;
    }
}

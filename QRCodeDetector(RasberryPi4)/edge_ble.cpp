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

    try
    {
        boost::asio::io_context io_context;

        // 서버 주소 및 포트 설정
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(server_ip_, std::to_string(server_port_));
        auto socket = std::make_shared<tcp::socket>(io_context);
        boost::asio::connect(*socket, endpoints);

        cv::Mat image = cv::imread("sample.jpg", cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            AZLOGDE("Failed to load image from sample.jpg", "error_log.txt", scanResults);
            std::cerr << "Failed to load image from sample.jpg" << std::endl;
            return;
        }

        cv::Mat dst = image + 100;
        std::vector<uchar> buffer;
        cv::imencode(".png", dst, buffer);
        std::string image_data(buffer.begin(), buffer.end());

        uint32_t data_size = static_cast<uint32_t>(image_data.size());
        uint32_t data_size_network_order = htonl(data_size);

        AZLOGDI("Sending image size: %d bytes", "debug_log.txt", scanResults, data_size);

        boost::asio::write(*socket, boost::asio::buffer(&data_size_network_order, sizeof(data_size_network_order)));
        boost::asio::write(*socket, boost::asio::buffer(image_data));

        AZLOGDI("Image sent successfully.", "debug_log.txt", scanResults);
    }
    catch (const std::exception &e)
    {
        AZLOGDE("Error in sendImageToServer: %s", "error_log.txt", scanResults, e.what());
        std::cerr << "Error in sendImageToServer: " << e.what() << std::endl;
    }
}

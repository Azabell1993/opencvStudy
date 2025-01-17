#include "edge_ble.h"
#include <boost/asio.hpp>
#include <opencv2/opencv.hpp>
#include <fstream>

using boost::asio::ip::tcp;

EdgeBLE::EdgeBLE(const std::string &server_ip, unsigned short server_port)
    : running(false), server_ip_(server_ip), server_port_(server_port) {}

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

void EdgeBLE::sendImageToServer()
{
    try
    {
        boost::asio::io_context io_context;

        // 서버 주소 및 포트 설정
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(server_ip_, std::to_string(server_port_));
        auto socket = std::make_shared<tcp::socket>(io_context);
        boost::asio::connect(*socket, endpoints);

        // sample.jpg 파일 읽기
        cv::Mat image = cv::imread("sample.jpg", cv::IMREAD_COLOR);
        if (image.empty())
        {
            std::cerr << "Failed to load image from sample.jpg" << std::endl;
            return;
        }

        // 이미지를 JPEG로 인코딩
        std::vector<uchar> buffer;
        cv::imencode(".jpg", image, buffer);
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
        std::cerr << "Error sending image: " << e.what() << std::endl;
    }
}

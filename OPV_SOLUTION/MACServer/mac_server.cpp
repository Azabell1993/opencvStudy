#include <iostream>
#include <boost/asio.hpp>
#include <opencv2/opencv.hpp>
#include <thread>
#include <memory>
#include <fstream>
#include <cstdlib>

using boost::asio::ip::tcp;

class Server
{
public:
    // Server 객체가 생성되면서 io_context_ 객체도 생성
    // tcp::endpoint(tcp::v4(), port) : TCP 프로토콜을 사용하여 IPv4 주소의 port번호에 바인딩하는 endpoint를 생성
    explicit Server(unsigned short port) : acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)) {}

    void start()
    {
        acceptConnection();
        io_context_.run();
    }

    void printHexDump(const std::string &filePath)
    {
        std::string command = "hexdump -C " + filePath;
        std::cout << "Hexdump of file : " << filePath << std::endl;
        int retCode = system(command.c_str());
        if (retCode != 0)
        {
            std::cerr << "Failed to execute hexdump. Return code: " << retCode << std::endl;
        }
    }

private:
    void acceptConnection()
    {
        auto socket = std::make_shared<tcp::socket>(io_context_);
        // 비동기적으로 소켓 연결을 수락함
        acceptor_.async_accept(*socket, [this, socket](const boost::system::error_code &ec)
                               {
                                   if (!ec)
                                   {
                                       std::cout << "Client connected: " << socket->remote_endpoint() << std::endl;
                                       receiveData(socket);
                                   }
                                   acceptConnection(); // 다음 연결을 수락
                               });
    }

    void receiveData(const std::shared_ptr<tcp::socket> &socket)
    {
        try
        {
            uint32_t data_size_network_order = 0;
            // 비동기적으로 소켓에서 데이터를 읽어옴(데이터의 크기를 네트워크 바이트 오더로 수신 후 이를 변환)
            boost::asio::read(*socket, boost::asio::buffer(&data_size_network_order, sizeof(data_size_network_order)));
            uint32_t data_size = ntohl(data_size_network_order);

            std::cout << "Expected data size: " << data_size << " bytes" << std::endl;

            if (data_size == 0)
            {
                std::cerr << "Error: Received data size is zero. Client may not have sent data." << std::endl;
                return;
            }

            std::vector<char> image_buffer(data_size);
            size_t total_read = 0;
            while (total_read < data_size)
            {
                size_t bytes_read = boost::asio::read(*socket, boost::asio::buffer(image_buffer.data() + total_read, data_size - total_read));
                total_read += bytes_read;
                std::cout << "Read " << bytes_read << " bytes, total: " << total_read << "/" << data_size << " bytes" << std::endl;
            }

            std::cout << "Received " << total_read << " bytes of image data." << std::endl;

            processImage(std::string(image_buffer.begin(), image_buffer.end()));
            sendResponse(socket, "Acknowledged");
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error in receiveData: " << e.what() << std::endl;
        }
    }

    void sendResponse(const std::shared_ptr<tcp::socket> &socket, const std::string &message)
    {
        // 비동기적으로 소켓에 데이터를 전송
        boost::asio::async_write(*socket, boost::asio::buffer(message), [socket](const boost::system::error_code &ec, std::size_t)
                                 {
            if (!ec) {
                std::cout << "Response sent." << std::endl;
            } });
    }

    void saveDebugData(const std::string &data)
    {
        std::string debugFilePath = "./debug/debug_image_data.raw";
        std::ofstream debugFile(debugFilePath, std::ios::binary);
        if (debugFile)
        {
            debugFile.write(data.data(), data.size());
            std::cout << "Raw image data saved to " << debugFilePath << std::endl;
        }
        else
        {
            std::cerr << "Failed to save debug data to " << debugFilePath << std::endl;
        }
    }

    void processImage(const std::string &data)
    {
        try
        {
            // 데이터를 JPEG 이미지로 디코딩
            std::vector<uchar> imageBuffer(data.begin(), data.end());
            cv::Mat receivedImage = cv::imdecode(imageBuffer, cv::IMREAD_COLOR);

            if (receivedImage.empty())
            {
                std::cerr << "Failed to decode the image." << std::endl;
                saveDebugData(data);
                return;
            }

            // 이미지를 파일로 저장
            std::string outputFilename = "received_image.png";
            cv::imwrite(outputFilename, receivedImage);
            std::cout << "Image saved to " << outputFilename << std::endl;

// GUI 환경에서만 이미지 표시
#ifdef SHOW_GUI
            cv::imshow("Received Image", receivedImage);
            cv::waitKey(1);
            std::cout << "Image displayed." << std::endl;
#else
            std::cout << "GUI display is disabled." << std::endl;
#endif
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error in processImage: " << e.what() << std::endl;
        }
    }

    // io_context_: I/O 작업을 위한 컨텍스트 객체. Boost.Asio 라이브러리에서 비동기 I/O 작업을 수행하기 위한 핵심 객체
    boost::asio::io_context io_context_;
    
    // 생성된 엔드포인트에서 들어오는 연결 요청을 수락하는 역할
    tcp::acceptor acceptor_;
};

int main()
{
    try
    {
        unsigned short port = 12345; // 서버에서 사용할 포트 번호
        Server server(port);
        std::cout << "Server is running on port " << port << std::endl;
        server.start();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}

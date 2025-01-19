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

    void
    sendResponse(const std::shared_ptr<tcp::socket> &socket, const std::string &message)
    {
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

    boost::asio::io_context io_context_;
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

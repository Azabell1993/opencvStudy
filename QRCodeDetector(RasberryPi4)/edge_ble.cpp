#include <opencv2/opencv.hpp>
#include "edge_ble.h"
#include "azlog.h"
#include <boost/asio.hpp>
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

cv::Mat EdgeBLE::calcGrayHist(const cv::Mat &img)
{
    CV_Assert(img.type() == CV_8UC1);

    cv::Mat hist;
    int channels[] = {0};
    int dims = 1;
    const int histSize[] = {256};
    float graylevel[] = {0, 256};
    const float *ranges[] = {graylevel};

    cv::calcHist(&img, 1, channels, cv::noArray(), hist, dims, histSize, ranges);

    return hist;
}

cv::Mat EdgeBLE::getGrayHistImage(const cv::Mat &hist)
{
    CV_Assert(hist.type() == CV_32FC1);
    CV_Assert(hist.size() == cv::Size(1, 256));

    double histMax;
    minMaxLoc(hist, 0, &histMax);

    cv::Mat imgHist(100, 256, CV_8UC1, cv::Scalar(255));
    for (int i = 0; i < 256; i++)
    {
        cv::line(imgHist, cv::Point(i, 100),
                 cv::Point(i, 100 - cvRound(hist.at<float>(i, 0) * 100 / histMax)),
                 cv::Scalar(0));
    }

    return imgHist;
}

cv::Mat EdgeBLE::filter_embossing(const cv::Mat &img)
{
    if (img.empty())
    {
        AZLOGDE("Failed to load image from sample.jpg", "error_log.txt", scanResults);
        std::cerr << "Failed to load image from sample.jpg" << std::endl;
    }

    float data[] = {-1, -1, 0, -1, 0, 1, 0, 1, 1};
    cv::Mat emboss(3, 3, CV_32FC1, data);

    cv::Mat dst;
    filter2D(img, dst, -1, emboss, cv::Point(-1, -1), 128);

    return dst;
}

cv::Mat EdgeBLE::blurring_mean(const cv::Mat &img)
{
    if (img.empty())
    {
        AZLOGDE("Failed to load image from sample.jpg", "error_log.txt", scanResults);
        std::cerr << "Failed to load image from sample.jpg" << std::endl;
    }

    cv::Mat dst;
    for (int ksize = 3; ksize <= 7; ksize += 2)
    {
        blur(img, dst, cv::Size(ksize, ksize));

        cv::String desc = cv::format("Mean: %dx%d", ksize, ksize);
        putText(dst, desc, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255), 1, cv::LINE_AA);
    }

    return dst;
}

cv::Mat EdgeBLE::blurring_affine_Transform(const cv::Mat &img)
{
    if (img.empty())
    {
        AZLOGDE("Failed to load image from sample.jpg", "error_log.txt", scanResults);
        std::cerr << "Failed to load image from sample.jpg" << std::endl;
        return cv::Mat(); // 비어있는 Mat 반환
    }

    // 원본 및 변환 좌표 설정
    cv::Point2f srcPts[3] = {
        cv::Point2f(0, 0),
        cv::Point2f(img.cols - 1, 0),
        cv::Point2f(img.cols - 1, img.rows - 1)};
    cv::Point2f dstPts[3] = {
        cv::Point2f(0, 0),
        cv::Point2f(img.cols - 1, 0),
        cv::Point2f(img.cols - 50, img.rows - 50)}; // 변환을 더 명확하게 하기 위해 x 좌표도 이동

    // 변환 행렬 계산
    cv::Mat M = getAffineTransform(srcPts, dstPts);

    // 행렬 출력 (디버깅용)
    std::cout << "Affine Transform Matrix:\n"
              << M << std::endl;

    // 블러링 적용
    cv::Mat blurred;
    blur(img, blurred, cv::Size(7, 7)); // 블러링 커널 크기 7x7

    // 어파인 변환 수행
    cv::Mat dst;
    warpAffine(blurred, dst, M, img.size()); // img.size()로 출력 이미지 크기 지정

    // 변환된 이미지 저장 (디버깅용)
    cv::imwrite("affine_transformed_image.jpg", dst);

    // 결과 반환
    return dst;
}

// 정적 멤버 변수 초기화
std::vector<cv::Point2f> EdgeBLE::selectedPoints;

cv::Mat EdgeBLE::event_lbuttondown(const cv::Mat &img)
{
    if (img.empty())
    {
        std::cerr << "Input image is empty!" << std::endl;
        return cv::Mat();
    }

    // // 마우스 이벤트 처리를 위한 창 생성
    cv::namedWindow("Select Points");
    cv::setMouseCallback("Select Points", EdgeBLE::onMouse, (void *)&img);
    std::cout << "이미지에서 투시 변환을 위한 4개의 점을 클릭하세요." << std::endl;

    cv::imshow("Select Points", img);
    while (selectedPoints.size() < 4)
    {
        cv::waitKey(1);
    }

    // std::cout << "이미지에서 투시 변환을 위한 4개의 점을 입력하세요 (x, y 순서로 4개):" << std::endl;
    // std::vector<cv::Point2f> selectedPoints(4);
    // for (int i = 0; i < 4; ++i)
    // {
    //     std::cout << "Point " << i + 1 << ": ";
    //     std::cin >> selectedPoints[i].x >> selectedPoints[i].y;
    // }

    // 투시 변환 후의 목적지 좌표
    std::vector<cv::Point2f> dstPoints = {
        cv::Point2f(0, 0),
        cv::Point2f(img.cols - 1, 0),
        cv::Point2f(img.cols - 1, img.rows - 1),
        cv::Point2f(0, img.rows - 1)};

    // 투시 변환 행렬 계산
    cv::Mat perspectiveMatrix = cv::getPerspectiveTransform(selectedPoints, dstPoints);

    // 투시 변환 적용
    cv::Mat transformed;
    cv::warpPerspective(img, transformed, perspectiveMatrix, img.size());

    cv::destroyAllWindows(); // 창 닫기
    // cv::imwrite("transformed_image.jpg",transtormed);

    return transformed;
}

void EdgeBLE::onMouse(int event, int x, int y, int flags, void *userdata)
{
    if (event == cv::EVENT_LBUTTONDOWN)
    {
        if (selectedPoints.size() < 4)
        {
            selectedPoints.emplace_back(cv::Point2f(x, y));
            std::cout << "Point selected: (" << x << ", " << y << ")" << std::endl;

            cv::Mat *img = static_cast<cv::Mat *>(userdata);
            cv::circle(*img, cv::Point(x, y), 5, cv::Scalar(0, 0, 255), -1);
            cv::imshow("Select Points", *img);
        }
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
        // Boost.Asio는 쓰레드와 명시적 Locking을 기반으로 한 동시성 모델의 사용을 프로그램에게 요구하지 않으면서 느린 I/O operation을 관리하는 도구를 제공.
        boost::asio::io_context io_context; // 비동기 작업의 실행을 조정하는 핵심 객체(하단 resolver를 통해 서버 주소와 포트를 조회)

        // 서버 주소 및 포트 설정(resolver- 호스트이름 확인, 리졸버)
        tcp::resolver resolver(io_context); // DNS 조회를 통해 호스트 이름을 IP 주소로 변환하는 역할 수행
        auto endpoints = resolver.resolve(server_ip_, std::to_string(server_port_));    // 조회되고 난 이후에는 endpoint
        auto socket = std::make_shared<tcp::socket>(io_context);
        boost::asio::connect(*socket, endpoints);   // 순서대로 각 endpoints에 연결하여 socket 연결을 수립

        cv::Mat image = cv::imread("sample2.jpg", cv::IMREAD_GRAYSCALE);
        if (image.empty())
        {
            AZLOGDE("Failed to load image from sample.jpg", "error_log.txt", scanResults);
            std::cerr << "Failed to load image from sample.jpg" << std::endl;
            return;
        }

        EdgeBLE edge;
        cv::Mat transformedImage = edge.event_lbuttondown(image);

        if (transformedImage.empty())
        {
            std::cerr << "Transformed image is empty!" << std::endl;
            return;
        }

        std::vector<uchar> buffer;
        // cv::imencode(".png", filter_embossing(image), buffer);
        // cv::imencode(".png", blurring_affine_Transform(image), buffer);
        cv::imencode(".png", transformedImage, buffer);
        std::string image_data(buffer.begin(), buffer.end());

        // 이미지 데이터를 네트워크로 전송하기 위해 먼저 데이터 크기를 계산
        uint32_t data_size = static_cast<uint32_t>(image_data.size());  // image_data.size()는 std::string 타입의 크기를 반환하며, 전송할 데이터의 총 바이트 수를 나타냄.
        
        // 데이터 크기를 네트워크 바이트 오더(빅 엔디언)로 변환
        // 네트워크 바이트 오더 → 호스트 바이트 오더로 변환
        // 네트워크 통신 표준상 데이터를 송수신할 때 항상 빅 엔디언을 사용하도록 규정됨
        // 인터넷 프로토콜(IP), TCP, UDP 등의 프로토콜이 정의된 RFC1700과 같은 표준 문서에 명시되어있음.
        // 빅 엔디언 : Most Significant Byte, MSB부터 순서대로 정렬한다.
        // 좌측에서 우측으로 가는 순서와 유사하여 직관적(0x12345678 -> [12][34][56][78]
        uint32_t data_size_network_order = htonl(data_size);

        AZLOGDI("Sending image size: %d bytes", "debug_log.txt", scanResults, data_size);

        // 변환된 데이터 크기를 네트워크 소켓으로 전송
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

#include <opencv2/opencv.hpp>
#include "edge_ble.h"
#include "azlog.h"
#include <boost/asio.hpp>
#include "scan_result.h"
#include <fstream>
#include <vector>

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

// 4개의 점을 찍고 송신
cv::Mat EdgeBLE::event_lbuttondown(const cv::Mat &img)
{
    if (img.empty())
    {
        std::cerr << "Input image is empty!" << std::endl;
        return cv::Mat();
    }

    // 마우스 이벤트 처리를 위한 창 생성
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

cv::Mat EdgeBLE::hough_lines(const cv::Mat &img)
{
    if(img.empty()) {
        std::cerr << "Input image is empty!" << std::endl;
        return cv::Mat();
    }

    cv::Mat edge;
    Canny(img, edge, 50, 150);

    std::vector<cv::Vec2f> lines;
    cv::HoughLines(edge, lines, 1, CV_PI / 180, 250);

    cv::Mat dst;
    cv::cvtColor(edge, dst, cv::COLOR_GRAY2BGR);

    for(size_t i=0; i<lines.size(); i++) {
        float r = lines[i][0], t = lines[i][1];
        double cos_t = cos(t), sin_t = sin(t);
        double x0 = r*cos_t, y0 = r * sin_t;
        double alpha = 1000;

        cv::Point pt1(cvRound(x0 + alpha * (-sin_t)), cvRound(y0 + alpha * cos_t));
        cv::Point pt2(cvRound(x0 - alpha * (-sin_t)), cvRound(y0 - alpha * cos_t));
        line(dst, pt1, pt2, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
    }

    return dst;
}

cv::Mat EdgeBLE::hough_lines_optimized(const cv::Mat &img)
{
    if(img.empty()) {
        std::cerr << "Input image is empty!" << std::endl;
        return cv::Mat();
    }

    // Step 1: Noise reduction
    cv::Mat blurred, edge;
    cv::GaussianBlur(img, blurred, cv::Size(5, 5), 1.5);
    cv::Canny(blurred, edge, 50, 150);

    // Step 2: Hough Line Transform
    std::vector<cv::Vec2f> lines;
    cv::HoughLines(edge, lines, 1, CV_PI / 180, 450);

    cv::Mat dst;
    cv::cvtColor(edge, dst, cv::COLOR_GRAY2BGR);

    // Step 3: Filter lines based on angle and position
    for(size_t i = 0; i < lines.size(); i++) {
        float r = lines[i][0], t = lines[i][1];
        // Filter for near-vertical or near-horizontal lines
        if (std::abs(t) < CV_PI / 18 || std::abs(t - CV_PI / 2) < CV_PI / 18) {
            double cos_t = cos(t), sin_t = sin(t);
            double x0 = r * cos_t, y0 = r * sin_t;
            double alpha = 1000;

            cv::Point pt1(cvRound(x0 + alpha * (-sin_t)), cvRound(y0 + alpha * cos_t));
            cv::Point pt2(cvRound(x0 - alpha * (-sin_t)), cvRound(y0 - alpha * cos_t));
            line(dst, pt1, pt2, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
        }
    }

    return dst;
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

cv::Mat EdgeBLE::process_image_all_advanced(const cv::Mat &img)
{
    if (img.empty()) {
        std::cerr << "Error: Input image is empty!" << std::endl;
        return cv::Mat();
    }

    // Step 1: 그레이스케일 변환 및 대비 조정
    cv::Mat gray, enhanced;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

    // CLAHE(Contrast Limited Adaptive Histogram Equalization) 적용
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
    clahe->apply(gray, enhanced);

    // Step 2: 가우시안 블러 적용 (노이즈 제거)
    cv::Mat blurred;
    cv::GaussianBlur(enhanced, blurred, cv::Size(5, 5), 1.5);

    // Step 3: Canny Edge Detection 수행
    cv::Mat edges;
    cv::Canny(blurred, edges, 50, 150);

    // Step 4: Hough Line Transform 적용
    std::vector<cv::Vec2f> lines;
    cv::Mat lineImage = img.clone();
    cv::HoughLines(edges, lines, 1, CV_PI / 180, 100);

    for (size_t i = 0; i < lines.size(); i++) {
        float rho = lines[i][0], theta = lines[i][1];
        double a = cos(theta), b = sin(theta);
        double x0 = a * rho, y0 = b * rho;
        cv::Point pt1(cvRound(x0 + 1000 * (-b)), cvRound(y0 + 1000 * (a)));
        cv::Point pt2(cvRound(x0 - 1000 * (-b)), cvRound(y0 - 1000 * (a)));
        cv::line(lineImage, pt1, pt2, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
    }

    // Step 5: 형태학적 연산을 활용한 노이즈 제거
    cv::Mat morphKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::Mat morphProcessed;
    cv::morphologyEx(edges, morphProcessed, cv::MORPH_CLOSE, morphKernel);

    // Step 6: 윤곽선 검출 및 강조
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::Mat contourImage = img.clone();
    cv::findContours(morphProcessed, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    cv::drawContours(contourImage, contours, -1, cv::Scalar(0, 255, 0), 2);

    // Step 7: 모든 이미지 크기 및 타입 맞추기
    cv::Size targetSize(img.cols, img.rows);

    std::vector<cv::Mat> images = {enhanced, edges, lineImage};

    for (auto &mat : images) {
        // 1. 크기 통일
        if (mat.size() != targetSize) {
            cv::resize(mat, mat, targetSize);
        }

        // 2. 채널 개수 통일
        if (mat.channels() == 1) {
            cv::cvtColor(mat, mat, cv::COLOR_GRAY2BGR);
        } else if (mat.channels() != 3) {
            std::cerr << "Unexpected channel count: " << mat.channels() << std::endl;
            return cv::Mat();
        }

        // 3. 데이터 타입 통일 (CV_8UC3)
        if (mat.type() != CV_8UC3) {
            mat.convertTo(mat, CV_8UC3);
        }
    }

    // Step 8: 최종 이미지 병합
    cv::Mat finalResult;
    cv::hconcat(images, finalResult);

    return finalResult;
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

        cv::Mat image = cv::imread("building.jpg");
        if (image.empty())
        {
            AZLOGDE("Failed to load image from sample.jpg", "error_log.txt", scanResults);
            std::cerr << "Failed to load image from sample.jpg" << std::endl;
            return;
        }

        // 새로운 이미지 처리 로직 적용
        cv::Mat processedImage = process_image_all_advanced(image);

        // 1. 이미지가 비어 있으면 오류 출력 후 종료
        if (processedImage.empty()) {
            AZLOGDE("Error: Processed image is empty", "error_log.txt", scanResults);
            std::cerr << "Error: Processed image is empty!" << std::endl;
            return;
        }

        // 2. 정확한 타입 변환 수행
        cv::Mat finalImage;
        if (processedImage.channels() == 1) {
            cv::cvtColor(processedImage, finalImage, cv::COLOR_GRAY2BGR);
        } else if (processedImage.channels() == 3) {
            finalImage = processedImage.clone();
        } else {
            AZLOGDE("Unexpected number of channels: %d", "error_log.txt", scanResults, processedImage.channels());
            std::cerr << "Unexpected number of channels: " << processedImage.channels() << std::endl;
            return;
        }

        // 3. 최종 이미지를 서버로 전송
        std::vector<uchar> buffer;
        cv::imencode(".jpg", finalImage, buffer);
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

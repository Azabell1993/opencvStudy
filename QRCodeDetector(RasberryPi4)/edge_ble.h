#ifndef EDGE_BLE_H
#define EDGE_BLE_H

#include <opencv2/opencv.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <string>
#include <thread>
#include "scan_result.h"

class EdgeBLE
{
public:
    EdgeBLE() = default;

    EdgeBLE(const std::string &server_ip, unsigned short server_port);
    void startScanning();
    void stopScanning();
    void setScanResults(const std::vector<ScanResult> &results);

    cv::Mat calcGrayHist(const cv::Mat &img);
    cv::Mat getGrayHistImage(const cv::Mat &hist);
    cv::Mat filter_embossing(const cv::Mat &img);
    cv::Mat blurring_mean(const cv::Mat &img);
    cv::Mat blurring_affine_Transform(const cv::Mat &img);
    cv::Mat event_lbuttondown(const cv::Mat &img);

private:
    void scanBLEDevices();
    void sendImageToServer();

    bool running;

    static void onMouse(int event, int x, int y, int flags, void *userdata);
    static std::vector<cv::Point2f> selectedPoints;

    std::shared_ptr<std::thread> scanningThread;
    std::mutex bleMutex;
    std::vector<ScanResult> scanResults;

    std::string server_ip_;
    unsigned short server_port_;
};

#endif // EDGE_BLE_H

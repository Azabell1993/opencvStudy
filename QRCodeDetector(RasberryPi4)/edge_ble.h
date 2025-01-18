#ifndef EDGE_BLE_H
#define EDGE_BLE_H

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
    EdgeBLE(const std::string &server_ip, unsigned short server_port);
    void startScanning();
    void stopScanning();
    void setScanResults(const std::vector<ScanResult> &results);

private:
    void scanBLEDevices();
    void sendImageToServer();

    bool running;
    std::shared_ptr<std::thread> scanningThread;
    std::mutex bleMutex;
    std::vector<ScanResult> scanResults;

    std::string server_ip_;
    unsigned short server_port_;
};

#endif // EDGE_BLE_H

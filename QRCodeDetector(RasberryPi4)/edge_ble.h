#ifndef EDGE_BLE_H
#define EDGE_BLE_H

#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <functional>

class EdgeBLE
{
public:
    EdgeBLE(const std::string &server_ip, unsigned short server_port);
    void startScanning();
    void stopScanning();

private:
    void scanBLEDevices();
    void processBLEData(const std::string &data);
    void sendImageToServer();

    bool running;
    std::shared_ptr<std::thread> scanningThread;
    std::mutex bleMutex;

    std::string server_ip_;
    unsigned short server_port_;
};

#endif // EDGE_BLE_H

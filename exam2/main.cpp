#include <opencv2/opencv.hpp>
#include <iostream>

int main()
{
    cv::Mat image = cv::imread("/Users/mac/Desktop/workspace/opencv/exam2/example.jpeg");
    if (image.empty())
    {
        std::cerr << "Could not read the image" << std::endl;
        return 1;
    }
    cv::imshow("Display Window", image);
    cv::waitKey(0);
    return 0;
}

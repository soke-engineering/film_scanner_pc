#include <iostream>
#include <opencv2/opencv.hpp>

int main()
{
    std::cout << "OpenCV version: " << CV_VERSION << std::endl;

    // Test basic OpenCV functionality
    cv::Mat test_image = cv::Mat::zeros(100, 100, CV_8UC3);
    cv::circle(test_image, cv::Point(50, 50), 30, cv::Scalar(0, 255, 0), 2);

    std::cout << "OpenCV test successful! Created a test image with a circle." << std::endl;
    std::cout << "Image size: " << test_image.size() << std::endl;

    return 0;
}
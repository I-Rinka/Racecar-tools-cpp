#pragma once
#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>
#include <string>

class VideoWrapper {
public:
    explicit VideoWrapper(const std::string &path);
    VideoWrapper(const VideoWrapper &other);

    void setFrame(int index);
    cv::Mat setAndGetFrame(int index);
    cv::Mat getNextFrame();

    double getFrameRate() const;
    int getFrameCount() const;
    bool isOpened() const;

private:
    std::string m_path;
    cv::VideoCapture m_cap;
};

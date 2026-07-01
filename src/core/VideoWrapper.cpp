#include "VideoWrapper.h"

VideoWrapper::VideoWrapper(const std::string &path)
    : m_path(path), m_cap(path) {}

VideoWrapper::VideoWrapper(const VideoWrapper &other)
    : m_path(other.m_path), m_cap(other.m_path) {}

void VideoWrapper::setFrame(int index) {
    if (index < getFrameCount())
        m_cap.set(cv::CAP_PROP_POS_FRAMES, index);
}

cv::Mat VideoWrapper::setAndGetFrame(int index) {
    if (index >= getFrameCount()) return {};
    m_cap.set(cv::CAP_PROP_POS_FRAMES, index);
    cv::Mat frame;
    m_cap.read(frame);
    return frame;
}

cv::Mat VideoWrapper::getNextFrame() {
    cv::Mat frame;
    if (m_cap.isOpened()) m_cap.read(frame);
    return frame;
}

double VideoWrapper::getFrameRate() const {
    return m_cap.get(cv::CAP_PROP_FPS);
}

int VideoWrapper::getFrameCount() const {
    return static_cast<int>(m_cap.get(cv::CAP_PROP_FRAME_COUNT));
}

bool VideoWrapper::isOpened() const {
    return m_cap.isOpened();
}

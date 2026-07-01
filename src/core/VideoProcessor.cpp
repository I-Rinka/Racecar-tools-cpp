#include "VideoProcessor.h"
#include <thread>
#include <future>
#include <algorithm>
#include <cmath>

static constexpr double SPEED_MAX = 500.0;
static constexpr double MAX_DELTA = 50.0;

VideoProcessor::VideoProcessor(double frameRate, int maxWorkers)
    : m_timeInterval(1.0 / frameRate), m_maxWorkers(maxWorkers) {
#ifdef HAS_TESSERACT
    m_tess.Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY);
    m_tess.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
    m_tess.SetVariable("tessedit_char_whitelist", "0123456789.");
#endif
}

VideoProcessor::~VideoProcessor() {
#ifdef HAS_TESSERACT
    m_tess.End();
#endif
}

bool VideoProcessor::sanityCheck(double value) const {
    if (value < 0 || value > SPEED_MAX) return false;
    if (m_lastSpeed >= 0 && std::abs(value - m_lastSpeed) > MAX_DELTA) return false;
    return true;
}

double VideoProcessor::tesseractOCR(const cv::Mat &frame) {
#ifdef HAS_TESSERACT
    cv::Mat gray;
    if (frame.channels() == 3)
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    else
        gray = frame;
    m_tess.SetImage(gray.data, gray.cols, gray.rows, 1, gray.step);
    char *text = m_tess.GetUTF8Text();
    if (!text) return 0.0;
    std::string s(text);
    delete[] text;
    // clean non-digit/dot chars
    std::string clean;
    for (char c : s)
        if ((c >= '0' && c <= '9') || c == '.') clean += c;
    if (clean.empty()) return 0.0;
    try { return std::stod(clean); }
    catch (...) { return 0.0; }
#else
    (void)frame;
    return 0.0;
#endif
}

double VideoProcessor::smartOCR(const cv::Mat &frame) {
    double value = tesseractOCR(frame);
    if (value > 0 && sanityCheck(value)) {
        learnFromOCRResult(m_digitCache, frame, static_cast<int>(std::round(value)));
        return value;
    }

    auto [tmplValue, score] = recognizeByTemplate(m_digitCache, frame);
    if (score > 0.5 && sanityCheck(tmplValue))
        return tmplValue;

    return value > 0 ? value : tmplValue;
}

double VideoProcessor::processFrame(const cv::Mat &frame, int frameIndex) {
    int idx = static_cast<int>(m_timeSpeed.size());
    double number = smartOCR(frame);
    m_timeSpeed.emplace_back(idx * m_timeInterval, number, frameIndex);
    if (number > 0) m_lastSpeed = number;
    return number;
}

std::vector<std::tuple<int, double, double>> VideoProcessor::processBatch(
    const std::vector<std::pair<cv::Mat, int>> &frames) {
    int baseIdx = static_cast<int>(m_timeSpeed.size());

    std::vector<std::future<std::tuple<int, double, double>>> futures;
    for (int i = 0; i < static_cast<int>(frames.size()); ++i) {
        double t = (baseIdx + i) * m_timeInterval;
        futures.push_back(std::async(std::launch::async,
            [this, &frames, i, t]() -> std::tuple<int, double, double> {
                double speed = smartOCR(frames[i].first);
                return {frames[i].second, t, speed};
            }));
    }

    std::vector<std::tuple<int, double, double>> results;
    for (auto &f : futures)
        results.push_back(f.get());

    for (auto &[frameIdx, time, speed] : results) {
        m_timeSpeed.emplace_back(time, speed, frameIdx);
        if (speed > 0) m_lastSpeed = speed;
    }
    return results;
}

SpeedData VideoProcessor::getSpeedData() const {
    SpeedData sd;
    if (m_timeSpeed.empty()) return sd;

    sd.distance.push_back(0);
    for (size_t i = 0; i < m_timeSpeed.size(); ++i) {
        auto &[t, spd, f] = m_timeSpeed[i];
        sd.frame.push_back(f);
        sd.speed.push_back(spd);
        sd.time_s.push_back(t);
        if (i > 0) {
            auto &[t0, v0, _] = m_timeSpeed[i - 1];
            double v0_mps = v0 / 3.6, v1_mps = spd / 3.6;
            double dt = t - t0;
            sd.distance.push_back(sd.distance.back() + (v0_mps + v1_mps) / 2.0 * dt);
        }
    }

    sd.accel.resize(sd.speed.size());
    for (int i = 0; i < static_cast<int>(sd.speed.size()); ++i) {
        // simple local slope
        int window = 5, n = static_cast<int>(sd.speed.size());
        int i0 = std::max(0, i - window), i1 = std::min(n - 1, i + window);
        double sumX = 0, sumY = 0, sumXX = 0, sumXY = 0;
        int cnt = i1 - i0 + 1;
        for (int j = i0; j <= i1; ++j) {
            sumX += sd.distance[j]; sumY += sd.speed[j];
            sumXX += sd.distance[j] * sd.distance[j];
            sumXY += sd.distance[j] * sd.speed[j];
        }
        double denom = cnt * sumXX - sumX * sumX;
        double dvdx = (std::abs(denom) < 1e-12) ? 0.0 : (cnt * sumXY - sumX * sumY) / denom;
        sd.accel[i] = sd.speed[i] * dvdx * 25.0 / 324.0;
    }
    return sd;
}

void VideoProcessor::saveCSV(const std::string &path) {
    SDAnalyzer::saveCSV(getSpeedData(), path);
}

void VideoProcessor::restart() {
    m_lastSpeed = -1.0;
    m_timeSpeed.clear();
}

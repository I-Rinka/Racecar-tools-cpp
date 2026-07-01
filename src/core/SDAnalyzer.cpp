#include "SDAnalyzer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <numeric>

static double localSlope(const std::vector<double> &x, const std::vector<double> &y,
                         int idx, int window) {
    int n = static_cast<int>(x.size());
    int i0 = std::max(0, idx - window);
    int i1 = std::min(n - 1, idx + window);
    if (i1 - i0 < 1) return 0.0;

    double sumX = 0, sumY = 0, sumXX = 0, sumXY = 0;
    int cnt = i1 - i0 + 1;
    for (int i = i0; i <= i1; ++i) {
        sumX += x[i]; sumY += y[i];
        sumXX += x[i] * x[i]; sumXY += x[i] * y[i];
    }
    double denom = cnt * sumXX - sumX * sumX;
    if (std::abs(denom) < 1e-12) return 0.0;
    return (cnt * sumXY - sumX * sumY) / denom;
}

SDAnalyzer::SDAnalyzer(const std::string &csvPath)
    : m_data(loadCSV(csvPath)) {
    buildDistanceMap();
}

SDAnalyzer::SDAnalyzer(const std::string &name, const SpeedData &data)
    : m_data(data) {
    m_data.name = name;
    if (m_data.accel.empty() && !m_data.speed.empty())
        m_data.accel = computeAccel(m_data.speed, m_data.distance);
    buildDistanceMap();
}

void SDAnalyzer::buildDistanceMap() {
    m_distMap.clear();
    for (int i = 0; i < m_data.size(); ++i) {
        double d = m_data.distance[i];
        if (m_distMap.find(d) == m_distMap.end())
            m_distMap[d] = i;
    }
}

void SDAnalyzer::adjustDistance(double step) {
    for (auto &d : m_data.distance)
        d += step;
    buildDistanceMap();
}

int SDAnalyzer::getIndexByDistance(double dist) const {
    if (m_distMap.empty()) return 0;
    auto it = m_distMap.lower_bound(dist);

    std::vector<std::map<double, int>::const_iterator> candidates;
    if (it != m_distMap.end()) candidates.push_back(it);
    if (it != m_distMap.begin()) { auto prev = std::prev(it); candidates.push_back(prev); }
    auto next = it;
    if (next != m_distMap.end()) { ++next; if (next != m_distMap.end()) candidates.push_back(next); }

    if (candidates.empty()) return 0;

    auto best = *std::min_element(candidates.begin(), candidates.end(),
        [dist](auto a, auto b) { return std::abs(a->first - dist) < std::abs(b->first - dist); });
    return best->second;
}

void SDAnalyzer::setCurrentIndexByDistance(double dist) {
    m_currentIndex = getIndexByDistance(dist);
}

void SDAnalyzer::setCurrentIndexByTime(double timeSec) {
    auto &ts = m_data.time_s;
    if (ts.empty()) { m_currentIndex = 0; return; }
    auto it = std::upper_bound(ts.begin(), ts.end(), timeSec);
    int idx = static_cast<int>(std::distance(ts.begin(), it)) - 1;
    m_currentIndex = std::clamp(idx, 0, m_data.size() - 1);
}

void SDAnalyzer::setCurrentIndexByFrame(int frameIdx) {
    auto &frames = m_data.frame;
    auto it = std::upper_bound(frames.begin(), frames.end(), frameIdx);
    int idx = static_cast<int>(std::distance(frames.begin(), it)) - 1;
    m_currentIndex = std::clamp(idx, 0, m_data.size() - 1);
}

double SDAnalyzer::getSpeed(double dist) const {
    int idx = getIndexByDistance(dist);
    return m_data.speed[idx];
}

double SDAnalyzer::getAccel() const {
    if (m_currentIndex < 0 || m_currentIndex >= m_data.size()) return 0;
    if (!m_data.accel.empty()) return m_data.accel[m_currentIndex];
    return 0;
}

int SDAnalyzer::getCurrentFrameIndex() const {
    if (m_currentIndex < 0 || m_currentIndex >= m_data.size()) return 0;
    return m_data.frame[m_currentIndex];
}

double SDAnalyzer::getCurrentDistance() const {
    if (m_currentIndex < 0 || m_currentIndex >= m_data.size()) return 0;
    return m_data.distance[m_currentIndex];
}

int SDAnalyzer::getInitialFrame() const {
    return m_data.frame.empty() ? 0 : m_data.frame[0];
}

void SDAnalyzer::incCurrentIndex() {
    if (m_currentIndex < m_data.size() - 1)
        ++m_currentIndex;
}

std::vector<double> SDAnalyzer::computeAccel(const std::vector<double> &speeds,
                                              const std::vector<double> &dists, int window) {
    std::vector<double> result(speeds.size());
    for (int i = 0; i < static_cast<int>(speeds.size()); ++i) {
        double dvdx = localSlope(dists, speeds, i, window);
        result[i] = speeds[i] * dvdx * 25.0 / 324.0;
    }
    return result;
}

SpeedData SDAnalyzer::loadCSV(const std::string &path) {
    SpeedData sd;
    // extract name from path
    auto pos = path.find_last_of("/\\");
    std::string base = (pos != std::string::npos) ? path.substr(pos + 1) : path;
    auto dot = base.find_last_of('.');
    sd.name = (dot != std::string::npos) ? base.substr(0, dot) : base;

    std::ifstream f(path);
    if (!f.is_open()) return sd;

    std::string header;
    std::getline(f, header);

    // parse header to find column indices
    std::vector<std::string> cols;
    std::istringstream hs(header);
    std::string col;
    while (std::getline(hs, col, ','))
        cols.push_back(col);

    auto findCol = [&](const std::string &name) -> int {
        for (int i = 0; i < static_cast<int>(cols.size()); ++i)
            if (cols[i] == name) return i;
        return -1;
    };

    int iFrame = findCol("frame"), iSpeed = findCol("speed");
    int iDist = findCol("distance"), iTime = findCol("time");
    int iAccel = findCol("accel");

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream ls(line);
        std::vector<double> vals;
        std::string token;
        while (std::getline(ls, token, ',')) {
            try { vals.push_back(std::stod(token)); }
            catch (...) { vals.push_back(0); }
        }

        auto get = [&](int i) -> double { return (i >= 0 && i < (int)vals.size()) ? vals[i] : 0; };
        if (iFrame >= 0) sd.frame.push_back(static_cast<int>(get(iFrame)));
        if (iSpeed >= 0) sd.speed.push_back(get(iSpeed));
        if (iDist >= 0) sd.distance.push_back(get(iDist));
        if (iTime >= 0) sd.time_s.push_back(get(iTime));
        if (iAccel >= 0) sd.accel.push_back(get(iAccel));
    }

    if (sd.accel.empty() && !sd.speed.empty())
        sd.accel = computeAccel(sd.speed, sd.distance);

    return sd;
}

void SDAnalyzer::saveCSV(const SpeedData &data, const std::string &path) {
    std::ofstream f(path);
    f << "frame,speed,distance,time,accel\n";
    for (int i = 0; i < data.size(); ++i) {
        f << data.frame[i] << ","
          << data.speed[i] << ","
          << data.distance[i] << ","
          << data.time_s[i] << ","
          << data.accel[i] << "\n";
    }
}

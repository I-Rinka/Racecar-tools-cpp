#pragma once
#include <vector>
#include <string>
#include <map>

struct SpeedData {
    std::vector<int> frame;
    std::vector<double> speed;
    std::vector<double> distance;
    std::vector<double> time_s;
    std::vector<double> accel;
    std::string name;

    int size() const { return static_cast<int>(speed.size()); }
};

class SDAnalyzer {
public:
    SDAnalyzer(const std::string &csvPath);
    SDAnalyzer(const std::string &name, const SpeedData &data);

    void adjustDistance(double step);
    int getIndexByDistance(double dist) const;
    void setCurrentIndexByDistance(double dist);
    void setCurrentIndexByFrame(int frameIdx);
    void setCurrentIndexByTime(double timeSec);
    double getSpeed(double dist) const;
    double getAccel() const;
    int getCurrentFrameIndex() const;
    double getCurrentDistance() const;
    int getInitialFrame() const;
    double getFps() const;
    void incCurrentIndex();
    void replaceData(const SpeedData &data);

    const SpeedData &data() const { return m_data; }
    const std::string &name() const { return m_data.name; }
    int currentIndex() const { return m_currentIndex; }

    static SpeedData loadCSV(const std::string &path);
    static void saveCSV(const SpeedData &data, const std::string &path);

private:
    void buildDistanceMap();
    static std::vector<double> computeAccel(const std::vector<double> &speeds,
                                            const std::vector<double> &dists, int window = 5);
    SpeedData m_data;
    std::map<double, int> m_distMap;
    int m_currentIndex = 0;
};

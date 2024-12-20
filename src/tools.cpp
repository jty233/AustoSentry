#include "tools.h"

inline double minx = 1e9, miny = 1e9, maxx = -1e9, maxy = -1e9;
inline double rate;
std::pair<int, int> trans(double x, double y)
{
    return {lround((x - minx) * rate), lround((y - miny) * rate)};
}

std::pair<double, double> invtrans(double x, double y)
{
    return {x / rate + minx, y / rate + miny};
}

void drawSquare(cv::Mat &img, int x, int y, int r)
{
    for (int i = -r; i <= r; i++)
        for (int j = -r; j <= r; j++)
            img.at<cv::Vec3b>(x + i, y + j) = {255,255,0};
}

void initPCD(pcl::PointCloud<pcl::PointXYZ>::Ptr ptr, int picSize)
{
    for (auto p : *ptr)
    {
        double x, y;
        x = p.x;
        y = p.y;
        minx = std::min(minx, x);
        maxx = std::max(maxx, x);
        miny = std::min(miny, y);
        maxy = std::max(maxy, y);
    }
    // cout << "minx " << minx << " maxx" << maxx << "\nminy" << miny << " maxy" << maxy << endl;

    rate = (picSize - 1) / std::max(maxx - minx, maxy - miny);
}


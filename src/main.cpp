#include <bits/stdc++.h>
#include "lidar.h"
#include "match.h"
#include "tools.h"
#include "route.h"
#include "control.h"
#include "config.h"

using namespace std;
using namespace cv;
int nx = 369;
int ny = 581;
double theta = -0.252242;

vector<Point> dest{
    {634,455},
    {369, 581}};

double getfps()
{
    static thread_local deque<int> q;
    auto cur = clock();
    q.push_back(cur);

    while (cur - q.front() > CLOCKS_PER_SEC)
        q.pop_front();
    if (q.size() == 1)
        return 0;
    return q.size() * CLOCKS_PER_SEC * 1. / (cur - q.front());
}

double x, y, t, score;
Eigen::Matrix4f transMat;
deque<pcl::PointXYZ> realtimeObstacle;
double locateFPS;
void locate()
{
    while (true)
    {
        auto p = getPointCloud(10000);
        transMat = match(p, score);
        mtx.lock();
        for (auto pt : p->points)
        {
            double dis = hypot(pt.x, pt.y, pt.z);
            if (dis > 2)
                continue;
            Eigen::Vector4f transPoint{pt.x, pt.y, pt.z, 1};
            transPoint = transMat * transPoint;
            realtimeObstacle.push_back({transPoint[0], transPoint[1], transPoint[2]});
        }
        while (realtimeObstacle.size() > 20000)
            realtimeObstacle.pop_front();
        mtx.unlock();
        x = transMat(0, 3);
        y = transMat(1, 3);
        t = atan2((transMat(1, 0) - transMat(0, 1)) / 2, (transMat(0, 0) + transMat(1, 1)) / 2);
        locateFPS = getfps();
        // this_thread::sleep_for(10ms);
    }
}

void endCtrl(int sig)
{
    sendCtrl(0, 0);
    exit(0);
}
double rotate = 0;
int main()
{
    signal(SIGINT, endCtrl);
    // signal(SIGTERM, endCtrl);
    // signal(SIGSEGV, endCtrl);
    initLidar("../mid360_config.json");
    initRoute("../obstacle.png");
    initCtrl();
    initMatch("../scans.pcd", nx, ny, theta);
    cout << "finish init" << endl;

    cout << fixed << setprecision(4);
    auto startTime = chrono::steady_clock::now();
    // resetPointCloud();

    auto curDest = dest.begin();
    thread _{locate};
    while (true)
    {
        // startTime += 100ms;
        // this_thread::sleep_until(startTime);

        cout << score << '\t' << x << '\t' << y << '\t' << t << '\t' << flush;

        Point start;
        tie(start.x, start.y) = trans(x, y);

        auto endPoint = *curDest;
        double dt = route_planning(start, endPoint, realtimeObstacle);
        auto realEnd = invtrans(endPoint.x, endPoint.y);
        double dis = hypot(realEnd.first - x, realEnd.second - y);

        double ddt = dt - t;
        if (abs(ddt) > M_PI)
        {
            if (ddt > 0)
                ddt = 2 * M_PI - ddt;
            else
                ddt = ddt + 2 * M_PI;
        }

        cout << dis << '\t' << ddt << '\t' << getfps() << '\t' << locateFPS << endl;
        if (dis < .5)
        {
            curDest++;
            if (curDest == dest.end())
                curDest = dest.begin();
        }

        // continue;
        static int lstTime = clock();
        int curTime = clock();
        double deltaTime = (curTime - lstTime) * 1. / CLOCKS_PER_SEC;
        lstTime = curTime;

        if (score > .1 || isnan(ddt))
        {
            sendCtrl(deltaTime / 2 * M_PI, 0);
            continue;
        }

        sendCtrl((ddt) * deltaTime / 2, 0);
        if (abs(ddt) < .5)
            sendCtrl(0, 0.5);

        // this_thread::sleep_for(1s);
    }
}

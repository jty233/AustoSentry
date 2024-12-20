#include "match.h"
#include "tools.h"
#include "config.h"
#include <iostream>
#include <thread>


using namespace std::chrono_literals;

Eigen::Matrix4f guess;
pcl::VoxelGrid<pcl::PointXYZ> voxel_filter;
pcl::NormalDistributionsTransform<pcl::PointXYZ, pcl::PointXYZ> ndt;
pcl::PointCloud<pcl::PointXYZ>::Ptr target_cloud(new pcl::PointCloud<pcl::PointXYZ>);
pcl::PointCloud<pcl::PointXYZ>::Ptr filtered_cloud(new pcl::PointCloud<pcl::PointXYZ>);
pcl::PointCloud<pcl::PointXYZ>::Ptr output_cloud(new pcl::PointCloud<pcl::PointXYZ>);
double scale = 0.2;

std::tuple<float, float, float> match(pcl::PointCloud<pcl::PointXYZ>::Ptr &input_cloud, double& res)
{
	voxel_filter.setInputCloud(input_cloud);
	voxel_filter.filter(*filtered_cloud);

	ndt.setInputSource(filtered_cloud);

	auto t1 = clock();
	ndt.align(*output_cloud, guess);
	std::cout << "match use time:" << (clock() - t1) * 1. / CLOCKS_PER_SEC << "s " << " score: " << ndt.getFitnessScore() << '\t';

	auto mat = ndt.getFinalTransformation();

	static Eigen::Matrix4f finalmat = Eigen::Matrix4f::Identity();
	// if (ndt.getFitnessScore() <= 0.1)
	{
		finalmat = mat;
	}
	// guess(0, 3) = finalmat(0, 3);
	// guess(1, 3) = finalmat(1, 3);
	guess = finalmat;

	return {finalmat(0, 3), finalmat(1, 3), atan2(finalmat(1, 0), finalmat(0, 0))};
}

void initMatch(std::string globalPointCloudPath, double approX, double approY, double approTheta)
{
	if (pcl::io::loadPCDFile<pcl::PointXYZ>(globalPointCloudPath, *target_cloud) == -1)
	{
		PCL_ERROR("Couldn't read global point cloud\n");
		return;
	}
	initPCD(target_cloud, picSize);
	std::tie(approX, approY) = invtrans(approX, approY);
	std::cout << "init x & y" << approX << ' ' << approY << std::endl;

	voxel_filter.setLeafSize(scale, scale, scale);
	voxel_filter.setInputCloud(target_cloud);
	voxel_filter.filter(*target_cloud);
	// scale = .2;
	// voxel_filter.setLeafSize(scale, scale, scale);

	ndt.setTransformationEpsilon(1e-6);
	ndt.setStepSize(0.1);
	ndt.setResolution(0.5);
	ndt.setMaximumIterations(60);
	ndt.setInputTarget(target_cloud);

	Eigen::AngleAxisf init_rotation(approTheta, Eigen::Vector3f::UnitZ());
	Eigen::Translation3f init_translation(approX, approY, 0);
	guess = (init_translation * init_rotation).matrix();

}
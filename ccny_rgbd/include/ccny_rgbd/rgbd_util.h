#ifndef CCNY_RGBD_RGBD_UTIL_H
#define CCNY_RGBD_RGBD_UTIL_H

#include <ros/ros.h>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <tf/transform_datatypes.h>
#include <pcl/filters/voxel_grid.h>
#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/features2d.hpp> // FIXME: only needed for SURF feature detection/description

#include "ccny_rgbd/types.h"

namespace ccny_rgbd
{

// **** util functions **************************************

/* given a transform, calculates the linear and angular 
 * distance between it and identity
 */
void getTfDifference(const tf::Transform& motion, double& dist, double& angle);

/* given two tf::transforms, calculates the linear and angular 
 * distance between them
 */
void getTfDifference(const tf::Transform& a, const tf::Transform& b, double& dist, double& angle);

/* given a tf::Transfom (possibly computed as a difference between two transforms)
 * checks if either its angular or linar component exedds a threshold
 */
bool tfGreaterThan(const tf::Transform& a, double dist, double angle);

/* converts and Eigen transform to a tf::Transform
 */
tf::Transform tfFromEigen(const Eigen::Matrix4f& E);

/* composes a tf::Transform from an Eigen 3x3 rotation matrix and a 3x1 translation vector
 */
tf::Transform tfFromEigenRt(
  const Matrix3f& R,
  const Vector3f& t);

/* composes a tf::Transform from an cv::Mat 3x3 rotation matrix and a 3x1 translation vector
 */
tf::Transform tfFromCVRt(
  const cv::Mat& R,
  const cv::Mat& t);

/* converts and tf::Transform transform to an Eigen transform
 */
Eigen::Matrix4f eigenFromTf(const tf::Transform& tf);

/* decomposes a tf into an Eigen 3x3 rotation matrix
 * and Eigen 3x1 rotation vector */
void tfToEigenRt(
  const tf::Transform& tf, 
  Matrix3f& R, 
  Vector3f& t);

/* decomposes a tf::Transform into a 3x3 OpenCV rotation matrix
 * and a 3x1 OpenCV translation vector
 */
void tfToOpenCVRt(
  const tf::Transform& transform,
  cv::Mat& R,
  cv::Mat& t);

// TODO: description
void openCVRtToTf(
  const cv::Mat& R,
  const cv::Mat& t,
  tf::Transform& transform);

//transforms a 3x3 Opencv matrix to a 3x3 Eigen matrix
void openCVRToEigenR(
  const cv::Mat& R,
  Matrix3f& R_eigen);

/* decomposes a tf::Transform into x, y, z, roll, pitch, yaw
 * TODO: rename to tfToXYZRPY
 */
void getXYZRPY(
  const tf::Transform& t,
  double& x,    double& y,     double& z,
  double& roll, double& pitch, double& yaw);

// TODO: comment
void convertCameraInfoToMats(
  const CameraInfoMsg::ConstPtr camera_info_msg,
  cv::Mat& intr,
  cv::Mat& dist);

// TODO: comment
void convertMatToCameraInfo(
  const cv::Mat& intr,
  CameraInfoMsg& camera_info_msg);

/* returns the duration, in ms, from a given time
 */
double getMsDuration(const ros::WallTime& start);

/* filters out a vector of means given a mask of valid 
 * entries
 */
void removeInvalidMeans(
  const Vector3fVector& means,
  const BoolVector& valid,
  Vector3fVector& means_f);

/* filters out a vector of means and a vector of 
 * covariances given a mask of valid entries
 */
void removeInvalidDistributions(
  const Vector3fVector& means,
  const Matrix3fVector& covariances,
  const BoolVector& valid,
  Vector3fVector& means_f,
  Matrix3fVector& covariances_f);

/* transforms a vector of means
 */
void transformMeans(
  Vector3fVector& means,
  const tf::Transform& transform);

/* transforms a vector of means and covariances
 */
void transformDistributions(
  Vector3fVector& means,
  Matrix3fVector& covariances,
  const tf::Transform& transform);

/* creates a pcl point cloud form a vector
 * of eigen matrix means
 */
void pointCloudFromMeans(
  const Vector3fVector& means,
  PointCloudFeature& cloud);

/* converts an Eigen 3x3 matrix into an OpenCV 3x3 matrix
 */
void cv3x3FromEigen(const Matrix3f& emat, cv::Mat& Q);

/* generates an RGB and depth images from the projection of a point cloud
 */
void projectCloudToImage(const PointCloudT& cloud,
                         const Matrix3f& rmat,
                         const Vector3f& tvec,
                         const Matrix3f& intrinsic,
                         int width,
                         int height,
                         cv::Mat& rgb_img,
                         cv::Mat& depth_img);

/* Fills the holes due to the sparse model in the projected image
 * N.B. mask size has to be a odd number 
 */
void holeFilling(const cv::Mat& rgb_img,
                 const cv::Mat& depth_img,
                 uint mask_size,
                 cv::Mat& filled_rgb_img,
                 cv::Mat& filled_depth_img);

void holeFilling2(const cv::Mat& rgb_img,
                 const cv::Mat& depth_img,
                 uint mask_size,
                 cv::Mat& filled_rgb_img,
                 cv::Mat& filled_depth_img);

} // namespace ccny_rgbd

#endif // CCNY_RGBD_RGBD_UTIL_H

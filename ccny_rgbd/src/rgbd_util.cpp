#include "ccny_rgbd/rgbd_util.h"

namespace ccny_rgbd
{

void getTfDifference(const tf::Transform& motion, double& dist, double& angle)
{
  dist = motion.getOrigin().length();
  double trace = motion.getBasis()[0][0] + motion.getBasis()[1][1] + motion.getBasis()[2][2];
  angle = acos(std::min(1.0, std::max(-1.0, (trace - 1.0)/2.0)));
}

void getTfDifference(const tf::Transform& a, const tf::Transform& b, double& dist, double& angle)
{
  tf::Transform motion = a.inverse() * b;
  getTfDifference(motion, dist, angle);
}

tf::Transform tfFromEigen(const Eigen::Matrix4f& E)
{
  tf::Matrix3x3 btm;
  btm.setValue(E(0,0),E(0,1),E(0,2),
            E(1,0),E(1,1),E(1,2),
            E(2,0),E(2,1),E(2,2));
  tf::Transform ret;
  ret.setOrigin(btVector3(E(0,3),E(1,3),E(2,3)));
  ret.setBasis(btm);
  return ret;
}

tf::Transform tfFromEigenRt(
  const Matrix3f& R,
  const Vector3f& t)
{
  tf::Matrix3x3 btm;
  btm.setValue(R(0,0),R(0,1),R(0,2),
            R(1,0),R(1,1),R(1,2),
            R(2,0),R(2,1),R(2,2));
  tf::Transform ret;
  ret.setOrigin(btVector3(t(0,0),t(1,0),t(2,0)));
  ret.setBasis(btm);
  return ret;
}

tf::Transform tfFromCVRt(
  const cv::Mat& R,
  const cv::Mat& t)
{
  tf::Transform transform;

  openCVRtToTf(R, t, transform);
  return transform;
}

Eigen::Matrix4f eigenFromTf(const tf::Transform& tf)
{
   Eigen::Matrix4f out_mat;

   double mv[12];
   tf.getBasis().getOpenGLSubMatrix(mv);

   tf::Vector3 origin = tf.getOrigin();

   out_mat (0, 0) = mv[0]; out_mat (0, 1) = mv[4]; out_mat (0, 2) = mv[8];
   out_mat (1, 0) = mv[1]; out_mat (1, 1) = mv[5]; out_mat (1, 2) = mv[9];
   out_mat (2, 0) = mv[2]; out_mat (2, 1) = mv[6]; out_mat (2, 2) = mv[10];

   out_mat (3, 0) = out_mat (3, 1) = out_mat (3, 2) = 0; out_mat (3, 3) = 1;
   out_mat (0, 3) = origin.x ();
   out_mat (1, 3) = origin.y ();
   out_mat (2, 3) = origin.z ();

   return out_mat;
}

void getXYZRPY(
  const tf::Transform& t,
  double& x,    double& y,     double& z,
  double& roll, double& pitch, double& yaw)
{
  x = t.getOrigin().getX();
  y = t.getOrigin().getY();
  z = t.getOrigin().getZ();

  tf::Matrix3x3  m(t.getRotation());
  m.getRPY(roll, pitch, yaw);
}

bool tfGreaterThan(const tf::Transform& tf, double dist, double angle)
{
  //TODO: can be optimized for squared length
  double d = tf.getOrigin().length();
  
  if (d >= dist) return true;

  double trace = tf.getBasis()[0][0] + tf.getBasis()[1][1] + tf.getBasis()[2][2];
  double a = acos(std::min(1.0, std::max(-1.0, (trace - 1.0)/2.0)));

  if (a > angle) return true;
  
  return false;
}

double getMsDuration(const ros::WallTime& start)
{
  return (ros::WallTime::now() - start).toSec() * 1000.0;
}

void removeInvalidMeans(
  const Vector3fVector& means,
  const BoolVector& valid,
  Vector3fVector& means_f)
{
  unsigned int size = valid.size(); 
  for(unsigned int i = 0; i < size; ++i)
  {
    if (valid[i])
    {
      const Vector3f& mean = means[i];
      means_f.push_back(mean);
    }
  }
}

void removeInvalidDistributions(
  const Vector3fVector& means,
  const Matrix3fVector& covariances,
  const BoolVector& valid,
  Vector3fVector& means_f,
  Matrix3fVector& covariances_f)
{
  unsigned int size = valid.size(); 
  for(unsigned int i = 0; i < size; ++i)
  {
    if (valid[i])
    {
      const Vector3f& mean = means[i];
      const Matrix3f& cov  = covariances[i];

      means_f.push_back(mean);
      covariances_f.push_back(cov);
    }
  }
}

void tfToEigenRt(
  const tf::Transform& tf,
  Matrix3f& R,
  Vector3f& t)
{
   double mv[12];
   tf.getBasis().getOpenGLSubMatrix(mv);

   tf::Vector3 origin = tf.getOrigin();

   R(0, 0) = mv[0]; R(0, 1) = mv[4]; R(0, 2) = mv[8];
   R(1, 0) = mv[1]; R(1, 1) = mv[5]; R(1, 2) = mv[9];
   R(2, 0) = mv[2]; R(2, 1) = mv[6]; R(2, 2) = mv[10];

   t(0, 0) = origin.x();
   t(1, 0) = origin.y();
   t(2, 0) = origin.z();
}

void tfToOpenCVRt(
  const tf::Transform& transform,
  cv::Mat& R,
  cv::Mat& t)
{
  // extract translation
  tf::Vector3 translation_tf = transform.getOrigin();
  t = cv::Mat(3, 1, CV_64F);
  t.at<double>(0,0) = translation_tf.getX();
  t.at<double>(1,0) = translation_tf.getY();
  t.at<double>(2,0) = translation_tf.getZ();

  // extract rotation
  tf::Matrix3x3 rotation_tf(transform.getRotation());
  R = cv::Mat(3, 3, CV_64F);
  for(int i = 0; i < 3; ++i)
    for(int j = 0; j < 3; ++j)
      R.at<double>(i,j) = rotation_tf[i][j];
}

void openCVRToEigenR(
  const cv::Mat& R,
  Matrix3f& R_eigen)
{
  for(int i = 0; i < 3; ++i)
    for(int j = 0; j < 3; ++j)
      R_eigen(i,j) =  R.at<double>(i,j);
}

void openCVRtToTf(
  const cv::Mat& R,
  const cv::Mat& t,
  tf::Transform& transform)
{
  tf::Vector3 translation_tf(
    t.at<double>(0,0),
    t.at<double>(1,0),
    t.at<double>(2,0));

  tf::Matrix3x3 rotation_tf;
  for(int i = 0; i < 3; ++i)
  for(int j = 0; j < 3; ++j)     
    rotation_tf[j][i] = R.at<double>(j,i);

  transform.setOrigin(translation_tf);
  transform.setBasis(rotation_tf);
}

void convertCameraInfoToMats(
  const CameraInfoMsg::ConstPtr camera_info_msg,
  cv::Mat& intr,
  cv::Mat& dist)
{
  // set intrinsic matrix from K vector
  intr = cv::Mat(3, 3, CV_64FC1);
  for (int idx = 0; idx < 9; ++idx)
  {
    int i = idx % 3;
    int j = idx / 3;
    intr.at<double>(j, i) = camera_info_msg->K[idx];
  }
  
  // set distortion matrix from D vector
  int d_size = camera_info_msg->D.size();
  dist = cv::Mat(1, d_size, CV_64FC1);
  for (int idx = 0; idx < d_size; ++idx)
  {
    dist.at<double>(0, idx) = camera_info_msg->D[idx];   
  }
}

void convertMatToCameraInfo(
  const cv::Mat& intr,
  CameraInfoMsg& camera_info_msg)
{
  // set D matrix to 0
  camera_info_msg.D.resize(5);
  std::fill(camera_info_msg.D.begin(), camera_info_msg.D.end(), 0.0);
  
  // set K matrix to optimal new camera matrix
  for (int i = 0; i < 3; ++i)
  for (int j = 0; j < 3; ++j)
    camera_info_msg.K[j*3 + i] = intr.at<double>(j,i);
  
  // set R matrix to identity
  std::fill(camera_info_msg.R.begin(), camera_info_msg.R.end(), 0.0);  
  camera_info_msg.R[0*3 + 0] = 1.0;
  camera_info_msg.R[1*3 + 1] = 1.0;
  camera_info_msg.R[2*3 + 2] = 1.0;
    
  //set P matrix to K
  std::fill(camera_info_msg.P.begin(), camera_info_msg.P.end(), 0.0);  
  for (int i = 0; i < 3; ++i)
  for (int j = 0; j < 3; ++j)
    camera_info_msg.P[j*4 + i] = intr.at<double>(j,i);
}

void transformMeans(
  Vector3fVector& means,
  const tf::Transform& transform)
{
  Matrix3f R;
  Vector3f t;
  tfToEigenRt(transform, R, t);
  
  unsigned int size = means.size(); 
  for(unsigned int i = 0; i < size; ++i)
  {
    Vector3f& m = means[i];
    m = R * m + t;
  }  
}

void transformDistributions(
  Vector3fVector& means,
  Matrix3fVector& covariances,
  const tf::Transform& transform)
{
  Matrix3f R;
  Vector3f t;
  tfToEigenRt(transform, R, t);
  Matrix3f R_T = R.transpose();
  
  unsigned int size = means.size(); 
  for(unsigned int i = 0; i < size; ++i)
  {
    Vector3f& m = means[i];
    Matrix3f& c = covariances[i];
    m = R * m + t;
    c = R * c * R_T;
  }
}

void pointCloudFromMeans(
  const Vector3fVector& means,
  PointCloudFeature& cloud)
{
  unsigned int size = means.size(); 
  cloud.points.resize(size);
  for(unsigned int i = 0; i < size; ++i)
  {
    const Vector3f& m = means[i];
    PointFeature& p = cloud.points[i];

    p.x = m(0,0);
    p.y = m(1,0);
    p.z = m(2,0);
  }
  
  cloud.height = 1;
  cloud.width = size;
  cloud.is_dense = true;
}

void cv3x3FromEigen(const Matrix3f& emat, cv::Mat& Q)
{
  Q = cv::Mat::zeros(3, 3, CV_64F);

  for(int i = 0; i < 3; ++i)
    for(int j = 0; j < 3; ++j)
      Q.at<double>(i,j) = emat(i,j);
}

void projectCloudToImage(const PointCloudT& cloud,
                         const Matrix3f& rmat,
                         const Vector3f& tvec,
                         const Matrix3f& intrinsic,
                         int width,
                         int height,
                         cv::Mat& rgb_img,
                         cv::Mat& depth_img
                         )
{
  
  rgb_img   = cv::Mat::zeros(height, width, CV_8UC3);
  depth_img = cv::Mat::zeros(height, width, CV_16UC1);

  for (uint i=0; i<cloud.points.size(); ++i)
  {
    // convert from pcl PointT to Eigen Vector3f
    PointT point = cloud.points[i];
    Vector3f p_world;
    if (isnan(point.x) || isnan(point.y) || isnan(point.z) ) continue;

    p_world(0,0) = point.x;
    p_world(1,0) = point.y;
    p_world(2,0) = point.z;
    

    // transforms into the camera frame  
    Vector3f p_cam = rmat * p_world + tvec; 
    double depth = p_cam(2,0) * 1000.0;       //depth in millimiter
    
    if (depth <= 0) continue;

    //projection into the imiage plane   
    Vector3f p_proj = intrinsic * p_cam;                    
    double z_proj = p_proj(2,0);

    int u = (p_proj(0,0))/z_proj;
    int v = (p_proj(1,0))/z_proj;
    
    //takes only the visible points  
    if ((u<width) && (u>=0) && (v<height) && (v>=0)) 
    {
      cv::Vec3b color_rgb;
      color_rgb[0] = point.b;  
      color_rgb[1] = point.g;
      color_rgb[2] = point.r;
          
      if (depth_img.at<uint16_t>(v,u) == 0)
      {
        rgb_img.at<cv::Vec3b>(v,u) = color_rgb;           
        depth_img.at<uint16_t>(v,u) = depth; 
      }
      else if  (depth > 0 && depth < depth_img.at<uint16_t>(v,u))
      {
        depth_img.at<uint16_t>(v,u) = depth;
        rgb_img.at<cv::Vec3b>(v,u) = color_rgb;
      }
    }
  }   
}

void holeFilling(const cv::Mat& rgb_img,
                 const cv::Mat& depth_img,
                 uint mask_size,
                 cv::Mat& filled_rgb_img,
                 cv::Mat& filled_depth_img)

{
  uint w = (mask_size-1)/2;

  filled_rgb_img   = cv::Mat::zeros(rgb_img.rows, rgb_img.cols, CV_8UC3);
  filled_depth_img = cv::Mat::zeros(depth_img.rows, depth_img.cols, CV_16UC1);
  
  for (uint u=0; u < depth_img.cols; ++u)
    for (uint v=0; v < depth_img.rows; ++v)
    {
      if (depth_img.at<uint16_t>(v,u) == 0)
      {
        double count = 0;
        double depth_sum = 0;
        double rgb_sum_b = 0; 
        double rgb_sum_g = 0;
        double rgb_sum_r = 0;

        for (uint uu = u - w; uu <= u+w; ++uu)
        for (uint vv = v - w; vv <= v+w; ++vv)
        {
          if (uu < 0 || uu >= depth_img.cols || vv < 0 || vv >= depth_img.rows ) continue;

          uint16_t neighbor_depth  = depth_img.at<uint16_t>(vv, uu);
          cv::Vec3b neighbor_color = rgb_img.at<cv::Vec3b>(vv, uu);

          if (neighbor_depth != 0)
          {
            double neighbor_color_b = (double)neighbor_color[0];
            double neighbor_color_g = (double)neighbor_color[1];
            double neighbor_color_r = (double)neighbor_color[2];

            depth_sum = depth_sum + neighbor_depth;
            rgb_sum_b = rgb_sum_b + neighbor_color_b;
            rgb_sum_g = rgb_sum_g + neighbor_color_g;
            rgb_sum_r = rgb_sum_r + neighbor_color_r;
            ++count;
          }
        }
        if (count != 0)
        { 
          filled_depth_img.at<uint16_t>(v,u) = depth_sum/count;
          cv::Vec3b color_rgb;
          color_rgb[0] = (uint8_t)(rgb_sum_b/count);  
          color_rgb[1] = (uint8_t)(rgb_sum_g/count);
          color_rgb[2] = (uint8_t)(rgb_sum_r/count);
          filled_rgb_img.at<cv::Vec3b>(v,u) = color_rgb;
        }
      }
      else 
      {
        filled_depth_img.at<uint16_t>(v,u) = depth_img.at<uint16_t>(v,u);
        filled_rgb_img.at<cv::Vec3b>(v,u)  = rgb_img.at<cv::Vec3b>(v,u); 
      }
    }
   
}

void holeFilling2(const cv::Mat& rgb_img,
                 const cv::Mat& depth_img,
                 uint mask_size,
                 cv::Mat& filled_rgb_img,
                 cv::Mat& filled_depth_img)

{
  uint w = (mask_size-1)/2;

  filled_rgb_img   = cv::Mat::zeros(rgb_img.rows, rgb_img.cols, CV_8UC3);
  filled_depth_img = cv::Mat::zeros(depth_img.rows, depth_img.cols, CV_16UC1);
  
  for (uint u=0; u < depth_img.cols; ++u)
    for (uint v=0; v < depth_img.rows; ++v)
    {
      if (depth_img.at<uint16_t>(v,u) == 0)
      {
        
        double min_depth = 0;
        for (uint uu = u - w; uu <= u+w; ++uu)
        for (uint vv = v - w; vv <= v+w; ++vv)
        {
          if (uu < 0 || uu >= depth_img.cols || vv < 0 || vv >= depth_img.rows ) continue;
          
          uint16_t neighbor_depth  = depth_img.at<uint16_t>(vv, uu);
          cv::Vec3b neighbor_color = rgb_img.at<cv::Vec3b>(vv, uu);

          if (neighbor_depth != 0 )
          {
            if (min_depth == 0 || neighbor_depth < min_depth )
            {
              min_depth = neighbor_depth;   
              filled_depth_img.at<uint16_t>(v,u) =  min_depth;  
              filled_rgb_img.at<cv::Vec3b>(v,u)  =  rgb_img.at<cv::Vec3b>(vv,uu);       
            }
          }
        }
      }  
      else 
      {
        filled_depth_img.at<uint16_t>(v,u) = depth_img.at<uint16_t>(v,u);
        filled_rgb_img.at<cv::Vec3b>(v,u)  = rgb_img.at<cv::Vec3b>(v,u); 
      }
    }
}

void tfFromImagePair(
  const cv::Mat& current_img,
  const cv::Mat& next_img,
  const cv::Mat& current_depth_img,
  const Matrix3f& intrinsic_matrix,
  tf::Transform& transform,
  double max_descriptor_space_distance,
  std::string feature_detection_alg,
  std::string feature_descriptor_alg,
  int number_of_iterations,
  float reprojection_error,
  int min_inliers_count,
  bool draw_matches,
  bool profile
  )
{
  ros::WallTime start_feature_matching = ros::WallTime::now();

  // Mask next image with depth img as mask (takes care of wholes where information is missing)
  cv::Mat current_img_mask;
  current_depth_img.convertTo(current_img_mask, CV_8U);
  if(draw_matches)
  {
    cv::namedWindow("Virtual Image", CV_WINDOW_KEEPRATIO);
    cv::imshow("Virtual Image", current_img);
    cv::namedWindow("Monocular Image", CV_WINDOW_KEEPRATIO);
    cv::imshow("Monocular Image", next_img);
    cv::namedWindow("Mask", CV_WINDOW_KEEPRATIO);
    cv::imshow("Mask", current_img_mask);
    cv::waitKey(1);
  }

  cv::Ptr<cv::FeatureDetector> feature_detector;

  if (feature_detection_alg == "ORB")
  {
    // ORB features
    int nfeatures=5;
    float scaleFactor=1.2f;
    int nlevels=8;
    int edgeThreshold=31;
    int firstLevel=0;
    int WTA_K=2;
    int scoreType=cv::ORB::HARRIS_SCORE;
    int patchSize=31;
    feature_detector = new cv::ORB(nfeatures, scaleFactor, nlevels, edgeThreshold, firstLevel,
                                   WTA_K, scoreType, patchSize); // Hessian threshold
  }
  else if (feature_detection_alg == "SURF")
  {
    // Construction of the SURF feature detector
    double hessian_thresh = 32;
    feature_detector = new cv::SURF(hessian_thresh); // Hessian threshold
  }
  else if (feature_detection_alg == "FAST")
  {
    int threshold=10;
    bool nonmaxSuppression=true;
    feature_detector = new cv::FastFeatureDetector(threshold, nonmaxSuppression);
  }
  else if (feature_detection_alg == "STAR")
  {
    int maxSize=45;
    int responseThreshold=30;
    int lineThresholdProjected=10;
    int lineThresholdBinarized=8;
    int suppressNonmaxSize=5;
    feature_detector = new cv::StarFeatureDetector(maxSize, responseThreshold,
                                                   lineThresholdProjected, lineThresholdBinarized, suppressNonmaxSize);
  }
  else if (feature_detection_alg == "MSER")
  {
    // TODO: check parameters
    int delta = 3;
    int min_area = 25;
    int max_area = 100;
    float max_variation = 40;
    float min_diversity = 10;
    int max_evolution = 4;
    double area_threshold = 64;
    double min_margin = 2;
    int edge_blur_size = 6;
    feature_detector = new cv::MSER(delta, min_area, max_area, max_variation, min_diversity,
                                    max_evolution, area_threshold, min_margin, edge_blur_size);
  }
  else
  {
    feature_detection_alg == "GFT";
    int n_features = 200;
    int grid_cells = 1;
    feature_detector = new cv::GoodFeaturesToTrackDetector(
        n_features, // maximum number of corners to be returned
        0.10,  // quality level
        10); // minimum allowed distance between points
    // point detection using FeatureDetector method

    cv::GridAdaptedFeatureDetector * grid_detector; // FIXME: bring outside the "else" case
    grid_detector = new cv::GridAdaptedFeatureDetector(
        feature_detector, n_features, grid_cells, grid_cells);
  }

  // vector of keypoints
  std::vector<std::vector<cv::KeyPoint> > keypoints_current_vector;
  std::vector<cv::KeyPoint> keypoints_current;
  std::vector<cv::KeyPoint> keypoints_next;

  feature_detector->detect(current_img, keypoints_current,current_img_mask);
  feature_detector->detect(next_img, keypoints_next);

  // Visualize keypoints
  /*
  if(draw_matches)
  {
    cv::Mat keypt_img_current;
    cv::drawKeypoints(current_img, keypoints_current, keypt_img_current);
    cv::namedWindow("Current Features");
    cv::imshow("Current Features", keypt_img_current);
    cv::Mat keypt_img_next;
    cv::drawKeypoints(next_img, keypoints_next, keypt_img_next);
    cv::namedWindow("Next Features");
    cv::imshow("Next Features", keypt_img_next);
    cv::waitKey(0);
  }
   */

  /*
  int radius_mask = 10;
  std::vector<cv::Mat> keypoints_masks;
  std::vector<cv::Mat> current_img_vector;

  for(int k=0; k<keypoints_next.size();k++)
  {
    cv::Mat mask = cv::Mat::zeros(next_img.rows, next_img.cols, CV_8UC1);
    cv::Point keypoint_position = keypoints_next[k].pt;
    cv::circle(mask, keypoint_position, radius_mask, cv::Scalar(255), -1);
    keypoints_masks.push_back(mask);
//    if(draw_matches)
//    {
//      cv::namedWindow("Masks", CV_WINDOW_KEEPRATIO);
//      cv::imshow("Masks", mask);
//      cv::waitKey(50);
//    }

    current_img_vector.push_back(current_img);
  }
  feature_detector->detect(current_img_vector, keypoints_current_vector, keypoints_masks);
  */

  std::cout << "Number of feature points (Reference): " << keypoints_current.size() << std::endl;
  std::cout << "Number of feature points (Virtual): " << keypoints_next.size() << std::endl;


  cv::Ptr<cv::DescriptorExtractor> descriptor_extractor;
  int norm_type; ///< normal distance type for matching (in descriptor space)

  // TODO: it should be done outside to allow for parameterization (and pass just the pointer to the detector)
  if (feature_descriptor_alg == "SURF")
  {
    ROS_INFO("Descriptor: SURF");
    norm_type = cv::NORM_L2;
    descriptor_extractor = new cv::SurfDescriptorExtractor();
  }
  else if (feature_descriptor_alg == "BRIEF")
  {
    norm_type = cv::NORM_HAMMING;
    descriptor_extractor = new cv::BriefDescriptorExtractor();
  }
  else if (feature_descriptor_alg == "FREAK")
  {
    norm_type = cv::NORM_HAMMING;
    //      norm_type = cv::NORM_L2;
    bool orientationNormalized = true;
    bool scaleNormalized = true;
    float patternScale = 22.0f;
    int nOctaves = 4;
    descriptor_extractor = new cv::FREAK(orientationNormalized, scaleNormalized, patternScale, nOctaves);
  }
  else // use ORB
  {
    // FIXME: pay attention that with ORB, since it uses Hamming distances, the FLANN matcher will not work
    // Use the BruteForce matcher with ORB!!!

    feature_descriptor_alg = "ORB";
    norm_type = cv::NORM_HAMMING;
    descriptor_extractor = new cv::OrbDescriptorExtractor();
  }

//  std::vector<cv::Mat> descriptors_current_vector;
  cv::Mat descriptors_current;
  cv::Mat descriptors_next;


//  descriptor_extractor->compute(current_img_vector, keypoints_current_vector, descriptors_current_vector);
  descriptor_extractor->compute(current_img, keypoints_current, descriptors_current);
  descriptor_extractor->compute(next_img, keypoints_next, descriptors_next);

  std::cout << "Current image's descriptor matrix size: " << descriptors_current.rows << " by " << descriptors_current.cols << std::endl;
  std::cout << "Next image's descriptor matrix size: " << descriptors_next.rows << " by " << descriptors_next.cols << std::endl;

  /*
  // Construction of the matcher
  cv::BFMatcher matcher_brute_force(norm_type, true);
//  matcher_brute_force.add(descriptors_current_vector); // Because we will be using masks

  // Match the two image descriptors
  std::vector<std::vector<cv::DMatch> > matches_radius;
  std::vector<std::vector<cv::DMatch> > matches_knn;

  // TODO: parametrize:
  float max_distance = max_descriptor_space_distance;
//  float max_distance = 2; // SURF
  bool use_radius_match = true;

  matcher_brute_force.radiusMatch(descriptors_next, descriptors_current, matches_radius, max_distance); // Match search within radius
  // Using masks
//  matcher_brute_force.radiusMatch(descriptors_next, matches_radius, max_distance, keypoints_masks); // Match search within radius

  std::cout << "Radius Matches: " << matches_radius.size() <<  std::endl;
//    int number_of_matches = matches_radius.size();
//    for(int m=0; m<number_of_matches; m++)
//    {
//      ROS_INFO( "Match[%d]: imgIdx = %d, queryIdx = %d, trainIdx = %d, distance = %f",
//                m, matches_radius[m].imgIdx, matches_radius[i][m].queryIdx, matches_radius[i][m].trainIdx, matches_radius[i][m].distance);
//      std::cout << "\tPoint at Query: " << keypointsLL[0][matches_radius[i][m].queryIdx].pt<< std::endl;
//      std::cout << "\tPoint at Train: " << keypointsRR[0][matches_radius[i][m].trainIdx].pt<< std::endl;
//    }

    if(draw_matches)
    {
      cv::Mat current_img_copy = current_img.clone();
      cv::Mat next_img_copy = next_img.clone();

      cv::Mat matches_result_img;
      cv::drawMatches(current_img_copy, keypoints_current, // 1st image and its keypoints
                      next_img_copy, keypoints_next, // 2nd image and its keypoints
                      matches_radius, // the matches
                      matches_result_img // the image produced
                     ); // color of the lines
      cv::namedWindow("Matches radius", CV_WINDOW_KEEPRATIO);
      cv::imshow("Matches radius", matches_result_img);
    }

*/


    /*// KNN
    matcher_brute_force.knnMatch(descriptors_current, descriptors_next, matches_knn, 1); // Match search within radius

    std::cout << "Knn Matches: " << matches_knn.size() << std::endl;
  //    int number_of_matches = matches_radius.size();
  //    for(int m=0; m<number_of_matches; m++)
  //    {
  //      ROS_INFO( "Match[%d]: imgIdx = %d, queryIdx = %d, trainIdx = %d, distance = %f",
  //                m, matches_radius[m].imgIdx, matches_radius[i][m].queryIdx, matches_radius[i][m].trainIdx, matches_radius[i][m].distance);
  //      std::cout << "\tPoint at Query: " << keypointsLL[0][matches_radius[i][m].queryIdx].pt<< std::endl;
  //      std::cout << "\tPoint at Train: " << keypointsRR[0][matches_radius[i][m].trainIdx].pt<< std::endl;
  //    }

      if(draw_matches)
      {
        cv::Mat current_img_copy = current_img.clone();
        cv::Mat next_img_copy = next_img.clone();

        cv::Mat matches_result_img;
        cv::drawMatches(current_img_copy, keypoints_current, // 1st image and its keypoints
                        next_img_copy, keypoints_next, // 2nd image and its keypoints
                        matches_knn, // the matches
                        matches_result_img // the image produced
                       ); // color of the lines
        cv::namedWindow("Matches knn", CV_WINDOW_KEEPRATIO);
        cv::imshow("Matches knn", matches_result_img);
      }

*/

  cv::FlannBasedMatcher matcher_flann;
  std::vector<cv::DMatch>  matches_flann;
  // **** build candidate matches ***********************************
  matcher_flann.match(descriptors_next, descriptors_current, matches_flann);

  // remove bad matches - too far away in descriptor space,
  // create vector of 2D and 3D point correspondences for PnP
  std::vector<cv::Point2f> corr_2D_points_vector;
  std::vector<cv::Point3f> corr_3D_points_vector;

  std::vector<cv::DMatch> candidate_matches;
  for (unsigned int m_idx = 0; m_idx < matches_flann.size(); ++m_idx)
  {
    cv::DMatch match = matches_flann[m_idx];

    if (match.distance < max_descriptor_space_distance)
    {
      int idx_query = match.queryIdx;
      int idx_train = match.trainIdx;
      cv::Point2f cv_2D_point_train = keypoints_current[idx_train].pt;
      // Compute 3D point
      cv::Point2f cv_2D_point_query = keypoints_next[idx_query].pt;
      float depth = (float) current_depth_img.at<uint16_t>((int) roundf(cv_2D_point_train.y), (int) roundf(cv_2D_point_train.x)) / 1000.0f;
      //printf("Depth(%f,%f) = %f \t", cv_2D_point_query.x, cv_2D_point_query.y, depth);
      if(depth > 0)
      {
        candidate_matches.push_back(match);
        corr_2D_points_vector.push_back(cv_2D_point_query);

        Vector3f p_cam;
        p_cam(0,0) = cv_2D_point_train.x * depth;
        p_cam(1,0) = cv_2D_point_train.y * depth;
        p_cam(2,0) = depth;
        //printf("Point in Camera frame: (%f,%f, %f) \t", p_cam(0,0), p_cam(1,0), p_cam(2,0));

        Vector3f p_world = intrinsic_matrix.inverse() * p_cam;

        cv::Point3d cv_p_world;
        cv_p_world.x = p_world(0,0);
        cv_p_world.y = p_world(1,0);
        cv_p_world.z = p_world(2,0);
        corr_3D_points_vector.push_back(cv_p_world);
        //printf("3D Point: (%f,%f, %f) \n", cv_p_world.x, cv_p_world.y, cv_p_world.z);

      }
//      else
//      {
//        ROS_ERROR("Depth value shouldn't be zero");
//      }
    }
  }

  if(profile)
  {
    printf("Feature-Matching delay =  %f ms \t using %s detector and %s descriptor\n", getMsDuration(start_feature_matching), feature_detection_alg.c_str(), feature_descriptor_alg.c_str());
  }

  if(draw_matches)
  {
    cv::Mat current_img_copy = current_img.clone();
    cv::Mat next_img_copy = next_img.clone();

    cv::Mat matches_result_img;
    cv::drawMatches(
        next_img_copy, keypoints_next, // Query image and its keypoints
        current_img_copy, keypoints_current, // Train image and its keypoints
        candidate_matches, // the matches
        matches_result_img // the image produced
    ); // color of the lines
    cv::namedWindow("Matches FLANN", CV_WINDOW_KEEPRATIO);
    cv::imshow("Matches FLANN", matches_result_img);
  }

  // Fix max inliers count to be reasonable within number of descriptors
  int number_of_candidate_matches = candidate_matches.size();
  if(number_of_candidate_matches < min_inliers_count)
    min_inliers_count = number_of_candidate_matches; // update minimum

  // -----------------------------------------------------------------------------
  // ------- transformation computation with PnP ---------------------------------
  cv::Mat M; // The intrinsic matrix
  cv3x3FromEigen(intrinsic_matrix, M);

  cv::Mat rvec, rmat;
  cv::Mat tvec;
  // FIXME: commented temporarily because there is not initial trasformation being passed
  //    tfToOpenCVRt(transform, rmat, tvec);
  //    cv::Rodrigues(rmat, rvec);

  bool useExtrinsicGuess = false;
  std::vector<int> inliers_indices;

  ros::WallTime start_PnP_RANSAC = ros::WallTime::now();
  //          cv::solvePnP(corr_3D_points_vector, corr_2D_points_vector, M, cv::Mat(), rvec, tvec, true);
  cv::solvePnPRansac(corr_3D_points_vector, corr_2D_points_vector, M, cv::Mat(), rvec, tvec, useExtrinsicGuess,
                     number_of_iterations, reprojection_error, min_inliers_count, inliers_indices);
  if(profile)
  {
    printf("PnP-RANSAC delay =  %f ms  using %d iterations (Finding %d inliers)\n", getMsDuration(start_PnP_RANSAC), number_of_iterations, (int) inliers_indices.size());
  }

  std::cout << inliers_indices.size() << " inliers" << std::endl;
  if(draw_matches)
  {
    std::vector<cv::DMatch> inliers_matches;

    for (unsigned int m_idx = 0; m_idx < inliers_indices.size(); ++m_idx)
    {
      cv::DMatch match = candidate_matches[inliers_indices[m_idx]];
      inliers_matches.push_back(match);
      cv::Mat current_img_copy = current_img.clone();
      cv::Mat next_img_copy = next_img.clone();

      cv::Mat matches_result_img;
      cv::drawMatches(
          next_img_copy, keypoints_next, // Query image and its keypoints
          current_img_copy, keypoints_current, // Train image and its keypoints
          inliers_matches, // the matches
          matches_result_img // the image produced
      ); // color of the lines
      cv::namedWindow("Matches Inliers", CV_WINDOW_KEEPRATIO);
      cv::imshow("Matches Inliers", matches_result_img);
    }
    cv::waitKey(1);

  }

  cv::Rodrigues(rvec, rmat);
  tf::Transform next2current_tf;
  openCVRtToTf(rmat, tvec, next2current_tf);
  transform = next2current_tf.inverse();
}

} //namespace ccny_rgbd

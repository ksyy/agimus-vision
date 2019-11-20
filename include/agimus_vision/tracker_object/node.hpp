#ifndef __TRACKER_BOX__NODE__HPP__
#define __TRACKER_BOX__NODE__HPP__

#include "agimus_vision/tracker_object/detector_apriltag.hpp"

#include "agimus_vision/AddAprilTagService.h"
#include "agimus_vision/SetChessboardService.h"

#include <mutex>
#include <string>
#include <memory>
#include <vector>

#include <ros/ros.h>

#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>

#include <geometry_msgs/TransformStamped.h>
#include <sensor_msgs/CameraInfo.h>
#include <sensor_msgs/Image.h>

#include <visp3/core/vpImage.h>
#include <visp3/core/vpCameraParameters.h>

namespace agimus_vision {
namespace tracker_object {

/// Tracking of April tag.
///
/// - It advertises a service "add_april_tag_detector" (See Node::addAprilTagService).
/// - It publishes to /tf the tag pose if ROS param "broadcastTf" is \c true.
///   In this case, the child node name is postfixed with ROS param "broadcastTfPostfix".
/// - It publishes to /agimus/vision/tags/tf the tag pose if ROS param "broadcastTopic" is \c true.
class Node
{
    ros::NodeHandle _node_handle;

    // Names of the topics sending the images and infos
    std::string _image_topic;
    std::string _camera_info_topic;
    
    uint32_t _queue_size;
    ros::Subscriber _image_sub;
    ros::Subscriber _camera_info_sub;
    tf2_ros::Buffer _tf_buffer;
    tf2_ros::TransformListener _tf_listener;

    // Names of the publication TF nodes   
    std::string _tf_camera_node;
    std::string _tf_node;
    std::string _broadcast_tf_postfix;

    // Images and parameters taken from the camera's topics
    std::mutex _image_lock;
    std::mutex _cam_param_lock;
    vpCameraParameters _cam_parameters;
    std_msgs::Header _image_header;
    vpImage<vpRGBa> _image;
    vpImage<unsigned char> _gray_image;
    bool _image_new;

    // Classes called to detect some object in the image and then track it
    // First string is the name of the parent node and second is the name of the detected object's node
    std::map< int, std::pair< DetectorPtr, std::pair< std::string, std::string > > > _detectors;
    //std::unique_ptr< Tracker > _tracker;
    
    bool _debug_display;
    bool _broadcast_tf;
    bool _broadcast_topic;

    // Wait for the first available image
    void waitForImage();


    // Init the tracker with the AprilTag found previously
    void initTracking( int id );
    
    // Publish the object position rt the camera to TF
    void publish_pose_topic( const vpHomogeneousMatrix &cMo_visp, const std::string &parent_node_name, const std::string &node_name, const ros::Time &timestamp );
    void publish_pose_tf( const vpHomogeneousMatrix &cMo_visp, const std::string &parent_node_name, const std::string &node_name, const ros::Time &timestamp );

    std::vector< ros::ServiceServer > _services;
    ros::Publisher _publisherVision;

public:
    Node();

    /// Callback to update the camera information
    void cameraInfoCallback(const sensor_msgs::CameraInfoConstPtr& camera_info);

    /// Callback to use the images
    void frameCallback(const sensor_msgs::ImageConstPtr& image);
    
    /// Add an April tag
    /// \include srv/AddAprilTagService.srv
    bool addAprilTagService( agimus_vision::AddAprilTagService::Request  &req,
                             agimus_vision::AddAprilTagService::Response &res );
    
    /// Setup detection of a chessboard
    /// \include srv/SetChessboardService.srv
    bool setChessboardService( agimus_vision::SetChessboardService::Request  &req,
                               agimus_vision::SetChessboardService::Response &res );

    void spin();
};

}
}

#endif // __VISP_TRACKER_NODE_HPP__

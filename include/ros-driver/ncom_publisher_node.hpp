/**
 * \file ncom_publisher_node.hpp
 * Defines node to take NCom data and publish it in ROS messages.
 */

#ifndef NCOM_PUBLISHER_NODE_HPP
#define NCOM_PUBLISHER_NODE_HPP

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <cmath>

// ROS includes
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "sensor_msgs/msg/nav_sat_fix.hpp"
#include "sensor_msgs/msg/imu.hpp"
// Boost includes
#include <boost/asio.hpp>

// gad-sdk includes
#include "nav/NComRxC.h"
#include "ros-driver/nav_const.hpp"
#include "ros-driver/ros_ncom_wrapper.hpp"
#include "ros-driver/udp_server_client.h"

using namespace std::chrono_literals;


/**
 * Enumeration of driver timestamp modes for published topics
 */
enum PUB_TIMESTAMP_MODE
{
  DRIVER = 0,
  NCOM = 1
};

struct NComPublisherParams
{
  rclcpp::Parameter ncom_rate;
  rclcpp::Parameter unit_ip;
  rclcpp::Parameter unit_port;
  rclcpp::Parameter timestamp_mode;
  rclcpp::Parameter frame_id;
  rclcpp::Parameter pub_string_rate;
  rclcpp::Parameter pub_odometry_rate;
  rclcpp::Parameter pub_nav_sat_fix_rate;
  rclcpp::Parameter pub_imu_rate;
  rclcpp::Parameter pub_velocity_rate;
  rclcpp::Parameter pub_time_reference_rate;
  rclcpp::Parameter pub_tf2_rate;
};

/**
 * This class creates a subclass of Node designed to take NCom data from the 
 * NCom decoder and publish it to pre-configured ROS topics.
 * 
 * @todo Add config struct to hold data which will hold config parsed from the 
 *       .yaml file.
 * @todo Change topic names to a standard form /imu/.. 
 */
class NComPublisherNode : public rclcpp::Node
{
private:

  NComPublisherParams params;

  std::string unitIp;
  short unitPort;
  short timestampMode;
  std::string frameId;
  std::chrono::duration<uint64_t,std::milli> ncomInterval;
  std::chrono::duration<uint64_t,std::milli> pubStringInterval;
  std::chrono::duration<uint64_t,std::milli> pubOdometryInterval;
  std::chrono::duration<uint64_t,std::milli> pubNavSatFixInterval;
  std::chrono::duration<uint64_t,std::milli> pubImuInterval;
  std::chrono::duration<uint64_t,std::milli> pubVelocityInterval;
  std::chrono::duration<uint64_t,std::milli> pubTimeReferenceInterval;
  std::chrono::duration<uint64_t,std::milli> pubTf2Interval;
  // ...

  rclcpp::TimerBase::SharedPtr timer_ncom_;
  rclcpp::TimerBase::SharedPtr timer_string_;
  rclcpp::TimerBase::SharedPtr timer_odometry_;
  rclcpp::TimerBase::SharedPtr timer_nav_sat_fix_;
  rclcpp::TimerBase::SharedPtr timer_imu_;
  rclcpp::TimerBase::SharedPtr timer_velocity_;
  rclcpp::TimerBase::SharedPtr timer_time_reference_;
  rclcpp::TimerBase::SharedPtr timer_tf2_;

  void timer_ncom_callback();
  void timer_string_callback();
  void timer_odometry_callback();
  void timer_nav_sat_fix_callback();
  void timer_imu_callback();
  void timer_time_reference_callback();
  void timer_velocity_callback();
  void timer_tf2_callback();

  /**
   * Publisher for std_msgs/msg/string. Only used for debugging, currently 
   * outputs lat, long, alt in string form.
   */
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr       pubString_;
  /**
   * Publisher for /sensor_msgs/msg/Odometry
   */
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr     pubOdometry_;
  /**
   * Publisher for /sensor_msgs/msg/NavSatFix
   */
  rclcpp::Publisher<sensor_msgs::msg::NavSatFix>::SharedPtr pubNavSatFix_;
  /**
   * Publisher for /sensor_msgs/msg/Imu
   */
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr       pubImu_;
  /**
   * Publisher for /sensor_msgs/msg/TwistStamped
   */
  rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr  pubVelocity_;
  /**
   * Publisher for /sensor_msgs/msg/TimeReference
   */
  rclcpp::Publisher<sensor_msgs::msg::TimeReference>::SharedPtr  pubTimeReference_;
  /**
   * Publisher for /geometry_msgs/msg/TransformStamped
   */
  rclcpp::Publisher<geometry_msgs::msg::TransformStamped>::SharedPtr  pubTf2_;

  rclcpp::Clock clock_;

public:
  NComPublisherNode() : Node("ncom_publisher"), count_(0)
  {
    
    // Initialise configurable parameters (all params should have defaults)
    this->declare_parameter("unit_ip", "0.0.0.0");
    this->declare_parameter("unit_port", 3000);
    this->declare_parameter("timestamp_mode", 1);
    this->declare_parameter("frame_id", "base_link");
    this->declare_parameter("ncom_rate", 100.0);
    this->declare_parameter("pub_string_rate", 1.0);
    this->declare_parameter("pub_odometry_rate", 1.0);
    this->declare_parameter("pub_nav_sat_fix_rate", 1.0);
    this->declare_parameter("pub_imu_rate", 1.0);
    this->declare_parameter("pub_velocity_rate", 1.0);
    this->declare_parameter("pub_time_reference_rate", 1.0);
    this->declare_parameter("pub_tf2_rate", 1.0);

    // Get parameters (from config, command line, or from default)
    params.ncom_rate                 = this->get_parameter("ncom_rate");
    params.unit_ip                   = this->get_parameter("unit_ip");
    params.unit_port                 = this->get_parameter("unit_port");
    params.timestamp_mode            = this->get_parameter("timestamp_mode");
    params.frame_id                  = this->get_parameter("frame_id");
    params.pub_string_rate           = this->get_parameter("pub_string_rate");
    params.pub_odometry_rate         = this->get_parameter("pub_odometry_rate");
    params.pub_nav_sat_fix_rate      = this->get_parameter("pub_nav_sat_fix_rate");
    params.pub_imu_rate              = this->get_parameter("pub_imu_rate");
    params.pub_velocity_rate         = this->get_parameter("pub_velocity_rate");
    params.pub_time_reference_rate   = this->get_parameter("pub_time_reference_rate");
    params.pub_tf2_rate              = this->get_parameter("pub_tf2_rate");
    // Convert parameters to useful variable types
    unitIp                = params.unit_ip.as_string();
    unitPort              = params.unit_port.as_int();
    timestampMode         = params.timestamp_mode.as_int();
    frameId               = params.frame_id.as_string();

    ncomInterval             = std::chrono::milliseconds(
                             int(1000.0 / params.ncom_rate.as_double()));
    pubStringInterval        = std::chrono::milliseconds(
                             int(1000.0 / params.pub_string_rate.as_double()));
    pubOdometryInterval      = std::chrono::milliseconds(
                             int(1000.0 / params.pub_odometry_rate.as_double()));
    pubNavSatFixInterval     = std::chrono::milliseconds(
                             int(1000.0 / params.pub_nav_sat_fix_rate.as_double()));
    pubImuInterval           = std::chrono::milliseconds(
                             int(1000.0 / params.pub_imu_rate.as_double()));
    pubVelocityInterval      = std::chrono::milliseconds(
                             int(1000.0 / params.pub_velocity_rate.as_double()));
    pubTimeReferenceInterval = std::chrono::milliseconds(
                             int(1000.0 / params.pub_time_reference_rate.as_double()));
    pubTf2Interval           = std::chrono::milliseconds(
                             int(1000.0 / params.pub_tf2_rate.as_double()));

 
    // Initialise publishers for each message - all are initialised, even if not
    // configured
    pubString_        = this->create_publisher<std_msgs::msg::String>               ("ins/debug_string_pos", 10); 
    pubOdometry_      = this->create_publisher<nav_msgs::msg::Odometry>             ("ins/odom",             10); 
    pubNavSatFix_     = this->create_publisher<sensor_msgs::msg::NavSatFix>         ("ins/nav_sat_fix",      10); 
    pubImu_           = this->create_publisher<sensor_msgs::msg::Imu>               ("imu/imu_data",         10); 
    pubVelocity_      = this->create_publisher<geometry_msgs::msg::TwistStamped>    ("ins/velocity",         10); 
    pubTimeReference_ = this->create_publisher<sensor_msgs::msg::TimeReference>     ("ins/time_reference",   10);
    pubTf2_           = this->create_publisher<geometry_msgs::msg::TransformStamped>("ins/tf2",              10); 

    clock_ = rclcpp::Clock(RCL_ROS_TIME); /*! @todo Add option for RCL_SYSTEM_TIME */

    timer_ncom_ = this->create_wall_timer(
                  ncomInterval, std::bind(&NComPublisherNode::timer_ncom_callback, this));
    timer_string_ = this->create_wall_timer(
                  pubStringInterval, std::bind(&NComPublisherNode::timer_string_callback, this));
    timer_odometry_ = this->create_wall_timer(
                  pubOdometryInterval, std::bind(&NComPublisherNode::timer_odometry_callback, this));
    timer_nav_sat_fix_ = this->create_wall_timer(
                  pubNavSatFixInterval, std::bind(&NComPublisherNode::timer_nav_sat_fix_callback, this));
    timer_imu_ = this->create_wall_timer(
                  pubImuInterval, std::bind(&NComPublisherNode::timer_imu_callback, this));
    timer_velocity_ = this->create_wall_timer(
                  pubVelocityInterval, std::bind(&NComPublisherNode::timer_velocity_callback, this));
    timer_time_reference_ = this->create_wall_timer(
                  pubTimeReferenceInterval, std::bind(&NComPublisherNode::timer_time_reference_callback, this));
    timer_tf2_ = this->create_wall_timer(
                  pubTf2Interval, std::bind(&NComPublisherNode::timer_tf2_callback, this));

    nrx = NComCreateNComRxC();

    unitEndpointNCom = boost::asio::ip::udp::endpoint(
      boost::asio::ip::address::from_string(this->unitIp), 3000);

    this->udpClient.set_local_port(3000);

  }

  /**
   * NCom decoder instance
   */
  NComRxC *nrx;
  /**
   * Buffer for UDP data
   */
  unsigned char buff[1024];
  /**
   * UDP Client to receive data from the device. 
   */
  networking_udp::client udpClient;
  /**
   * Endpoint for the udpClient to receive data from 
   */
  boost::asio::ip::udp::endpoint unitEndpointNCom;

  /**
   * Count of callbacks run by the node (will correspond to number of ncom 
   * packets received)
   */
  int count_;
  /**
   * Get the IP address of the OxTS unit, as set in the .yaml params file
   * 
   * @returns IP address as a string
   */
  std::string get_unit_ip();
  /**
   * Get the endpoint port of the OxTS unit, as set in the .yaml params file
   * 
   * @returns Port as a short
   */
  short get_unit_port();

};






#endif //NCOM_PUBLISHER_NODE_HPP
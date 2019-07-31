#ifndef CONVERT_HPP
#define CONVERT_HPP

#include <geometry_msgs/Quaternion.h>
#include <tf/tf.h>
#include <Eigen/Geometry>

namespace ExpLib
{
double qToYaw(const tf::Quaternion& q){
    double roll, pitch, yaw;
    tf::Matrix3x3(q).getRPY(roll,pitch,yaw);
    return yaw;
}

double qToYaw(const geometry_msgs::Quaternion& q){
    return qToYaw(tf::Quaternion(q.x, q.y, q.z, q.w));
}

double qToYaw(const Eigen::Quaterniond& q){
    return qToYaw(tf::Quaternion(q.x(), q.y(), q.z(), q.w()));
}

geometry_msgs::Quaternion yawToQ(double yaw){
    geometry_msgs::Quaternion msg;
    tf::quaternionTFToMsg(tf::createQuaternionFromRPY(0,0,yaw), msg);
    return msg;
}

geometry_msgs::Pose pointToPose(const geometry_msgs::Point& point){
    geometry_msgs::Pose msg;
    msg.position.x = point.x;
    msg.position.y = point.y;
    msg.position.z = point.z;
    msg.orientation.w = 1.0;
    return msg;
}

Eigen::Vector3d pointToVector3d(const geometry_msgs::Point& point){
    Eigen::Vector3d vec;
    vec.x() = point.x;
    vec.y() = point.y;
    vec.z() = point.z;
    return vec;
}

Eigen::Vector2d pointToVector2d(const geometry_msgs::Point& point){
    Eigen::Vector2d vec;
    vec.x() = point.x;
    vec.y() = point.y;
    return vec;
}

Eigen::Vector2d msgVectorToVector2d(const geometry_msgs::Vector3& vector){
    Eigen::Vector2d vec;
    vec.x() = vector.x;
    vec.y() = vector.y;
    return vec;
}

geometry_msgs::PoseStamped pointToPoseStamped(const geometry_msgs::Point& p, const std::string& f){
    geometry_msgs::PoseStamped msg;
    msg.pose = pointToPose(p);
    msg.header.frame_id = f;
    return msg;
}

geometry_msgs::PoseStamped poseToPoseStamped(const geometry_msgs::Pose& p, const std::string& f){
    geometry_msgs::PoseStamped msg;
    msg.pose = p;
    msg.header.frame_id = f;
    return msg;
}

geometry_msgs::Quaternion tfQuaToGeoQua(const tf::Quaternion& tq){
    geometry_msgs::Quaternion q;
    q.x = tq.getX();
    q.y = tq.getY();
    q.z = tq.getZ();
    q.w = tq.getW();
    return q;
}

geometry_msgs::Quaternion eigenQuaToGeoQua(const Eigen::Quaterniond& eq){
    geometry_msgs::Quaternion q;
    q.x = eq.x();
    q.y = eq.y();
    q.z = eq.z();
    q.w = eq.w();
    return q;
}

}

#endif // CONVERT_HPP
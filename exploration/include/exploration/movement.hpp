#ifndef MOVEMENT_HPP
#define MOVEMENT_HPP

#include <actionlib/client/simple_action_client.h>
#include <exploration_libraly/convert.hpp>
#include <exploration_libraly/struct.hpp>
#include <exploration_libraly/utility.hpp>
#include <Eigen/Geometry>
#include <geometry_msgs/Point.h>
#include <geometry_msgs/PointStamped.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Twist.h>
#include <kobuki_msgs/BumperEvent.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <ros/ros.h>
#include <sensor_msgs/LaserScan.h>

//センサーデータを受け取った後にロボットの動作を決定する
//障害物回避を含む

/*
movement tutorial

In source file

    #include <exploration/movement.hpp>

        Movement mv;

    if you want to move to goal with move_base
        mv.moveToGoal(goal);//goal type == geometry_msgs::Point

    if you want to move forward
        mv.moveToForward();

    if you want to one rotations
        mv.oneRotation();
*/

class Movement 
{
private:
    //parameters
    double SAFE_DISTANCE;
    double SAFE_SPACE;
    double SCAN_THRESHOLD;
    double FORWARD_VELOCITY;
    double BACK_VELOCITY;
    double BACK_TIME;
    double BUMPER_ROTATION_TIME;
    double FORWARD_ANGLE;
    double ROTATION_VELOCITY;
    double EMERGENCY_THRESHOLD;
    double ROAD_CENTER_THRESHOLD;
    double ROAD_THRESHOLD;
    double CURVE_GAIN;
    double VELOCITY_GAIN;
    double AVOIDANCE_GAIN;
    double VFH_GAIN;
    double ROAD_CENTER_GAIN;
    double ROTATION_GAIN;
    std::string MOVEBASE_NAME;
    double WALL_FORWARD_ANGLE;
    double WALL_RATE_THRESHOLD;
    double WALL_DISTANCE_UPPER_THRESHOLD;
    double WALL_DISTANCE_LOWER_THRESHOLD;
    double EMERGENCY_DIFF_THRESHOLD;
    double ANGLE_BIAS;    

    ExpLib::Struct::subStruct<sensor_msgs::LaserScan> scan_;
    ExpLib::Struct::subStruct<geometry_msgs::PoseStamped> pose_;
    ExpLib::Struct::subStruct<kobuki_msgs::BumperEvent> bumper_;
    ExpLib::Struct::pubStruct<geometry_msgs::Twist> velocity_;

    double previousOrientation_;
    
    void approx(std::vector<float>& scanRanges);
    void vfhMovement(sensor_msgs::LaserScan& scan,bool straight, double angle);
    bool bumperCollision(const kobuki_msgs::BumperEvent& bumper);
    double vfhCalculation(sensor_msgs::LaserScan scan, bool isCenter, double angle);
    bool emergencyAvoidance(const sensor_msgs::LaserScan& scan);
    geometry_msgs::Twist velocityGenerator(double theta, double v, double t);
    bool roadCenterDetection(const sensor_msgs::LaserScan& scan);
    void recoveryRotation(void);

    //moveToForwardのときの障害物回避で、前方に壁があったときの処理
    bool forwardWallDetection(const sensor_msgs::LaserScan& scan, double& angle);
    double sideSpaceDetection(const sensor_msgs::LaserScan& scan, int plus, int minus);

public:
    Movement();

    void moveToGoal(geometry_msgs::PointStamped goal);
    void moveToForward(void);
    void oneRotation(void);
};

Movement::Movement()
    :scan_("scan",1)
    ,pose_("pose",1)
    ,bumper_("bumper",1)
    ,velocity_("velocity", 1)
    ,previousOrientation_(1.0){

    ros::NodeHandle p("~");
    p.param<double>("safe_distance", SAFE_DISTANCE, 0.75);
    p.param<double>("safe_space", SAFE_SPACE, 0.6);
    p.param<double>("scan_threshold", SCAN_THRESHOLD, 1.5);
    p.param<double>("forward_velocity", FORWARD_VELOCITY, 0.2);
    p.param<double>("back_velocity", BACK_VELOCITY, -0.2);
    p.param<double>("back_time", BACK_TIME, 0.5);
    p.param<double>("bumper_rotation_time", BUMPER_ROTATION_TIME, 1.5);
    p.param<double>("forward_angle", FORWARD_ANGLE, 0.09);
    p.param<double>("rotation_velocity", ROTATION_VELOCITY, 0.5);
    p.param<double>("emergency_threshold", EMERGENCY_THRESHOLD, 0.5);
    p.param<double>("road_center_threshold", ROAD_CENTER_THRESHOLD, 5.0);
    p.param<double>("road_threshold", ROAD_THRESHOLD, 1.5);
    p.param<double>("curve_gain", CURVE_GAIN, 2.0);
    p.param<double>("velocity_gain", VELOCITY_GAIN, 1.0);
    p.param<double>("rotation_gain", ROTATION_GAIN, 1.0);
    p.param<double>("avoidance_gain", AVOIDANCE_GAIN, 0.3);
    p.param<double>("vfh_gain", VFH_GAIN, 0.5);
    p.param<double>("road_center_gain", ROAD_CENTER_GAIN, 0.8);
    p.param<std::string>("movebase_name", MOVEBASE_NAME, "move_base");
    p.param<double>("wall_forward_angle", WALL_FORWARD_ANGLE, 0.17);
    p.param<double>("wall_rate_threshold", WALL_RATE_THRESHOLD, 0.8);
    p.param<double>("wall_distance_upper_threshold", WALL_DISTANCE_UPPER_THRESHOLD, 5.0);
    p.param<double>("wall_distance_lower_threshold", WALL_DISTANCE_LOWER_THRESHOLD, 0.5);
    p.param<double>("emergency_diff_threshold", EMERGENCY_DIFF_THRESHOLD, 0.3);
    p.param<double>("angle_bias", ANGLE_BIAS, 10.0);
}

void Movement::approx(std::vector<float>& scanRanges){
    float depth,depth1,depth2;
    depth = depth1 = depth2 =0;

    for(int j=0,count=0,e=scanRanges.size()-1;j!=e;++j){
        depth=scanRanges[j];
        //|val|nan|のとき
        if(!std::isnan(depth) && std::isnan(scanRanges[j+1])){
            depth1=depth;
            count++;
        }
        if(std::isnan(depth)){
            //|nan|nan|の区間
            if(std::isnan(scanRanges[j+1])){
                count++;
            }
            //|nan|val|のとき
            else{
                depth2=scanRanges[j+1];
                //左端がnanのとき
                if(std::isnan(depth1)){
                    for(int k=0,e=count+1;k!=e;++k)
                        scanRanges[j-k]=0.01;//depth2;
                }
                else{
                    for(int k=0;k!=count;++k)
                        scanRanges[j-k]=depth2-(depth2-depth1)/(count+1)*(k+1);
                }
                count=0;
            }
        }
        //右端がnanのとき
        if(j==(scanRanges.size()-1)-1 && std::isnan(scanRanges[j+1])){
            for(int k=0;k!=count;++k)
                scanRanges[j+1-k]=0.01;//depth1;
            count=0;
        }
    }		
    if(std::isnan(scanRanges[0])){
        scanRanges[0] = scanRanges[1] - (scanRanges[2] - scanRanges[1]);
        if(scanRanges[0] < 0) scanRanges[0] = 0;
    }
}

void Movement::moveToGoal(geometry_msgs::PointStamped goal){
    static actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> ac(MOVEBASE_NAME, true);
    
    while(!ac.waitForServer(ros::Duration(1.0)) && ros::ok()) ROS_INFO_STREAM("wait for action server << " << MOVEBASE_NAME);

    if(pose_.q.callOne(ros::WallDuration(1.0))) return;

    if(pose_.data.header.frame_id != goal.header.frame_id){
        static bool initialized = false;
        static tf::TransformListener listener;
        if(!initialized){
            listener.waitForTransform(pose_.data.header.frame_id, goal.header.frame_id, ros::Time(), ros::Duration(1.0));
            initialized = true;
        }
        ExpLib::Utility::coordinateConverter2d<void>(listener, pose_.data.header.frame_id, goal.header.frame_id, goal.point);
    }

    move_base_msgs::MoveBaseGoal to;
    to.target_pose.header.frame_id = pose_.data.header.frame_id;
    to.target_pose.header.stamp = ros::Time::now();

    // 回転角度の補正値
    double yaw = ExpLib::Convert::qToYaw(pose_.data.pose.orientation);
    Eigen::Vector3d cross = Eigen::Vector3d(cos(yaw),sin(yaw),0.0).normalized().cross(Eigen::Vector3d(goal.point.x-pose_.data.pose.position.x,goal.point.y-pose_.data.pose.position.y,0.0).normalized());
    double rotateTheta = ANGLE_BIAS * M_PI/180 * (cross.z() > 0 ? 1.0 : cross.z() < 0 ? -1.0 : 0);
    Eigen::Matrix2d rotation;
    rotation << cos(rotateTheta), -sin(rotateTheta), sin(rotateTheta), cos(rotateTheta);

    // 目標での姿勢
    Eigen::Vector2d startToGoal = rotation * Eigen::Vector2d(goal.point.x-pose_.data.pose.position.x,goal.point.y-pose_.data.pose.position.y);

    ROS_INFO_STREAM("after_vector_x : " << startToGoal.x() << ", after_vector_y : " << startToGoal.y());

    Eigen::Quaterniond q = Eigen::Quaterniond::FromTwoVectors(Eigen::Vector3d::UnitX(),Eigen::Vector3d(startToGoal.x(),startToGoal.y(),0.0));

    to.target_pose.pose = ExpLib::Construct::msgPose(goal.point,ExpLib::Convert::eigenQuaToGeoQua(q));

    ROS_DEBUG_STREAM("goal pose : " << to.target_pose.pose);
    ROS_INFO_STREAM("send goal to move_base");
    ac.sendGoal(to);
    ROS_INFO_STREAM("wait for result");
    ac.waitForResult();
    ROS_INFO_STREAM("move_base was finished");
    ROS_INFO_STREAM((ac.getState() == actionlib::SimpleClientGoalState::SUCCEEDED ? "I Reached Given Target" : "I did not Reach Given Target"));
}

void Movement::moveToForward(void){
    ROS_INFO_STREAM("Moving Straight");
    ROS_INFO_STREAM("previous orientation : " << previousOrientation_);

    if(scan_.q.callOne(ros::WallDuration(1))) return;

    double angle;
    if(forwardWallDetection(scan_.data, angle)) vfhMovement(scan_.data,false,std::move(angle));
    else if(!roadCenterDetection(scan_.data)) vfhMovement(scan_.data,true,0.0);
    
}

void Movement::vfhMovement(sensor_msgs::LaserScan& scan, bool straight, double angle){
    if(!bumper_.q.callOne(ros::WallDuration(1)) && !bumperCollision(bumper_.data)){
        double resultAngle = vfhCalculation(scan,straight,angle);
        if((int)resultAngle == INT_MAX){
            if(!emergencyAvoidance(scan)) recoveryRotation();
        }
        else{
            velocity_.pub.publish(velocityGenerator(resultAngle * VELOCITY_GAIN, FORWARD_VELOCITY * VELOCITY_GAIN, VFH_GAIN));
        }
    }
}

bool Movement::bumperCollision(const kobuki_msgs::BumperEvent& bumper){
    //壁に衝突してるかを確認して、してたらバック
    //バックの後に回転動作をさせる
    if(bumper.state){
        //バック部分
        ROS_WARN_STREAM("Bumper Hit !!");
        geometry_msgs::Twist vel;
        vel.linear.x = BACK_VELOCITY;
        ros::Duration duration(BACK_TIME);
        ros::Time setTime = ros::Time::now();

        while(ros::Time::now()-setTime < duration){
            velocity_.pub.publish(vel);
        }

        //回転部分
        vel.linear.x = 0;
        switch (bumper.bumper){
            case 0:
                vel.angular.z = -ROTATION_VELOCITY;
                break;
            case 1:
                vel.angular.z = -previousOrientation_ * ROTATION_VELOCITY;
                break;
            case 2:
                vel.angular.z = ROTATION_VELOCITY;
                break;
            default:
                break;
        }

        ros::Duration duration2(BUMPER_ROTATION_TIME);
        setTime = ros::Time::now();
        while(ros::Time::now()-setTime < duration2){
            velocity_.pub.publish(vel);
        }

        return true;
    }
    return false;
}

double Movement::vfhCalculation(sensor_msgs::LaserScan scan, bool isCenter, double angle){
    //要求角度と最も近くなる配列の番号を探索
    //安全な角度マージンの定義
    //要求角度に最も近くなる右側と左側の番号を探索
    ROS_DEBUG_STREAM("VFH Calculation");

    ROS_DEBUG_STREAM("Goal Angle : " << angle << " [deg]");

    static int centerPosition = 0;
    int goalI;

    if(isCenter){
        goalI = scan.ranges.size() / 2 - centerPosition;
        centerPosition = centerPosition == 0 ? 1 : 0;

        // ここでとりあえず正面の安全を確認(問題なければ正面に進ませる)
        int PLUS = goalI + (int)(FORWARD_ANGLE/scan.angle_increment);
        int MINUS = goalI - (int)(FORWARD_ANGLE/scan.angle_increment);
        ROS_INFO_STREAM("ranges.size() : " << scan.ranges.size() << ", goalI : " << goalI << ", goalI(rad) : " << scan.angle_min + goalI*scan.angle_increment << ", calc : " << (int)(FORWARD_ANGLE/scan.angle_increment));

        int count = 0;

        for(int i=MINUS;i!=PLUS;++i){
            // ROS_INFO_STREAM("range[" << i << "] : " << scan.ranges[i]);
            if(!std::isnan(scan.ranges[i])) ++count;
        }

        double rate = (double)count/(PLUS-MINUS);

        ROS_INFO_STREAM("PLUS : " << PLUS << ", MINUS : " << MINUS << ", count : " << count << ", rate : " << rate);

        if(rate < 0.1) {
            ROS_INFO_STREAM("center is safety");
            return scan.angle_min + goalI*scan.angle_increment;
        }
    }
    else{
        double min = DBL_MAX;
        for(int i=0,e=scan.ranges.size();i!=e;++i){
		    double diff = std::abs(angle - (scan.angle_min + scan.angle_increment * i));
		    if(diff < min){
			    min = std::move(diff);
			    goalI = i;
		    }
        }
    }

    approx(scan.ranges);

    const int SAFE_NUM = (asin((SAFE_SPACE)/(2*SAFE_DISTANCE))) / scan.angle_increment ;
    int start;
    int k;
    int count;
    int plus = INT_MAX;
    int minus = INT_MAX;

    //plus側
    for(int i=goalI,e=scan.ranges.size();i!=e;++i){
		if(scan.ranges[i] > SCAN_THRESHOLD){
			start = i;
			k = i;
			count = 0;
			while(scan.ranges[k] > SCAN_THRESHOLD && count < SAFE_NUM && k < scan.ranges.size()-1){
				++count;
				++k;
			}
			if(count == SAFE_NUM && start >= SAFE_NUM){
				count = 0;
				for(int j=start,f=start-SAFE_NUM;j!=f;--j){
					if(scan.ranges[j] > SCAN_THRESHOLD && count < SAFE_NUM) ++count;
				}
				if(count == SAFE_NUM){
					plus = start;
					break;
				}	
			}
		}
    }

    //minus側
	for(int i=goalI;i!=-1;--i){
		if(scan.ranges[i] > SCAN_THRESHOLD){
			start = i;
			k = i;
			count = 0;
			while(scan.ranges[k] > SCAN_THRESHOLD && count < SAFE_NUM && k > 0){
				++count;
				--k;
			}
			if(count == SAFE_NUM && start <= scan.ranges.size()-SAFE_NUM){
				count = 0;
				for(int j=start,e=start+SAFE_NUM;j!=e;++j){
					if(scan.ranges[j] > SCAN_THRESHOLD && count < SAFE_NUM) ++count;
				}
				if(count == SAFE_NUM){
					minus = start;
					break;
				}	
			}
		}
	}

    if(plus != INT_MAX || minus != INT_MAX){
		double pd = plus == INT_MAX ? INT_MAX : std::abs((scan.angle_min + scan.angle_increment * goalI) - (scan.angle_min + scan.angle_increment * plus));
		double md = minus == INT_MAX ? INT_MAX : std::abs((scan.angle_min + scan.angle_increment * goalI) - (scan.angle_min + scan.angle_increment * minus));
        return pd<=md ? scan.angle_min + scan.angle_increment * plus : scan.angle_min + scan.angle_increment * minus;
    }
    else{
        ROS_DEBUG_STREAM("Move Angle : Not Found");
        return INT_MAX;
    }
}

bool Movement::emergencyAvoidance(const sensor_msgs::LaserScan& scan){

    //minus側の平均
    double aveM=0;
    int nanM = 0;
    for(int i=0,e=scan.ranges.size()/2;i!=e;++i){
        if(!std::isnan(scan.ranges[i])) aveM += scan.ranges[i];
        else ++nanM;
    }
    aveM = nanM > (scan.ranges.size()/2)*0.8 ? DBL_MAX : aveM / scan.ranges.size()/2;

    // aveM /= scan.ranges.size()/2;


    //plus側
    double aveP=0;
    int nanP = 0;
    for(int i=scan.ranges.size()/2,e=scan.ranges.size();i!=e;++i){
        if(!std::isnan(scan.ranges[i])) aveP += scan.ranges[i];
        else ++nanP;
    }
    aveP = nanP > (scan.ranges.size()/2)*0.8 ? DBL_MAX : aveP / scan.ranges.size()/2;
    // aveP /= scan.ranges.size()/2;

    //左右の差がそんなにないなら前回避けた方向を採用する
    //一回目に避けた方向に基本的に従う
    //一回避けたら大きく差が出ない限りおなじほうこうに避ける

    ROS_DEBUG_STREAM("aveP : " << aveP << ", aveM : " << aveM <<  ", nanP : " << nanP << ", nanM : " << nanM << "\n");

    // ROS_DEBUG_STREAM("aveP : " << aveP << ", aveM : " << aveM << "\n");


    //まずよけれる範囲か見る
    if(aveP > EMERGENCY_THRESHOLD || aveM > EMERGENCY_THRESHOLD){
        //センサの安全領域の大きさが変わった時の処理//大きさがほとんど同じだった時の処理//以前避けた方向に避ける
        if(std::abs(aveM-aveP) > EMERGENCY_DIFF_THRESHOLD) previousOrientation_ = aveP > aveM ? 1.0 : -1.0;

        ROS_INFO_STREAM((previousOrientation_ > 0 ? "Avoidance to Left" : "Avoidance to Right"));

        velocity_.pub.publish(velocityGenerator(previousOrientation_*scan.angle_max/6 * VELOCITY_GAIN, FORWARD_VELOCITY * VELOCITY_GAIN, AVOIDANCE_GAIN));
        return true;
    }
    else{
        ROS_WARN_STREAM("I can not avoid it");
        return false;
    }
}

void Movement::recoveryRotation(void){
    ROS_WARN_STREAM("Recovery Rotation !");
    velocity_.pub.publish(ExpLib::Construct::msgTwist(0,previousOrientation_ * ROTATION_VELOCITY * VELOCITY_GAIN));
}

geometry_msgs::Twist Movement::velocityGenerator(double theta,double v,double t){
    return ExpLib::Construct::msgTwist(v,(CURVE_GAIN*theta)/(t/ROTATION_GAIN));
}

bool Movement::roadCenterDetection(const sensor_msgs::LaserScan& scan){
    ExpLib::Struct::scanStruct scanRect(scan.ranges.size(),scan.angle_max);

    for(int i=0,e=scan.ranges.size();i!=e;++i){
        if(!std::isnan(scan.ranges[i])){
            double tempAngle = scan.angle_min+(scan.angle_increment*i);
            if(scan.ranges[i]*cos(tempAngle) <= ROAD_CENTER_THRESHOLD){
                scanRect.ranges.emplace_back(scan.ranges[i]);
                scanRect.angles.emplace_back(std::move(tempAngle));
            }
        }
    }

    if(scanRect.ranges.size() < 2) return false;

    for(int i=0,e=scanRect.ranges.size()-1;i!=e;++i){
        if(std::abs(scanRect.ranges[i+1]*sin(scanRect.angles[i+1]) - scanRect.ranges[i]*sin(scanRect.angles[i])) >= ROAD_THRESHOLD){
            ROS_DEBUG_STREAM("Road Center Found");
            velocity_.pub.publish(velocityGenerator((scanRect.angles[i]+scanRect.angles[i+1])/2*VELOCITY_GAIN,FORWARD_VELOCITY*VELOCITY_GAIN,ROAD_CENTER_GAIN));
            return true;
        }
    }
    ROS_DEBUG_STREAM("Road Center Do Not Found");
    return false;
}

void Movement::oneRotation(void){
    //ロボットがz軸周りに一回転する
    ROS_DEBUG_STREAM("rotation");

    if(pose_.q.callOne(ros::WallDuration(1))) return;

    double initYaw,yaw = ExpLib::Convert::qToYaw(pose_.data.pose.orientation);
    double initSign = initYaw / std::abs(initYaw);

    if(std::isnan(initSign)) initSign = 1.0;

    //initYawが+の時は+回転
    //initYawが-の時は-回転
    geometry_msgs::Twist vel = ExpLib::Construct::msgTwist(0,initSign * ROTATION_VELOCITY);
    
    for(int count=0;(count < 3 && (count < 2 || std::abs(yaw) < std::abs(initYaw))) && ros::ok();){
        double yawOld = yaw;
        velocity_.pub.publish(vel);
        pose_.q.callOne(ros::WallDuration(1));
        yaw = ExpLib::Convert::qToYaw(pose_.data.pose.orientation);
        if(yawOld * yaw < 0) ++count;
    }
}

bool Movement::forwardWallDetection(const sensor_msgs::LaserScan& scan, double& angle){
    //前方に壁があるかどうかを判定する
    //前方何度まで見るかを決める
    //その範囲にセンサデータが何割存在するかで壁かどうか決める
    //壁があった場合右と左のどっちに避けるか決定

    const int CENTER_SUBSCRIPT = scan.ranges.size()/2;
    int PLUS = CENTER_SUBSCRIPT + (int)(WALL_FORWARD_ANGLE/scan.angle_increment);
    int MINUS = CENTER_SUBSCRIPT - (int)(WALL_FORWARD_ANGLE/scan.angle_increment);

    ROS_INFO_STREAM("ranges.size() : " << scan.ranges.size() << ", CENTER_SUBSCRIPT : " << CENTER_SUBSCRIPT << ", calc : " << (int)(WALL_FORWARD_ANGLE/scan.angle_increment));

    int count = 0;
    double wallDistance = 0;

    for(int i=MINUS;i!=PLUS;++i){
        if(!std::isnan(scan.ranges[i])){
            ++count;
            wallDistance += scan.ranges[i];
        }
    }
    ROS_INFO_STREAM("PLUS : " << PLUS << ", MINUS : " << MINUS << ", count : " << count << ", rate : " << (double)count/(PLUS-MINUS));

    wallDistance /= count;

    //壁の割合が少ないときと、壁までの距離が遠い時は検出判定をしない
    //追加で壁に近くなりすぎると判定ができなくなるので無効とする
    if((double)count/(PLUS-MINUS) > WALL_RATE_THRESHOLD && wallDistance < WALL_DISTANCE_UPPER_THRESHOLD && wallDistance > WALL_DISTANCE_LOWER_THRESHOLD){
        ROS_INFO_STREAM("Wall Found : " << wallDistance << " [m]");
        angle = sideSpaceDetection(scan,PLUS, MINUS);
        return true;
    }
    else{
        ROS_INFO_STREAM("Wall not Found or Out of Range");
        return false;
    }
}

double Movement::sideSpaceDetection(const sensor_msgs::LaserScan& scan, int plus, int minus){
    //minus
    int countNanMinus = 0;
    double maxSpaceMinus = 0;
    double aveMinus = 0;
    for(int i=0;i!=minus;++i){
        if(!std::isnan(scan.ranges[i])){
            aveMinus += scan.ranges[i];
            double temp;
            for(int j=i+1;j!=minus;++j){
                if(!std::isnan(scan.ranges[j])){
                    temp = std::abs(scan.ranges[i]*cos(scan.angle_min + scan.angle_increment*i)-scan.ranges[j]*cos(scan.angle_min + scan.angle_increment*j));
                    break;
                }
            }
            if(temp > maxSpaceMinus) maxSpaceMinus = std::move(temp);
        }
        else{
            ++countNanMinus;
        }
    }

    //plus
    double avePlus = 0;
    int countNanPlus = 0;
    double maxSpacePlus = 0;
    for(int i=plus,e=scan.ranges.size();i!=e;++i){
        if(!std::isnan(scan.ranges[i])){
            avePlus += scan.ranges[i];
            double temp;
            for(int j=i-1;j!=-1;--j){
                if(!std::isnan(scan.ranges[j])){
                    temp = std::abs(scan.ranges[i]*cos(scan.angle_min + scan.angle_increment*i)-scan.ranges[j]*cos(scan.angle_min + scan.angle_increment*j));
                    break;
                }
            }
            if(temp > maxSpacePlus) maxSpacePlus = std::move(temp);
        }
        else{
            ++countNanPlus;
        }
    }

    // ROS_INFO_STREAM("minus : " << minus << ", sum range : " << sumMinus << ", ave range : " << sumMinus/(minus - countNanMinus) << ", Nan count : " << countNanMinus << ", true count : " << minus - countNanMinus << ", space : " << maxSpaceMinus << "\n");    
    // ROS_INFO_STREAM("plus : " << plus << ", sum range : " << sumPlus << ", ave range : " << sumPlus/(scan.ranges.size() - plus - countNanPlus) << ", Nan count : " << countNanPlus << ", true count : " << scan.ranges.size() - plus - countNanPlus << ", space" << maxSpacePlus << "\n");

    aveMinus /= (minus - countNanMinus);
    avePlus /= (scan.ranges.size() - plus - countNanPlus);

    //不確定 //壁までの距離が遠いときは平均距離が長いほうが良い、近いときは開いてる領域が大きい方が良い
    if(maxSpaceMinus > maxSpacePlus && aveMinus > EMERGENCY_THRESHOLD){
        ROS_INFO_STREAM("Found Right Space");
        return (scan.angle_min + (scan.angle_increment * (scan.ranges.size()/2+minus)/2))/2;
    }
    else if(maxSpacePlus > maxSpaceMinus && avePlus > EMERGENCY_THRESHOLD){
        ROS_INFO_STREAM("Found Left Space");
        return (scan.angle_min + (scan.angle_increment * (plus + scan.ranges.size()/2)/2))/2;
    }
    else{
        ROS_INFO_STREAM("Not Found Space");
        return 0;
    }
}
#endif //MOVEMENT_HPP

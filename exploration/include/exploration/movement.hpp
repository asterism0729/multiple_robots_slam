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
#include <exploration_libraly/path_planning.hpp>
#include <navfn/navfn_ros.h>
// #include <pcl/segmentation/extract_clusters.h>
// #include <pcl_ros/point_cloud.h>

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
    int PATH_BACK_INTERVAL;
    double GOAL_RESET_RATE;
    double COSTMAP_MARGIN;
    int DIV_X;
    int DIV_Y;
    double ESC_MAP_X;
    double ESC_MAP_Y;

    ExpLib::Struct::subStruct<sensor_msgs::LaserScan> scan_;
    ExpLib::Struct::subStruct<geometry_msgs::PoseStamped> pose_;
    ExpLib::Struct::subStruct<kobuki_msgs::BumperEvent> bumper_;
    ExpLib::Struct::subStruct<nav_msgs::OccupancyGrid> gCostmap_;
    ExpLib::Struct::pubStruct<geometry_msgs::Twist> velocity_;
    ExpLib::Struct::pubStruct<geometry_msgs::PointStamped> goal_;

    ExpLib::PathPlanning<navfn::NavfnROS> pp_;

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

    bool lookupCostmap(const geometry_msgs::PoseStamped& goal);
    bool lookupCostmap(void);
    void escapeFromCostmap(const geometry_msgs::PoseStamped& pose);
    bool resetGoal(geometry_msgs::PoseStamped& goal, const geometry_msgs::PoseStamped& pose);
    void rotationFromTo(const geometry_msgs::Quaternion& from, const geometry_msgs::Quaternion& to);

    void nonGoalMove(const sensor_msgs::LaserScan& scan, bool straight, double angle);
    double isMoveable(const sensor_msgs::LaserScan& scan, double angle);
public:
    Movement();

    void moveToGoal(geometry_msgs::PointStamped goal);
    void moveToForward(void);
    void oneRotation(void);
    void testFunc(void);
};

Movement::Movement()
    :scan_("scan",1)
    ,pose_("pose",1)
    ,bumper_("bumper",1)
    ,velocity_("velocity", 1)
    ,previousOrientation_(1.0)
    ,pp_("movement_costmap","movement_planner") //クラス名
    ,goal_("goal", 1)
    ,gCostmap_("global_costmap",1){

    ros::NodeHandle p("~");
    p.param<double>("safe_distance", SAFE_DISTANCE, 0.25);
    p.param<double>("safe_space", SAFE_SPACE, 0.3);
    p.param<double>("scan_threshold", SCAN_THRESHOLD, 0.5);
    p.param<double>("forward_velocity", FORWARD_VELOCITY, 0.2);
    p.param<double>("back_velocity", BACK_VELOCITY, -0.2);
    p.param<double>("back_time", BACK_TIME, 0.5);
    p.param<double>("bumper_rotation_time", BUMPER_ROTATION_TIME, 1.5);
    p.param<double>("forward_angle", FORWARD_ANGLE, 0.09);
    p.param<double>("rotation_velocity", ROTATION_VELOCITY, 0.5);
    p.param<double>("emergency_threshold", EMERGENCY_THRESHOLD, 0.1);
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
    p.param<double>("emergency_diff_threshold", EMERGENCY_DIFF_THRESHOLD, 0.1);
    p.param<double>("angle_bias", ANGLE_BIAS, 10.0);
    p.param<int>("path_back_interval", PATH_BACK_INTERVAL, 10);
    p.param<double>("goal_reset_rate", GOAL_RESET_RATE, 1);
    p.param<double>("costmap_margin", COSTMAP_MARGIN, 0.4); // コストマップの検索窓の直径
    p.param<int>("div_x", DIV_X, 3);
    p.param<int>("div_y", DIV_Y, 3);
    p.param<double>("esc_map_x", ESC_MAP_X, 0.9);
    p.param<double>("esc_map_y", ESC_MAP_Y, 0.9);
}

void Movement::testFunc(void){
    ROS_INFO_STREAM("Moving Straight");
    ROS_INFO_STREAM("previous orientation : " << previousOrientation_);

    if(pose_.q.callOne(ros::WallDuration(1))) return;

    if(lookupCostmap(pose_.data)){
        escapeFromCostmap(pose_.data);
        pose_.q.callOne(ros::WallDuration(1.0));
    } 

    if(scan_.q.callOne(ros::WallDuration(1))) return;

    static bool ii = true;

    // if(ii){
    //      for(int i=0,ie=scan_.data.ranges.size();i!=ie;++i){
    //         ROS_INFO_STREAM("ranges.size() : " << scan_.data.ranges.size() << ", i : " << i << ", rad : " << scan_.data.angle_min + i*scan_.data.angle_increment);
    //     }
    //     ii = false;
    // }

    double angle;
    if(forwardWallDetection(scan_.data, angle)) nonGoalMove(scan_.data,false,std::move(angle));
    else if(!roadCenterDetection(scan_.data)) nonGoalMove(scan_.data,true,0.0);
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

    if(lookupCostmap(pose_.data)){
        escapeFromCostmap(pose_.data);
        pose_.q.callOne(ros::WallDuration(1.0));
    } 

    move_base_msgs::MoveBaseGoal mbg;
    mbg.target_pose.header.frame_id = pose_.data.header.frame_id;
    mbg.target_pose.header.stamp = ros::Time::now();

    // 目標での姿勢
    Eigen::Vector2d startToGoal;
    if(!pp_.getVec(pose_.data,ExpLib::Convert::pointStampedToPoseStamped(goal),startToGoal)){
        // pathが取得できなかった場合の回転角度の補正値
        double yaw = ExpLib::Convert::qToYaw(pose_.data.pose.orientation);
        Eigen::Vector3d cross = Eigen::Vector3d(cos(yaw),sin(yaw),0.0).normalized().cross(Eigen::Vector3d(goal.point.x-pose_.data.pose.position.x,goal.point.y-pose_.data.pose.position.y,0.0).normalized());
        double rotateTheta = ANGLE_BIAS * M_PI/180 * (cross.z() > 0 ? 1.0 : cross.z() < 0 ? -1.0 : 0);
        Eigen::Matrix2d rotation;
        rotation << cos(rotateTheta), -sin(rotateTheta), sin(rotateTheta), cos(rotateTheta);
        startToGoal = rotation * Eigen::Vector2d(goal.point.x-pose_.data.pose.position.x,goal.point.y-pose_.data.pose.position.y);
    }

    Eigen::Quaterniond q = Eigen::Quaterniond::FromTwoVectors(Eigen::Vector3d::UnitX(),Eigen::Vector3d(startToGoal.x(),startToGoal.y(),0.0));
    mbg.target_pose.pose = ExpLib::Construct::msgPose(goal.point,ExpLib::Convert::eigenQuaToGeoQua(q));

    ROS_DEBUG_STREAM("goal pose : " << mbg.target_pose.pose);
    ROS_INFO_STREAM("send goal to move_base");
    ac.sendGoal(mbg);
    ROS_INFO_STREAM("wait for result");

    ros::Rate rate(GOAL_RESET_RATE);

    while(!ac.getState().isDone() && ros::ok()){
        if(lookupCostmap(mbg.target_pose)){ //コストマップに被っているばあい
            // 目的地を再設定
            do{
                while(pose_.q.callOne(ros::WallDuration(1.0))&&ros::ok()) ROS_INFO_STREAM("Waiting pose ...");
                if(!resetGoal(mbg.target_pose,pose_.data)){ 
                    ROS_INFO_STREAM("current goal is canceled");
                    ac.cancelGoal(); //リセット出来ないばあいは目標をキャンセ留守る
                    ac.waitForResult();
                    break;
                }
            }while(lookupCostmap(mbg.target_pose) && ros::ok()); // 大丈夫な位置になるまでゴールを再設定したくないからwhile
                
            if(ac.getState().isDone()) break;
            // 大丈夫な目的地に変わっているので再設定
            ROS_INFO_STREAM("set a new goal pose : " << mbg.target_pose.pose);
            ROS_INFO_STREAM("send new goal to move_base");
            ac.sendGoal(mbg);
            // ゴールtopicに再出力
            goal_.pub.publish(ExpLib::Convert::poseStampedToPointStamped(mbg.target_pose));
            ROS_INFO_STREAM("wait for result");
        }
        rate.sleep();
    };

    ROS_INFO_STREAM("move_base was finished");
    ROS_INFO_STREAM((ac.getState() == actionlib::SimpleClientGoalState::SUCCEEDED ? "I Reached Given Target" : "I did not Reach Given Target"));
}

bool Movement::lookupCostmap(const geometry_msgs::PoseStamped& goal){
    ROS_INFO_STREAM("lookup global costmap");
    // 現在設定されているゴールがコストマップに被っているかだけを見る関数
    // 正確にはposeStampedの座標がコストマップに被ってるかを見る
    // true:被ってる, false:被ってない
    // コストマップを更新
    while(gCostmap_.q.callOne(ros::WallDuration(1.0))&&ros::ok()) ROS_INFO_STREAM("Waiting global costmap ...");
    ROS_INFO_STREAM("get global costmap");
    ROS_INFO_STREAM("recieve goal : " << goal);
    // コストマップの配列を二次元に変換
    std::vector<std::vector<int8_t>> cmap(ExpLib::Utility::mapArray1dTo2d(gCostmap_.data.data,gCostmap_.data.info));
    // ROS_INFO_STREAM("create costmap array(2d)");
    // ゴールを中心としたマップの検索窓を作る
    ExpLib::Struct::mapSearchWindow msw(goal.pose.position,gCostmap_.data.info,COSTMAP_MARGIN);
    // ゴールにコストマップが被ってないか検索
    for(int y=msw.top,ey=msw.bottom+1;y!=ey;++y){
        for(int x=msw.left,ex=msw.right+1;x!=ex;++x){
            if(cmap[x][y] > 0){
                ROS_INFO_STREAM("current goal is over the costmap !!");
                return true; //被ってたら終了
            }
        }
    }
    ROS_INFO_STREAM("this goal is ok");
    return false;
}

bool Movement::lookupCostmap(void){
    while(pose_.q.callOne(ros::WallDuration(1.0))&&ros::ok()) ROS_INFO_STREAM("Waiting pose ...");
    return lookupCostmap(pose_.data);
}

bool Movement::resetGoal(geometry_msgs::PoseStamped& goal, const geometry_msgs::PoseStamped& pose){
    // 現在のゴール地点までのパスを一定間隔だけ遡って新たな目的地にする
    // true:リセっと可能, false:リセット不可能
    //現在のパスを取得
    std::vector<geometry_msgs::PoseStamped> path;
    int count = 0;
    int PATH_LIMIT = 120;
    ros::Rate rate(0.5);
    while(!pp_.createPath(pose,goal,path) && ros::ok()){
        ROS_INFO_STREAM("Waiting path ..."); // 一生パスが作れない場合もあるので注意
        if(++count > PATH_LIMIT) return false;
        rate.sleep();
    }
    ROS_INFO_STREAM("get path");
    // パスを少し遡ったところを目的地にする
    ROS_INFO_STREAM("path size: " << path.size());
    ROS_INFO_STREAM("PATH_BACK_INTERVAL: " << PATH_BACK_INTERVAL);
    
    if(PATH_BACK_INTERVAL < path.size()){
        goal = path[path.size() - PATH_BACK_INTERVAL];
        Eigen::Vector2d vec;
        pp_.getVec(pose,goal,vec,path);
        goal.pose.orientation = ExpLib::Convert::eigenQuaToGeoQua(Eigen::Quaterniond::FromTwoVectors(Eigen::Vector3d::UnitX(),Eigen::Vector3d(vec.x(),vec.y(),0.0)));
        ROS_INFO_STREAM("reset goal");
        return true;
    }
    else{
        ROS_INFO_STREAM("Can't reset goal");
        return false;
    }
    
}

void Movement::escapeFromCostmap(const geometry_msgs::PoseStamped& pose){
    // 目標設定前に足元にコストマップあったら外に出るようにする
    // true: 脱出成功, false: 脱出不可
    // ローカルコストマップを分割して安全そうなエリアに向かって脱出

    // // コストマップ
    while(gCostmap_.q.callOne(ros::WallDuration(1.0))&&ros::ok()) ROS_INFO_STREAM("Waiting global costmap ...");
    ROS_INFO_STREAM("get global costmap");
    std::vector<std::vector<int8_t>> gMap(ExpLib::Utility::mapArray1dTo2d(gCostmap_.data.data,gCostmap_.data.info));

    ExpLib::Struct::mapSearchWindow msw(pose.pose.position,gCostmap_.data.info,ESC_MAP_X,ESC_MAP_Y);

    const int gw = (msw.right-msw.left) / DIV_X;
    const int gh = (msw.bottom-msw.top) / DIV_Y;

    struct escMap{
        Eigen::Vector2i cIndex; //中心のいんでっくす
        geometry_msgs::Pose pose;
        double risk; // コストマップの影響度
        double grad;
    };

    std::vector<std::vector<escMap>> gmm(DIV_X,std::vector<escMap>(DIV_Y));

    for(int dh=0, dhe=DIV_Y; dh!=dhe; ++dh){
        for(int dw=0, dwe=DIV_X; dw!=dwe; ++dw){
            double risk = 0;
            for(int h=msw.top+dh*gh,he=msw.top+(dh+1)*gh;h!=he;++h){
                for(int w=msw.left+dw*gw,we=msw.left+(dw+1)*gw;w!=we;++w) risk += std::abs(gMap[w][h]);
            }
            gmm[dw][dh].cIndex = Eigen::Vector2i((msw.left*2+(2*dw+1)*gw)/2,(msw.top*2+(2*dh+1)*gh)/2);
            gmm[dw][dh].pose.position = ExpLib::Utility::mapIndexToCoordinate(gmm[dw][dh].cIndex.x(),gmm[dw][dh].cIndex.y(),gCostmap_.data.info);
            // gmm[dw][dh].pose.orientation = ExpLib::Convert::eigenQuaToGeoQua(Eigen::Quaterniond::FromTwoVectors(Eigen::Vector3d::UnitX(),Eigen::Vector3d(gmm[dw][dh].pose.position.x-pose.pose.position.x,gmm[dw][dh].pose.position.y-pose.pose.position.y,0.0)));
            gmm[dw][dh].risk = risk / (gw*gh);
        }
    }

    Eigen::Vector2i mci(DIV_X/2,DIV_Y/2);
    
    for(int y=DIV_Y-1;y!=-1;--y){
        for(int x=0;x!=DIV_X;++x){
            gmm[x][y].grad = x!=mci.x()||y!=mci.y() ? gmm[x][y].risk - gmm[mci.x()][mci.y()].risk : 100;
            gmm[x][y].pose.orientation = ExpLib::Convert::eigenQuaToGeoQua(Eigen::Quaterniond::FromTwoVectors(Eigen::Vector3d::UnitX(),Eigen::Vector3d(gmm[x][y].pose.position.x-gmm[mci.x()][mci.y()].pose.position.x,gmm[x][y].pose.position.y-gmm[mci.x()][mci.y()].pose.position.y,0.0))); 
        } 
    }
    
    Eigen::Quaterniond cAng = ExpLib::Convert::geoQuaToEigenQua(pose.pose.orientation);

    double minad = DBL_MAX;
    double mingr = DBL_MAX;
    Eigen::Vector2i escIndex = mci; //回避する方向のインデックス
    for(int y=DIV_Y-1;y!=-1;--y){
        for(int x=0;x!=DIV_X;++x){
            if(gmm[x][y].grad <= mingr){
                double tempad = cAng.angularDistance(ExpLib::Convert::geoQuaToEigenQua(gmm[x][y].pose.orientation));
                if(gmm[x][y].grad == mingr){
                    if(tempad <= minad){
                        minad = tempad;
                        mingr = gmm[x][y].grad;
                        escIndex << x,y;
                    }
                }
                else{
                    minad = tempad;
                    mingr = gmm[x][y].grad;
                    escIndex << x,y;
                }   
            }
        }
    }  

    if(escIndex.x()==mci.x()&&escIndex.y()==mci.y()) ROS_WARN_STREAM("Can't avoid !!");

    //図で出力してみる	
    ROS_INFO_STREAM("position map");	
    for(int y=DIV_Y-1;y!=-1;--y){	
        std::cout << "|";	
        for(int x=0;x!=DIV_X;++x) std::cout << std::fixed << std::setprecision(2) << gmm[x][y].pose.position << "|";		
        std::cout << std::endl;	
    }	
    
    ROS_INFO_STREAM("orientation map");	
    for(int y=DIV_Y-1;y!=-1;--y){	
        std::cout << "|";	
        for(int x=0;x!=DIV_X;++x) std::cout << std::fixed << std::setprecision(2) << gmm[x][y].pose.orientation << "|";		
        std::cout << std::endl;	
    }	
    
    ROS_INFO_STREAM("risk map");	
    for(int y=DIV_Y-1;y!=-1;--y){	
        std::cout << "|";	
        for(int x=0;x!=DIV_X;++x) std::cout << std::fixed << std::setprecision(2) << gmm[x][y].risk  << "|";		
        std::cout << std::endl;	
    }	

     //勾配が小さくなっている	
    ROS_INFO_STREAM("grad map");	
    for(int y=DIV_Y-1;y!=-1;--y){	
        std::cout << "|";	
        for(int x=0;x!=DIV_X;++x) std::cout << std::fixed << std::setprecision(2) << gmm[x][y].grad << "|";
        std::cout << std::endl;	
    }

    ROS_INFO_STREAM("avoid angle map");
    std::cout << "y↑\n →\n  x" << std::endl;
    for(int y=DIV_Y-1;y!=-1;--y){
        std::cout << "|";
        for(int x=0;x!=DIV_X;++x) std::cout << (x == escIndex.x() && y == escIndex.y() ? "*" : " ") << "|";
        std::cout << std::endl;
    }

    // これの向きに合わせてから直進で抜けるまで
    // 回転部分
    // pose.pose.orientation // 現在向き
    // 回避方向 escape:true && gmm[x][y].pose.orientation
    // どこかでコストマップを見直す処理かもういちどけいさんしなおすかしたほうが良さそう

    rotationFromTo(pose.pose.orientation,gmm[escIndex.x()][escIndex.y()].pose.orientation);
    while(lookupCostmap() && ros::ok()) {
        ROS_INFO_STREAM("escape to forward");
        velocity_.pub.publish(ExpLib::Construct::msgTwist(FORWARD_VELOCITY,0));
        while(pose_.q.callOne(ros::WallDuration(1.0))&&ros::ok()) ROS_INFO_STREAM("Waiting pose ...");
        escapeFromCostmap(pose_.data);
    }
    
}

void Movement::rotationFromTo(const geometry_msgs::Quaternion& from, const geometry_msgs::Quaternion& to){
    double rotation = ExpLib::Utility::shorterRotationAngle(from,to);

    ROS_INFO_STREAM("need rotation : " << rotation);
    ROS_INFO_STREAM("from : " << ExpLib::Convert::qToYaw(from) << ", from(rad) : " << ExpLib::Convert::qToYaw(from)*180/M_PI);
    ROS_INFO_STREAM("to : " << ExpLib::Convert::qToYaw(to) << ", to(rad) : " << ExpLib::Convert::qToYaw(to)*180/M_PI);

    if(rotation>=0){
        if(rotation+ExpLib::Convert::qToYaw(from)>M_PI){
            while(ExpLib::Convert::qToYaw(pose_.data.pose.orientation) > 0 && ros::ok()){
                velocity_.pub.publish(ExpLib::Construct::msgTwist(0,ROTATION_VELOCITY));
                while(pose_.q.callOne(ros::WallDuration(1.0))&&ros::ok()) ROS_INFO_STREAM("Waiting pose ...");
                ROS_DEBUG_STREAM("pose1-1 : " << ExpLib::Convert::qToYaw(pose_.data.pose.orientation) << "pose1-1(rad) : " << ExpLib::Convert::qToYaw(pose_.data.pose.orientation)*180/M_PI);
            }
        }
        while(ExpLib::Convert::qToYaw(pose_.data.pose.orientation) < ExpLib::Convert::qToYaw(to)&& ros::ok()){
            velocity_.pub.publish(ExpLib::Construct::msgTwist(0,ROTATION_VELOCITY));
            while(pose_.q.callOne(ros::WallDuration(1.0))&&ros::ok()) ROS_INFO_STREAM("Waiting pose ...");
            ROS_DEBUG_STREAM("pose1-2 : " << ExpLib::Convert::qToYaw(pose_.data.pose.orientation) << "pose1-2(rad) : " << ExpLib::Convert::qToYaw(pose_.data.pose.orientation)*180/M_PI);
        }
    }
    else{
        if(rotation+ExpLib::Convert::qToYaw(from)<-M_PI){
            while(ExpLib::Convert::qToYaw(pose_.data.pose.orientation) < 0 && ros::ok()){
                velocity_.pub.publish(ExpLib::Construct::msgTwist(0,-ROTATION_VELOCITY));
                while(pose_.q.callOne(ros::WallDuration(1.0))&&ros::ok()) ROS_INFO_STREAM("Waiting pose ...");
                ROS_DEBUG_STREAM("pose2-1 : " << ExpLib::Convert::qToYaw(pose_.data.pose.orientation) << "pose2-1(rad) : " << ExpLib::Convert::qToYaw(pose_.data.pose.orientation)*180/M_PI);
            }
        }
        while(ExpLib::Convert::qToYaw(pose_.data.pose.orientation) > ExpLib::Convert::qToYaw(to)&& ros::ok()){
            velocity_.pub.publish(ExpLib::Construct::msgTwist(0,-ROTATION_VELOCITY));
            while(pose_.q.callOne(ros::WallDuration(1.0))&&ros::ok()) ROS_INFO_STREAM("Waiting pose ...");
            ROS_DEBUG_STREAM("pose2-2 : " << ExpLib::Convert::qToYaw(pose_.data.pose.orientation) << "pose2-2(rad) : " << ExpLib::Convert::qToYaw(pose_.data.pose.orientation)*180/M_PI);
        }
    }
}

void Movement::moveToForward(void){
    ROS_INFO_STREAM("Moving Straight");
    ROS_INFO_STREAM("previous orientation : " << previousOrientation_);

    if(pose_.q.callOne(ros::WallDuration(1))) return;

    if(lookupCostmap(pose_.data)){
        escapeFromCostmap(pose_.data);
        pose_.q.callOne(ros::WallDuration(1.0));
    } 

    if(scan_.q.callOne(ros::WallDuration(1))) return;

    double angle;
    if(forwardWallDetection(scan_.data, angle)) vfhMovement(scan_.data,false,std::move(angle));
    else if(!roadCenterDetection(scan_.data)) vfhMovement(scan_.data,true,0.0);
    
}



double Movement::isMoveable(const sensor_msgs::LaserScan& scan, double angle=0){
    // 目標の周辺がNanになってればtrueでそのまま通す
    // 安全の確認ができなければその近くで安全になるアングルに行く
    // nan もしくは障害物距離がx以上であれば安全角度判定

    int ti; //target i
    // 中心の要素番号設定
    static Eigen::Vector2i cp = scan.ranges.size()%2==0 ? Eigen::Vector2i(scan.ranges.size()/2,scan.ranges.size()/2-3) : Eigen::Vector2i(scan.ranges.size()/2,scan.ranges.size()/2);//中心の位置調整
    std::swap(cp[0],cp[1]);

    // 目標角に一番近い要素番号を計算 0 radのときは特殊処理(要素サイズが偶数の場合))
    if(angle==0) ti = cp[0];
    else{
        double min = DBL_MAX;
        for(int i=0,ie=scan.ranges.size();i!=ie;++i){
            double diff = std::abs(angle - (scan.angle_min + scan.angle_increment * i));
            if(diff < min){
                min = std::move(diff);
                ti = i;
            }
        }
    }

    // その方向が安全であるかを見る

    // ここでとりあえず正面の安全を確認(問題なければ正面に進ませる)
    // int tPLUS = ti + FORWARD_ANGLE/scan.angle_increment;
    // int tMINUS = ti - FORWARD_ANGLE/scan.angle_increment;
    // int PLUS = tPLUS > scan.ranges.size() ? scan.ranges.size() : tPLUS;
    // int MINUS = tMINUS < 0 ? 0 : tMINUS;
        
    ROS_INFO_STREAM("angle : " << angle << ", ranges.size() : " << scan.ranges.size() << ", ti : " << ti << ", ti(rad) : " << scan.angle_min + ti*scan.angle_increment);

    int count = 0;
    double SAFETY_RANGE_THRESHOLD = 0.5;
    double SAFETY_RATE_THRESHOLD = 0.1;

    // ここでrateがthreshold以下になるまでずらして計算　// plusとminusに足したり引いたりすれば良い

    int sw = 0;
    double rate = DBL_MAX;

    // これtiを動かしていけばいいのでは
    do{
        ti += sw;
        // tiをずらすごとにminusとplusを再計算
        int tPLUS = ti + FORWARD_ANGLE/scan.angle_increment;
        int tMINUS = ti - FORWARD_ANGLE/scan.angle_increment;
        int PLUS = tPLUS > scan.ranges.size() ? scan.ranges.size() : tPLUS;
        int MINUS = tMINUS < 0 ? 0 : tMINUS;

        if(tMINUS >= scan.ranges.size() || tPLUS < 0){
            ROS_INFO_STREAM("safety angle search is failed");
            return DBL_MAX;
        }

        int c = 0;
        for(int i=MINUS;i!=PLUS;++i) if(!std::isnan(scan.ranges[i])&&scan.ranges[i]<SAFETY_RANGE_THRESHOLD) ++c;
        rate = (double)c/(PLUS-MINUS);
        ROS_INFO_STREAM("ti : " << ti << "PLUS : " << PLUS << ", MINUS : " << MINUS  << ", rate : " << rate);
        sw = sw > 0 ? -sw-1 : -sw+1;
    }while(rate>SAFETY_RATE_THRESHOLD);

    ROS_INFO_STREAM("ti : " << ti <<  "angle : " << scan.angle_min + ti * scan.angle_increment << ", rate : " << rate);
    ROS_INFO_STREAM("this angle is safety");
    return ti==cp[0] ? 0 : scan.angle_min + ti * scan.angle_increment;
}

void Movement::nonGoalMove(const sensor_msgs::LaserScan& scan, bool straight, double angle){
    //vfhMovementの代わり
    // 目標アングルの周辺が大丈夫そうか見る
    if(!bumper_.q.callOne(ros::WallDuration(1)) && !bumperCollision(bumper_.data)){// 障害物に接触してないか確認
        double resultAngle = straight ? isMoveable(scan) : isMoveable(scan,angle);
        if(resultAngle == DBL_MAX){
            if(!emergencyAvoidance(scan)) recoveryRotation();
        }
        else{
            velocity_.pub.publish(velocityGenerator(resultAngle * VELOCITY_GAIN, FORWARD_VELOCITY * VELOCITY_GAIN, VFH_GAIN));
        }
    }
}

void Movement::vfhMovement(sensor_msgs::LaserScan& scan, bool straight, double angle){
    //vfhいらない？？？
    //  距離見て大丈夫そうだったらそのまま移動させる？？
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

    ROS_DEBUG_STREAM("Goal Angle : " << angle << " [rad]");

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
        else ROS_INFO_STREAM("found any obstacle on forward");
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
        ROS_INFO_STREAM("goal I : " << goalI);
    }

    approx(scan.ranges);

    const int SAFE_NUM_lag = (asin((SAFE_SPACE)/(2*SAFE_DISTANCE))) / scan.angle_increment ;

    const int SAFE_NUM = 2*atan(SAFE_SPACE/(2*SAFE_DISTANCE)) / scan.angle_increment ;    
    int start;
    int k;
    int count;
    int plus = INT_MAX;
    int minus = INT_MAX;

    ROS_INFO_STREAM("SAFE_NUM lag : " << (asin((SAFE_SPACE)/(2*SAFE_DISTANCE)))*180/M_PI << ", SAFE_NUM new: " << 2*atan(SAFE_SPACE/SAFE_DISTANCE)*180/M_PI);
    ROS_INFO_STREAM("SAFE_SPACE : " << SAFE_SPACE << ", SAFE_DISTANCE : " << SAFE_DISTANCE << ", SAFE_NUM : " << SAFE_NUM << ", SCAN_THRESHOLD : " << SCAN_THRESHOLD);

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

    ROS_DEBUG_STREAM("aveP : " << aveP << ", aveM : " << aveM <<  ", nanP : " << nanP << ", nanM : " << nanM);

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

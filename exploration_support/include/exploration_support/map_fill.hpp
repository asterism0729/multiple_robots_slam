#ifndef MAP_FILL_HPP
#define MAP_FILL_HPP

#include <exploration_libraly/struct.hpp>
#include <nav_msgs/OccupancyGrid.h>
#include <opencv2/core/utility.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <ros/ros.h>

#include <dynamic_reconfigure/server.h>
#include <exploration_support/map_fill_parameter_reconfigureConfig.h>
#include <fstream>

class MapFill
{
private:
    ExpLib::Struct::subStructSimple map_;
    ExpLib::Struct::pubStruct<nav_msgs::OccupancyGrid> mapImage_;

    double FILL_SIZE_MAX;
    double FILL_SIZE_MIN;

    ros::NodeHandle nh;
    dynamic_reconfigure::Server<exploration_support::map_fill_parameter_reconfigureConfig> server;
    dynamic_reconfigure::Server<exploration_support::map_fill_parameter_reconfigureConfig>::CallbackType cbt;
    bool OUTPUT_FILL_PARAMETERS;
    std::string FILL_PARAMETER_FILE_PATH;

    void mapCB(const nav_msgs::OccupancyGrid::ConstPtr& msg);

    void dynamicParamCallback(exploration_support::map_fill_parameter_reconfigureConfig &cfg, uint32_t level);
    void outputParams(void);
public:
    MapFill():map_("map", 1, &MapFill::mapCB, this),mapImage_("fill_map",1),nh("~/map_fill"),server(nh){

        nh.param<double>("fill_size_max",FILL_SIZE_MAX,2000);//px
        nh.param<double>("fill_size_min",FILL_SIZE_MIN,5);//px
        nh.param<bool>("output_fill_parameters",OUTPUT_FILL_PARAMETERS,true);
        nh.param<std::string>("fill_parameter_file_path",FILL_PARAMETER_FILE_PATH,"fill_last_parameters.yaml");

        cbt = boost::bind(&MapFill::dynamicParamCallback,this, _1, _2);
        server.setCallback(cbt);
    };
    ~MapFill(){if(OUTPUT_FILL_PARAMETERS) outputParams();};
};

void MapFill::dynamicParamCallback(exploration_support::map_fill_parameter_reconfigureConfig &cfg, uint32_t level){
    FILL_SIZE_MAX = cfg.fill_size_max;
    FILL_SIZE_MIN = cfg.fill_size_min;
}

void MapFill::outputParams(void){
    std::cout << "writing last parameters ... ..." << std::endl;
    std::ofstream ofs(FILL_PARAMETER_FILE_PATH);

    if(ofs) std::cout << "file open succeeded" << std::endl;
    else {
        std::cout << "file open failed" << std::endl;
        return;
    }
    ofs << "fill_size_max: " << FILL_SIZE_MAX << std::endl;
    ofs << "fill_size_min: " << FILL_SIZE_MIN << std::endl;
 }

void MapFill::mapCB(const nav_msgs::OccupancyGrid::ConstPtr& msg){
    ROS_INFO_STREAM("map input");
    //マップから画像に変換 //-1 -> 255 , 100 -> 100 , 0 -> 0
    cv::Mat image(msg->info.height,msg->info.width,CV_8UC1,const_cast<signed char*>(msg->data.data()));

    //二値化
    cv::Mat binImage;
    cv::threshold(image,binImage,254,255,cv::THRESH_BINARY);
    
    //輪郭検出
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(image,contours,CV_RETR_LIST,CV_CHAIN_APPROX_NONE);

    //輪郭の面積を計算して、面積がある範囲内の場合のみ輪郭として抽出する // 大きすぎる領域を埋めないように
    std::vector<std::vector<cv::Point>> contoursRect;
    contoursRect.reserve(contours.size());
    for(int i=0,e=contours.size();i!=e;++i){
        double area = cv::contourArea(contours[i]);
        if(FILL_SIZE_MIN <= area && area <= FILL_SIZE_MAX) contoursRect.emplace_back(contours[i]);
    }

    //輪郭から塗りつぶし用のマスク画像を作成
    cv::Mat mask(image.size(),CV_8UC1,255);
    cv::drawContours(mask,contoursRect,-1,cv::Scalar(0),-1);

    //マスクで塗りつぶし
    cv::Mat result;
    image.copyTo(result,mask);

    //画像からマップに変換
    nav_msgs::OccupancyGrid map;
    map.info = msg->info;
    map.header = msg -> header;
    map.data = result.reshape(0,1);//画像データを一行に変換
    //未知領域部分の色を塗り替え
    std::replace(map.data.begin(),map.data.end(),127,-1);

    ROS_INFO_STREAM("map_image publish");
    mapImage_.pub.publish(map);
}

#endif //MAP_FILL_HPP
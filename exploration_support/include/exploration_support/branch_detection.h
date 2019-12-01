#ifndef BRANCH_DETECTION_H
#define BRANCH_DETECTION_H

#include <memory>
#include <vector>

// 前方宣言
namespace ExpLib{
    namespace Struct{
        template<typename T>
        struct subStruct;
        template<typename T>
        struct pubStruct;
        struct subStructSimple;
    }
}
namespace geometry_msgs{
    template <class ContainerAllocator>
    struct Point_;
    typedef ::geometry_msgs::Point_<std::allocator<void>> Point;
    template <class ContainerAllocator>
    struct PoseStamped_;
    typedef ::geometry_msgs::PoseStamped_<std::allocator<void>> PoseStamped;     
}
namespace exploration_msgs{
    template <class ContainerAllocator>
    struct PointArray_;
    typedef ::exploration_msgs::PointArray_<std::allocator<void>> PointArray;
}
namespace exploration_support{
    class branch_detection_parameter_reconfigureConfig;
}
namespace dynamic_reconfigure{
    template <class ConfigType>
    class Server;
}
namespace boost{
    template<class T> 
    class shared_ptr;
}
namespace sensor_msgs{
    template <class ContainerAllocator>
    struct LaserScan_;
    typedef ::sensor_msgs::LaserScan_<std::allocator<void>> LaserScan;
    typedef boost::shared_ptr< ::sensor_msgs::LaserScan const> LaserScanConstPtr;
}
// 前方宣言ここまで

namespace ExStc = ExpLib::Struct;

class BranchDetection{
    private:
        // dynamic parameters
        double OBSTACLE_CHECK_ANGLE;
        double OBSTACLE_RANGE_THRESHOLD;
        double BRANCH_RANGE_THRESHOLD;
        double BRANCH_DIFF_X_MIN;
        double BRANCH_DIFF_X_MAX;
        double BRANCH_DIFF_Y_MIN;
        double BRANCH_DIFF_Y_MAX;

        // static parameters
        std::string BRANCH_PARAMETER_FILE_PATH;
        bool OUTPUT_BRANCH_PARAMETERS;

        // variables
        std::unique_ptr<ExStc::subStructSimple> scan_;
        std::unique_ptr<ExStc::subStruct<geometry_msgs::PoseStamped>> pose_;
        std::unique_ptr<ExStc::pubStruct<exploration_msgs::PointArray>> branch_;
        std::unique_ptr<dynamic_reconfigure::Server<exploration_support::branch_detection_parameter_reconfigureConfig>> drs_;

        // functions
        void scanCB(const sensor_msgs::LaserScanConstPtr& msg);
        void publishBranch(const std::vector<geometry_msgs::Point>& branches, const std::string& frameId);
        void loadParams(void);
        void dynamicParamsCB(exploration_support::branch_detection_parameter_reconfigureConfig &cfg, uint32_t level);
        void outputParams(void);

    public:
        BranchDetection();
        ~BranchDetection();
};

#endif // BRANCH_DETECTION_H
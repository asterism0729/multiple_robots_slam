#include <map_merging/euclidean_clustering.h>

int main (int argc, char** argv)
{
  ros::init(argc, argv, "merged_clustering_euclidean");

  //Clustering clu;
  EuclideanClustering ec;

  while(ros::ok())
  {
    ec.queueC2.callOne(ros::WallDuration(1));

    if(ec.isInput())
    {
      ec.euclideanClustering();
      ec.coloring();
      ec.ListAndCentroid();
      ec.clusterPublisher2();
    }
    else
    {
      std::cout << "not input" << '\n';
    }
    ec.resetFlag();
  }

  return 0;
}

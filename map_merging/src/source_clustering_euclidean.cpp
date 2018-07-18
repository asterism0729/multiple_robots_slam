#include <map_merging/clustering.h>

int main (int argc, char** argv)
{
  ros::init(argc, argv, "source_clustering_euclidean");

  Clustering clu;

  while(ros::ok())
  {
    clu.queueC.callOne(ros::WallDuration(1));

    if(clu.isInput())
    {
      clu.euclideanClustering();
      clu.coloring();
      clu.ListAndCentroid();
      clu.clusterPublisher();
    }
    else
    {
      std::cout << "not input" << '\n';
    }
    clu.resetFlag();
  }

  return 0;
}

#include<pcl/io/pcd_io.h>
#include<pcl/point_types.h>

int main()
{
pcl::PointCloud<pcl::PointXYZ>cloud;
//Fillintheclouddata
cloud.width=50;
cloud.height=1;
cloud.is_dense=false;
cloud.points.resize(cloud.width*cloud.height);
for(size_t i=0; i<cloud.points.size(); ++i)
{
cloud.points[i].x=1024*rand()/(32768+1.0f);
cloud.points[i].y=1024*rand()/(32768+1.0f);
cloud.points[i].z=1024*rand()/(32768+1.0f);
}
pcl::io::savePCDFileASCII("testpcd.pcd", cloud);
return(0);
}
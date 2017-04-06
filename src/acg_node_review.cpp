#include <ros/ros.h>
#include "ndt_feature/NDTGraphMsg.h"
#include "ndt_feature/ndt_feature_graph.h"
#include "ndt_feature/ndtgraph_conversion.h"
#include "auto_complete_graph/ACG.hpp"
#include "auto_complete_graph/OptimizableAutoCompleteGraph.hpp"
#include "auto_complete_graph/Basement.hpp"
#include "auto_complete_graph/BasementFull.hpp"
#include "auto_complete_graph/VisuACG.hpp"
#include "auto_complete_graph/GoodMatchings.hpp"


bool abort_f = false;

ros::Publisher map_pub_;


ros::Time timef;

std::vector<double> all_node_times;
double node_process_time = 0;
int cycles = 0;

int count = 0;

bool new_node = false;
bool was_init = false;

inline void printImages(AASS::acg::AutoCompleteGraph* oacg){
	nav_msgs::OccupancyGrid* omap_tmpt = new nav_msgs::OccupancyGrid();
	nav_msgs::OccupancyGrid::Ptr occ_outt(omap_tmpt);
	AASS::acg::ACGtoOccupancyGrid(*oacg, occ_outt);
	grid_map::GridMap gridMap({"all"});
	grid_map::GridMapRosConverter::fromOccupancyGrid(*occ_outt, "all", gridMap);
// 
// 	
// 	std::cout << "WELLL HERE IT IS : " << occ_outt->info.origin.position << " ori " << occ_outt->info.origin.orientation << std::endl << std::endl;	
// 	
	cv::Mat originalImageP;
	grid_map::GridMapCvConverter::toImage<unsigned short, 1>(gridMap, "all", CV_16UC1, 0.0, 1, originalImageP);
	std::string file_outg = "/home/malcolm/ACG_folder/ACG_RVIZ_SMALL/occupancygrid_full_";
	std::ostringstream convert;   // stream used for the conversion
	convert << oacg->getRobotNodes().size(); 
	
	std::ostringstream count_str;   // stream used for the conversion
	count_str << count; 
	file_outg = file_outg + convert.str();
	file_outg = file_outg + "nodes_";
	file_outg = file_outg + count_str.str();
	file_outg = file_outg + ".png";

	cv::imwrite(file_outg, originalImageP);
	
	nav_msgs::OccupancyGrid* omap_tmpt_partial = new nav_msgs::OccupancyGrid();
	nav_msgs::OccupancyGrid::Ptr occ_outt_partial(omap_tmpt_partial);
	AASS::acg::ACGtoOccupancyGrid(*oacg, occ_outt_partial, oacg->getRobotNodes().size() - 1);
	grid_map::GridMap gridMap_partial({"all"});
	grid_map::GridMapRosConverter::fromOccupancyGrid(*occ_outt_partial, "all", gridMap_partial);
// 
// 	
	std::cout << "WELLL HERE IT IS : " << occ_outt_partial->info.origin.position << " ori " << occ_outt_partial->info.origin.orientation << std::endl << std::endl;	
// 	
	cv::Mat originalImageP_partial;
	grid_map::GridMapCvConverter::toImage<unsigned short, 1>(gridMap_partial, "all", CV_16UC1, 0.0, 1, originalImageP_partial);
	std::string file_outg_partial = "/home/malcolm/ACG_folder/ACG_RVIZ_SMALL/occupancygrid_full_partial_";
	std::ostringstream convertg_partial;   // stream used for the conversion
	convertg_partial << oacg->getRobotNodes().size(); 
	file_outg_partial = file_outg_partial + convert.str();
	file_outg_partial = file_outg_partial + "nodes_";
	file_outg_partial = file_outg_partial + count_str.str();
	file_outg_partial = file_outg_partial + ".png";
	
// 
	cv::imwrite(file_outg_partial, originalImageP_partial);
}

inline void moveOccupancyMap(nav_msgs::OccupancyGrid &occ_grid, const Eigen::Affine3d &pose) {

  Eigen::Affine3d map_origin;
  tf::poseMsgToEigen(occ_grid.info.origin, map_origin);
  Eigen::Affine3d new_map_origin = pose*map_origin;
  tf::poseEigenToMsg(new_map_origin, occ_grid.info.origin);
}



void gotGraph(const ndt_feature::NDTGraphMsg::ConstPtr msg, AASS::acg::AutoCompleteGraph* acg, AASS::acg::VisuAutoCompleteGraph& visu){
	std::cout << "Got a new graph " << std::endl;
	
	ndt_feature::NDTFeatureGraph graph;
	
	std::string frame;
	ndt_feature::msgToNDTGraph(*msg, graph, frame);	
	

	acg->updateNDTGraph(graph);
	
	
	std::string file_out = "/home/malcolm/ACG_folder/ACG_RVIZ_SMALL/acg_";
	std::ostringstream convert;   // stream used for the conversion
	convert << graph.getNbNodes(); 
	file_out = file_out + convert.str();
	file_out = file_out + "nodes.g2o";
	acg->getGraph().save(file_out.c_str());
	
	std::cout << "saved to " << file_out << std::endl;
	
// 	exit(0);
		
}


void testMsg(const ndt_feature::NDTGraphMsg::ConstPtr msg){
	abort_f = true;
	std::string frame;
	ndt_feature::NDTFeatureGraph graph;
	ndt_feature::msgToNDTGraph(*msg, graph, frame);
	
	ndt_map::NDTMapMsg msgio;
// 			ATTENTION Frame shouldn't be fixed
	bool good = lslgeneric::toMessage(graph.getNode(0).map->map, msgio, "/world");
// 			lslgeneric::NDTMap* map_copy = new lslgeneric::NDTMap(new lslgeneric::LazyGrid(resolution));
	
	lslgeneric::NDTMap* map_copy;
	lslgeneric::LazyGrid* lz;
// 			bool good = lslgeneric::fromMessage(lz, fuser.map, m.map, frame);
	bool good2 = lslgeneric::fromMessage(lz, map_copy, msgio, frame, true);
	std::shared_ptr<lslgeneric::NDTMap> s(map_copy);
	
// 	delete map_copy;
}



void gotGraphandOptimize(const ndt_feature::NDTGraphMsg::ConstPtr msg, AASS::acg::AutoCompleteGraph* oacg, AASS::acg::VisuAutoCompleteGraph& visu){
// void gotGraphandOptimize(const ndt_feature::NDTGraphMsg::ConstPtr msg, AASS::acg::AutoCompleteGraph* oacg){
	
	
	new_node = true;
// 	abort_f = true;
	std::cout << "Got a new graph " << std::endl;
	
	ros::Time start = ros::Time::now();
	
	ndt_feature::NDTFeatureGraph graph;
	
	std::string frame;
	//ATTENTION: THE BAD GUY !
	ndt_feature::msgToNDTGraph(*msg, graph, frame);	
	

	oacg->updateNDTGraph(graph);
	
	
// 	std::string file_out = "/home/malcolm/ACG_folder/ACG_RVIZ_SMALL/oacg_before_";
	std::ostringstream convert;   // stream used for the conversion
	convert << graph.getNbNodes(); 
// 	file_out = file_out + convert.str();
// 	file_out = file_out + "nodes.g2o";
// 	oacg->getGraph().save(file_out.c_str());
// 	
// 	std::cout << "saved to " << file_out << std::endl;
	
	/*** image****/
	visu.updateRvizNoNDT();
	
	
// 	oacg->initializeOptimization();
// 	oacg->initialGuess();
	
	//Prepare the graph : marginalize + initializeOpti
	
	if(was_init == true){
		oacg->setFirst();
		oacg->prepare();
		oacg->optimize();
		count++;
	}
	
// 	printImages(oacg);
// 	std::string file_out_after = "/home/malcolm/ACG_folder/ACG_RVIZ_SMALL/oacg_after_";
// 	std::ostringstream convert_after;   // stream used for the conversion
// 	convert_after << graph.getNbNodes(); 
// 	file_out_after = file_out_after + convert.str();
// 	file_out_after = file_out_after + "nodes.g2o";
// 	oacg->getGraph().save(file_out_after.c_str());
	
// 	visu.toRviz(*oacg);
	
// 	visu.updateRviz();
// 	visu.updateRviz();
// 	

	timef = ros::Time::now();
	
	node_process_time = node_process_time + (timef - start).toSec();
	all_node_times.push_back((timef - start).toSec());
	cycles++;
	
// 	nav_msgs::OccupancyGrid omap; 
// 	lslgeneric::toOccupancyGrid(graph.getMap(), omap, 0.4, "/world");
// 	moveOccupancyMap(omap, graph.getT());
// 	
// 	map_pub_.publish(omap);
	
// 	visu.updateRviz();
	
// 	std::cout << "saved to " << file_out_after << std::endl;
	
// 	exit(0);
		
}



void initAll(AASS::acg::AutoCompleteGraph& oacg, AASS::acg::RvizPoints& initialiser, AASS::acg::PriorLoaderInterface& priorloader){
	
	std::vector<cv::Point2f> slam_pt;
	std::vector<cv::Point2f> prior_pt;
	auto match = initialiser.getMatches();
	for(auto it = match.begin() ; it != match.end() ; ++it){

		Eigen::Vector3d pose = it->getPriorNode()->estimate().toVector();
		cv::Point2f pose2d; pose2d.x = pose(0) ; pose2d.y = pose(1);
		prior_pt.push_back(pose2d);
		
		Eigen::Vector2d pose2de = it->getLandmarkNode()->estimate();
		pose2d.x = pose2de(0) ; pose2d.y = pose2de(1);
		slam_pt.push_back(pose2d);
	}
	
	priorloader.initialize(slam_pt, prior_pt);
	
	//We use the already registered points
// 	priorloader.extractCornerPrior();
	priorloader.transformOntoSLAM();
	auto graph_prior = priorloader.getGraph();	
	
	oacg.clearPrior();
	oacg.addPriorGraph(graph_prior);
	
	initialiser.clear();
	
}


int main(int argc, char **argv)
{
	ros::init(argc, argv, "auto_complete_graph_rviz_small_optimi");
	ros::Subscriber ndt_graph_sub;
    ros::NodeHandle nh("~");
	
	
	double deviation = 0;
	double angle = 0;
	double scale = 1;
	cv::Point2f center(19.5, 4.5);
	if(argc > 1){
		deviation = strtod(argv[1], NULL);
		if(argc > 2){
			angle = strtod(argv[2], NULL);
			if(argc > 3){
				scale = strtod(argv[3], NULL);
				if(argc > 5){
					center.x = strtod(argv[4], NULL);
					center.y = strtod(argv[5], NULL);
				}
			}
		}
	}

	AASS::acg::BasementFull basement(deviation, angle, scale, center);
	basement.extractCornerPrior();
	basement.transformOntoSLAM();
	auto graph_prior = basement.getGraph();
	
	//Create graph instance
	AASS::acg::AutoCompleteGraph oacg(g2o::SE2(0.2, 0.1, -0.1), "/home/malcolm/ACG_folder/param.txt");
	
	//ATTENTION
	oacg.addPriorGraph(graph_prior);	
		
	//Create initialiser
	AASS::acg::RvizPoints initialiser(nh, &oacg);
	
	
	AASS::acg::VisuAutoCompleteGraph visu(&oacg, nh);
	visu.setImageFileNameOut("/home/malcolm/ACG_folder/ACG_RVIZ_SMALL/optimization_rviz_small");
	
	ndt_graph_sub = nh.subscribe<ndt_feature::NDTGraphMsg>("/ndt_graph", 1000, boost::bind(&gotGraphandOptimize, _1, &oacg, visu));
// 	ndt_graph_sub = nh.subscribe<ndt_feature::NDTGraphMsg>("/ndt_graph", 1000, boost::bind(&testMsg, _1));
// 	ndt_graph_sub = nh.subscribe<ndt_feature::NDTGraphMsg>("/ndt_graph", 1000, boost::bind(&gotGraphandOptimize, _1, &oacg));
	map_pub_ = nh.advertise<nav_msgs::OccupancyGrid>("map_grid", 1000);
	
// 	visu.updateRvizNoNDT();
	
	timef = ros::Time::now();
	ros::Time time_begin = ros::Time::now();
	
	bool flag = true;
	while(ros::ok() && flag == true ){
// 		std::cout <<"SPIN auto_complete" << std::endl;
		ros::spinOnce();
		
		if(abort_f == true){
			return EXIT_SUCCESS;
		}
		
		if(initialiser.size() == 4 && was_init == false){
			was_init = true;
			initAll(oacg, initialiser, basement);
			visu.updateRvizNoNDT();
		}
		
// 		visu.updateRvizV2();
// 		std::cout << oacg.getLinkEdges().size()<< std::endl;
		
// 		ros::Time future = ros::Time::now();
// 		std::cout << "future " << (future - timef).toSec() << " since " << oacg.getRobotNodes().size() << ">" << 5 << " "<< new_node << "==" << true  << " "<< was_init << "==" << true << std::endl;
// 		exit(0);
		
// 		if( (future - timef).toSec() >= 10 && oacg.getRobotNodes().size() > 5 && new_node == true && was_init == true){
// // 			std::cout << "Out " << (future - timef).toSec() << " "<< (timef).toSec() << " " <<(future).toSec() <<std::endl;
// // 			exit(0);
// // 			visu.updateRvizNoNDT();	
// 		// 	oacg->initializeOptimization();
// 		// 	oacg->initialGuess();
// 			//Prepare the graph : marginalize + initializeOpti
// 			oacg.getGraph().setFirst();
// 			oacg.prepare();
// 			oacg.optimize();
// 			count++;
// // 			printImages(&oacg);
// 			
// 			std::cout << "********************************************************" << std::endl << std::endl;
// 			std::cout << "Final number of nodes : " << oacg.getGraph().vertices().size() << " time : " << (future - time_begin).toSec() << std::endl;
// 			std::cout << "Mean time for processing a node " << node_process_time/cycles <<std::endl;
// 			std::cout << "********************************************************" << std::endl << std::endl;
// // 			flag = false;
// 			
// 			visu.updateRviz();
// 						
// 			new_node = false;
// 			
// 		}	
		
		visu.updateRviz();
		
	}
	

    return 0;
}
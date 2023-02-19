#include <boost/graph/adjacency_list.hpp>
#include <iostream>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/array.hpp>
#include <boost/graph/graphviz.hpp>

int main(){
 	  enum class Kind {
		  Place = 0, Transition = 1 //Made it more expicit. 
	  };

	  struct VProp {
		  Kind kind;
		  int id;
	  };

	  //NOTE: boost::Vecs selects std::vector, but it needs a type. Which 
	  //NOTE: He just defines these things here, so that he does not have to write all of this stuff again and again. 
    using EdgeProperty = boost::property<boost::edge_weight_t, int>;
	  using GraphType = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, VProp, EdgeProperty>; //NOTE: Why do you make it bidirectional?

    GraphType g;

    boost::adjacency_list<>::vertex_descriptor v1 = add_vertex(g);
    boost::adjacency_list<>::vertex_descriptor v2 = add_vertex(g);
    boost::adjacency_list<>::vertex_descriptor v3 = add_vertex(g);
    boost::adjacency_list<>::vertex_descriptor v4 = add_vertex(g);
    boost::adjacency_list<>::vertex_descriptor v5 = add_vertex(g);
    std::pair<boost::adjacency_list<>::vertex_iterator ,boost::adjacency_list<>::vertex_iterator> vec_it = vertices(g);
 

//    for (auto it = vec_it.first; it != vec_it.second; ++it){
//        std::cout << *it << "\n";
//    }

    auto edge1 = add_edge(v1,v2,EdgeProperty(30),g);
    auto edge2 = add_edge(v2,v3,EdgeProperty(31),g);
    auto edge3 = add_edge(v3,v4,EdgeProperty(32),g);
    auto edge4 = add_edge(v4,v5,EdgeProperty(33),g);

    add_edge(6,7,EdgeProperty(34),g); // add_edge can create the vertices themselves, if they do not already exist. 
    
//    std::cout << g[v1].id << "\n";

//    boost::write_graphviz(std::cout,g);
    boost::dynamic_properties dp;
    int w = get(boost::edge_weight, g, edge1.first);
  
    auto edges_it = edges(g);

    for (auto it = edges_it.first; it != edges_it.second; ++it){
      int w = get(boost::edge_weight,g,*it);
      std::cout << w << "\n";
    }
  

    /*
    std::pair<boost::adjacency_list<>::edge_iterator,boost::adjacency_list<>::edge_iterator> edge_it = edges(g);

    std::copy(edge_it.first, edge_it.second,
    std::ostream_iterator<boost::adjacency_list<>::edge_descriptor>{
      std::cout, "\n"});

    boost::array<int, 4> distances{{0}}; // Creates an array with four 0's. 
    
    //for (auto it = distances.begin(); it != distances.end(); ++it){
    //  std::cout << *it << "\n"; 
    //}

    //boost::breadth_first_search(g, v1,boost::visitor(boost::make_bfs_visitor(
    //  boost::record_distances(distances.begin(),boost::on_tree_edge{}))));

    //boost::array<int, 4> distances{{0}}; // Creates an array with four 0's. 
    std::vector<boost::default_color_type> colors;
    auto vis = boost::visitor(boost::make_dfs_visitor(boost::default_dfs_visitor()));
    boost::depth_first_search(g,v1,vis);

    for (auto it = distances.begin(); it != distances.end(); ++it){
      std::cout << *it << "\n"; 
    }

    */
}



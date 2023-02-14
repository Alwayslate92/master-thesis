#include <boost/graph/adjacency_list.hpp>
#include <iostream>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/array.hpp>
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
	  using GraphType = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, VProp, int>; //NOTE: Why do you make it bidirectional?

    GraphType g;

    boost::adjacency_list<>::vertex_descriptor v1 = add_vertex(g);
    boost::adjacency_list<>::vertex_descriptor v2 = add_vertex(g);
    boost::adjacency_list<>::vertex_descriptor v3 = add_vertex(g);
    boost::adjacency_list<>::vertex_descriptor v4 = add_vertex(g);
    boost::adjacency_list<>::vertex_descriptor v5 = add_vertex(g);
    std::pair<boost::adjacency_list<>::vertex_iterator ,boost::adjacency_list<>::vertex_iterator> vec_it = vertices(g);
 

    for (auto it = vec_it.first; it != vec_it.second; ++it){
        std::cout << *it << "\n";
    }

    add_edge(v1,v2,g);
    add_edge(v2,v3,g);
    add_edge(v3,v4,g);
    add_edge(v4,v5,g);

    add_edge(6,7,g); // add_edge can create the vertices themselves, if they do not already exist. 
    
    std::cout << g[v1].id << "\n";
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



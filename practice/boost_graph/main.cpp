#include <boost/graph/adjacency_list.hpp>
#include <iostream>
#include <boost/graph/breadth_first_search.hpp>

int main(){
    using namespace boost;
    adjacency_list<> g;

    adjacency_list<>::vertex_descriptor v1 = add_vertex(g);
    adjacency_list<>::vertex_descriptor v2 = add_vertex(g);
    adjacency_list<>::vertex_descriptor v3 = add_vertex(g);
    adjacency_list<>::vertex_descriptor v4 = add_vertex(g);
    adjacency_list<>::vertex_descriptor v5 = add_vertex(g);
    std::pair<adjacency_list<>::vertex_iterator ,adjacency_list<>::vertex_iterator> vec_it = vertices(g);
 

    for (auto it = vec_it.first; it != vec_it.second; ++it){
        std::cout << *it << "\n";
    }

    add_edge(v1,v2,g);
    add_edge(v2,v3,g);
    add_edge(v3,v4,g);
    add_edge(v4,v5,g);

    add_edge(6,7,g); // add_edge can create the vertices themselves, if they do not already exist. 

    std::pair<adjacency_list<>::edge_iterator,adjacency_list<>::edge_iterator> edge_it = edges(g);

    std::copy(edge_it.first, edge_it.second,
    std::ostream_iterator<boost::adjacency_list<>::edge_descriptor>{
      std::cout, "\n"});
    breadth_first_search(g,v1,);
}



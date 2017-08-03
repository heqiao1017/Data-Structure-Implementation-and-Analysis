#include <string>
#include <iostream>
#include <fstream>
#include "ics46goody.hpp"
#include "array_queue.hpp"
#include "hash_graph.hpp"
#include "dijkstra.hpp"



std::string get_node_in_graph(const ics::DistGraph& g, std::string prompt, bool allow_QUIT) {
  std::string node;
  for(;;) {
    node = ics::prompt_string(prompt + " (must be in graph" + (allow_QUIT ? " or QUIT" : "") + ")");
    if ( (allow_QUIT && node == "QUIT") || g.has_node(node) )
      break;
  }
  return node;
}


int main() {
  try {
    std::ifstream in_graph;
    ics::safe_open(in_graph,"Enter graph file name","flightcost.txt");
    ics::DistGraph graph;
    graph.load(in_graph);
    std::cout<<graph;

    std::string start = get_node_in_graph(graph,"\nEnter start node",true);
    auto answer_map=ics::extended_dijkstra(graph,start);
    std::cout<<answer_map<<std::endl;
    for(;;) {
      std::string end = get_node_in_graph(graph,"\nEnter stop node",true);
      if (end == "QUIT")
        break;
      std::cout<<"Cost is "<<answer_map[end].cost<<"; path is "<<recover_path(answer_map,end)<<std::endl;
    }

  } catch (ics::IcsError& e) {
    std::cout << e.what() << std::endl;
  }

  return 0;
}

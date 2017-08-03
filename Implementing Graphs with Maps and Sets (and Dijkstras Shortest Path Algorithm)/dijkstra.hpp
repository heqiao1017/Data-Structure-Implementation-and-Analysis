#ifndef DIJKSTRA_HPP_
#define DIJKSTRA_HPP_

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>                    //Biggest int: std::numeric_limits<int>::max()
#include "array_queue.hpp"
#include "array_stack.hpp"
#include "heap_priority_queue.hpp"
#include "hash_graph.hpp"
#include "array_map.hpp"


namespace ics {


class Info {
  public:
    Info() { }

    Info(std::string a_node) : node(a_node) { }

    bool operator==(const Info &rhs) const { return cost == rhs.cost && from == rhs.from; }

    bool operator!=(const Info &rhs) const { return !(*this == rhs); }

    friend std::ostream &operator<<(std::ostream &outs, const Info &i) {
      outs << "Info[" << i.node << "," << i.cost << "," << i.from << "]";
      return outs;
    }

    //Public instance variable definitions
    std::string node = "?";
    int cost = std::numeric_limits<int>::max();
    std::string from = "?";
  };
  bool gt_info(const Info &a, const Info &b) { return a.cost < b.cost; }

  typedef ics::HashGraph<int>                  DistGraph;
  typedef ics::HeapPriorityQueue<Info, gt_info> CostPQ;
  typedef ics::ArrayMap<std::string, Info>       CostMap;
  typedef ics::pair<std::string, Info>          CostMapEntry;


//Return the final_map as specified in the lecture-note description of
//  extended Dijkstra algorithm
  CostMap extended_dijkstra(const DistGraph &g, std::string start_node) {
    CostMap info_map, answer_map;
    std::string node_name;

    for(const auto& node : g.all_nodes()){
      node_name=node.first;
      info_map.put(node_name,Info(node_name));
    }
    info_map[start_node].cost=0;

    //CostPQ info_pq(info_map);
    CostPQ info_pq;
    for(const auto& info : info_map)
      info_pq.enqueue(info.second);

    Info mini_cost_info;
    std::string min_node;
    int min_cost, temp_min_cost;
    int edge_value;

    for(;!info_map.empty();){
      mini_cost_info=info_pq.dequeue();

      if(mini_cost_info.cost==std::numeric_limits<int>::max())
        break;
      while(answer_map.has_key(mini_cost_info.node))
        mini_cost_info=info_pq.dequeue();

      min_node=mini_cost_info.node;
      min_cost=mini_cost_info.cost;

      info_map.erase(min_node);
      answer_map.put(min_node,mini_cost_info);

      auto destinations = g.out_nodes(min_node);
      for(const auto& desti : destinations){
        if(!answer_map.has_key(desti)){
          edge_value=g.edge_value(min_node,desti);
          temp_min_cost=edge_value+min_cost;
          if(temp_min_cost < info_map[desti].cost){
            info_map[desti].cost=temp_min_cost;
            info_map[desti].from=min_node;
            info_pq.enqueue(info_map[desti]);
          }
        }
      }
    }
    return answer_map;
  }


//Return a queue whose front is the start node (implicit in answer_map) and whose
//  rear is the end node
  ArrayQueue <std::string> recover_path(const CostMap &answer_map, std::string end_node) {
    ArrayQueue <std::string> result_path;
    ArrayStack < std::string> stack;

    for(;answer_map[end_node].from!="?";end_node=answer_map[end_node].from)
      stack.push(end_node);
    stack.push(end_node);

    while(!stack.empty())
      result_path.enqueue(stack.pop());

    return result_path;
  }


}

#endif /* DIJKSTRA_HPP_ */

// Submitter: qiaoh3 (He, Qiao)
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "ics46goody.hpp"
#include "array_queue.hpp"
#include "array_priority_queue.hpp"
#include "array_set.hpp"
#include "array_map.hpp"


typedef ics::ArrayQueue<std::string>                InputsQueue;
typedef ics::ArrayMap<std::string,std::string>      InputStateMap;

typedef ics::ArrayMap<std::string,InputStateMap>    FA;
typedef ics::pair<std::string,InputStateMap>        FAEntry;

bool gt_FAEntry (const FAEntry& a, const FAEntry& b)
{return a.first<b.first;}

typedef ics::ArrayPriorityQueue<FAEntry,gt_FAEntry> FAPQ;

typedef ics::pair<std::string,std::string>          Transition;
typedef ics::ArrayQueue<Transition>                 TransitionQueue;


//Read an open file describing the finite automaton (each line starts with
//  a state name followed by pairs of transitions from that state: (input
//  followed by new state, all separated by semicolons), and return a Map
//  whose keys are states and whose associated values are another Map with
//  each input in that state (keys) and the resulting state it leads to.
const FA read_fa(std::ifstream &file) {
  FA fa;
  InputStateMap inputStateMap;
  std::string line;
  while(getline(file,line)){
    std::vector<std::string> words=ics::split(line,";");
    for(int i=1;i<words.size();i++)
      if(i%2==0)
        inputStateMap[words[i-1]]=words[i];
    fa[words[0]]=inputStateMap;
  }
  file.close();
  return fa;
}


//Print a label and all the entries in the finite automaton Map, in
//  alphabetical order of the states: each line has a state, the text
//  "transitions:" and the Map of its transitions.
void print_fa(const FA& fa) {
  std::cout<<"The Finite Automaton Description\n";
  for(const FAEntry& kv : FAPQ(fa))
    std::cout<<"  "<<kv.first<<" transitions: "<<kv.second<<std::endl;
}


//Return a queue of the calculated transition pairs, based on the finite
//  automaton, initial state, and queue of inputs; each pair in the returned
//  queue is of the form: input, new state.
//The first pair contains "" as the input and the initial state.
//If any input i is illegal (does not lead to a state in the finite
//  automaton), then the last pair in the returned queue is i,"None".
TransitionQueue process(const FA& fa, std::string state, const InputsQueue& inputs) {
  TransitionQueue transitionQueue;
  Transition first_transition;
  first_transition.second=state;
  transitionQueue.enqueue(first_transition);//with empty input
  for(const std::string& input : inputs) {
    if(!fa[state].has_key(input)){
      transitionQueue.enqueue(Transition(input, "None"));
      break;
    }
    std::string new_state = fa[state][input];
    transitionQueue.enqueue(Transition(input, new_state));
    state = new_state;
  }
  return transitionQueue;
}



//Print a TransitionQueue (the result of calling the process function above)
// in a nice form.
//Print the Start state on the first line; then print each input and the
//  resulting new state (or "illegal input: terminated", if the state is
//  "None") indented on subsequent lines; on the last line, print the Stop
//  state (which may be "None").
void interpret(TransitionQueue& tq) {  //or TransitionQueue or TransitionQueue&&
  std::ostringstream os;
  std::string stop_state;
  std::cout<<"Start state = "<<tq.dequeue().second<<std::endl;
  while(!tq.empty()){
    const Transition& transition=tq.dequeue();
    if(tq.empty())
      stop_state=transition.second;
    os<<"  "<<"Input = "<<transition.first<<"; ";
    transition.second=="None" ? os<<"illegal input: simulation terminated\n" : os<<"new state = "<<transition.second<<std::endl;
    std::cout<<os.str();
    os.str("");
  }
  std::cout<<"Stop state = "<<stop_state<<std::endl;
}



//Prompt the user for a file, create a finite automaton Map, and print it.
//Prompt the user for a file containing any number of simulation descriptions
//  for the finite automaton to process, one description per line; each
//  description contains a start state followed by its inputs, all separated by
//  semicolons.
//Repeatedly read a description, print that description, put each input in a
//  Queue, process the Queue and print the results in a nice form.
int main() {
  try {
    std::ifstream text_file;
    ics::safe_open(text_file,"\nEnter some finite automaton file name ","faparity.txt");
    FA fa=read_fa(text_file);
    print_fa(fa);
    ics::safe_open(text_file,"\nEnter some file name with start-state and inputs ", "fainputparity.txt");
    std::string line;
    while(getline(text_file,line)){
      std::cout<<"\nStarting up a new simulation with description: "<<line<<std::endl;
      std::vector<std::string> inputs=ics::split(line,";");
      InputsQueue inputsQueue(inputs);
      TransitionQueue tq=process(fa,inputsQueue.dequeue(),inputsQueue);
      interpret(tq);
    }
    text_file.close();
  } catch (ics::IcsError& e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}

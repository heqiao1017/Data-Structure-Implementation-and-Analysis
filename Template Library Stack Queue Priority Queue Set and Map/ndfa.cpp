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


typedef ics::ArraySet<std::string>                     States;
typedef ics::ArrayQueue<std::string>                   InputsQueue;
typedef ics::ArrayMap<std::string,States>              InputStatesMap;

typedef ics::ArrayMap<std::string,InputStatesMap>       NDFA;
typedef ics::pair<std::string,InputStatesMap>           NDFAEntry;

bool gt_NDFAEntry (const NDFAEntry& a, const NDFAEntry& b)
{return a.first<b.first;}

typedef ics::ArrayPriorityQueue<NDFAEntry,gt_NDFAEntry> NDFAPQ;

typedef ics::pair<std::string,States>                   Transitions;
typedef ics::ArrayQueue<Transitions>                    TransitionsQueue;


//Read an open file describing the non-deterministic finite automaton (each
//  line starts with a state name followed by pairs of transitions from that
//  state: (input followed by a new state, all separated by semicolons), and
//  return a Map whose keys are states and whose associated values are another
//  Map with each input in that state (keys) and the resulting set of states it
//  can lead to.
const NDFA read_ndfa(std::ifstream &file) {
  NDFA ndfa;
  std::string line;
  while(getline(file,line)){
    std::vector<std::string> words = ics::split(line, ";");
    ndfa[words[0]];//create the empty inner map in the outer map
    for(int i=1;i<words.size();i++)
      if(i%2==0)
        ndfa[words[0]][words[i-1]].insert(words[i]);
  }
  file.close();
  return ndfa;
}


//Print a label and all the entries in the finite automaton Map, in
//  alphabetical order of the states: each line has a state, the text
//  "transitions:" and the Map of its transitions.
void print_ndfa(const NDFA& ndfa) {
  std::cout<<"The Non-Deterministic Finite Automaton Description\n";
  for(const NDFAEntry& kv : NDFAPQ(ndfa))
    std::cout<<"  "<<kv.first<<" transitions: "<<kv.second<<std::endl;
}


//Return a queue of the calculated transition pairs, based on the non-deterministic
//  finite automaton, initial state, and queue of inputs; each pair in the returned
//  queue is of the form: input, set of new states.
//The first pair contains "" as the input and the initial state.
//If any input i is illegal (does not lead to any state in the non-deterministic finite
//  automaton), ignore it.
TransitionsQueue process(const NDFA& ndfa, std::string state, const InputsQueue& inputs) {
  TransitionsQueue transitionsQueue;
  States current_state, possible_state;
  current_state.insert(state);
  transitionsQueue.enqueue(Transitions("",current_state));//put the initial state pair first
  for(const std::string input : inputs){
    for(const std::string state : current_state)
      if(ndfa[state].has_key(input))
        possible_state.insert_all(ndfa[state][input]);
    transitionsQueue.enqueue(Transitions(input,possible_state));
    current_state=possible_state;
    possible_state.clear();
  }
  return transitionsQueue;
}


//Print a TransitionsQueue (the result of calling process) in a nice form.
//Print the Start state on the first line; then print each input and the
//  resulting new states indented on subsequent lines; on the last line, print
//  the Stop state.
void interpret(TransitionsQueue& tq) {  //or TransitionsQueue or TransitionsQueue&&
  std::cout<<"Start state = "<<tq.dequeue().second<<std::endl;
  while(tq.empty()==false){
    Transitions transitions=tq.dequeue();
    std::cout<<"  Input = "<<transitions.first<<"; new possible states = "<<transitions.second<<std::endl;
    if(tq.empty())
      std::cout<<"Stop state(s) = "<<transitions.second<<std::endl;
  }
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
    ics::safe_open(text_file, "Enter some non-deterministic finite automaton file name", "ndfaendin01.txt");
    NDFA ndfa=read_ndfa(text_file);
    print_ndfa(ndfa);
    ics::safe_open(text_file, "\nEnter some file name with start-state and inputs", "ndfainputendin01.txt");
    std::string line;
    while(getline(text_file,line)){
      std::cout<<"\nStarting up new simulation with description: "<<line<<std::endl;
      std::vector<std::string> words=ics::split(line,";");
      InputsQueue inputsQueue(words);
      TransitionsQueue tq=process(ndfa,inputsQueue.dequeue(),inputsQueue);
      interpret(tq);
    }
    text_file.close();
  } catch (ics::IcsError& e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}

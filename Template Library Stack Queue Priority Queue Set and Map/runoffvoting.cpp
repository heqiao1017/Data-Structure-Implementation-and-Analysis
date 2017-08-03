// Submitter: qiaoh3 (He, Qiao)
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <limits>                    //Biggest int: std::numeric_limits<int>::max()
#include "ics46goody.hpp"
#include "array_queue.hpp"
#include "array_priority_queue.hpp"
#include "array_set.hpp"
#include "array_map.hpp"


typedef ics::ArrayQueue<std::string>              CandidateQueue;
typedef ics::ArraySet<std::string>                CandidateSet;
typedef ics::ArrayMap<std::string,int>            CandidateTally;

typedef ics::ArrayMap<std::string,CandidateQueue> Preferences;
typedef ics::pair<std::string,CandidateQueue>     PreferencesEntry;
typedef ics::ArrayPriorityQueue<PreferencesEntry> PreferencesEntryPQ; //Must supply gt at construction

typedef ics::pair<std::string,int>                TallyEntry;
typedef ics::ArrayPriorityQueue<TallyEntry>       TallyEntryPQ;



//Read an open file stating voter preferences (each line is (a) a voter
//  followed by (b) all the candidates the voter would vote for, in
//  preference order (from most to least preferred candidate, separated
//  by semicolons), and return a Map of preferences: a Map whose keys are
//  voter names and whose values are a queue of candidate preferences.
Preferences read_voter_preferences(std::ifstream &file) {
  Preferences preferences;
  std::string line;
  while(getline(file,line)){
    std::vector<std::string> words=ics::split(line,";");
    for(const std::string& word: words)
      if(word!=words[0])
        preferences[words[0]].enqueue(word);
  }
  file.close();
  return preferences;
}


//Print a label and all the entries in the preferences Map, in alphabetical
//  order according to the voter.
//Use a "->" to separate the voter name from the Queue of candidates.
void print_voter_preferences(const Preferences& preferences) {
  std::cout<<"\nVoter name -> queue[Preferences]\n";
  for(const PreferencesEntry& kv: PreferencesEntryPQ(preferences, (bool (*)(const PreferencesEntry& a, const PreferencesEntry& b))
      {[](const PreferencesEntry& a, const PreferencesEntry& b) { return a.first<b.first; }}))
    std::cout<<"  "<<kv.first<<" -> "<<kv.second<<std::endl;
}


//Print the message followed by all the entries in the CandidateTally, in
//  the order specified by has_higher_priority: i is printed before j, if
//  has_higher_priority(i,j) returns true: sometimes alphabetically by candidate,
//  other times by decreasing votes for the candidate.
//Use a "->" to separate the candidat name from the number of votes they
//  received.
void print_tally(std::string message, const CandidateTally& tally, bool (*has_higher_priority)(const TallyEntry& i,const TallyEntry& j)) {
  std::cout<<std::endl<<message<<std::endl;
  for(const TallyEntry& kv: TallyEntryPQ(tally, has_higher_priority))
    std::cout<<"  "<<kv.first<<" -> "<<kv.second<<std::endl;
}


//Return the CandidateTally: a Map of candidates (as keys) and the number of
//  votes they received, based on the unchanging Preferences (read from the
//  file) and the candidates who are currently still in the election (which changes).
//Every possible candidate should appear as a key in the resulting tally.
//Each voter should tally one vote: for their highest-ranked candidate who is
//  still in the the election.
CandidateTally evaluate_ballot(const Preferences& preferences, const CandidateSet& candidates) {
  CandidateTally candidateTally;
  for(const PreferencesEntry&kv:preferences){
    for(const std::string& element: kv.second){
      if(candidates.contains(element)){
        candidateTally[element]++;
        break;
      }
    }
  }
  return candidateTally;
}


//Return the Set of candidates who are still in the election, based on the
//  tally of votes: compute the minimum number of votes and return a Set of
//  all candidates receiving more than that minimum; if all candidates
//  receive the same number of votes (that would be the minimum), the empty
//  Set is returned.
CandidateSet remaining_candidates(const CandidateTally& tally) {
  CandidateSet candidateSet;
  int minimal_votes=std::numeric_limits<int>::max();
  for(const TallyEntry& kv:TallyEntryPQ(tally, (bool (*)(const TallyEntry& a, const TallyEntry& b)){
    [](const TallyEntry&a, const TallyEntry&b){return a.second<b.second;}}))
    kv.second<=minimal_votes? minimal_votes=kv.second : candidateSet.insert(kv.first);
  return candidateSet;
}


//Prompt the user for a file, create a voter preference Map, and print it.
//Determine the Set of all the candidates in the election, from this Map.
//Repeatedly evaluate the ballot based on the candidates (still) in the
//  election, printing the vote count (tally) two ways: with the candidates
//  (a) shown alphabetically increasing and (b) shown with the vote count
//  decreasing (candidates with equal vote counts are shown alphabetically
//  increasing); from this tally, compute which candidates remain in the
//  election: all candidates receiving more than the minimum number of votes;
//  continue this process until there are less than 2 candidates.
//Print the final result: there may 1 candidate left (the winner) or 0 left
//   (no winner).
int main() {
  try {
    std::ifstream text_file;
    ics::safe_open(text_file,"Enter some voter preferences file name","votepref1.txt");
    Preferences preferences=read_voter_preferences(text_file);
    print_voter_preferences(preferences);
    CandidateSet candidateSet=CandidateSet(preferences.begin()->second);
    CandidateTally candidateTally=evaluate_ballot(preferences,candidateSet);
    int ballot_number=1;
    while(candidateSet.size()>=2){
      std::ostringstream message_alpha_order, message_numer_order;
      message_alpha_order<<"Vote count on ballot #"<<ballot_number
                         <<": candidates (alphabetically ordered) with remaining candidates = "<<candidateSet;
      print_tally(message_alpha_order.str(),candidateTally,
                  [](const TallyEntry& a,const TallyEntry& b ){return a.first<b.first;});
      message_numer_order<<"Vote count on ballot #"<<ballot_number
                         <<": candidates (numerically ordered) with remaining candidates = "<<candidateSet;
      print_tally(message_numer_order.str(),candidateTally,
                  [](const TallyEntry& a,const TallyEntry& b ){return (a.second==b.second? a.first<b.first : a.second>b.second );});

      candidateSet=remaining_candidates(candidateTally);//update the candidate set
      candidateTally=evaluate_ballot(preferences,candidateSet);
      ballot_number++;
    }
    candidateSet.size()>0 ? std::cout<<"\nWinner is " <<candidateTally.begin()->first<<std::endl : std::cout << "\nNot any unique winner: election is a tie among all the candidates remaining on the last ballot"<<std::endl;
  } catch (ics::IcsError& e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}

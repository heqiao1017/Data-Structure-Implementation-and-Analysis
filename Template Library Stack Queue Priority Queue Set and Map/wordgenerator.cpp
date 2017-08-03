// Submitter: qiaoh3 (He, Qiao)
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <limits>                           //I used std::numeric_limits<int>::max()
#include "ics46goody.hpp"
#include "array_queue.hpp"
#include "array_priority_queue.hpp"
#include "array_set.hpp"
#include "array_map.hpp"


typedef ics::ArrayQueue<std::string>         WordQueue;
typedef ics::ArraySet<std::string>           FollowSet;
typedef ics::pair<WordQueue,FollowSet>       CorpusEntry;
typedef ics::ArrayPriorityQueue<CorpusEntry> CorpusPQ;     //Convenient to supply gt at construction
typedef ics::ArrayMap<WordQueue,FollowSet>   Corpus;


//Return a random word in the words set (use in produce_text)
std::string random_in_set(const FollowSet& words) {
  int index = ics::rand_range(1, words.size());
  int i = 0;
  for (const std::string& s : words)
    if (++i == index)
      return s;
  return "?";
}


//Read an open file of lines of words (separated by spaces) and return a
//  Corpus (Map) of each sequence (Queue) of os (Order-Statistic) words
//  associated with the Set of all words that follow them somewhere in the
//  file.
Corpus read_corpus(int os, std::ifstream &file) {
  Corpus corpus;
  std::string line;
  WordQueue wordQueue;
  while(getline(file,line)){
    std::vector<std::string> words = ics::split(line," ");
    for(const std::string& word : words){
      if(wordQueue.size()!=os)
        wordQueue.enqueue(word);
      else{
        corpus[wordQueue].insert(word);
        wordQueue.dequeue();
        wordQueue.enqueue(word);
      }
    }
  }
  file.close();
  return corpus;
}


//Print "Corpus" and all entries in the Corpus, in lexical alphabetical order
//  (with the minimum and maximum set sizes at the end).
//Use a "can be followed by any of" to separate the key word from the Set of words
//  that can follow it.

//One queue is lexically greater than another, if its first value is smaller; or if
//  its first value is the same and its second value is smaller; or if its first
//  and second values are the same and its third value is smaller...
//If any of its values is greater than the corresponding value in the other queue,
//  the first queue is not greater.
//Note that the queues sizes are the same: each stores Order-Statistic words
//Important: Use iterators for examining the queue values: DO NOT CALL DEQUEUE.

bool queue_gt(const CorpusEntry& a, const CorpusEntry& b) {
  WordQueue a_queue=a.first, b_queue=b.first;
  WordQueue::Iterator a_iterator=a_queue.begin(), b_iterator=b_queue.begin();
  for(int i=0; i<a_queue.size();i++){
    if(*a_iterator == *b_iterator){
      a_iterator++;
      b_iterator++;
      continue;
    }
    else
      return *a_iterator > *b_iterator;
  }
  return false;
}

void print_corpus(const Corpus& corpus) {
  int min=std::numeric_limits<int>::max(), max=std::numeric_limits<int>::min(), size;
  std::cout<<"\nCorpus had "<<corpus.size()<<" Entries\n";
  for(const CorpusEntry& kv : CorpusPQ(corpus, queue_gt)){
    std::cout<<"  "<<kv.first<<" -> "<<kv.second<<std::endl;
    size=kv.second.size();
    if(size>max)
      max=size;
    if(size<min)
      min=size;
  }
  std::cout<<"Corpus had "<<corpus.size()<<" Entries\nmax/min = "<<max<<"/"<<min<<std::endl;
}


//Return a Queue of words, starting with those in start and including count more
//  randomly selected words using corpus to decide which word comes next.
//If there is no word that follows the previous ones, put "None" into the queue
//  and return immediately this list (whose size is <= start.size() + count).
WordQueue produce_text(const Corpus& corpus, const WordQueue& start, int count) {
  WordQueue wordQueue_generated=start, wordQueue_key=start;
  for(int i=0; i<count; i++){
    if(!corpus.has_key(wordQueue_key)){
      wordQueue_generated.enqueue("None");
      return wordQueue_generated;
    }
    FollowSet followSet=corpus[wordQueue_key];
    std::string random_follow_word=random_in_set(followSet);
    wordQueue_key.enqueue(random_follow_word);
    wordQueue_key.dequeue();
    wordQueue_generated.enqueue(random_follow_word);
  }
  return wordQueue_generated;
}



//Prompt the user for (a) the order statistic and (b) the file storing the text.
//Read the text as a Corpus and print it appropriately.
//Prompt the user for order statistic words from the text.
//Prompt the user for number of random words to generate
//Call the above functions to solve the problem, and print the appropriate information
int main() {
  try {
    int os=ics::prompt_int("Enter some order statistic", 2);
    std::ifstream text_file;
    ics::safe_open(text_file,"Enter some file name to process", "wginput1.txt");
    Corpus corpus=read_corpus(os,text_file);
    print_corpus(corpus);
    std::cout<<"\nEnter "<<os<<" word(s) for starting\n";
    std::ostringstream ss;
    WordQueue wordQueue;
    for(int i=0; i< os; i++){
      ss<<"Enter word "<<i+1;
      std::string input=ics::prompt_string(ss.str());
      wordQueue.enqueue(input);
      ss.str("");
    }
    int random_count=ics::prompt_int("Enter # of words to generate");
    std::cout<<"Random text = "<<produce_text(corpus,wordQueue,random_count)<<std::endl;
  } catch (ics::IcsError& e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}

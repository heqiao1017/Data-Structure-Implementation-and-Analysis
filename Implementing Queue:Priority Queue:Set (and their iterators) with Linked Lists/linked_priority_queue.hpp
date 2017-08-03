#ifndef LINKED_PRIORITY_QUEUE_HPP_
#define LINKED_PRIORITY_QUEUE_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"
#include "array_stack.hpp"      //See operator <<


namespace ics {


#ifndef undefinedgtdefined
#define undefinedgtdefined
template<class T>
bool undefinedgt (const T& a, const T& b) {return false;}
#endif /* undefinedgtdefined */

//Instantiate the templated class supplying tgt(a,b): true, iff a has higher priority than b.
//If tgt is defaulted to undefinedgt in the template, then a constructor must supply cgt.
//If both tgt and cgt are supplied, then they must be the same (by ==) function.
//If neither is supplied, or both are supplied but different, TemplateFunctionError is raised.
//The (unique) non-undefinedgt value supplied by tgt/cgt is stored in the instance variable gt.
template<class T, bool (*tgt)(const T& a, const T& b) = undefinedgt<T>> class LinkedPriorityQueue {
  public:
    //Destructor/Constructors
    ~LinkedPriorityQueue();

    LinkedPriorityQueue          (bool (*cgt)(const T& a, const T& b) = undefinedgt<T>);
    LinkedPriorityQueue          (const LinkedPriorityQueue<T,tgt>& to_copy, bool (*cgt)(const T& a, const T& b) = undefinedgt<T>);
    explicit LinkedPriorityQueue (const std::initializer_list<T>& il, bool (*cgt)(const T& a, const T& b) = undefinedgt<T>);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit LinkedPriorityQueue (const Iterable& i, bool (*cgt)(const T& a, const T& b) = undefinedgt<T>);


    //Queries
    bool empty      () const;
    int  size       () const;
    T&   peek       () const;
    std::string str () const; //supplies useful debugging information; contrast to operator <<


    //Commands
    int  enqueue (const T& element);
    T    dequeue ();
    void clear   ();

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    int enqueue_all (const Iterable& i);


    //Operators
    LinkedPriorityQueue<T,tgt>& operator = (const LinkedPriorityQueue<T,tgt>& rhs);
    bool operator == (const LinkedPriorityQueue<T,tgt>& rhs) const;
    bool operator != (const LinkedPriorityQueue<T,tgt>& rhs) const;

    template<class T2, bool (*gt2)(const T2& a, const T2& b)>
    friend std::ostream& operator << (std::ostream& outs, const LinkedPriorityQueue<T2,gt2>& pq);



  private:
    class LN;

  public:
    class Iterator {
      public:
        //Private constructor called in begin/end, which are friends of LinkedPriorityQueue<T,tgt>
        ~Iterator();
        T           erase();
        std::string str  () const;
        LinkedPriorityQueue<T,tgt>::Iterator& operator ++ ();
        LinkedPriorityQueue<T,tgt>::Iterator  operator ++ (int);
        bool operator == (const LinkedPriorityQueue<T,tgt>::Iterator& rhs) const;
        bool operator != (const LinkedPriorityQueue<T,tgt>::Iterator& rhs) const;
        T& operator *  () const;
        T* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const LinkedPriorityQueue<T,tgt>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator LinkedPriorityQueue<T,tgt>::begin () const;
        friend Iterator LinkedPriorityQueue<T,tgt>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        LN*             prev;            //initialize prev to the header
        LN*             current;         //current == prev->next
        LinkedPriorityQueue<T,tgt>* ref_pq;
        int             expected_mod_count;
        bool            can_erase = true;

        //Called in friends begin/end
        Iterator(LinkedPriorityQueue<T,tgt>* iterate_over, LN* initial);
    };


    Iterator begin () const;
    Iterator end   () const;


  private:
    class LN {
      public:
        LN ()                      {}
        LN (const LN& ln)          : value(ln.value), next(ln.next){}
        LN (T v,  LN* n = nullptr) : value(v), next(n){}

        T   value;
        LN* next = nullptr;
    };


    bool (*gt) (const T& a, const T& b); // The gt used by enqueue (from template or constructor)
    LN* front     =  new LN();
    int used      =  0;                  //Cache count of nodes in linked list
    int mod_count =  0;                  //Allows sensing concurrent modification

    //Helper methods
    void delete_list(LN*& front);        //Deallocate all LNs, and set front's argument to nullptr;
};





////////////////////////////////////////////////////////////////////////////////
//
//LinkedPriorityQueue class and related definitions

//Destructor/Constructors

template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::~LinkedPriorityQueue() {
  delete_list(front);
}


template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::LinkedPriorityQueue(bool (*cgt)(const T& a, const T& b))
:gt(tgt != undefinedgt<T> ? tgt : cgt){
  if (gt == undefinedgt<T>)
    throw TemplateFunctionError("LinkedPriorityQueue::default constructor: neither specified");
  if (tgt != undefinedgt<T> && cgt != undefinedgt<T> && tgt != cgt)
    throw TemplateFunctionError("LinkedPriorityQueue::default constructor: both specified and different");
}


template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::LinkedPriorityQueue(const LinkedPriorityQueue<T,tgt>& to_copy, bool (*cgt)(const T& a, const T& b))
:gt(tgt != undefinedgt<T> ? tgt : cgt){
  if (gt == undefinedgt<T>)
    gt = to_copy.gt;//throw TemplateFunctionError("LinkedPriorityQueue::copy constructor: neither specified");
  if (tgt != undefinedgt<T> && cgt != undefinedgt<T> && tgt != cgt)
    throw TemplateFunctionError("LinkedPriorityQueue::copy constructor: both specified and different");
  if(gt==to_copy.gt){
    if(to_copy.used!=0) {
      LN *current = front->next = new LN(to_copy.front->next->value);
      for (LN *p = to_copy.front->next->next; p != nullptr; p = p->next, current = current->next)
        current->next= new LN(p->value);
    }
  }else
    for(const T& element: to_copy)
      enqueue(element);
  used=to_copy.used;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::LinkedPriorityQueue(const std::initializer_list<T>& il, bool (*cgt)(const T& a, const T& b))
:gt(tgt != undefinedgt<T> ? tgt : cgt){
  if (gt == undefinedgt<T>)
    throw TemplateFunctionError("LinkedPriorityQueue::initializer_list constructor: neither specified");
  if (tgt != undefinedgt<T> && cgt != undefinedgt<T> && tgt != cgt)
    throw TemplateFunctionError("LinkedPriorityQueue::initializer_list constructor: both specified and different");
  for(const T& element : il)
    enqueue(element);
}


template<class T, bool (*tgt)(const T& a, const T& b)>
template<class Iterable>
LinkedPriorityQueue<T,tgt>::LinkedPriorityQueue(const Iterable& i, bool (*cgt)(const T& a, const T& b))
    :gt(tgt != undefinedgt<T> ? tgt : cgt){
  if (gt == undefinedgt<T>)
    throw TemplateFunctionError("LinkedPriorityQueue::Iterable constructor: neither specified");
  if (tgt != undefinedgt<T> && cgt != undefinedgt<T> && tgt != cgt)
    throw TemplateFunctionError("LinkedPriorityQueue::Iterable constructor: both specified and different");
  for(const T& element : i)
    enqueue(element);
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class T, bool (*tgt)(const T& a, const T& b)>
bool LinkedPriorityQueue<T,tgt>::empty() const {
  return used==0;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
int LinkedPriorityQueue<T,tgt>::size() const {
  return used;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
T& LinkedPriorityQueue<T,tgt>::peek () const {
  if(empty())
    throw EmptyError("LinkedPriorityQueue::peek");
  return front->next->value;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
std::string LinkedPriorityQueue<T,tgt>::str() const {
  ArrayStack<T> temp;
  if(used!=0)
    for(LN* p=front->next;p!= nullptr;p=p->next)
      temp.push(p->value);
  std::ostringstream answer;
  answer<<"LinkedPriorityQueue[HEADER";
  while(!temp.empty())
    answer<<"->"<<temp.pop();
  answer<<"](used="<<used<<",front="<<front<<",mod_count="<<mod_count<<")";
  return answer.str();
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands

template<class T, bool (*tgt)(const T& a, const T& b)>
int LinkedPriorityQueue<T,tgt>::enqueue(const T& element) {
  //find the insert spot then insert a new node inside, need to consider insert in the beginning, middle, and the end(when did not find)
  LN* prev=front->next;
  int used_history=used;
  for(LN* p=front->next;p!= nullptr;p=p->next){
    if(gt(element,p->value)){
      if(p==front->next)
        front->next=new LN(element,p);
      else
        prev->next=new LN(element,p);
      ++used;
      break;
    }
    prev=p;
  }
  if(used==0) {
    front->next = new LN(element, front->next);
    ++used;
  }
  else if(used_history==used) {
    prev->next = new LN(element);
    ++used;
  }
// Insert the element in the front at the beginning ang shifting the value inside when comparing, easy to write but not that efficient
//  front->next=new LN(element,front->next);
//  for(LN* p=front->next;p->next!= nullptr;p=p->next){
//    if(!gt(p->value, p->next->value)){
//      p->value=p->next->value;
//      p->next->value=element;
//    }else
//      break;
//  }
  //++used;
  ++mod_count;
  return 1;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
T LinkedPriorityQueue<T,tgt>::dequeue() {
  if (this->empty())
    throw EmptyError("LinkedPriorityQueue::dequeue");
  ++mod_count;
  --used;
  LN* to_delete=front->next;
  T to_return=to_delete->value;
  front->next=front->next->next;
  delete to_delete;
  return to_return;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
void LinkedPriorityQueue<T,tgt>::clear() {
  delete_list(front);
  ++mod_count;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
template <class Iterable>
int LinkedPriorityQueue<T,tgt>::enqueue_all (const Iterable& i) {
  int count=0;
  for(const T& v:i)
    count += enqueue(v);
  return count;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>& LinkedPriorityQueue<T,tgt>::operator = (const LinkedPriorityQueue<T,tgt>& rhs) {
  if (this == &rhs)
    return *this;
  gt = rhs.gt;
  LN* source=rhs.front->next;
  LN* prev=front;
  for ( LN *p = front->next ; p != nullptr; p = p->next){
    if(source!= nullptr){
      p->value = source->value;
      source=source->next;
      prev=p;
    }else{
      while(p!= nullptr){
        LN* to_delete=p;
        prev->next=p=p->next;
        delete to_delete;
      }
      break;
    }
  }
  while(source!= nullptr){
    enqueue(source->value);
    source=source->next;
  }
  ++mod_count;
  used = rhs.used;
  return *this;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool LinkedPriorityQueue<T,tgt>::operator == (const LinkedPriorityQueue<T,tgt>& rhs) const {
  if(this==&rhs)
    return true;
  if(gt!=rhs.gt)
    return false;
  if(used!=rhs.used)
    return false;
  LN* p_lhs=front->next;
  for(LN* p_rhs=rhs.front->next;p_rhs!= nullptr;p_rhs=p_rhs->next,p_lhs=p_lhs->next)
    if(p_lhs->value!=p_rhs->value)
      return false;
  return true;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool LinkedPriorityQueue<T,tgt>::operator != (const LinkedPriorityQueue<T,tgt>& rhs) const {
  return !(*this==rhs);
}


template<class T, bool (*tgt)(const T& a, const T& b)>
std::ostream& operator << (std::ostream& outs, const LinkedPriorityQueue<T,tgt>& pq) {
  ArrayStack<T> temp;
  if(pq.used!=0)
    for(typename LinkedPriorityQueue<T,tgt>::LN* p=pq.front->next;p!= nullptr;p=p->next)
      temp.push(p->value);
  outs<<"priority_queue[";
  if(!temp.empty())
    outs<<temp.pop();
  while(!temp.empty())
    outs<<","<<temp.pop();
  outs<<"]:highest";
  return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors


template<class T, bool (*tgt)(const T& a, const T& b)>
auto LinkedPriorityQueue<T,tgt>::begin () const -> LinkedPriorityQueue<T,tgt>::Iterator {
  return Iterator(const_cast<LinkedPriorityQueue<T,tgt>*>(this),front->next);
}


template<class T, bool (*tgt)(const T& a, const T& b)>
auto LinkedPriorityQueue<T,tgt>::end () const -> LinkedPriorityQueue<T,tgt>::Iterator {
  return Iterator(const_cast<LinkedPriorityQueue<T,tgt>*>(this), nullptr);
}


////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class T, bool (*tgt)(const T& a, const T& b)>
void LinkedPriorityQueue<T,tgt>::delete_list(LN*& front) {
  while(front->next!= nullptr){
    LN* to_delete=front->next;
    front->next=front->next->next;
    delete to_delete;
    --used;
  }
}





////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::Iterator::Iterator(LinkedPriorityQueue<T,tgt>* iterate_over, LN* initial)
:prev(iterate_over->front),current(initial),ref_pq(iterate_over),expected_mod_count(ref_pq->mod_count){
}


template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::Iterator::~Iterator()
{}


template<class T, bool (*tgt)(const T& a, const T& b)>
T LinkedPriorityQueue<T,tgt>::Iterator::erase() {
  if (expected_mod_count != ref_pq->mod_count)
    throw ConcurrentModificationError("LinkedPriorityQueue::Iterator::erase");
  if (!can_erase)
    throw CannotEraseError("LinkedPriorityQueue::Iterator::erase Iterator cursor already erased");
  if (current == nullptr)
    throw CannotEraseError("LinkedPriorityQueue::Iterator::erase Iterator cursor beyond data structure");
  can_erase=false;
  T to_return = current->value;
//  if(prev==ref_pq->front){
//    current=current->next;
//    ref_pq->dequeue();
//  }
//  else{
  LN* to_delete=current;
  prev->next=current=current->next;
  delete to_delete;
  ref_pq->used--;
  ref_pq->mod_count++;
//  }
  expected_mod_count=ref_pq->mod_count;
  return to_return;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
std::string LinkedPriorityQueue<T,tgt>::Iterator::str() const {
  std::ostringstream answer;
  answer<<ref_pq->str()<<"(current=";
  current!= nullptr ? answer<<current->value : answer<<"nullptr";
  answer<<",expected_mod_count="<<expected_mod_count<<",can_erase="<<can_erase<<")";
  return answer.str();
}


template<class T, bool (*tgt)(const T& a, const T& b)>
auto LinkedPriorityQueue<T,tgt>::Iterator::operator ++ () -> LinkedPriorityQueue<T,tgt>::Iterator& {
  if (expected_mod_count != ref_pq->mod_count)
    throw ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator ++");
  if(current== nullptr)
    return *this;
  if(can_erase){
    prev=current;
    current=current->next;
  }else
    can_erase=true;
  return *this;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
auto LinkedPriorityQueue<T,tgt>::Iterator::operator ++ (int) -> LinkedPriorityQueue<T,tgt>::Iterator {
  if (expected_mod_count != ref_pq->mod_count)
    throw ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator ++");
  if(current== nullptr)
    return *this;
  Iterator to_return(*this);
  if(can_erase){
    prev=current;
    current=current->next;
  }else
    can_erase=true;
  return to_return;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool LinkedPriorityQueue<T,tgt>::Iterator::operator == (const LinkedPriorityQueue<T,tgt>::Iterator& rhs) const {
  const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
  if (rhsASI == 0)
    throw IteratorTypeError("LinkedPriorityQueue::Iterator::operator ==");
  if (expected_mod_count != ref_pq->mod_count)
    throw ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator ==");
  if (ref_pq != rhsASI->ref_pq)
    throw ComparingDifferentIteratorsError("LinkedPriorityQueue::Iterator::operator ==");
  return current==rhsASI->current;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool LinkedPriorityQueue<T,tgt>::Iterator::operator != (const LinkedPriorityQueue<T,tgt>::Iterator& rhs) const {
  const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
  if (rhsASI == 0)
    throw IteratorTypeError("LinkedPriorityQueue::Iterator::operator ==");
  if (expected_mod_count != ref_pq->mod_count)
    throw ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator ==");
  if (ref_pq != rhsASI->ref_pq)
    throw ComparingDifferentIteratorsError("LinkedPriorityQueue::Iterator::operator ==");
  return current!=rhsASI->current;
}

template<class T, bool (*tgt)(const T& a, const T& b)>
T& LinkedPriorityQueue<T,tgt>::Iterator::operator *() const {
  if(expected_mod_count!=ref_pq->mod_count)
    throw ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator *");
  if(!can_erase || current== nullptr){
    std::ostringstream where;
    where << current << " when used = " << ref_pq->used;
    throw IteratorPositionIllegal("LinkedPriorityQueue::Iterator::operator * Iterator illegal: "+where.str());
  }
  return current->value;
}

template<class T, bool (*tgt)(const T& a, const T& b)>
T* LinkedPriorityQueue<T,tgt>::Iterator::operator ->() const {
  if(expected_mod_count!=ref_pq->mod_count)
    throw ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator ->");
  if(!can_erase || current== nullptr) {
    std::ostringstream where;
    where << current << " when used = " << ref_pq->used;
    throw IteratorPositionIllegal("LinkedPriorityQueue::Iterator::operator -> Iterator illegal: " + where.str());
  }
    return current;
}


}

#endif /* LINKED_PRIORITY_QUEUE_HPP_ */

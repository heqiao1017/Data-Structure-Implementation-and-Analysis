#ifndef LINKED_QUEUE_HPP_
#define LINKED_QUEUE_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"


namespace ics {


template<class T> class LinkedQueue {
  public:
    //Destructor/Constructors
    ~LinkedQueue();

    LinkedQueue          ();
    LinkedQueue          (const LinkedQueue<T>& to_copy);
    explicit LinkedQueue (const std::initializer_list<T>& il);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit LinkedQueue (const Iterable& i);


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
    LinkedQueue<T>& operator = (const LinkedQueue<T>& rhs);
    bool operator == (const LinkedQueue<T>& rhs) const;
    bool operator != (const LinkedQueue<T>& rhs) const;

    template<class T2>
    friend std::ostream& operator << (std::ostream& outs, const LinkedQueue<T2>& q);



  private:
    class LN;

  public:
    class Iterator {
      public:
        //Private constructor called in begin/end, which are friends of LinkedQueue<T>
        ~Iterator();
        T           erase();
        std::string str  () const;
        LinkedQueue<T>::Iterator& operator ++ ();
        LinkedQueue<T>::Iterator  operator ++ (int);
        bool operator == (const LinkedQueue<T>::Iterator& rhs) const;
        bool operator != (const LinkedQueue<T>::Iterator& rhs) const;
        T& operator *  () const;
        T* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const LinkedQueue<T>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator LinkedQueue<T>::begin () const;
        friend Iterator LinkedQueue<T>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        LN*             prev = nullptr;  //if nullptr, current at front of list
        LN*             current;         //current == prev->next (if prev != nullptr)
        LinkedQueue<T>* ref_queue;
        int             expected_mod_count;
        bool            can_erase = true;

        //Called in friends begin/end
        Iterator(LinkedQueue<T>* iterate_over, LN* initial);
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


    LN* front     =  nullptr;
    LN* rear      =  nullptr;
    int used      =  0;            //Cache count of nodes in linked list
    int mod_count =  0;            //Alllows sensing concurrent modification

    //Helper methods
    void delete_list(LN*& front);  //Deallocate all LNs, and set front's argument to nullptr;
};





////////////////////////////////////////////////////////////////////////////////
//
//LinkedQueue class and related definitions

//Destructor/Constructors

template<class T>
LinkedQueue<T>::~LinkedQueue() {
  delete_list(front);
}


template<class T>
LinkedQueue<T>::LinkedQueue() {
}


template<class T>
LinkedQueue<T>::LinkedQueue(const LinkedQueue<T>& to_copy) {
  LN* temp=to_copy.front;
  if(temp!= nullptr){
    LN* front_walker=rear=front=new LN(temp->value);
    temp=temp->next;
    while(temp!= nullptr){
      rear=front_walker==front_walker->next=new LN(temp->value);
      temp=temp->next;
    }
    used=to_copy.used;
  }
}


template<class T>
LinkedQueue<T>::LinkedQueue(const std::initializer_list<T>& il) {
  for (const T& q_elem : il)
    enqueue(q_elem);
}


template<class T>
template<class Iterable>
LinkedQueue<T>::LinkedQueue(const Iterable& i) {
  for (const T& v : i)
    enqueue(v);
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class T>
bool LinkedQueue<T>::empty() const {
  return front== nullptr;
}


template<class T>
int LinkedQueue<T>::size() const {
  return used;
}


template<class T>
T& LinkedQueue<T>::peek () const {
  if(this->empty())
    throw EmptyError("LinkedQueue::peek");
  return front->value;
}


template<class T>
std::string LinkedQueue<T>::str() const {
  std::ostringstream answer;
  answer << "queue[";
  LN* walker=front;
  if(walker!= nullptr){
    answer << walker->value;
    walker=walker->next;
  }
  while(walker!= nullptr){
    answer<<"->"<<walker->value;
    walker=walker->next;
  }
  answer <<"](used="<<used<<",front="<<front<<",rear="<<rear<<",mod_count="<<mod_count<<")";
  return answer.str();
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands

template<class T>
int LinkedQueue<T>::enqueue(const T& element) {
  if(front== nullptr)
    rear=front=new LN(element);
  else
    rear=rear->next=new LN(element);
  ++used;
  ++mod_count;
  return 1;
}


template<class T>
T LinkedQueue<T>::dequeue() {
  if(front== nullptr)
    throw EmptyError("LinkedQueue::dequeue");
  LN* temp=front;
  T to_return=temp->value;
  front=front->next;
  delete temp;
  --used;
  ++mod_count;
  return to_return;
}


template<class T>
void LinkedQueue<T>::clear() {
  delete_list(front);
  rear=front= nullptr;
  used=0;
  ++mod_count;
}


template<class T>
template<class Iterable>
int LinkedQueue<T>::enqueue_all(const Iterable& i) {
  int count=0;
  for(const T& v:i)
    count+=enqueue(v);
  return count;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class T>
LinkedQueue<T>& LinkedQueue<T>::operator = (const LinkedQueue<T>& rhs) {
  if(this==&rhs)
    return *this;
  LN* lhs=front;
  for(LN* rhs_front=rhs.front;rhs_front!= nullptr;rhs_front=rhs_front->next, lhs=lhs->next){
    if(front== nullptr)
      rear=lhs=front=new LN(rhs_front->value);
    else if(lhs== nullptr){
      rear->next=new LN(rhs_front->value);
      lhs=rear=rear->next;
    }
    else{
      lhs->value=rhs_front->value;
    }
    rear=lhs;
  }
  LN* lhs_temp=lhs;
  for(;lhs!= nullptr;lhs=lhs->next){
    LN* temp=lhs;
    delete temp;
  }
  if(lhs_temp==front)
    rear=front= nullptr;
  else
    rear->next= nullptr;
  used=rhs.used;
  return *this;
}


template<class T>
bool LinkedQueue<T>::operator == (const LinkedQueue<T>& rhs) const {
  if(this==&rhs)
    return true;
  LN* lhs=front,* p=rhs.front;
  while(lhs!= nullptr && p!= nullptr){
    if(lhs->value!=p->value)
      return false;
    lhs=lhs->next;
    p=p->next;
  }
  return lhs==p;
}


template<class T>
bool LinkedQueue<T>::operator != (const LinkedQueue<T>& rhs) const {
  return !(*this == rhs);
}


template<class T>
std::ostream& operator << (std::ostream& outs, const LinkedQueue<T>& q) {
  outs << "queue[";
  typename LinkedQueue<T>::LN* walker=q.front;
  if(walker!= nullptr){
    outs << walker->value;
    walker=walker->next;
  }
  while(walker!= nullptr){
    outs<<","<<walker->value;
    walker=walker->next;
  }
  outs <<"]:rear";
  return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class T>
auto LinkedQueue<T>::begin () const -> LinkedQueue<T>::Iterator {
  return Iterator(const_cast<LinkedQueue<T>*>(this),front);
}

template<class T>
auto LinkedQueue<T>::end () const -> LinkedQueue<T>::Iterator {
  return Iterator(const_cast<LinkedQueue<T>*>(this), rear->next);
}


////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class T>
void LinkedQueue<T>::delete_list(LN*& front) {
  while(front!= nullptr){
    LN* temp=front;
    front=front->next;
    delete temp;
  }
}





////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class T>
LinkedQueue<T>::Iterator::Iterator(LinkedQueue<T>* iterate_over, LN* initial)
:current(initial), ref_queue(iterate_over),expected_mod_count(iterate_over->mod_count){
}


template<class T>
LinkedQueue<T>::Iterator::~Iterator()
{}


template<class T>
T LinkedQueue<T>::Iterator::erase() {
  if(expected_mod_count!=ref_queue->mod_count)
    throw ConcurrentModificationError("LinkedQueue::Iterator::erase");
  if(!can_erase)
    throw CannotEraseError("LinkedQueue::Iterator::erase Iterator cursor already erased");
  if(current== nullptr)
    throw CannotEraseError("LinkedQueue::Iterator::erase Iterator cursor beyond data structure");
  can_erase=false;
  T to_return=current->value;
  if(prev== nullptr){
    current=current->next;
    ref_queue->dequeue();
  }
  else{
    LN* temp=current;
    prev->next=current=current->next;
    if(current== nullptr)
      ref_queue->rear=prev;
    delete temp;
    ref_queue->used--;
    ref_queue->mod_count++;
  }
  expected_mod_count=ref_queue->mod_count;
  return to_return;
}


template<class T>
std::string LinkedQueue<T>::Iterator::str() const {
  std::ostringstream answer;
  answer<<ref_queue->str()<<"(current=";
  current!= nullptr ? answer<<current->value : answer<<"nullptr";
  answer<<",expected_mod_count="<<expected_mod_count<<",can_erase="<<can_erase<<")";
  return answer.str();
}


template<class T>
auto LinkedQueue<T>::Iterator::operator ++ () -> LinkedQueue<T>::Iterator& {
  if(expected_mod_count!=ref_queue->mod_count)
    throw ConcurrentModificationError("LinkedQueue::Iterator::operator ++");
  if(current==ref_queue->rear->next)
    return *this;
  if(can_erase) {
    prev = current;
    current = current->next;
  }
  else
    can_erase=true;
  return *this;
}


template<class T>
auto LinkedQueue<T>::Iterator::operator ++ (int) -> LinkedQueue<T>::Iterator {
  if(expected_mod_count!=ref_queue->mod_count)
    throw ConcurrentModificationError("LinkedQueue::Iterator::operator ++");
  if(current==ref_queue->rear->next)
    return *this;
  Iterator to_return(*this);
  if(can_erase){
    prev=current;
    current=current->next;
  }
  else
    can_erase=true;
  return to_return;
}


template<class T>
bool LinkedQueue<T>::Iterator::operator == (const LinkedQueue<T>::Iterator& rhs) const {
  const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
  if(rhsASI == 0)
    throw IteratorTypeError("LinkedQueue::Iterator::operator ==");
  if(expected_mod_count!=ref_queue->mod_count)
    throw ConcurrentModificationError("LinkedQueue::Iterator::operator ==");
  if(ref_queue != rhsASI->ref_queue)
    throw ComparingDifferentIteratorsError("LinkedQueue::Iterator::operator ==");

  return current == rhsASI->current;
}


template<class T>
bool LinkedQueue<T>::Iterator::operator != (const LinkedQueue<T>::Iterator& rhs) const {
  const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
  if(rhsASI == 0)
    throw IteratorTypeError("LinkedQueue::Iterator::operator ==");
  if(expected_mod_count!=ref_queue->mod_count)
    throw ConcurrentModificationError("LinkedQueue::Iterator::operator ==");
  if(ref_queue != rhsASI->ref_queue)
    throw ComparingDifferentIteratorsError("LinkedQueue::Iterator::operator ==");

  return current != rhsASI->current;
}


template<class T>
T& LinkedQueue<T>::Iterator::operator *() const {
  if(expected_mod_count!=ref_queue->mod_count)
    throw ConcurrentModificationError("LinkedQueue::Iterator::operator *");
  if(!can_erase || current== nullptr){
    std::ostringstream where;
    where << current
          << " when front = " << ref_queue->front
          << " and rear = " << ref_queue->rear;
    throw IteratorPositionIllegal("LinkedQueue::Iterator::operator * Iterator illegal: "+where.str());
  }
  return current->value;
}


template<class T>
T* LinkedQueue<T>::Iterator::operator ->() const {
  if(expected_mod_count!=ref_queue->mod_count)
    throw ConcurrentModificationError("LinkedQueue::Iterator::operator ->");
  if(!can_erase || current== nullptr){
    std::ostringstream where;
    where << current
          << " when front = " << ref_queue->front
          << " and rear = " << ref_queue->rear;
    throw IteratorPositionIllegal("LinkedQueue::Iterator::operator -> Iterator illegal: "+where.str());
  }

  return current;
}


}

#endif /* LINKED_QUEUE_HPP_ */

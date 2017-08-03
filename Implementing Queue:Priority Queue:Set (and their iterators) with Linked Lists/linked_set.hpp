#ifndef LINKED_SET_HPP_
#define LINKED_SET_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"


namespace ics {


template<class T> class LinkedSet {
  public:
    //Destructor/Constructors
    ~LinkedSet();

    LinkedSet          ();
    explicit LinkedSet (int initialLength);
    LinkedSet          (const LinkedSet<T>& to_copy);
    explicit LinkedSet (const std::initializer_list<T>& il);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit LinkedSet (const Iterable& i);


    //Queries
    bool empty      () const;
    int  size       () const;
    bool contains   (const T& element) const;
    std::string str () const; //supplies useful debugging information; contrast to operator <<

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    bool contains_all (const Iterable& i) const;


    //Commands
    int  insert (const T& element);
    int  erase  (const T& element);
    void clear  ();

    //Iterable class must support "for" loop: .begin()/.end() and prefix ++ on returned result

    template <class Iterable>
    int insert_all(const Iterable& i);

    template <class Iterable>
    int erase_all(const Iterable& i);

    template<class Iterable>
    int retain_all(const Iterable& i);


    //Operators
    LinkedSet<T>& operator = (const LinkedSet<T>& rhs);
    bool operator == (const LinkedSet<T>& rhs) const;
    bool operator != (const LinkedSet<T>& rhs) const;
    bool operator <= (const LinkedSet<T>& rhs) const;
    bool operator <  (const LinkedSet<T>& rhs) const;
    bool operator >= (const LinkedSet<T>& rhs) const;
    bool operator >  (const LinkedSet<T>& rhs) const;

    template<class T2>
    friend std::ostream& operator << (std::ostream& outs, const LinkedSet<T2>& s);



  private:
    class LN;

  public:
    class Iterator {
      public:
        //Private constructor called in begin/end, which are friends of LinkedSet<T>
        ~Iterator();
        T           erase();
        std::string str  () const;
        LinkedSet<T>::Iterator& operator ++ ();
        LinkedSet<T>::Iterator  operator ++ (int);
        bool operator == (const LinkedSet<T>::Iterator& rhs) const;
        bool operator != (const LinkedSet<T>::Iterator& rhs) const;
        T& operator *  () const;
        T* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const LinkedSet<T>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator LinkedSet<T>::begin () const;
        friend Iterator LinkedSet<T>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        LN*           current;  //if can_erase is false, this value is unusable
        LinkedSet<T>* ref_set;
        int           expected_mod_count;
        bool          can_erase = true;

        //Called in friends begin/end
        Iterator(LinkedSet<T>* iterate_over, LN* initial);
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
        LN* next   = nullptr;
    };


    LN* front     = new LN();
    LN* trailer   = front;         //Always point to the special trailer LN
    int used      =  0;            //Cache the number of values in linked list
    int mod_count = 0;             //For sensing concurrent modification

    //Helper methods
    int  erase_at   (LN* p);
    void delete_list(LN*& front);  //Deallocate all LNs (but trailer), and set front's argument to trailer;
};





////////////////////////////////////////////////////////////////////////////////
//
//LinkedSet class and related definitions

//Destructor/Constructors

template<class T>
LinkedSet<T>::~LinkedSet() {
  delete_list(front);
}


template<class T>
LinkedSet<T>::LinkedSet() {
}


template<class T>
LinkedSet<T>::LinkedSet(const LinkedSet<T>& to_copy) : used(to_copy.used) {
  for(LN* p=to_copy.front;p->next!= nullptr;p=p->next){
    trailer->value=p->value;
    trailer=trailer->next=new LN();
  }
}


template<class T>
LinkedSet<T>::LinkedSet(const std::initializer_list<T>& il) {
  for(const T& il_e:il)
    insert(il_e);
}


template<class T>
template<class Iterable>
LinkedSet<T>::LinkedSet(const Iterable& i) {
  for(const T& v : i)
    insert(v);
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class T>
bool LinkedSet<T>::empty() const {
  return used==0;
}


template<class T>
int LinkedSet<T>::size() const {
  return used;
}


template<class T>
bool LinkedSet<T>::contains (const T& element) const {
  for(LN* p=front;p->next!= nullptr;p=p->next)
    if(p->value==element)
      return true;
  return false;
}


template<class T>
std::string LinkedSet<T>::str() const {
  std::ostringstream answer;
  answer << "linked_set[";
  if(used!=0){
    answer<<front->value;
    for(LN* p=front->next;p->next!= nullptr;p=p->next)
      answer<<","<<p->value;
  }
  answer<<"TRAILER](used="<<used<<",front="<<front<<",trailer="<<trailer<<",mod_count="<<mod_count<<")";
}


template<class T>
template<class Iterable>
bool LinkedSet<T>::contains_all (const Iterable& i) const {
  for (const T& v : i)
    if (!contains(v))
      return false;
  return true;
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands


template<class T>
int LinkedSet<T>::insert(const T& element) {
  for(LN* p=front;p->next!= nullptr;p=p->next)
    if(p->value==element)
      return 0;
  trailer->value=element;
  trailer=trailer->next=new LN();
  ++used;
  ++mod_count;
  return 1;
}


template<class T>
int LinkedSet<T>::erase(const T& element) {
  for(LN* p=front;p->next!= nullptr;p=p->next)
    if(p->value==element)
      return erase_at(p);
  return 0;
}


template<class T>
void LinkedSet<T>::clear() {
  delete_list(front);
}


template<class T>
template<class Iterable>
int LinkedSet<T>::insert_all(const Iterable& i) {
  int count=0;
  for (const T& v : i)
    count += insert(v);
  return count;
}


template<class T>
template<class Iterable>
int LinkedSet<T>::erase_all(const Iterable& i) {
  int count = 0;
  for (const T& v : i)
    count += erase(v);
  return count;
}


template<class T>
template<class Iterable>
int LinkedSet<T>::retain_all(const Iterable& i) {
  LinkedSet<T> s(i);
  int count=0;
  LN* p=front;
  while(p->next!= nullptr){
    if(!s.contains(p->value)){
      erase_at(p);
      ++count;
    }
    else
      p=p->next;
  }
  return count;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class T>
LinkedSet<T>& LinkedSet<T>::operator = (const LinkedSet<T>& rhs) {
  LN* source=rhs.front;
  for(LN* p=front;p->next!= nullptr;p=p->next){
    if(source->next== nullptr) {
      p->value=source->value;
      trailer=p;
      while (p->next!= nullptr) {
        LN *to_delete = p->next;
        *p = *to_delete;
        delete to_delete;
      }
      break;
    }
    else{
      p->value=source->value;
      source=source->next;
    }
  }
  while(source->next!= nullptr){
    insert(source->value);
    source=source->next;
  }
  used=rhs.used;
  --mod_count;
  return *this;
}


template<class T>
bool LinkedSet<T>::operator == (const LinkedSet<T>& rhs) const {
  if(this==&rhs)
    return true;
  if(used!=rhs.used)
    return false;
  for(LN* p=front;p->next!= nullptr;p=p->next)
    if(!rhs.contains(p->value))
      return false;
  return true;
}


template<class T>
bool LinkedSet<T>::operator != (const LinkedSet<T>& rhs) const {
  return !(*this==rhs);
}


template<class T>
bool LinkedSet<T>::operator <= (const LinkedSet<T>& rhs) const {
  if(this==&rhs)
    return true;
  if(used > rhs.used)
    return false;
  for(LN* p=front;p->next!= nullptr;p=p->next)
    if(!rhs.contains(p->value))
      return false;
  return true;
}


template<class T>
bool LinkedSet<T>::operator < (const LinkedSet<T>& rhs) const {
  if(this==&rhs)
    return true;
  if(used >= rhs.used)
    return false;
  for(LN* p=front;p->next!= nullptr;p=p->next)
    if(!rhs.contains(p->value))
      return false;
  return true;
}


template<class T>
bool LinkedSet<T>::operator >= (const LinkedSet<T>& rhs) const {
  return rhs <= *this;
}


template<class T>
bool LinkedSet<T>::operator > (const LinkedSet<T>& rhs) const {
  return rhs < *this;
}


template<class T>
std::ostream& operator << (std::ostream& outs, const LinkedSet<T>& s) {
  outs<<"set[";
  if(!s.empty()){
    outs<<s.front->value;
    for(typename LinkedSet<T>::LN* p=s.front->next;p->next!= nullptr;p=p->next)
      outs<<","<<p->value;
  }
  outs<<"]";
  return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class T>
auto LinkedSet<T>::begin () const -> LinkedSet<T>::Iterator {
  return Iterator(const_cast<LinkedSet<T>*>(this),front);
}


template<class T>
auto LinkedSet<T>::end () const -> LinkedSet<T>::Iterator {
  return Iterator(const_cast<LinkedSet<T>*>(this),trailer);
}


////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class T>
int LinkedSet<T>::erase_at(LN* p) {
  for(LN* q=front;q->next!= nullptr;q=q->next){
    if(q->value==p->value){
      LN* to_delete=q->next;
      *q=*to_delete;
      delete to_delete;
      used--;
      mod_count++;
      if(q->next== nullptr)
        trailer=q;
      break;
    }
  }
  return 1;
}


template<class T>
void LinkedSet<T>::delete_list(LN*& front) {
  while(front->next!= nullptr){
    LN* to_delete=front;
    front=front->next;
    delete to_delete;
    --used;
  }
  ++mod_count;
}





////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class T>
LinkedSet<T>::Iterator::Iterator(LinkedSet<T>* iterate_over, LN* initial)
:current(initial),ref_set(iterate_over),expected_mod_count(iterate_over->mod_count){
}


template<class T>
LinkedSet<T>::Iterator::~Iterator()
{}


template<class T>
T LinkedSet<T>::Iterator::erase() {
  if (expected_mod_count != ref_set->mod_count)
    throw ConcurrentModificationError("LinkedSet::Iterator::erase");
  if (!can_erase)
    throw CannotEraseError("LinkedSet::Iterator::erase Iterator cursor already erased");
  if (current->next == nullptr)
    throw CannotEraseError("LinkedSet::Iterator::erase Iterator cursor beyond data structure");
  can_erase=false;
  T to_return=current->value;
  ref_set->erase_at(current);
  expected_mod_count=ref_set->mod_count;
  return to_return;
}


template<class T>
std::string LinkedSet<T>::Iterator::str() const {
  std::ostringstream answer;
  answer << ref_set->str() << "(current=" << (current->next== nullptr? "TRAILER" : current->value) << ",expected_mod_count=" << expected_mod_count << ",can_erase=" << can_erase << ")";
  return answer.str();
}


template<class T>
auto LinkedSet<T>::Iterator::operator ++ () -> LinkedSet<T>::Iterator& {
  if(expected_mod_count!=ref_set->mod_count)
    throw ConcurrentModificationError("LinkedSet::Iterator::operator ++");
  if(current->next== nullptr)
    return *this;
  if(can_erase)
    current=current->next;
  else
    can_erase=true;
  return *this;
}


template<class T>
auto LinkedSet<T>::Iterator::operator ++ (int) -> LinkedSet<T>::Iterator {
  if (expected_mod_count != ref_set->mod_count)
    throw ConcurrentModificationError("LinkedSet::Iterator::operator ++(int)");
  if (current->next== nullptr)
    return *this;
  Iterator to_return(*this);
  if (can_erase)
    current=current->next;
  else
    can_erase = true;
  return to_return;
}


template<class T>
bool LinkedSet<T>::Iterator::operator == (const LinkedSet<T>::Iterator& rhs) const {
  const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
  if (rhsASI == 0)
    throw IteratorTypeError("LinkedSet::Iterator::operator ==");
  if (expected_mod_count != ref_set->mod_count)
    throw ConcurrentModificationError("LinkedSet::Iterator::operator ==");
  if (ref_set != rhsASI->ref_set)
    throw ComparingDifferentIteratorsError("LinkedSet::Iterator::operator ==");
  return current==rhsASI->current;
}


template<class T>
bool LinkedSet<T>::Iterator::operator != (const LinkedSet<T>::Iterator& rhs) const {
  const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
  if (rhsASI == 0)
    throw IteratorTypeError("LinkedSet::Iterator::operator ==");
  if (expected_mod_count != ref_set->mod_count)
    throw ConcurrentModificationError("LinkedSet::Iterator::operator ==");
  if (ref_set != rhsASI->ref_set)
    throw ComparingDifferentIteratorsError("LinkedSet::Iterator::operator ==");
  return current!=rhsASI->current;
}


template<class T>
T& LinkedSet<T>::Iterator::operator *() const {
  if (expected_mod_count != ref_set->mod_count)
    throw ConcurrentModificationError("LinkedSet::Iterator::operator *");
  if (!can_erase || current->next== nullptr) {
    std::ostringstream where;
    where << current << " when size = " << ref_set->used;
    throw IteratorPositionIllegal("LinkedSet::Iterator::operator * Iterator illegal: "+where.str());
  }
  return current->value;
}


template<class T>
T* LinkedSet<T>::Iterator::operator ->() const {
  if (expected_mod_count != ref_set->mod_count)
    throw ConcurrentModificationError("LinkedSet::Iterator::operator *");
  if (!can_erase || current->next== nullptr) {
    std::ostringstream where;
    where << current << " when size = " << ref_set->used;
    throw IteratorPositionIllegal("LinkedSet::Iterator::operator * Iterator illegal: "+where.str());
  }
  return current;
}



}

#endif /* LINKED_SET_HPP_ */

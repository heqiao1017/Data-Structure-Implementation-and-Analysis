#ifndef HASH_MAP_HPP_
#define HASH_MAP_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"
#include "pair.hpp"


namespace ics {


#ifndef undefinedhashdefined
#define undefinedhashdefined
template<class T>
int undefinedhash (const T& a) {return 0;}
#endif /* undefinedhashdefined */

//Instantiate the templated class supplying thash(a): produces a hash value for a.
//If thash is defaulted to undefinedhash in the template, then a constructor must supply chash.
//If both thash and chash are supplied, then they must be the same (by ==) function.
//If neither is supplied, or both are supplied but different, TemplateFunctionError is raised.
//The (unique) non-undefinedhash value supplied by thash/chash is stored in the instance variable hash.
template<class KEY,class T, int (*thash)(const KEY& a) = undefinedhash<KEY>> class HashMap {
  public:
    typedef ics::pair<KEY,T>   Entry;
    typedef int (*hashfunc) (const KEY& a);

    //Destructor/Constructors
    ~HashMap ();

    HashMap          (double the_load_threshold = 1.0, int (*chash)(const KEY& a) = undefinedhash<KEY>);
    explicit HashMap (int initial_bins, double the_load_threshold = 1.0, int (*chash)(const KEY& k) = undefinedhash<KEY>);
    HashMap          (const HashMap<KEY,T,thash>& to_copy, double the_load_threshold = 1.0, int (*chash)(const KEY& a) = undefinedhash<KEY>);
    explicit HashMap (const std::initializer_list<Entry>& il, double the_load_threshold = 1.0, int (*chash)(const KEY& a) = undefinedhash<KEY>);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit HashMap (const Iterable& i, double the_load_threshold = 1.0, int (*chash)(const KEY& a) = undefinedhash<KEY>);


    //Queries
    bool empty      () const;
    int  size       () const;
    bool has_key    (const KEY& key) const;
    bool has_value  (const T& value) const;
    std::string str () const; //supplies useful debugging information; contrast to operator <<


    //Commands
    T    put   (const KEY& key, const T& value);
    T    erase (const KEY& key);
    void clear ();

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    int put_all(const Iterable& i);


    //Operators

    T&       operator [] (const KEY&);
    const T& operator [] (const KEY&) const;
    HashMap<KEY,T,thash>& operator = (const HashMap<KEY,T,thash>& rhs);
    bool operator == (const HashMap<KEY,T,thash>& rhs) const;
    bool operator != (const HashMap<KEY,T,thash>& rhs) const;

    template<class KEY2,class T2, int (*hash2)(const KEY2& a)>
    friend std::ostream& operator << (std::ostream& outs, const HashMap<KEY2,T2,hash2>& m);



  private:
    class LN;

  public:
    class Iterator {
      public:
         typedef pair<int,LN*> Cursor;

        //Private constructor called in begin/end, which are friends of HashMap<T>
        ~Iterator();
        Entry       erase();
        std::string str  () const;
        HashMap<KEY,T,thash>::Iterator& operator ++ ();
        HashMap<KEY,T,thash>::Iterator  operator ++ (int);
        bool operator == (const HashMap<KEY,T,thash>::Iterator& rhs) const;
        bool operator != (const HashMap<KEY,T,thash>::Iterator& rhs) const;
        Entry& operator *  () const;
        Entry* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const HashMap<KEY,T,thash>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator HashMap<KEY,T,thash>::begin () const;
        friend Iterator HashMap<KEY,T,thash>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        Cursor                current; //Bin Index + LN* pointer; stops if LN* == nullptr
        HashMap<KEY,T,thash>* ref_map;
        int                   expected_mod_count;
        bool                  can_erase = true;

        //Helper methods
        void advance_cursors();

        //Called in friends begin/end
        Iterator(HashMap<KEY,T,thash>* iterate_over, bool from_begin);
    };


    Iterator begin () const;
    Iterator end   () const;


  private:
    class LN {
    public:
      LN ()                         : next(nullptr){}
      LN (const LN& ln)             : value(ln.value), next(ln.next){}
      LN (Entry v, LN* n = nullptr) : value(v), next(n){}

      Entry value;
      LN*   next;
  };

  int (*hash)(const KEY& k);  //Hashing function used (from template or constructor)
  LN** map      = nullptr;    //Pointer to array of pointers: each bin stores a list with a trailer node
  double load_threshold;      //used/bins <= load_threshold
  int bins      = 1;          //# bins in array (should start >= 1 so hash_compress doesn't divide by 0)
  int used      = 0;          //Cache for number of key->value pairs in the hash table
  int mod_count = 0;          //For sensing concurrent modification


  //Helper methods
  int   hash_compress        (const KEY& key)          const;  //hash function ranged to [0,bins-1]
  LN*   find_key             (const KEY& key) const;           //Returns reference to key's node or nullptr
  LN*   copy_list            (LN*   l)                 const;  //Copy the keys/values in a bin (order irrelevant)
  LN**  copy_hash_table      (LN** ht, int bins)       const;  //Copy the bins/keys/values in ht tree (order in bins irrelevant)
    
    
    
  void  ensure_load_threshold(int new_used);                   //Reallocate if load_factor > load_threshold
  void  delete_hash_table    (LN**& ht, int bins);             //Deallocate all LN in ht (and the ht itself; ht == nullptr)
};

////////////////////////////////////////////////////////////////////////////////
//
//HashMap class and related definitions

//Destructor/Constructors

template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::~HashMap() {
  delete_hash_table(map,bins);
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::HashMap(double the_load_threshold, int (*chash)(const KEY& k))
: hash(thash != (hashfunc)undefinedhash<KEY> ? thash : chash),load_threshold(the_load_threshold){
  if (hash == (hashfunc)undefinedhash<KEY>)
    throw TemplateFunctionError("HashMap::default constructor: neither specified");
  if (thash != (hashfunc)undefinedhash<KEY> && chash != (hashfunc)undefinedhash<KEY> && thash != chash)
    throw TemplateFunctionError("HashMap::default constructor: both specified and different");

  map=new LN*[bins];
  for(int i=0;i<bins;++i)
    map[i]=new LN();
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::HashMap(int initial_bins, double the_load_threshold, int (*chash)(const KEY& k))
    : hash(thash != (hashfunc)undefinedhash<KEY> ? thash : chash),load_threshold(the_load_threshold), bins(initial_bins){
  if (hash == (hashfunc)undefinedhash<KEY>)
    throw TemplateFunctionError("HashMap::initial_bins constructor: neither specified");
  if (thash != (hashfunc)undefinedhash<KEY> && chash != (hashfunc)undefinedhash<KEY> && thash != chash)
    throw TemplateFunctionError("HashMap::initial_bins constructor: both specified and different");

  if(bins<1)
    bins=1;
  map=new LN*[bins];
  for(int i=0;i<bins;++i)
    map[i]=new LN();
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::HashMap(const HashMap<KEY,T,thash>& to_copy, double the_load_threshold, int (*chash)(const KEY& a))
    : hash(thash != (hashfunc)undefinedhash<KEY> ? thash : chash),load_threshold(the_load_threshold){
  if (hash == (hashfunc)undefinedhash<KEY>)
    hash = to_copy.hash;
    //throw TemplateFunctionError("HashMap::copy constructor: neither specified");
  if (thash != (hashfunc)undefinedhash<KEY> && chash != (hashfunc)undefinedhash<KEY> && thash != chash)
    throw TemplateFunctionError("HashMap::copy constructor: both specified and different");

  if(hash == to_copy.hash){
    bins=to_copy.bins;
    used=to_copy.used;
    map=copy_hash_table(to_copy.map,bins);
  }
  else{
    map=new LN*[bins];
    for(int i=0;i<bins;++i)
      map[i]=new LN();
    put_all(to_copy);
//    for(int i=0;i<to_copy.bins;++i)
//      for (LN *p = to_copy.map[i]; p->next != nullptr; p = p->next)
//        put(p->value.first, p->value.second);
  }
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::HashMap(const std::initializer_list<Entry>& il, double the_load_threshold, int (*chash)(const KEY& k))
    : hash(thash != (hashfunc)undefinedhash<KEY> ? thash : chash),load_threshold(the_load_threshold) {
  if (hash == (hashfunc) undefinedhash<KEY>)
    throw TemplateFunctionError("HashMap::initializer_list constructor: neither specified");
  if (thash != (hashfunc) undefinedhash<KEY> && chash != (hashfunc) undefinedhash<KEY> && thash != chash)
    throw TemplateFunctionError("HashMap::initializer_list constructor: both specified and different");

  map = new LN *[bins];
  for (int i = 0; i < bins; ++i)
    map[i] = new LN();
  put_all(il);
//  for(const auto& ile : il)
//    put(ile.first,ile.second);
}



template<class KEY,class T, int (*thash)(const KEY& a)>
template <class Iterable>
HashMap<KEY,T,thash>::HashMap(const Iterable& i, double the_load_threshold, int (*chash)(const KEY& k))
    : hash(thash != (hashfunc)undefinedhash<KEY> ? thash : chash),load_threshold(the_load_threshold){
  if (hash == (hashfunc)undefinedhash<KEY>)
    throw TemplateFunctionError("HashMap::Iterable constructor: neither specified");
  if (thash != (hashfunc)undefinedhash<KEY> && chash != (hashfunc)undefinedhash<KEY> && thash != chash)
    throw TemplateFunctionError("HashMap::Iterable constructor: both specified and different");

  map=new LN*[bins];
  for(int i=0;i<bins;++i)
    map[i]=new LN();
  put_all(i);
//  for(const auto& e : i)
//    put(e.first,e.second);
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::empty() const {
  return used == 0;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
int HashMap<KEY,T,thash>::size() const {
  return used;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::has_key (const KEY& key) const {
  return find_key(key)!= nullptr;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::has_value (const T& value) const {
  for(int i=0;i<bins;++i){
    for(LN* p=map[i];p->next!= nullptr;p=p->next){
      if(p->value.second==value)
        return true;
    }
  }
  return false;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
std::string HashMap<KEY,T,thash>::str() const {
  std::ostringstream result;
  result<<"map[";
  if(used!=0){
    for(int i=0;i<bins;++i){
      result<<"bin["<<i<<"]: ";
      for(LN* p=map[i];p->next!= nullptr;p=p->next)
        result<<p->value.first<<"->"<<p->value.second<<" -> ";
      result<<"TRAILER"<<std::endl;
    }
    result<<"(bins="<<bins<<", used="<<used<<",mod_count="<<mod_count<<")\n";
  }
  result<<"]";

  return result.str();
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands

template<class KEY,class T, int (*thash)(const KEY& a)>
T HashMap<KEY,T,thash>::put(const KEY& key, const T& value) {
  ++mod_count;
  LN* find = find_key(key);
  if(find != nullptr){
    T to_return=find->value.second;
    find->value.second=value;
    //std::cout<<str()<<std::endl;
    return to_return;
  }else{
    //std::cout<<"bin1"<<bins<<std::endl;
    ensure_load_threshold(++used);
    //std::cout<<"bin2"<<bins<<std::endl;
    int bin_index=hash_compress(key);
    //std::cout<<"bin_index= "<<bin_index<<std::endl;
    LN* front = map[bin_index];
    map[bin_index]=new LN(Entry(key,value),front);
    //std::cout<<str()<<std::endl;
    return value;
  }
}


template<class KEY,class T, int (*thash)(const KEY& a)>
T HashMap<KEY,T,thash>::erase(const KEY& key) {
  LN* to_erase = find_key(key);
  if(to_erase== nullptr){
    std::ostringstream answer;
    answer<<"HashMap::erase: key("<<key<<") not in the Map";
    throw KeyError(answer.str());
  }
  T to_return=to_erase->value.second;
  LN* to_erase_next = to_erase->next;
  *to_erase=*to_erase_next;

  delete to_erase_next;
  ++mod_count;
  --used;
  return to_return;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
void HashMap<KEY,T,thash>::clear() {
  //delete_hash_table(map,bins);
  for(int i=0;i<bins;++i){
    LN* front = map[i];
    while(front->next!= nullptr){
      LN* to_delete=front;
      front=front->next;
      delete to_delete;
    }
    map[i]=front;
  }
  used=0;
  ++mod_count;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
template<class Iterable>
int HashMap<KEY,T,thash>::put_all(const Iterable& i) {
  int count=0;
  for(const auto& e : i){
    count++;
    //std::cout<<e.first<<std::endl;
    put(e.first,e.second);
  }
  ++mod_count;
  return count;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class KEY,class T, int (*thash)(const KEY& a)>
T& HashMap<KEY,T,thash>::operator [] (const KEY& key) {
  LN* find = find_key(key);
  if(find != nullptr)
    return find->value.second;

  ensure_load_threshold(++used);
  int hash_value=hash_compress(key);
  map[hash_value]=new LN(Entry(key,T()),map[hash_value]);
  ++mod_count;
  return map[hash_value]->value.second;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
const T& HashMap<KEY,T,thash>::operator [] (const KEY& key) const {
  LN* find = find_key(key);
  if(find == nullptr){
    std::ostringstream answer;
    answer<<"HashMap::erase: key("<<key<<") not in the Map";
    throw KeyError(answer.str());
  }
  return find->value.second;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>& HashMap<KEY,T,thash>::operator = (const HashMap<KEY,T,thash>& rhs) {
  if (this == &rhs)
    return *this;

  if(hash == rhs.hash){
    delete_hash_table(map,bins);
    bins=rhs.bins;
    used=rhs.used;
    map=copy_hash_table(rhs.map,bins);
  }
  else {
    this->clear();
    //std::cout<<"before put all"<<std::endl;
    //std::cout<<rhs<<std::endl;
    put_all(rhs);
    //std::cout<<"after put all"<<std::endl;
    hash = rhs.hash;
  }

  ++mod_count;
  return *this;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::operator == (const HashMap<KEY,T,thash>& rhs) const {
  if(this==&rhs)
    return true;
//  if(hash!=rhs.hash)
//    return false;
  if(used!=rhs.size())
    return false;
//  if(bins!=rhs.bins)
//    return false;

  for(int i=0; i<bins; ++i){
    for(LN* p=map[i];p->next!= nullptr;p=p->next){
      LN* to_find = rhs.find_key(p->value.first);
      if(to_find== nullptr || to_find->value.second!=p->value.second)
        return false;
    }
  }

  return true;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::operator != (const HashMap<KEY,T,thash>& rhs) const {
  return !(*this==rhs);
}


template<class KEY,class T, int (*thash)(const KEY& a)>
std::ostream& operator << (std::ostream& outs, const HashMap<KEY,T,thash>& m) {
  outs<<"map[";
  if(m.used!=0){
    typename HashMap<KEY,T,thash>::Iterator i = m.begin();
    outs << i->first << "->" << i->second;
    ++i;
    for (/*See above*/; i != m.end(); ++i)
      outs << "," << i->first << "->" << i->second;
  }
  outs<<"]";

  return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class KEY,class T, int (*thash)(const KEY& a)>
auto HashMap<KEY,T,thash>::begin () const -> HashMap<KEY,T,thash>::Iterator {
  return Iterator(const_cast<HashMap<KEY,T,thash>*>(this),true); //from_begin = true
}


template<class KEY,class T, int (*thash)(const KEY& a)>
auto HashMap<KEY,T,thash>::end () const -> HashMap<KEY,T,thash>::Iterator {
  return Iterator(const_cast<HashMap<KEY,T,thash>*>(this),false); //from_begin = false
}


///////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class KEY,class T, int (*thash)(const KEY& a)>
int HashMap<KEY,T,thash>::hash_compress (const KEY& key) const {
  int hash_value=hash(key);
  return std::abs(hash_value)%bins;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
typename HashMap<KEY,T,thash>::LN* HashMap<KEY,T,thash>::find_key (const KEY& key) const {
  //std::cout<<"bin3"<<bins<<std::endl;
  int bin_index=hash_compress(key);
  //std::cout<<"bin4"<<bins<<std::endl;
  for(LN* p=map[bin_index];p->next!= nullptr;p=p->next)
    if(p->value.first==key)
      return p;
  return nullptr;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
typename HashMap<KEY,T,thash>::LN* HashMap<KEY,T,thash>::copy_list (LN* l) const {
  LN* front= nullptr, *rear=front;
  for(LN* p=l;p->next!= nullptr;p=p->next){
    if(front== nullptr)
      rear = front = new LN(p->value);
    else
      rear = rear->next = new LN(p->value);
  }
  if(rear!= nullptr)
    rear->next=new LN();
  else
    front=new LN();
  return front;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
typename HashMap<KEY,T,thash>::LN** HashMap<KEY,T,thash>::copy_hash_table (LN** ht, int bins) const {
  LN** result_table= new LN*[bins];
  for(int i=0;i<bins;++i)
    result_table[i]=copy_list(ht[i]);
  return result_table;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
void HashMap<KEY,T,thash>::ensure_load_threshold(int new_used) {
  double load_factor = new_used/(bins*1.0);

  if(load_factor > load_threshold){

    bins*=2;
    LN** new_table = new LN*[bins];
    int hash_value=0;

    for(int i=0;i<bins;++i)
      new_table[i]=new LN();//add trailer to each bin

    for(int i=0;i<bins/2;++i) {
      LN *p = map[i];
      for (; p->next != nullptr; ){
        LN* current=p;
        p = p->next;//update the p before the node get changed
        //std::cout<<"first: "<<current->value.first<<std::endl;
        hash_value=hash_compress(current->value.first);
        LN* old_front = new_table[hash_value];
        new_table[hash_value]=current;
        current->next=old_front;
      }
      delete p;//deallocate the old trialler
    }
    delete [] map;
    map=new_table;
  }
}


template<class KEY,class T, int (*thash)(const KEY& a)>
void HashMap<KEY,T,thash>::delete_hash_table (LN**& ht, int bins) {
  for(int i=0;i<bins;++i){
    LN* front = ht[i];
    while(front!= nullptr){
      LN* to_delete=front;
      front=front->next;
      delete to_delete;
    }
  }
  delete [] ht;
  ht= nullptr;
  bins=1;
  used=0;
}






////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions


template<class KEY,class T, int (*thash)(const KEY& a)>
void HashMap<KEY,T,thash>::Iterator::advance_cursors(){
  if(current.first!=-1 && current.second!= nullptr) {
    current.second = current.second->next;

    if (current.second->next == nullptr) {
      for (int i = current.first + 1; i < ref_map->bins; ++i) {
        LN *node = ref_map->map[i];
        if (node->next != nullptr) {
          current.first = i;
          current.second = node;
          return;
        }
      }
      current.first = -1;
      current.second = nullptr;
    }

  }
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::Iterator::Iterator(HashMap<KEY,T,thash>* iterate_over, bool from_begin)
: ref_map(iterate_over), expected_mod_count(ref_map->mod_count) {
  if(ref_map->used==0 || !from_begin) {
    current.first = -1;
    current.second = nullptr;
  }else{
    for(int i=0;i<ref_map->bins;++i) {
      if(ref_map->map[i]->next!= nullptr){
        current.first = i;
        current.second = ref_map->map[i];
        break;
      }
    }
  }
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::Iterator::~Iterator()
{}


template<class KEY,class T, int (*thash)(const KEY& a)>
auto HashMap<KEY,T,thash>::Iterator::erase() -> Entry {
  if (expected_mod_count != ref_map->mod_count)
    throw ConcurrentModificationError("HashMap::Iterator::erase");
  if (!can_erase)
    throw CannotEraseError("HashMap::Iterator::erase Iterator cursor already erased");
  if (current.first==-1 || current.second== nullptr)
    throw CannotEraseError("HashMap::Iterator::erase Iterator cursor beyond data structure");

  can_erase=false;
  Entry to_return=current.second->value;
  if(current.second->next->next== nullptr)
    advance_cursors();
  ref_map->erase(to_return.first);
  expected_mod_count = ref_map->mod_count;
  return to_return;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
std::string HashMap<KEY,T,thash>::Iterator::str() const {
  std::ostringstream answer;
  answer << ref_map->str() << "(current=[" << current.first<<","<<current.second << "],expected_mod_count=" << expected_mod_count << ",can_erase=" << can_erase << ")";
  return answer.str();
}

template<class KEY,class T, int (*thash)(const KEY& a)>
auto  HashMap<KEY,T,thash>::Iterator::operator ++ () -> HashMap<KEY,T,thash>::Iterator& {
  if (expected_mod_count != ref_map->mod_count)
    throw ConcurrentModificationError("HashMap::Iterator::operator ++");

  if(current.first==-1 && current.second== nullptr)
    return *this;

  if(can_erase)
    advance_cursors();
  else
    can_erase=true;

  return *this;

}


template<class KEY,class T, int (*thash)(const KEY& a)>
auto  HashMap<KEY,T,thash>::Iterator::operator ++ (int) -> HashMap<KEY,T,thash>::Iterator {
  if (expected_mod_count != ref_map->mod_count)
    throw ConcurrentModificationError("HashMap::Iterator::operator ++(int)");

  if(current.first==-1 && current.second== nullptr)
    return *this;

  Iterator to_return(*this);
  if(can_erase)
    advance_cursors();
  else
    can_erase=true;

  return to_return;

}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::Iterator::operator == (const HashMap<KEY,T,thash>::Iterator& rhs) const {
  const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
  if (rhsASI == 0)
    throw IteratorTypeError("HashMap::Iterator::operator ==");
  if (expected_mod_count != ref_map->mod_count)
    throw ConcurrentModificationError("HashMap::Iterator::operator ==");
  if (ref_map != rhs.ref_map)
    throw ComparingDifferentIteratorsError("HashMap::Iterator::operator ==");

  return this->current.second==rhs.current.second;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::Iterator::operator != (const HashMap<KEY,T,thash>::Iterator& rhs) const {
  const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
  if (rhsASI == 0)
    throw IteratorTypeError("HashMap::Iterator::operator ==");
  if (expected_mod_count != ref_map->mod_count)
    throw ConcurrentModificationError("HashMap::Iterator::operator ==");
  if (ref_map != rhs.ref_map)
    throw ComparingDifferentIteratorsError("HashMap::Iterator::operator ==");

  return this->current.second!=rhs.current.second;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
pair<KEY,T>& HashMap<KEY,T,thash>::Iterator::operator *() const {
  if (expected_mod_count != ref_map->mod_count)
    throw ConcurrentModificationError("HashMap::Iterator::operator *");
  if (!can_erase || current.first==-1 || current.second== nullptr)
    throw IteratorPositionIllegal("HashMap::Iterator::operator * Iterator illegal");

  return current.second->value;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
pair<KEY,T>* HashMap<KEY,T,thash>::Iterator::operator ->() const {
  if (expected_mod_count != ref_map->mod_count)
    throw ConcurrentModificationError("HashMap::Iterator::operator *");
  if (!can_erase || current.first==-1 || current.second== nullptr)
    throw IteratorPositionIllegal("HashMap::Iterator::operator * Iterator illegal");

  return &current.second->value;
}


}

#endif /* HASH_MAP_HPP_ */

#ifndef INSERTION_ORDER_MAP_H
#define INSERTION_ORDER_MAP_H

#include <unordered_map>
#include <list>

/**
 * @brief Map type where elements are ordered not by some comparison
 * operator, but by the chronological order in which they are inserted.
 * 
 * @tparam KeyT the type of the keys
 * @tparam MappedT the type of the mapped values
 * @tparam Hash the has object used to hash the keys; defaults to @c std::hash<KeyT>.
 * @tparam Allocator the allocator used to allocate values; 
 *      defaults to @c std::allocator<value_type> where @c value_type is a key-value pair.
 * 
 * For most purposes, this map behaves exactly like an @c std::unordered_map with the same
 * template parameters. The complexity of all operations is the same, albeit with a slight
 * time and space overhead. The main difference is that iterators to the map traverse it
 * in the order elements were added, as if the map were a @c std::list of key-value pairs
 * into which inserted elements are pushed back. Existing elements can be modified without 
 * modifying their place in the order, but erased elements can not be reinserted in their 
 * old locations.
 */
template< 
    typename KeyT, 
    typename MappedT, 
    class Hash = std::hash<KeyT>,
    class Allocator = std::allocator< std::pair<const KeyT, MappedT> >
>
class insertion_order_map {
public:
    using key_type                  = KeyT;
    using mapped_type               = MappedT;
    using value_type                = std::pair<const key_type, mapped_type>;
    using size_type                 = std::size_t;
    using allocator_type            = Allocator;
    using reference                 = mapped_type&;
    using const_reference           = const mapped_type&;
    using pointer                   = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer             = typename std::allocator_traits<allocator_type>::const_pointer;
    
private:
    using key_val_list = typename std::list< value_type, Allocator >;
    
public:
    using iterator                  = typename key_val_list::iterator;
    using const_iterator            = typename key_val_list::const_iterator;
    using reverse_iterator          = typename key_val_list::reverse_iterator;
    using const_reverse_iterator    = typename key_val_list::const_reverse_iterator;
    
private:
    using key_iter_map = typename std::unordered_map< key_type, iterator >;
    
private:
    key_val_list elements;
    key_iter_map map;
    
public:
    /* Constructors, destructors and assignment */
    insertion_order_map(const insertion_order_map&) = default;
    insertion_order_map(insertion_order_map&&) = default;
    
    ~insertion_order_map() = default;
    
    explicit insertion_order_map(const allocator_type& a = allocator_type()) 
    : elements(a), map() {};
    
    insertion_order_map(std::initializer_list<value_type> il, const allocator_type& a = allocator_type()) 
    : elements(il, a), map(){
        for(iterator it = elements.begin(); it != elements.end(); ++it)
            map[ it->first ] = it;
    }
    template <typename FwdIter>
    insertion_order_map(FwdIter first, FwdIter last, const allocator_type& a = allocator_type()) 
    : elements(first, last, a), map(){
        for(iterator it = elements.begin(); it != elements.end(); ++it)
            map[ it->first ] = it;
    }

    insertion_order_map& operator= (const insertion_order_map&) = default;
    insertion_order_map& operator= (insertion_order_map&&) = default;
    
    /* Selectors */
    const_iterator find(const key_type& key) const {
        auto where = map.find(key);
        if(where != map.cend())
            return where->second;
        else
            return cend();
    }
    size_type size() const {
        return elements.size();         //equivalently, map.size()
    }
    size_type max_size() const {
        return std::min( elements.max_size(), map.max_size() );
    }
    bool empty() const {
        return elements.empty();        //equivalently, map.empty()
    }
    const_reference at(const key_type& key) const {
        return *( map.at(key).second );
    }
    
    allocator_type get_allocator()  {
        return elements.get_allocator();
    }

    /* Mutators */
    iterator find(const key_type& key){
        auto where = map.find(key);
        if(where != map.end())
            return where->second;
        else
            return end();
    }
    reference at(const key_type& key){
        return *( map.at(key).second );
    }
    
    std::pair<iterator, bool> insert(const value_type& value){
        auto where = map.find(value.first);
        if(where != map.end()){
            return std::make_pair(where, false);
        }
        else{
            //create new element in back
            elements.push_back( value );
            return (map[value.first] = --elements.end())->second;
        }         
    }
    std::pair<iterator, bool> insert(value_type&& value){
        auto where = map.find(value.first);
        if(where != map.end()){
            return std::make_pair(where, false);
        }
        else{
            //create new element in back
            elements.push_back( value );
            return (map[value.first] = --elements.end())->second;
        }         
    }
    template< typename InputIt >
    void insert(InputIt first, InputIt last){
        for(; first != last; ++first){
            auto where = map.find( first->first );
            if(where == map.end()){
                elements.push_back( *first );
                map[first->first] = --elements.end();
            }
        }
    }
    void insert(std::initializer_list<value_type> ilist){
        for(value_type& value : ilist){
            auto where = map.find( value.first );
            if(where == map.end()){
                elements.push_back( value );
                map[value.first] = --elements.end();
            }
        }
    }
    template< class... Args >
    std::pair<iterator, bool> emplace( Args&&... args ){
        auto& it_bool = elements.emplace(args...);
        if(it_bool.second)
            map[it_bool.first->first] = it_bool.first;
        
        return it_bool;
    }
    
    //Hinted insertion is pointless, but included for compatibility
    iterator insert(const_iterator hint, const value_type& value){  return insert(value).first; }
    iterator insert(const_iterator hint, value_type&& value){       return insert(value).first; }
    template <class... Args>
    iterator emplace_hint( const_iterator hint, Args&&... args ){   return emplace(args...).first;}
    
    reference operator[] (const key_type& key){
        auto where = map.find(key);
        if(where != map.end()){
            //where = {key, iterator to {key, val}} so the expression below is val
            return where->second->second;
        }
        else{
            //create new element in back
            elements.push_back( value_type(key, mapped_type()) );
            map[key] = --elements.end();
            return elements.back().second;
        }           
    }
    
    size_type erase(const key_type& key){
        auto where = map.find(key);
        if(where != map.end()){
            elements.erase(where->second);
            map.erase(where);
            return 1;
        }
        else
            return 0;        
    }
    iterator erase(iterator pos){
        map.erase(pos->first);
        return elements.erase(pos);
    }
    iterator erase(iterator first, iterator last){
        for(; first != last; first = elements.erase(first))
            map.erase(first->first);
        return first;
    }
    
    void clear(){
        elements.clear();
        map.clear();
    }
    
    iterator begin()                { return elements.begin();      }
    iterator end()                  { return elements.end();        }
    const_iterator begin()  const   { return elements.begin();      }
    const_iterator end()    const   { return elements.end();        }
    const_iterator cbegin() const   { return elements.cbegin();     }
    const_iterator cend()   const   { return elements.cend();       }
    reverse_iterator rbegin()                { return elements.rbegin();      }
    reverse_iterator rend()                  { return elements.rend();        }
    const_reverse_iterator rbegin()  const   { return elements.rbegin();      }
    const_reverse_iterator rend()    const   { return elements.rend();        }
    const_reverse_iterator crbegin() const   { return elements.crbegin();     }
    const_reverse_iterator crend()   const   { return elements.crend();       }
    
};

#endif

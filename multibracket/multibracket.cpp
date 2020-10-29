#include <iostream>
#include <regex>
#include <string>
#include <functional>
#include <algorithm>
#include <list>
#include <cstring>
#include <unordered_map>
#include <memory>

//Multibracket tag (special symbol output by FORM macro)
#define MULTIBRACKET "       + [_MULTIBRACKET_]"

using list = typename std::list<std::string>;

/*
 * A simple ordered map type with the feature that elements are ordered
 * not by some comparison function, but by the order in which they were
 * inserted.
 * 
 * This is by no means an STL-compliant container; I have just implemented
 * the minimal functionality ad-hoc to allow the function to preserve
 * FORM's ordering conventions, which are based on information not present
 * in the output.
 * 
 * (Sometime in the future, it would maybe be fun to extend it to a full
 * STL-compliant container. It wouldn't be too hard; it's mainly a matter
 * of writing all the boilerplate.)
 */
template< typename key_type, typename val_type, class Hash = std::hash<key_type> >
class insertion_order_map {
private:
    
    /* 
     * The structure is a val_type of key-value pairs, where elements are
     * added to the end, except for those with empty keys, which are
     * added to the beginning. Map-ness is implemented via a map
     * from keys to key-value-val_type iterators, which act as indices
     * to the corresponding location in the val_type (i.e. they are never
     * iterated). The benefit is that iterator lookup is constant-time
     * in a std::list, and that they are not invalidated by changes to the
     * val_type.
     */ 
    using key_val_list = typename std::list< std::pair<key_type, val_type> >;
    key_val_list elements;
    std::unordered_map<key_type, typename key_val_list::iterator, Hash> map;
    
public:
    
    /* 
     * These are the methods needed for the functionality of this
     * program. They act exactly like the corresponding methods
     * of STL map types.
     */
    
    val_type& operator[] (const key_type& key){
        auto where = map.find(key);
        if(where != map.end()){
            //where = {key, iterator to {key, val}} so the expression below is val
            return where->second->second;
        }
        else if(key.empty()){
            //create new element in front
            elements.push_front( std::make_pair(key, val_type()) );
            return (map[key] = elements.begin())->second;
        }
        else{
            //create new element in back
            elements.push_back( std::make_pair(key, val_type()) );
            return (map[key] = --elements.end())->second;
        }           
    }
    
    size_t size() const {
        return elements.size();
    }
    bool empty() const {
        return elements.empty();
    }
    void clear() {
        elements.clear();
        map.clear();
    }
    
    typename key_val_list::iterator find(const key_type& key){
        //where already points to the desired iterator; we just have to wrap "end"
        auto where = map.find(key);
        if(where != map.end())
            return where->second;
        else
            return end();
    }
    typename key_val_list::iterator begin() {
        return elements.begin();
    }
    typename key_val_list::iterator end() {
        return elements.end();
    }
    typename key_val_list::const_iterator begin() const {
        return elements.begin();
    }
    typename key_val_list::const_iterator end() const {
        return elements.end();
    }
    typename key_val_list::const_iterator cbegin() const {
        return elements.cbegin();
    }
    typename key_val_list::const_iterator cend() const {
        return elements.cend();
    }
};

class indent_buf : public std::stringbuf {
private:
    static const size_t max_depth = 79;
    static const size_t par_indent = 6;
    static const size_t basic_indent = 8;
    static const size_t indent_step = 3;
    
    size_t depth;
    size_t indent_level;
    
    void indent_line(bool par = false){
        depth = (par ? par_indent : basic_indent) + indent_level*indent_step;
                
        if(depth > max_depth)
            throw std::runtime_error("ERROR: indent larger than maximum depth");
        
        std::cout << std::string(depth, ' ');
    }
public:
    indent_buf(size_t ind = 0) : indent_level(ind) {
        indent_line();
    }
    
    virtual int sync(){
        std::string s = str();
        
        for(size_t i = 0; i < s.length(); i++){
            if(depth > max_depth || s[i] == '\n'){
                std::cout.put('\n');
                indent_line();
                
                if(s[i] == '\n')
                    continue;
            }  
            
            std::cout.put(s[i]);
            depth++;
            
        }
        
        str("");        
    }
    
    void paragraph(){
        sync();
        std::cout.put('\n');
        indent_line(true);
    }
    
    void incr_indent(size_t incr){
        indent_level += incr;
    }
    void decr_indent(size_t decr){
        if(indent_level < decr)
            throw std::runtime_error("ERROR: negative indent");
            
        indent_level -= decr;
    }
};
class indent_stream : public std::ostream {
public:
    indent_stream(size_t indent = 0) : std::ostream(new indent_buf(indent)) {}
    virtual ~indent_stream() {
        delete rdbuf();
    }
    
    indent_stream& paragraph(){ 
        static_cast<indent_buf*>(rdbuf())->paragraph();   
        return *this; 
    }
    indent_stream& incr_indent(size_t incr = 1){ 
        static_cast<indent_buf*>(rdbuf())->incr_indent(incr); 
        return *this; 
    }
    indent_stream& decr_indent(size_t decr = 1){ 
        static_cast<indent_buf*>(rdbuf())->decr_indent(decr); 
        return *this; 
    }
};

/* 
 * Splits a string s at all occurrences of a delimiter delim,
 * except when it occurs between (possibly nested) parentheses lpar, rpar.
 * If the character end is specified, its first occurrence outside 
 * parentheseses treated as the end of the string.
 * Unbalanced parentheses and empty substrings are ignored.
 */
list split(const std::string& s, size_t& pos, char delim, char lpar, char rpar, char end = 0)
{
    list split;
    size_t prev = pos, par = 0;
    for(;; pos++){
        //End-of-string always counts as delimiter, 
        //otherwise delimiters count if not parenthesised
        if(pos == s.length() || (par == 0 && (s[pos] == delim || s[pos] == end))){
            std::string sub = s.substr(prev, pos-prev);
            //Ignore empty substrings
            if(!sub.empty())
                split.push_back(sub);
            
            prev = pos+1;
            if(pos == s.length() || s[pos] == end)
                return split;
        }
        //left paren increases parenthetisation level
        else if(s[pos] == lpar)
            par++;
        //ignore right paren if parenthetisation would be negative
        else if(par > 0 && s[pos] == rpar)
            par--;
        //also ignore unclosed parens -- this is not a parenthetisation checker!
    }
}

bool is_plusminus(char c){
    return c == '+' || c == '-';
}

std::string read_broken_line(std::string& line, size_t& pos){
    
    //Move ahead to first non-space, assuming properly formatted input
    size_t start = pos;
    while(start < line.length() && std::isspace(line[start]))
        start++;
    
    //Line is not actually broken, return empty string
    if(start == line.length()){
        pos = 0;
        std::getline(std::cin, line);
        
        return "";
    }
    
    std::string full_line = "";
    
    //Scan until closing parenthesis
    size_t par = 0, fpar = 0;
    for(pos = start;; pos++){
        while(pos >= line.length()){
                
            full_line += line.substr(start);
            if(is_plusminus(line[line.length() - 1]))
                full_line += ' ';
            
            //Read next line, skipping initial whitespace and assuming proper input
            start = 0;
            if(!std::getline(std::cin, line))
                throw std::runtime_error("ERROR: unexpected EOF");
            
            while(start < line.length() && std::isspace(line[start]))
                start++;
            
            pos = start;
            if(pos < line.length() && is_plusminus(line[pos]))
                full_line += ' ';
        }
//         std::cout <<  line[pos];
        
        //Handle formal names
        if(line[pos] == '[')
            fpar++;
        else if(line[pos] == ']')
            fpar--;
        //Handle parentheses, but only if not inside formal name
        else if(fpar == 0){
            if(line[pos] == '(')
                par++;
            if(line[pos] == ')'){
                if(par > 0)
                    par--;
                else{
                    //Found the closing parenthesis!
                    //Complete the line without including the close-paren,
                    //and leave pos pointing to it.
                    full_line += line.substr(start, pos-1 - start);
                    return full_line;
                }
            }
        }
    }
    
}

void read_multiple_lines(std::string& line, size_t& pos, list& lines){
    for(;;){
        while(std::isspace(line[pos]))
            pos++;
        
        if(line[pos] == ')')
            return;
        
        lines.push_back( line.substr(pos) );
        
        std::getline(std::cin, line);
        pos = 0;
    }
}

std::string symbol_head(const std::string& sym){
    return sym.substr(0, sym.find_first_of("(^"));
}
        
struct bracket {
    using br_ptr = bracket*;
    
private:
    std::string key;
    list content;
    
    insertion_order_map< std::string, br_ptr > sub_brackets;
    
public:
    bracket( const std::string& k ) : key(k), content(), sub_brackets() {};
    virtual ~bracket() {
        clear();
    };
    
    void parse(std::string& line, size_t& pos,
               insertion_order_map< std::string, size_t > br_symbols,
               size_t n_level)
    {
        std::vector<std::string> br_keys(n_level + 1);
        list symbols = split(line, pos, '*', '[', ']', ' ');
        
        for(std::string symbol : symbols){
            std::string head = symbol_head(symbol);
            auto br_symbol = br_symbols.find(head);
            
            size_t lvl;
            if(br_symbol == br_symbols.end()){
                br_symbols[head] = n_level;
                lvl = n_level;
            }
            else
                lvl = br_symbol->second;
            
            if(br_keys[lvl].empty())
                br_keys[lvl] = symbol;
            else
                br_keys[lvl] += "*" + symbol;
        }
        
        bracket *br = this;
        for(size_t lvl = 0; lvl <= n_level; lvl++){
            if(br_keys[lvl].empty())
                continue;
                        
            auto sub = br->sub_brackets.find( br_keys[lvl] );
            
            if(sub == br->sub_brackets.end())
                br = (br->sub_brackets[ br_keys[lvl] ] = new bracket(br_keys[lvl]));
            else
                br = sub->second;
        }
        
        //Skip "* ( "
        pos += 5;
        std::string inlin = read_broken_line( line, pos );
        
        if(inlin.empty())
            read_multiple_lines( line, pos, br->content );
        else
            br->content.push_back(inlin);
        
        //line[pos] is now the closing parenthesis of this expression
    }
    
    void print(indent_stream& out, bool root = false) const {
        out << key;
        
        if(sub_brackets.empty()){
            if(!root)
                out << " * ( ";
            
            if(content.size() > 1){
                out.incr_indent().paragraph();
                
                for(const std::string& line : content){
                    out.incr_indent() << line;
                    out.decr_indent().paragraph();
                }
                
                if(!root)
                    out << ")";
                out.decr_indent();
            }
            else{
                out.incr_indent(2) << content.front();
                if(!root)
                    out << " )";
                out.decr_indent(2);
            }
        }
        else{
            if(content.empty() && sub_brackets.size() == 1){
                if(!root)
                    out << "*";
                out.incr_indent() << "+ ";
                sub_brackets.cbegin()->second->print(out);
                out.decr_indent();
            }
            else{
                if(!root)
                    out << " * ( ";
                out.incr_indent().paragraph();
                
                if(!content.empty()){
                    if(content.size() == 1 && content.front()[0] != '-')
                        out << "+ ";
                    
                    for(const std::string& line : content){
                        out.incr_indent() << line;
                        out.decr_indent().paragraph();
                    }
                }
                
                for(const auto& sub : sub_brackets){
                    out.paragraph() << "+ ";
                    sub.second->print(out);
                    out.paragraph();
                }
                if(!root)
                    out << ")";
                out.decr_indent();
            }
        }
    }
    
    void clear(){
        content.clear();
        for(auto& [k, ptr] : sub_brackets)
            delete ptr;
        sub_brackets.clear();
    }
};
        

/*
 * Main method. Standard input should be a pipe from a FORM program,
 * or read from a FORM log file. It will simply echo its input to
 * standard output until a multibracket tag [_MULTIBRACKET_] is found,
 * after which it reformats the expression using a similar style to
 * FORM's bracket feature, but using multiple indentation levels for
 * greater readability.
 *
 * The command line parameters should be comma-separated lists of symbols
 * present in the FORM program. Each argument will correspond to one level
 * of indentation. 
 */
int main(int argc, const char** argv){
    
    insertion_order_map< std::string, size_t > br_symbols;
    
    //Parse the bracket specifications
    for(int arg = 1; arg < argc; arg++){
        size_t pos = 0;
        for(const std::string& br_symbol : split(std::string(argv[arg]), pos, ',', '[', ']'))
            br_symbols[br_symbol] = arg-1;
    }
    
    bracket root("");
    indent_stream out;
    
    bool multibracket = false;
    //Read lines from input until EOF
    for(std::string line; std::getline(std::cin, line); ){
        
        
        if(line.find(MULTIBRACKET) == 0){
            multibracket = true;
            
            size_t pos = std::strlen(MULTIBRACKET);
            root.parse(line, pos, br_symbols, argc-1);
            
            pos++;
            while(pos < line.length() && std::isspace(line[pos]))
                pos++;
            if(pos >= line.length()){
                if(!std::getline(std::cin, line))
                    break;
                
                pos = 0;
                while(pos < line.length() && std::isspace(line[pos]))
                    pos++;
            }
            if(pos < line.length() && line[pos] == ';'){
                out.paragraph();
                root.print(out, true);
                (out << ";").flush();
                
                root.clear();
                multibracket = false;
                continue;
            }
        }
        else if(!multibracket)
            std::cout << "\n" << line;
    }
    
    if(multibracket)
        throw std::runtime_error("ERROR: unexpected EOF");
    
    std::cout << "\n";
    return 0;
    
}

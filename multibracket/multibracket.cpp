#include <iostream>
#include <regex>
#include <string>
#include <algorithm>
#include <list>
#include <unordered_map>

using str = std::string;
using list = typename std::list<str>;

#define MULTIBRACKET "[_MULTIBRACKET_]"
#define REG_MULTIBRACKET "\\[_MULTIBRACKET_\\]"
#define INDENT(lvl) std::string(lvl*3 + 6, ' ')


template< typename val_type >
struct insertion_order_map {
private:
    
    using key_val_vector = typename std::vector< std::pair<str, val_type> >;
    key_val_vector elements;
    std::unordered_map<str, size_t> index_map;
    
    
public:
    
    val_type& operator[] (const str& key){
        auto where = index_map.find(key);
        if(where != index_map.end()){
            return elements[where->second].second;
        }
        else if(key.empty()){        
            if(!elements.empty()){
                elements.push_back( elements.back() );
                for(size_t i = elements.size() - 2; i > 0; i--)
                    elements[i] = elements[i-1];
                elements[0] = std::make_pair(key, val_type());
            }
            else
                elements.push_back( std::make_pair(key, val_type()) );
            
            for(auto& [k, idx] : index_map)
                idx++;
            index_map[key] = 0;
            
            return elements[0].second;
        }
        else{
            index_map[key] = elements.size();
            elements.push_back( std::make_pair(key, val_type()) );
            return elements.back().second;
        }           
    }
    
    size_t size() const {
        return elements.size();
    }
    
    typename key_val_vector::iterator find(const str& key){
        auto where = index_map.find(key);
        if(where != index_map.end())
            return begin() + where->second;
        else
            return end();
    }
    typename key_val_vector::iterator begin() {
        return elements.begin();
    }
    typename key_val_vector::iterator end() {
        return elements.end();
    }
};

list split(const str& s, const str& delim)
{
    list tokens;
    size_t prev = 0, pos = 0;
    do{
        pos = s.find(delim, prev);
        if(pos == str::npos) 
            pos = s.length();
        
        str token = s.substr(prev, pos-prev);
        if(!token.empty()) 
            tokens.push_back(token);
        
        prev = pos + delim.length();
    } while (pos < s.length() && prev < s.length());
    
    return tokens;
}

void print_bracket(size_t b_lvl, size_t i_lvl, bool head, bool first, const list& lines, const std::vector<list>& br_symb){
    insertion_order_map<list> brackets;
    std::smatch match;
    
    str key, val;
    
    if(b_lvl >= br_symb.size()){
        
        if(head && i_lvl == 0)
            std::cout << "\n\n";
        
        if(lines.size() > 1){
            if(!head)
                std::cout << " * (\n";
            
            bool first = true;
            for(const str& line : lines){
                if(first)
                    first = false;
                else
                    std::cout << "\n";
                std::cout << INDENT(i_lvl) << line;
            }
            
            if(!head)
                std::cout << "\n" << INDENT(i_lvl) << " )";
        }
        else if(lines.size() > 0){
            if(head)
                std::cout << INDENT(i_lvl) << lines.front();
            else
                std::cout << " * (" << lines.front() << " )";
        }
        
        return;
    }
    
    for(const str& line : lines){
  ////      std::cout << INDENT(b_lvl) << "# line: \"" << line << "\"\n";
        if( std::regex_match(line, match, std::regex("(.*)" REG_MULTIBRACKET "(\\*(.*))? \\* (.*)")) ){
////            std::cout <<  INDENT(b_lvl) << "# matches bracket\n";
            
            key = "", val = "";
            
            bool any_bracket_match = false;
            if(!match[2].str().empty()){
                for(str& symbol : split(match[3], "*")){
                    bool bracket_match = false;
                    for(const str& br : br_symb[b_lvl]){
                        if( std::regex_match(symbol, std::regex(br + ".*")) ){
                            if(key.empty())
                                key = symbol;
                            else
                                key += "*" + symbol;
                            
                            bracket_match = true;
                            break;
                        }
                    }
                    if(bracket_match)
                        any_bracket_match = true;
                    else
                        val += "*" + symbol;
                }
            }
            if(brackets.find(key) == brackets.end())
                brackets[key] = list();
            
            if(b_lvl < br_symb.size() - 1)
                brackets[key].push_back(" + " MULTIBRACKET +  val + " * (");
            
////            std::cout << INDENT(b_lvl) << "# key: \"" << key << "\", val: \"" << val << "\"\n";

        }
        else if( std::regex_match(line, match, std::regex("\\s*( [-+].*)")) ){
////            std::cout << INDENT(b_lvl) << "# matches line\n";
            if(brackets.find(key) == brackets.end())
                brackets[key] = list();
            
            brackets[key].push_back( match[1] );
        }
////         else{
////            std::cout << INDENT(b_lvl) << "# no match\n";
////         }
    }

////    std::cout << "MAP:\n";
////    for(auto&[ br, cont ] : brackets){
////        std::cout << "key: \"" << br << "\"\n";
////        for(str& line : cont)
////            std::cout << "   val: \"" << line << "\"\n";
////    }

    if(brackets.size() > 1){
        bool first = true;
        if(!head)
            std::cout << " * (\n"; 
            
        for( auto&[ br, cont ] : brackets ){
            if(br.empty()){
                print_bracket(b_lvl+1, i_lvl, true, first, cont, br_symb);
            }
            else{
                if(!first)
                    std::cout << "\n\n";
                
                std::cout << INDENT(i_lvl) << " + " << br;
                print_bracket(b_lvl+1, i_lvl+1, false, first, cont, br_symb);
            }
            first = false;
        }
        
        if(!head)
            std::cout << "\n" << INDENT(i_lvl) << " )";
    }
    else if(brackets.size() > 0){
        if(brackets.begin()->first.empty()){
            print_bracket(b_lvl+1, i_lvl, head, true, brackets.begin()->second, br_symb);
        }
        else{
            if(head)
                std::cout << "\n\n" << INDENT(i_lvl) << " + " << brackets.begin()->first;
            else
                std::cout << "*" << brackets.begin()->first;
            
            print_bracket(b_lvl+1, i_lvl+1, false, true, brackets.begin()->second, br_symb);
        }
    }
    
    if(b_lvl == 0)
        std::cout << ";\n";
}

int main(int argc, const char** argv){
    
    std::smatch match;
 
    std::vector<list> br_symb( argc-1 );
    for(int arg = 1; arg < argc; arg++){
        br_symb[arg-1] = split(str(argv[arg]), ",");
    }
    
    list lines;
    bool bracket = false, multibracket = false;
    for(str line; std::getline(std::cin, line); ){
//         std::cout << "." << line << "\n";
        if(bracket){
            lines.push_back(line);
            
            if(line.find(MULTIBRACKET) != str::npos)
                multibracket = true;
            
            if(line.find(";") != str::npos){
                if(multibracket)
                    print_bracket(0, 0, true, true, lines, br_symb);
                else{
                    for(const str& ln : lines)
                        std::cout << ln << "\n";
                }
                
                multibracket = false;
                bracket = false;
            }
                
        }
        else if( std::regex_match( line, std::regex(".*=\\s*$")) ){
            std::cout << line;
            lines.clear();
            bracket = true;
        }
        else
            std::cout << line << "\n";
    }
    
    return 0;
    
}

#include <iostream>
#include <regex>
#include <string>
#include <algorithm>
#include <list>
#include <unordered_map>

using str = std::string;
using list = typename std::list<str>;
using map = typename std::unordered_map<str, list>;

#define MULTIBRACKET "[multibracket]"
#define REG_MULTIBRACKET "\\[multibracket\\]"
#define INDENT(lvl) std::string(lvl*4, ' ')

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

const std::string WHITESPACE = " \n\r\t\f\v";

std::string ltrim(const str& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == str::npos) ? "" : s.substr(start);
}

std::string rtrim(const str& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == str::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const str& s)
{
    return rtrim(ltrim(s));
}

void print_bracket_level(size_t level, bool indent_first, const list& lines, const std::vector<list>& br_symb){
    map brackets;
    std::smatch match;
    
    str key, val;
    
    if(level >= br_symb.size()){
        
        for(const str& line : lines)
            std::cout << INDENT(level) << line << "\n";       
        
        return;
    }
    
    for(const str& line : lines){
        if( std::regex_match(line, match, std::regex("(.*)" REG_MULTIBRACKET "\\*(.*) \\* (.*)")) ){
            
            key = "", val = "";
            
            for(str& symbol : split(match[2], "*")){
                bool bracket_match = false;
                for(const str& br : br_symb[level]){
                    if( std::regex_match(symbol, std::regex(br + ".*")) ){
                        if(key.empty())
                            key = symbol;
                        else
                            key += "*" + symbol;
                        
                        bracket_match = true;
                        break;
                    }
                }
                if(!bracket_match)
                    val += "*" + symbol;
            }
            
            if(brackets.find(key) == brackets.end())
                brackets[key] = list();
            
            brackets[key].push_back("\n+ " MULTIBRACKET "*" +  val + " * (");
        }
        else{
            if(brackets.find(key) == brackets.end())
                brackets[key] = list();
            
            brackets[key].push_back( trim(line) );
        }
    }
    
    bool first = true;
    for( auto&[ br, cont ] : brackets ){
        if(!first || indent_first)
            std::cout << "\n" << INDENT(level) << " + ";
        
        first = false;
        
        std::cout << br;
        if(cont.size() > 1 || level == br_symb.size() - 1){
            std::cout << " * (\n";
            print_bracket_level(level+1, true,  cont, br_symb);
        }
        else{
            std::cout << "*";
            print_bracket_level(level+1, false, cont, br_symb);
        }
        
        if(indent_first){
            std::cout << "\n" << INDENT(level) << ")";
            if(level == 0)
                std::cout << ";";
            std::cout << "\n";
        }
    }
}

int main(int argc, const char** argv){
 
    std::vector<list> br_symb( argc-1 );
    for(int arg = 1; arg < argc; arg++){
        br_symb[arg-1] = split(str(argv[arg]), " ");
        
        for(str& br : br_symb[arg-1])
            br = trim(br);
    }
    
    list lines;
    bool active_bracket = false;
    for(str line; std::getline(std::cin, line); ){
        if( line.find(";") != str::npos ){
            print_bracket_level(0, true, lines, br_symb);
            lines.clear();
            active_bracket = false;
        }
        
        if(active_bracket)
            lines.push_back(line);
        else
            std::cout << line << "\n";
            
        
        if( std::regex_match( line, std::regex(".*=\\s*$")) )
            active_bracket = true;
    }
    
    print_bracket_level(0, true, lines, br_symb);
    
    return 0;
    
}

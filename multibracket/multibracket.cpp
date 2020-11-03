#include <iostream>
#include <string>
#include <functional>
#include <algorithm>
#include <cstring>

#include "insertion_order_map.hpp"
#include "indent_stream.hpp"

//Multibracket tag (special symbol output by FORM macro)
#define MULTIBRACKET "       + [_MULTIBRACKET_]"

using list = typename std::list<std::string>;

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
    if(start >= line.length()){
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
        while(pos < line.length() && std::isspace(line[pos]))
            pos++;
        
        if(pos < line.length()){
            if(line[pos] == ')')
                return;
        
            lines.push_back( line.substr(pos) );
        }
        
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
            if(!root && content.empty() && sub_brackets.size() == 1){
                out << "*";
                out.incr_indent();
                
                sub_brackets.cbegin()->second->print(out);
                
                out.decr_indent();
            }
            else{
                if(!root){
                    out << " * ( ";
                    out.incr_indent();
                }
                
                if(!content.empty()){
                    out.paragraph();
                    
                    if(content.size() == 1 && !is_plusminus(content.front()[0]))
                        out << "+ ";
                    
                    for(const std::string& line : content){
                        out.incr_indent() << line;
                        out.decr_indent().paragraph();
                    }
                }
                
                for(auto it = sub_brackets.begin(); it != sub_brackets.end(); ){
                    out.paragraph() << "+ ";
                    it->second->print(out);
                    
                    if(++it != sub_brackets.end() || !root)
                        out.paragraph();
                }
                if(!root){
                    out << ")";
                    out.decr_indent();
                }
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
    indent_stream out(std::cout, 0, 3, 8, -2, 79);
    out << "\n";
    
    try{
    
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
        
    } catch (std::runtime_error& e) {
        std::cout << std::endl;
        std::cerr << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n";
    
    for(auto& [name, lvl] : br_symbols)
        std::cout << name << ": " << lvl << "\n";
    return 0;
    
}

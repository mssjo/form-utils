#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include <algorithm>
#include <cstring>

#include "insertion_order_map.hpp"
#include "indent_stream.hpp"

//Multibracket tag (special symbol output by FORM macro)
#define MULTIBRACKET_TAG "[_MB_]"
#define MULTIBRACKET "       + " MULTIBRACKET_TAG

using list = typename std::list<std::string>;

/* 
 * Splits a string s at all occurrences of any of the characters in delim,
 * except when they occur between (possibly nested) parentheses. The string
 * is considered to start at pos, and pos is left as the index of the last
 * character considered by the method (or as past-the-end).
 * 
 * Parentheses are specified through par, in which character 2n is a left
 * parenthesis, and character 2n+1 is the corresponding right parenthesis
 * (default: "()[]{}"). Parentheses must be matched, but they may be interleaved
 * (e.g. "[(])"). This method doesn't care about mismatched parentheses.
 * 
 * The method terminates when it reaches the end of the string, or any
 * of the characters in end (default: ""), or when a right parenthesis
 * without a corresponding left parenthesis. Like delim, end is ignored
 * inside parentheses.
 * 
 * Empty substrings are ignored.
 */
list split(const std::string& s, size_t& pos, 
           const std::string& delim, 
           const std::string& par = "()[]{}",
           const std::string& end = ""
          )
{
    list split;
    
    if(par.length() % 2)
        throw std::runtime_error("ERROR: par must consist of matching pairs");
    size_t npar = par.length() / 2;
        
    bool in_par = false, add_sub = false, done = false;
    std::vector<size_t> par_count(npar, 0);
    size_t prev = pos, idx;

    for(;; pos++){
        //Treat a single character, sett add_sub = true if it is time to add a new substring
        //to the list, and done = true if it is time to terminate the function after adding the
        //final substring.
        
        if(pos >= s.length()){
            done = true;
        }
        else if((idx = par.find(s[pos])) != std::string::npos){
            if(idx % 2){    //right paren
                if(par_count[idx / 2] == 0){    //final closing paren
                    done = true;
                }
                else{   //just decrement
                    par_count[idx / 2]--;
                    
                    //Update in_par
                    if(par_count[idx / 2] == 0){
                        in_par = false;
                        for(size_t i = 0; i < npar; i++){
                            if(par_count[i] != 0){
                                in_par = true;
                                break;
                            }
                        }
                    }
                }
            }
            else{   //left paren
                in_par = true;
                par_count[idx / 2]++;
            }
        }
        else if(!in_par){    //not parenthesised, look for end or delim
            if(end.find(s[pos]) != std::string::npos){
                done = true;
            }
            if(delim.find(s[pos]) != std::string::npos){
                //done = false;
                add_sub = true;
            }
        }
                
        if(add_sub || done){
            std::string sub = s.substr(prev, pos-prev);
            //Ignore empty substrings
            if(!sub.empty())
                split.push_back(sub);
            
            prev = pos+1;
            if(done)
                return split;
            
            add_sub = false;
        }
    }
}

bool is_plusminus(char c){
    return c == '+' || c == '-';
}

std::string read_broken_line(std::string& line, size_t& pos, char endchar, std::istream* in = &std::cin){
    
    //Move ahead to first non-space, assuming properly formatted input
    size_t start = pos;
    while(start < line.length() && std::isspace(line[start]))
        start++;
    
    //Line is not actually broken, read new line and return empty string
    if(start >= line.length()){
        pos = 0;
        std::getline(*in, line);
        
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
            if(!std::getline(*in, line))
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
            if(par == 0 && line[pos] == endchar){
                //Found the end!
                //Complete the line without including the endchar,
                //and leave pos pointing to it.
                if(pos > start)
                    full_line += line.substr(start, pos-1 - start);
                return full_line;   
            }            
            else if(line[pos] == '(')
                par++;
            else if(line[pos] == ')')
                par--;
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
    size_t pos = 0;
    return split(sym, pos, "^(", "[]").front();
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
        list symbols = split(line, pos, "*", "[]", " ");
                
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
        std::string inlin = read_broken_line( line, pos, ')' );
        
        if(inlin.empty())
            read_multiple_lines( line, pos, br->content );
        else
            br->content.push_back(inlin);
        
        //line[pos] is now the closing parenthesis of this expression
    }
    
    //Return value is true if printout was single-line
    // (line breaks due to overlong lines don't count)
    bool print(indent_stream& out, bool root = false) const {
        out << key;
        bool single_line;
        
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
                
                return false;
            }
            else{
                out.incr_indent(2) << content.front();
                if(!root)
                    out << " )";
                out.decr_indent(2);
                
                return true;
            }
        }
        else{
            if(!root && content.empty() && sub_brackets.size() == 1){
                out << "*";
//                 out.incr_indent();
                
                single_line = sub_brackets.cbegin()->second->print(out);
                
//                 out.decr_indent();
                return single_line;
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
                    single_line = it->second->print(out);
                    
                    ++it;
                    
                    //This is a bit confusing. What it does is:
                    //   sub-brackets are normally separated by an empty line
                    //   ...but not single-line ones (NOTE: this makes expressions more compact)
                    //   ...and not the last one (that is handled later)
                    //   ...however, if a single-line is followed by a mutliple-line, insert blank line.
                    if(it != sub_brackets.end() && (!single_line || !it->second->is_single_line())){             
                        out.paragraph();
                    }
                }
                if(!root){
                    out.paragraph() << ")";
                    out.decr_indent();
                }
                
                return false;
            }
        }
    }
    
    //Basically a "dry run" of print(out, true)
    bool is_single_line() const {
        bool single_line;
        
        if(sub_brackets.empty()){
            return (content.size() <= 1);
        }
        else{
            if(content.empty() && sub_brackets.size() == 1){
                return sub_brackets.cbegin()->second->is_single_line();
            }
            else{
                return false;
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

void parse_bracket_symbols(size_t level, const std::string& symbol_group, 
                           insertion_order_map< std::string, size_t >& br_symbols)
{

    size_t pos = 0;
    auto split_group = split(symbol_group, pos, ", ", "[]");
    for(auto it = split_group.begin(); it != split_group.end(); it++){
        
        //Handle FORM's ... operator
        //This is cunningly done by invoking FORM's preprocessor on a tiny
        //temporary file that basically only contains the ... operator to
        //be expanded. This way, we save some work while ensuring that the
        //handling is consistent with FORM.
        if(*it == "..."){
            //Create temporary file:
            /*
             *   * Temporary file for use by multibracket
             *   #-
             *   [_MULTIBRACKET_],<*(it-1)>,...,<*(it+1)>;
             *   .end
             */
            //The multibracket tag is used to find the line containing the results
            std::ofstream tmp(".multibracket_tmp.frm");
            tmp << "* Temporary file for use by multibracket\n#-\n" MULTIBRACKET_TAG ",";
            if(it == split_group.begin())
                throw std::runtime_error("ERROR: empty beginning of ... range");
            --it;
            br_symbols.erase(*it);
            tmp << *it << ",...,";
            ++it; ++it;
            if(it == split_group.end())
                throw std::runtime_error("ERROR: empty end of ... range");
            tmp << *it << ";\n#+\n#+\n.end\n";
            tmp.close();
            
            system("form -y .multibracket_tmp.frm > .multibracket_tmp.log");
            
            //Find output line, parse it, and ignore the rest
            //NOTE: FORM doesn't support it (yet), but this is compatible
            //with nested use of the ... operator!
            size_t pos;
            bool valid = false;
            std::ifstream processed_tmp(".multibracket_tmp.log");
            for(std::string line; std::getline(processed_tmp, line); ){
                if((pos = line.find(MULTIBRACKET_TAG)) != std::string::npos){
                    pos += std::strlen(MULTIBRACKET_TAG) + 1;
                    
                    parse_bracket_symbols(
                        level, 
                        //This is a bit hacky, but does the job nicely when the expansion is long
                        read_broken_line(line, pos, '#', &processed_tmp),
                        br_symbols
                    );
                    
                    processed_tmp.close();
                    
                    valid = true;
                    break;
                }
            }
            
            //No output means something went awry
            if(!valid)
                throw std::runtime_error("ERROR: improper use of ... operator");
        }
        //No ... operator, just insert symbol
        else            
            br_symbols.insert(std::make_pair(*it, level));
    }
}

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
        parse_bracket_symbols(arg-1, std::string(argv[arg]), br_symbols);
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
    
//     // For debugging
//     for(auto& brs : br_symbols)
//         std::cout << std::string(brs.second, '\t') << brs.first << "\n";
//     
    return 0;
    
}

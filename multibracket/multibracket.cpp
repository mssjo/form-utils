#include <iostream>
#include <regex>
#include <string>
#include <algorithm>
#include <list>
#include <unordered_map>

//Multibracket tag (special symbol output by FORM macro)
#define MULTIBRACKET "[_MULTIBRACKET_]"
//Escaped version for regexes
#define REG_MULTIBRACKET "\\[_MULTIBRACKET_\\]"
//Matches FORM's indentation as of 4.2
#define INDENT(lvl) std::string(lvl*3 + 6, ' ')

using str = std::string;
using list = typename std::list<str>;

/*
 * A simple ordered map type with the feature that elements are ordered
 * not by some comparison function, but by the order in which they were
 * inserted. The exception is the empty key, which is always put first.
 * 
 * This is by no means an STL-compliant container; I have just implemented
 * the minimal functionality ad-hoc to allow the function to preserve
 * FORM's ordering conventions, which are based on information not present
 * in the output.
 * 
 * (Sometime in the future, it would maybe be fun to extend it to a full
 * STL-compliant container. Making the value type a template parameter
 * would be easy, and for the key type, the only difficulty would be in
 * adding the feature where a special key overrides the sorting. Should
 * it just be passed to each individual map upon construction, or should
 * it be some weird template parameter?)
 */
struct insertion_order_map {
private:
    
    /* 
     * The structure is a list of key-value pairs, where elements are
     * added to the end, except for those with empty keys, which are
     * added to the beginning. Map-ness is implemented via a map
     * from keys to key-value-list iterators, which act as indices
     * to the corresponding location in the list (i.e. they are never
     * iterated). The benefit is that iterator lookup is constant-time
     * in a list, and that they are not invalidated by changes to the
     * list.
     */ 
    using key_val_list = typename std::list< std::pair<str, list> >;
    key_val_list elements;
    std::unordered_map<str, key_val_list::iterator> index_map;
    
public:
    
    /* 
     * These are the methods needed for the functionality of this
     * program. They act exactly like the corresponding methods
     * of STL map types.
     */
    
    list& operator[] (const str& key){
        auto where = index_map.find(key);
        if(where != index_map.end()){
            //where = {key, iterator to {key, val}} so the expression below is val
            return where->second->second;
        }
        else if(key.empty()){
            //create new element in front
            elements.push_front( std::make_pair(key, list()) );
            return (index_map[key] = elements.begin())->second;
        }
        else{
            //create new element in back
            elements.push_back( std::make_pair(key, list()) );
            return (index_map[key] = --elements.end())->second;
        }           
    }
    
    size_t size() const {
        return elements.size();
    }
    
    typename key_val_list::iterator find(const str& key){
        //where already points to the desired iterator; we just have to wrap "end"
        auto where = index_map.find(key);
        if(where != index_map.end())
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
};

/* 
 * Splits a string at all occurrences of a delimiter delim,
 * except when it occurs between (possibly nested) parentheses lpar, rpar.
 * Unbalanced parentheses and empty substrings are ignored.
 */
list split(const str& s, char delim, char lpar, char rpar)
{
    list split;
    size_t prev = 0, par = 0;
    for(size_t pos = 0; pos <= s.length(); pos++){
        //End-of-string always counts as delimiter, 
        //otherwise delimiters count if not parenthesised
        if(pos == s.length() || (par == 0 && s[pos] == delim)){
            str sub = s.substr(prev, pos-prev);
            //Ignore empty substrings
            if(!sub.empty())
                split.push_back(sub);
            
            prev = pos+1;
        }
        //left paren increases parenthetisation level
        else if(s[pos] == lpar)
            par++;
        //ignore right paren if parenthetisation would be negative
        else if(par > 0 && s[pos] == rpar)
            par--;
        //also ignore unclosed parens -- this is not a parenthetisation checker!
    }
    
    return split;
}

/*
 * This is the central method for printing multibrackets. It recursively
 * extracts bracketed symbols, prints them, and applies itself to the
 * expression inside the bracket.
 * 
 * b_lvl    the bracket level, i.e. the element in br_symb currently
 *          being extracted.
 * i_lvl    the level of indentation, <= b_lvl (smaller if symbols from
 *          some lower bracket level are missing).
 * head     true if whatever this method prints is the beginning of a line.
 * first    true if whatever this method prints is the first thing in
 *          a surrounding bracket.
 * lines    a list of lines to be treated by this method.
 * br_symb  a list of lists of symbols to be bracketed. Everything in the
 *          first level will be extracted first, then everything on the
 *          second level, and so on.
 */
void print_bracket(size_t b_lvl, size_t i_lvl, 
                   bool head, bool first, 
                   const list& lines, const std::vector<list>& br_symb)
{
    // Maps bracketed symbols to the lines inside that bracket.
    // An insertion_order_map is used to preserve FORM's expression ordering.
    insertion_order_map brackets;
    
    str key, val;
    
    //If all bracket levels have been handled, print the lines as-is
    //with just a bit of formatting
    if(b_lvl >= br_symb.size()){
        
        if(head && i_lvl == 0)
            std::cout << "\n\n";
        
        if(lines.size() > 1){
            //If not head, then parentheses are needed
            if(!head)
                std::cout << " * (\n";
            
            bool first = true;
            for(const str& line : lines){
                //Extra linebreak on the first line separates groups
                if(first)
                    first = false;
                else
                    std::cout << "\n";
                
                std::cout << INDENT(i_lvl) << line;
            }
            
            if(!head)
                std::cout << "\n" << INDENT(i_lvl) << " )";
        }
        //Single-line brackets are printed inline
        else if(lines.size() > 0){
            if(head)
                std::cout << INDENT(i_lvl) << lines.front();
            else
                std::cout << " * (" << lines.front() << " )";
        }
        
        return;
    }
    
    std::smatch match;
    for(const str& line : lines){
        //If we match a line tagged as a multibracket...
        if( std::regex_match(line, match, std::regex("\\s+\\+ " REG_MULTIBRACKET "(\\*(.*))? \\* \\(\\s*")) ){
            key = "", val = "";
            
            //If there are bracket symbols besides the multibracket tag...
            if(!match[1].str().empty()){
                //Iterate over all bracketed symbols, separate those that should go at this
                //level of bracketing from the rest (key and val, respectively)
                for(str& symbol : split(match[2], '*', '[', ']')){
                    bool bracket_match = false;
                    for(const str& br : br_symb[b_lvl]){
                        //Keep any function arguments applied to the symbols;
                        //different ones are treated as distinct, like in FORM
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
            }
            //Prepare line for recursive call, using the same format as the input
            if(b_lvl < br_symb.size() - 1)
                brackets[key].push_back(" + " MULTIBRACKET +  val + " * (");

        }
        //If the line contains an expression, add it to the lines to be treated
        //using the last encountered bracket key
        else if( std::regex_match(line, match, std::regex("\\s*( [-+].*)")) ){
            if(brackets.find(key) == brackets.end())
                brackets[key] = list();
            
            brackets[key].push_back( match[1] );
        }
        //Other lines are just ignored; they contain parentheses and spaces, which
        //will be rebuilt when printing
    }
    
    //Now, we print and recurse!
    
    //Treat brackets with multiple contents differently
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
    
    //Terminate with a semicolon
    if(b_lvl == 0)
        std::cout << ";\n";
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
    
    std::smatch match;
 
    //Parse the bracket specifications
    std::vector<list> br_symb( argc-1 );
    for(int arg = 1; arg < argc; arg++){
        br_symb[arg-1] = split(str(argv[arg]), ',', '[', ']');
    }
    
    list lines;
    bool bracket = false, multibracket = false;
    //Read lines from input until EOF
    for(str line; std::getline(std::cin, line); ){
        //If we are in a (potentially) bracketed expression...
        if(bracket){
            //Store lines
            lines.push_back(line);
            
            //If the expression contains at least one multibracket tag, 
            //it is acutally a multibracket expression
            if(!multibracket && line.find(MULTIBRACKET) != str::npos){
                multibracket = true;
            }
            
            
            //Expressions are semicolon-terminated
            if(line.find(";") != str::npos){
                if(multibracket){
                    //Print multibracket
                    print_bracket(0, 0, true, true, lines, br_symb);
                }
                else{
                    //Ordinary expressions (i.e. not multibracketed)
                    //are just printed verbatim
                    std::cout << "\n";
                    for(const str& ln : lines)
                        std::cout << ln << "\n";
                }
                
                multibracket = false;
                bracket = false;
            }
                
        }
        //Match beginning of (potentially bracketed) expression output
        else if( std::regex_match( line, std::regex(".*=\\s*$")) ){
            std::cout << line;
            lines.clear();
            bracket = true;
        }
        //Print other lines verbatim
        else
            std::cout << line << "\n";
    }
    
    return 0;
    
}

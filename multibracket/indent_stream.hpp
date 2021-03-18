#ifndef INDENT_STREAM_H
#define INDENT_STREAM_H

#include <iostream>
#include <sstream>


namespace detail {

    class indent_buf : public std::stringbuf {
    private:
        
        std::ostream& out;
        
        size_t depth;
        size_t indent_level;
        
        void indent_line(bool par = false){
            depth = (par ? par_indent : basic_indent) + indent_level*indent_step;
                    
            if(depth > max_depth)
                throw std::runtime_error("ERROR: indent larger than maximum depth");
            
            out << std::string(depth, ' ');
        }
    public:
        
        size_t max_depth;
        size_t par_indent;
        size_t basic_indent;
        size_t indent_step;
        
        indent_buf(std::ostream& o = std::cout, size_t ind = 0) : out(o), indent_level(ind) {};
        
        virtual int sync(){
            std::string s = str();
            
            for(size_t i = 0; i < s.length(); i++){
                if(depth > max_depth || s[i] == '\n'){
                    out.put('\n');
                    indent_line();
                    
                    if(s[i] == '\n')
                        continue;
                }  
                
                out.put(s[i]);
                depth++;
                
            }
            
            str("");        
        }
        
        void paragraph(){
            sync();
            out.put('\n');
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
        void set_indent(size_t level){
            indent_level = level;
        }
    };
}

/**
 * @brief an ostream that automatically indents its output.
 * 
 * An indent stream wraps another ostream (default: @c std::cout) and normally 
 * prints verbatim to that stream, but when the length of the current line
 * exceeds a certain number of characters (default: 80), a linebreak is inserted.
 * Whenever a new line is started, either this way or by printing a linebreak,
 * the next line is indented. The number of indent steps can be incremented and
 * decremented as needed, and the size of each step can be set (default: 4 spaces).
 * Instead of normal linebreaks, the @c paragraph can be called to break the
 * line and make a special paragraph indent, which can be set deeper or shallower
 * than the normal indent (default: 2 extra spaces). It is also possible to set
 * a number of spaces that always appears to the left regardless of the indent
 * level (default: 0).
 * 
 * This class has no way to guard against independent use of the underlying
 * ostream, and will not behave correctly in that case. Currently, it does not 
 * handle tabs and non-printable characters correctly. 
 */
class indent_stream : public std::ostream {
private:
    detail::indent_buf& indent_buf(){
        return *( static_cast<detail::indent_buf*>(rdbuf()) );
    }
    
    const detail::indent_buf& indent_buf() const {
        return *( static_cast<detail::indent_buf*>(rdbuf()) );
    }
        
public:
    indent_stream(std::ostream& out = std::cout, size_t indent = 0,
        size_t step = 4, size_t b_ind = 0, int p_ind = 4, int max_d = 80
    ) 
    : std::ostream(new detail::indent_buf(out, indent)) 
    {
        set_indent_step(step);
        set_max_depth(max_d);
        set_basic_indent(b_ind);
        set_par_indent(p_ind);
    }
    
    virtual ~indent_stream() {
        indent_buf().sync();
        delete &indent_buf();
    }
    
    indent_stream& paragraph(){ 
        indent_buf().paragraph();   
        return *this; 
    }
    indent_stream& incr_indent(size_t incr = 1){ 
        indent_buf().incr_indent(incr); 
        return *this; 
    }
    indent_stream& decr_indent(size_t decr = 1){ 
        indent_buf().decr_indent(decr); 
        return *this; 
    }
    indent_stream& set_indent(size_t level){
        indent_buf().set_indent(level);
        return *this;
    }
    
    size_t get_indent_step() const {        
        return indent_buf().indent_step;                
    }
    indent_stream& set_indent_step(size_t new_step){  
        indent_buf().indent_step = new_step;
        return *this;
    }
    size_t get_max_depth() const {          
        return indent_buf().max_depth;                  
    }
    indent_stream& set_max_depth(size_t new_depth){   
        indent_buf().max_depth = new_depth; 
        return *this;
    }
    size_t get_basic_indent() const {       
        return indent_buf().basic_indent;               
    }
    indent_stream& set_basic_indent(size_t new_ind){  
        indent_buf().basic_indent = new_ind;
        return *this;
    }
    size_t get_par_indent() const {         
        return indent_buf().par_indent;                 
    }
    indent_stream& set_par_indent(int extra_ind){     
        if(-extra_ind > indent_buf().basic_indent)
            throw std::runtime_error("ERROR: paragraph indent would be negative");
        indent_buf().par_indent = indent_buf().basic_indent + extra_ind;
        return *this;
    }
};

#endif

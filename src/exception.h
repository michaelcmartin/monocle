#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#include <string>
#include <exception>

namespace monocle {
    // monocle::panic is our general exception class. Detail strings are
    // not optional!
    class panic : public std::exception {
    public:
        panic(const std::string &w) throw() { what_ = w; }
        panic(const char *w) throw() { what_ = w; }
        virtual ~panic() throw() { }
        virtual const char* what() const throw () { return what_.c_str(); }
    private:
        std::string what_;
    };
}

#endif

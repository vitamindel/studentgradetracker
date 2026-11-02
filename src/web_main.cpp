// =====================================================================
//  web_main.cpp  -  the BRIDGE between the website and the C++ engine
// ---------------------------------------------------------------------
//  This file is compiled to WebAssembly (gradebook.wasm). The website
//  can't call our classes directly, so we give it ONE function: run().
//
//  How a button click becomes C++:
//    1. JavaScript builds a command string, e.g.  grade 1001 "Quiz 3" quiz 88
//    2. JavaScript calls run(thatString)
//    3. run() points cin at the string and cout at an empty string buffer
//    4. CommandShell reads the command with cin >> ... and answers with cout << ...
//    5. run() hands whatever was written to cout back to JavaScript
//
//  Analogy: run() is a drive-through window. JavaScript drops an order
//  (text) in the window; inside, the C++ "staff" read it with cin and
//  put the answer in a bag with cout; the bag comes back out the window.
// =====================================================================
#include "Student.h"
#include "GradeBook.h"
#include "CommandShell.h"

#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>   // malloc / free

// export_name tells the WebAssembly compiler "let JavaScript see this
// function under this name". When compiling normally (not for the web)
// the macro turns into nothing.
#ifdef __wasm__
#define WASM_EXPORT(name) __attribute__((export_name(name)))
#else
#define WASM_EXPORT(name)
#endif

// ONE gradebook and ONE shell live for as long as the web page is open.
// (They're global so every run() call sees the same students.)
GradeBook gradeBook;
CommandShell shell(gradeBook);

extern "C" {   // "C" style names, so the exported names aren't scrambled

// JavaScript can't create C++ strings, so it asks us for raw memory,
// copies the command text into it, and later gives it back.
WASM_EXPORT("alloc")
char* alloc(int size) {
    return static_cast<char*>(std::malloc(size));
}

WASM_EXPORT("dealloc")
void dealloc(char* pointer) {
    std::free(pointer);
}

// Runs ONE command and returns everything it printed.
WASM_EXPORT("run")
const char* run(const char* line) {
    // "static" = this string survives after run() returns, so the pointer
    // we hand back to JavaScript is still valid while JS reads it.
    static std::string result;

    std::istringstream input(line);   // the command, as a stream we can read
    std::ostringstream output;        // an empty "bag" to collect the answer

    // Swap cin's and cout's buffers for our string streams. From now on,
    // "cin >> x" reads from the command and "cout << y" writes into output.
    std::streambuf* oldCinBuffer = std::cin.rdbuf(input.rdbuf());
    std::streambuf* oldCoutBuffer = std::cout.rdbuf(output.rdbuf());
    std::cin.clear();   // forget any "end of input" flag left from last time

    shell.execute(std::cin, std::cout);   // <-- the real work happens here
    std::cout.flush();

    // Put the real buffers back (important: input/output are about to be
    // destroyed when this function ends).
    std::cin.rdbuf(oldCinBuffer);
    std::cout.rdbuf(oldCoutBuffer);
    std::cin.clear();   // the command hit "end of input"; reset for next time

    result = output.str();
    return result.c_str();
}

}   // extern "C"

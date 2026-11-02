// web_main.cpp - the bridge between the website and the C++ classes.
// Compiled to WebAssembly. JavaScript calls run("some command"); run() points
// cin at that text and cout at an empty string, lets CommandShell handle it,
// and returns whatever was printed.
#include "Student.h"
#include "GradeBook.h"
#include "CommandShell.h"

#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>   // malloc / free

// Makes a function visible to JavaScript (does nothing in a normal build).
#ifdef __wasm__
#define WASM_EXPORT(name) __attribute__((export_name(name)))
#else
#define WASM_EXPORT(name)
#endif

// Global, so every run() call sees the same students.
GradeBook gradeBook;
CommandShell shell(gradeBook);

extern "C" {   // keep the exported names plain

// JavaScript borrows memory from us to pass in the command text.
WASM_EXPORT("alloc")
char* alloc(int size) {
    return static_cast<char*>(std::malloc(size));
}

WASM_EXPORT("dealloc")
void dealloc(char* pointer) {
    std::free(pointer);
}

// Runs one command and returns everything it printed.
WASM_EXPORT("run")
const char* run(const char* line) {
    static std::string result;   // static: still valid after we return

    std::istringstream input(line);
    std::ostringstream output;

    // Redirect cin to read the command and cout to write into "output".
    std::streambuf* oldCinBuffer = std::cin.rdbuf(input.rdbuf());
    std::streambuf* oldCoutBuffer = std::cout.rdbuf(output.rdbuf());
    std::cin.clear();

    shell.execute(std::cin, std::cout);
    std::cout.flush();

    // Put the real buffers back before input/output are destroyed.
    std::cin.rdbuf(oldCinBuffer);
    std::cout.rdbuf(oldCoutBuffer);
    std::cin.clear();

    result = output.str();
    return result.c_str();
}

}   // extern "C"

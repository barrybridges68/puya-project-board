#ifndef FORTH_H
#define FORTH_H

#include <stdint.h>

// Your code here
// Basic Forth VM structures
typedef struct {
    int32_t *sp;              // Data stack pointer
    int32_t *rsp;             // Return stack pointer
    uint8_t *ip;              // Instruction pointer
    int32_t stack[32];        // Data stack
    int32_t rstack[32];       // Return stack
    uint8_t mem[1024];        // Main memory
    struct Word *latest;       // Most recent dictionary entry
} Forth;

// Dictionary word structure 
typedef struct Word {
    struct Word *next;        // Link to previous word
    uint8_t flags;            // Flags byte
    uint8_t len;             // Name length
    char name[32];           // Word name
    void (*code)(Forth*);    // Code pointer
} Word;

#endif // FORTH_H
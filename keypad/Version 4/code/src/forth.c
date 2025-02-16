#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "forth.h"

// Core primitives
void forth_dup(Forth *f) {
    int32_t x = f->stack[f->sp];
    f->stack[++f->sp] = x;
}

void forth_drop(Forth *f) {
    f->sp--;
}

void forth_swap(Forth *f) {
    int32_t x = f->stack[f->sp];
    int32_t y = f->stack[f->sp-1];
    f->stack[f->sp] = y;
    f->stack[f->sp-1] = x;
}

// Inner interpreter
void forth_run(Forth *f) {
    for(;;) {
        Word *w = (Word*)f->ip;
        f->ip += sizeof(Word*);
        w->code(f);
    }
}

/**
 * @brief Search dictionary for a word
 * @param name Word to find
 * @return Word* Found word or NULL
 */
Word* forth_find(const char *name) {
    Word *word = latest;  // Start at most recent word
    size_t len = strlen(name);

    while(word) {
        // Check name length first for quick rejection
        if(word->len == len) {
            // Case-insensitive compare
            if(strncasecmp(name, word->name, len) == 0) {
                return word;
            }
        }
        word = word->next;  // Move to next word in chain
    }
    return NULL;  // Word not found
}

// Simple parser/compiler
void forth_compile(Forth *f, char *word) {
    Word *w = forth_find(word);
    if(w) {
        // Compile word reference
        *(Word**)(f->mem + f->here) = w;
        f->here += sizeof(Word*);
    }
    else if(isdigit(*word)) {
        // Compile literal number
        forth_literal(f, atoi(word));
    }
}

void forth_interpret(char *word) {
    Word *w = forth_find(word);
    if(w) {
        // Execute found word
        w->code();
    }
    else if(isdigit(*word)) {
        // Push number to stack
        forth_push(atoi(word));
    }
    else {
        printf("Word not found: %s\n", word);
    }
}
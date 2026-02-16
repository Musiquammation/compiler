#include "runtimePrint.h"

#include <stdio.h>

int level = 0;

static void printTabs() {
    for (int i = 0; i < level; i++)
        printf("|  ");
}


void runtimePrint_u08( uint8_t value) {
    printTabs();
    printf("runtime08: %d\n", value);
}

void runtimePrint_u16(uint16_t value) {
    printTabs();
    printf("runtime16: %d\n", value);
}

void runtimePrint_u32(uint32_t value) {
    printTabs();
    printf("runtime32: %d\n", value);
}

void runtimePrint_u64(uint64_t value) {
    printTabs();
    printf("runtime64: %p\n", (void*)value);
}

void runtimePrint_line() {
    printTabs();
    printf("\n");
}

void runtimePrint_present(const char* name) {
    printTabs();
    printf("present %s\n", name);
}

void runtimePrint_push() {
    printTabs();
    printf("runtime++\n");
    level++;
}

void runtimePrint_pop() {
    printTabs();
    printf("runtime--\n\n");
    level--;
}

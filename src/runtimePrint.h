#pragma once

#include <stdint.h>

#define RUNTIME_PRINTER_ENABLED 1

void runtimePrint_u08( uint8_t value);
void runtimePrint_u16(uint16_t value);
void runtimePrint_u32(uint32_t value);
void runtimePrint_u64(uint64_t value);
void runtimePrint_line();
void runtimePrint_present(const char* name);
void runtimePrint_push();
void runtimePrint_pop();
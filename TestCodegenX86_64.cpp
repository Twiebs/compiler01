#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#define Assert(expr) assert(expr)

#define ASM_X86_64_IMPLEMENTATION
#include "asm_x86_64.h"

#include <Zydis.hpp>

void PrintInstruction(uint8_t *bytes, size_t count){
  for(size_t i = 0; i < count; i++){
    printf("%X ", (int)bytes[i]);
  }
  printf("\n");
}

int main() {
  uint8_t buffer[1024];
  size_t current_offset = 0;
  current_offset += mov_reg_reg(RAX, RDX, &buffer[current_offset]);
  current_offset += mov_reg_reg(RSI, RBP, &buffer[current_offset]);
  current_offset += mov_regoffset_reg(RSP, 0x8, RDI, &buffer[current_offset]);
  current_offset += sub_reg_const(RSP, 0x20, &buffer[current_offset]);
  current_offset += mov_reg_reg(RDI, RSP, &buffer[current_offset]);


  current_offset += op_reg_regoffset(OPERATION_ADD, RAX, RSP, 0x10, &buffer[current_offset]);
  current_offset += op_reg_regoffset(OPERATION_ADD, RAX, RSP, 0x20, &buffer[current_offset]);
  current_offset += op_reg_regoffset(OPERATION_ADD, RAX, RSP, 0x30, &buffer[current_offset]);

  current_offset += op_reg_regoffset(OPERATION_SUB, RAX, RSP, 0x10, &buffer[current_offset]);
  current_offset += op_reg_regoffset(OPERATION_SUB, RAX, RSP, 0x20, &buffer[current_offset]);
  current_offset += op_reg_regoffset(OPERATION_SUB, RAX, RSP, 0x30, &buffer[current_offset]);

  
  uint8_t test_buffer[] = { 0x48, 	0x03, 	0x44, 	0x24, 	0x10};

  Zydis::MemoryInput input(buffer, current_offset);
  //Zydis::MemoryInput input(test_buffer, sizeof(test_buffer));
  Zydis::InstructionDecoder decoder;
  Zydis::IntelInstructionFormatter formatter;
  decoder.setDisassemblerMode(Zydis::DisassemblerMode::M64BIT);
  decoder.setDataSource(&input);

  Zydis::InstructionInfo info;
  while(decoder.decodeInstruction(info)){
    printf("%s\n", formatter.formatInstruction(info));
  }

  return 0;
}
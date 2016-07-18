#ifndef ASM_X86_64_INCLUDE
#define ASM_X86_64_INCLUDE

//These are only 4 Bits
enum Register {
  //NOTE(Torin)Lower 3 bits of the RM field of the ModR/M Byte
  RAX = 0,
  RCX = 1,
  RDX = 2,
  RBX = 3,
  RSP = 4,
  RBP = 5,
  RSI = 6,
  RDI = 7,
  //NOTE(Torin( Any register 
  R8 = 8,
  R9 = 9,
  R10 = 10,
  R11 = 11,
  R12 = 12,
  R13 = 13,
  R14 = 14,
  R15 = 15
};

//All procedures return the number of bytes that were encoded
//These procedures write to the provided buffer unconditionaly without
//any bounds checking.  To ensure safe writes ensure that your buffer contains
//space for 15bytes of memory(Maximum X86_64 instruction encoding length)
size_t mov_reg_reg(Register dest, Register src, uint8_t *buffer);
size_t add_reg_reg(Register dest, Register src, uint8_t *buffer);
size_t sub_reg_reg(Register dest, Register src, uint8_t *buffer);
size_t op_reg_regoffset(Register dest, Register src, uint64_t offset, uint8_t *buffer);

size_t mov_regoffset_const(Register dest, uint64_t offset, uint64_t value, uint8_t *buffer);
size_t mov_regoffset_reg(Register dest, uint64_t offset, Register src, uint8_t *buffer);

#endif//ASM_X86_64_INCLUDE
//=====================================================================================================
#ifdef ASM_X86_64_IMPLEMENTATION
#undef ASM_X86_64_IMPLEMENTATION

static const uint8_t OPCODE_SUB_REG_CONST = 0x83;

uint8_t encode_rex(uint8_t is_64_bit, uint8_t extend_sib_index, uint8_t extend_modrm_reg, uint8_t extend_modrm_rm) {
  struct Result {
    uint8_t b : 1; //MODRM.rm extention bit (3bits to 4bits)
    uint8_t x : 1; //MODRM.reg extention bit (3bits to 4bits)
    uint8_t r : 1; //extention to SIB.index
    uint8_t w : 1; //64 bit operand is used
    uint8_t fixed : 4; //Fixed bit pattern: value is allways 0100
  } result { extend_modrm_rm, extend_modrm_reg, extend_sib_index, is_64_bit, 0b100 };
  return *(uint8_t*)&result;
}

uint8_t encode_modrm(uint8_t mod, uint8_t rm, uint8_t reg){
  Assert(reg < R8);
  Assert(rm < R8);
  struct Result {
    uint8_t rm  : 3; //Destination register
    uint8_t reg : 3; //Source register
    uint8_t mod : 2; //Addressing mode
  } result { rm, reg, mod };
  return *(uint8_t*)&result;
}

uint8_t encode_sib(uint8_t scale, uint8_t index, uint8_t base){
  struct Result {
    uint8_t base  : 3;
    uint8_t index : 3;
    uint8_t scale : 2;
  } result { base, index, scale };
  return *(uint8_t*)&result;
}

uint8_t encode_disp8(uint64_t value){
  assert(value <= 0xFF);
  uint8_t result = (uint8_t)value;
  return result;
}


size_t mov_reg_reg(Register dest, Register src, uint8_t *buffer){
  static uint8_t OPCODE_MOV_REG_REG = 0x89;
  uint8_t rex_prefix = encode_rex(1, 0, 0, 0);
  uint8_t opcode = OPCODE_MOV_REG_REG;
  uint8_t modrm = encode_modrm(3, dest, src);
  buffer[0] = rex_prefix;
  buffer[1] = opcode;
  buffer[2] = modrm;
  return 3;
}

size_t add_reg_reg(Register dest, Register src, uint8_t *buffer){
  static uint8_t OPCODE_ADD_REG_REG = 0x01;
  uint8_t rex_prefix = encode_rex(1, 0, 0, 0);
  uint8_t opcode = OPCODE_ADD_REG_REG;
  uint8_t modrm = encode_modrm(3, dest, src);
  buffer[0] = rex_prefix;
  buffer[1] = opcode;
  buffer[2] = modrm;
  return 3;
}


size_t sub_reg_reg(Register dest, Register src, uint8_t *buffer){
  static uint8_t OPCODE_SUB_REG_REG = 0x29;
  uint8_t rex_prefix = encode_rex(1, 0, 0, 0);
  uint8_t opcode = OPCODE_SUB_REG_REG;
  uint8_t modrm = encode_modrm(3, dest, src);
  buffer[0] = rex_prefix;
  buffer[1] = opcode;
  buffer[2] = modrm;
  return 3;
}

size_t mov_regoffset_constant(Register dest, uint64_t offset, uint64_t value, uint8_t *buffer) {
  uint8_t rex_prefix = encode_rex(1, 0, 0, 0);
  uint8_t opcode = 0x89;
  uint8_t modrm = encode_modrm(1, dest, dest);
  uint8_t sib = encode_sib(0, 4, 4);
  uint8_t disp = encode_disp8(value);
  buffer[0] = rex_prefix;
  buffer[1] = opcode;
  buffer[2] = modrm;
  buffer[3] = sib;
  buffer[4] = disp;
  return 5;
}

size_t mov_regoffset_reg(Register dest, uint64_t offset, Register src, uint8_t *buffer) {
  uint8_t rex_prefix = encode_rex(1, 0, 0, 0);
  uint8_t opcode = 0x89;
  uint8_t modrm = encode_modrm(1, dest, src);
  uint8_t sib = encode_sib(0, 4, 4);
  uint8_t disp = encode_disp8(offset);
  buffer[0] = rex_prefix;
  buffer[1] = opcode;
  buffer[2] = modrm;
  buffer[3] = sib;
  buffer[4] = disp;
  return 5;
}

size_t sub_reg_const(Register reg, uint64_t value, uint8_t *buffer){
  buffer[0] = encode_rex(1, 0, 0, 0);
  buffer[1] = OPCODE_SUB_REG_CONST;
  buffer[2] = encode_modrm(3, reg, RBP);
  buffer[3] = encode_disp8(value);
  return 4;
}

enum Operation {
  OPERATION_ADD,
  OPERATION_SUB,
  OPERATION_MUL,
  OPERATION_DIV,
  OPERATION_COUNT,
}; 	

size_t op_reg_regoffset(Operation op, Register dest, Register src, uint64_t offset, uint8_t *buffer){
  assert(op == OPERATION_ADD || op == OPERATION_SUB);
  static const uint8_t OPCODES[] = {
    0x03, 0x2B, 0x00, 0x00
  };

  buffer[0] = encode_rex(1, 0, 0, 0);
  buffer[1] = OPCODES[op];
  buffer[2] = encode_modrm(1, src, dest);
  buffer[3] = encode_sib(0, src, src);
  buffer[4] = encode_disp8(offset);
  return 5;
}

#endif//ASM_X86_64_IMPLEMENTATION
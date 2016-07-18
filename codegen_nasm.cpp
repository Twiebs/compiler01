enum Register {
  RAX = 0,
  RCX = 1,
  RDX = 2,
  RBX = 3,
  RSP = 4,
  RBP = 5,
  RSI = 6,
  RDI = 7,
  R8 = 8,
  R9 = 9,
  R10 = 10,
  R11 = 11,
  R12 = 12,
  R13 = 13,
  R14 = 14,
  R15 = 15,
  Register_COUNT
};

enum RegisterState {
  RegisterState_AVAILABLE,
  RegisterState_USED,
};

static const char *REGISTER_NAME[] = {
  "RAX",
  "RCX",
  "RDX", 
  "RBX", 
  "RSP", 
  "RBP", 
  "RSI", 
  "RDI", 
  "R8",
  "R9",
  "R10", 
  "R11", 
  "R12", 
  "R13", 
  "R14", 
  "R15", 
};

struct CodegenContext {
  //FILE *file;
  RegisterState regstate[Register_COUNT];

  uint8_t *buffer;
  size_t bufferSize;
  size_t bufferUsed; 
};

static inline
size_t mov_regoffset_reg(uint64_t dest, uint64_t offset, uint64_t src, uint8_t *buffer){
  return sprintf((char*)buffer, "  mov qword [%s + 0x%lX], %s\n", REGISTER_NAME[dest], offset, REGISTER_NAME[src]);
}

static inline
size_t mov_reg_regoffset(uint64_t dest, uint64_t src, uint64_t offset, uint8_t *buffer){
  return sprintf((char*)buffer, "  mov %s, [%s + 0x%lX]\n", REGISTER_NAME[dest], REGISTER_NAME[src], offset);
}

static inline
size_t call_const(uint64_t value, uint8_t *buffer){
  return sprintf((char *)buffer, "  call 0x%lX\n", value);
}


static inline
size_t add_reg_reg(uint64_t dest, uint64_t src, uint8_t *buffer){
  return sprintf((char *)buffer, "  add %s %s\n", REGISTER_NAME[dest], REGISTER_NAME[src]);
}

static inline
size_t sub_reg_reg(uint64_t dest, uint64_t src, uint8_t *buffer){
  return sprintf((char *)buffer, "  sub %s %s\n", REGISTER_NAME[dest], REGISTER_NAME[src]);
}

static inline
size_t sub_reg_const(Register reg, uint64_t value, uint8_t *buffer){
  return sprintf((char *)buffer, "  sub %s, 0x%lX\n", REGISTER_NAME[reg], value);
}

#include "CodegenASM.cpp"

static inline
void WriteFmt(CodegenContext *ctx, const char *fmt, ...){
  va_list args;
  va_start(args, fmt);
  size_t bytes_written = vsprintf((char *)&ctx->buffer[ctx->bufferUsed], fmt, args);
  ctx->bufferUsed += bytes_written;
  va_end(args);
}

#if 0
enum ExpressionValueType {
  ExpressionValue_None,
  ExpressionValue_Literal,
  ExpressionValue_Register,
  ExpressionValue_Address
};

static ExpressionValueType CodegenBinop(BinaryOperation *binop, ExpressionValueType parentOpExprType, char *outValue, CodegenContext *ctx);



static ExpressionValueType
CodegenExpression(Node *node, char *out, CodegenContext *ctx) {
  switch(node->nodeType){

    case NodeType_Variable:{
      auto variable = (Variable *)node;
      sprintf(out, "[rsp + 0x%lX]", variable->offset);
      return ExpressionValue_Address;
    } break;

    case NodeType_Literal:{
      auto literal = (Literal *)node;
      sprintf(out, "0x%lX", literal->intValue);
      return ExpressionValue_Literal;
    } break;

    case NodeType_BinaryOperation: {
      auto exprValueType = CodegenBinop((BinaryOperation *)node,  ExpressionValue_None, out, ctx);
      return exprValueType;
    } break;

    default: {
      Assert(false);
    }break;
  }
}


static Register
GetFirstAvailableRegister(CodegenContext *ctx){
  for(size_t i = 0; i < Register_COUNT; i++){
    if(ctx->regstate[i] == RegisterState_AVAILABLE){
      ctx->regstate[i] = RegisterState_USED;
      return (Register)i;
    }

  }
  Assert(false);
}

static void
SetRegisterAvailable(const char *reg, CodegenContext *ctx){
  for(size_t i = 0; i < Register_COUNT; i++){
    if(strcmp(reg, REGISTER_NAME[i]) == 0){
      ctx->regstate[i] = RegisterState_AVAILABLE;
      return;
    }
  }
}

static Register
GetRegisterFromString(const char *reg){
  for(size_t i = 0; i < Register_COUNT; i++){
    if(strcmp(reg, REGISTER_NAME[i]) == 0){
      return (Register)i;
    }
  }
  assert(false);
  return Register(0);
}

static const char *
OperationStrings[] = {
  "add",
  "sub",
  "mul",
  "div"
};

#endif

//TODO(Torin) We need to look ahead in the list of statements and find the next call within the code block
//We then backtrace though the list of statements and...
//Actualy this should be done as a preprocess step on the AST where we find every Call statement within a givien 
//Codeblock and we check the arguments to see where they are declared / mutated and set a flag(index) to use the
//respective x86_64 register a similar thing can be done with return values if nessecary

#if 0
static ExpressionValueType
CodegenBinop(BinaryOperation *binop, ExpressionValueType parentOpExprType, char *outValue, CodegenContext *ctx){
  char lhsValue[64], rhsValue[64];
  auto lhsValueType = CodegenExpression(binop->lhs, lhsValue, ctx);
  auto rhsValueType = CodegenExpression(binop->rhs, rhsValue, ctx);
  OperationType operation = OperationType(binop->operation - TokenType_Plus);

  if(lhsValueType == ExpressionValue_Register){
    if(operation == OperationType_MUL || operation == OperationType_DIV){
      auto reg = GetRegisterFromString(lhsValue);
      if(reg != RAX){
        //TODO Save rax
        WriteFmt(ctx, "  mov rax, %s\n", lhsValue);
      }
      WriteFmt(ctx, "  mov rdx, %s\n", rhsValue);
      WriteFmt(ctx, "  %s rdx\n", OperationStrings[operation]);
      sprintf(outValue, "rax");
      return ExpressionValue_Register;
    } else {
      WriteFmt(ctx, "  %s %s, %s\n", OperationStrings[operation], lhsValue, rhsValue);
      if(rhsValueType == ExpressionValue_Register){
        SetRegisterAvailable(rhsValue, ctx);
      }
      sprintf(outValue, lhsValue);
      return ExpressionValue_Register;
    }
  }

  else if(lhsValueType == ExpressionValue_Literal || lhsValueType == ExpressionValue_Address){
    if(operation == OperationType_MUL || operation == OperationType_DIV){
      //TODO(Torin) The compiler should do a look ahead on the next expression to be evaluated 
      //and see where the evaluation expects the target RHS value to reside and pass that into the 
      //child codegeneration expression so we can optimize to insert values directly when they need to be 
      if(operation == OperationType_DIV){
        //TODO(Torin) Ensure RAX, RDX are free
        //And Get a unused register for RCX
        WriteFmt(ctx, "  xor rdx, rdx\n");
        WriteFmt(ctx, "  mov rax, %s\n", lhsValue);
        if(rhsValueType != ExpressionValue_Register)
          WriteFmt(ctx, "  mov rcx, %s\n", rhsValue);
        WriteFmt(ctx, "  div rcx\n");
        sprintf(outValue, "rax");
        return ExpressionValue_Register;
      }

      
      WriteFmt(ctx, "  mov rax, %s\n", lhsValue);
      WriteFmt(ctx, "  mov rdx, %s\n", rhsValue);
      WriteFmt(ctx, "  %s rdx\n", OperationStrings[operation]);
      sprintf(outValue, "rax");
      return ExpressionValue_Register;
    } else {
      auto reg = GetFirstAvailableRegister(ctx);
      WriteFmt(ctx, "  mov %s, %s\n", REGISTER_NAME[reg], lhsValue);
      WriteFmt(ctx, "  %s %s, %s\n", OperationStrings[operation], REGISTER_NAME[reg], rhsValue);
      if(rhsValueType == ExpressionValue_Register){
        SetRegisterAvailable(rhsValue, ctx);
      }
      sprintf(outValue, REGISTER_NAME[reg]);
      return ExpressionValue_Register;
    }


  }
  Assert(false);
}


static const char *
GetNASMOperationSize(size_t size){
  switch(size){
    case 1: return "byte"; break;
    case 2: return "word"; break;
    case 4: return "dword"; break;
    case 8: return "qword"; break;
    default: Assert(false); break;
  }
}
#endif

static void 
CodegenBlockBody(CodeBlock *block, CodegenContext *ctx){
  for(size_t i = 0; i < block->statements.count; i++){
    auto statement = block->statements[i];
    switch(statement->nodeType){
      case NodeType_Variable: {
        WriteFmt(ctx, ";%.*s\n", (int)statement->textLength, statement->text);
        auto variable = (Variable *)statement;
        if(variable->initalExpr == 0) assert(false);

        auto exprValue = CodegenExpression(variable->initalExpr, ctx);
        if(exprValue.type == ExpressionValue::Register) {
          codegen(ctx, mov_regoffset_reg, RSP, variable->offset, exprValue.reg);
          codegen(ctx, mov_reg_regoffset, RDI, RSP, variable->offset);
          codegen(ctx, call_const, 0x0);
        } else {
          Assert(false);
        }
      } break;
    }
  }
}

static void
CodegenStartProcedure(Procedure *procedure, CodegenContext *ctx){
  //TODO(Torin) Main function arguments
  WriteFmt(ctx, "global _start\n");
  WriteFmt(ctx, "_start:\n");
  CodegenBlockEntry(procedure, ctx);
  CodegenBlockBody(procedure, ctx);
  WriteFmt(ctx, "  mov rax, 60\n");
  WriteFmt(ctx, "  syscall\n");
}

static void
CodegenProcedure(Procedure *procedure, CodegenContext *ctx){
  WriteFmt(ctx, "global %s\n", procedure->name);
  WriteFmt(ctx, "%s:\n", procedure->name);
  CodegenBlockEntry(procedure, ctx);

  //TODO(Torin) Inefficantly push input registers into stack memory
  //to keep codgen simple for the time beging
  size_t currentIntegerInputCount = 0;
  size_t currentFloatingPointInputCount = 0;
  for(size_t i = 0; i < procedure->inputCount; i++){
    static const Register SYSTEMV_CALL_INT_REGISTERS[] = {
      RDI, RSI, RDX, RCX, R8, R9
    };

    auto variable = procedure->variables[i];
    if(IsIntegerType(variable->type)){
      codegen(ctx, mov_regoffset_reg, RSP, variable->offset, SYSTEMV_CALL_INT_REGISTERS[currentIntegerInputCount]);
      currentIntegerInputCount++;
    }else if(IsFloatingPointType(variable->type)){
      //TODO(TOrin)
      Assert(false);
    }
  }

  CodegenBlockBody(procedure, ctx);
  WriteFmt(ctx, "  ret\n");
}

static inline
void CodegenNASMFile(CodeBlock *root, CodegenContext *ctx){
  WriteFmt(ctx, "section .text\n");
  WriteFmt(ctx, "%%include \"NasmUtils.asm\"\n\n");
  for(size_t i = 0; i < root->procedures.count; i++){
    if(strcmp(root->procedures[i]->name, "Main") == 0){
      CodegenStartProcedure(root->procedures[i], ctx);
    } else {
      CodegenProcedure(root->procedures[i], ctx);
    }
  }
}

static int
CodegenAST(const char *filename, CodeBlock *root){
  FILE *file = fopen(filename, "wb");
  if(file == 0) return 0;

  CodegenContext ctx = {};
  ctx.bufferSize = 1024*1024;
  ctx.buffer = (uint8_t *)malloc(ctx.bufferSize);
  ctx.bufferUsed = 0;
  CodegenNASMFile(root, &ctx);

  fwrite(ctx.buffer, 1, ctx.bufferUsed, file);
  fclose(file);

  system("nasm -g -felf64 output.asm");
  system("ld output.o -o output");
  return 1;
}
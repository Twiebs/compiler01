
static inline
void CodegenSafetyCheck(CodegenContext *ctx){
  if(ctx->bufferUsed + 15 > ctx->bufferSize){
    Assert(false);
  }
}

#define codegen(ctx, proc, ...) { CodegenSafetyCheck(ctx); ctx->bufferUsed += proc(__VA_ARGS__, &ctx->buffer[ctx->bufferUsed]); } 

static void 
CodegenBlockEntry(CodeBlock *block, CodegenContext *ctx){
  //TODO(Torin) For now this is going to be wastefull
  //and always pad to an 8 byte boundray for simplicity
  for(size_t i = 0; i < block->variables.count; i++){
    auto variable = block->variables[i];
    variable->offset = block->variableMemoryRequired;
    block->variableMemoryRequired += variable->type->size;
    block->variableMemoryRequired = (block->variableMemoryRequired + 0x7) & ~0x7;
  }
  codegen(ctx, sub_reg_const, RSP, block->variableMemoryRequired);
}

struct ExpressionValue {
  enum ExpressionValueType {
    Register,
    Constant,
    RegisterOffset,
  } type;

  union {
    struct {
      uint64_t reg;
      uint64_t offset;
    };

    uint64_t int_value;
    float64_t float_value;
  };
};

static ExpressionValue CodegenBinop(BinaryOperation *binop, CodegenContext *ctx);

static ExpressionValue
CodegenExpression(Node *node, CodegenContext *ctx) {
  switch(node->nodeType){

    case NodeType_Variable:{
      auto variable = (Variable *)node;
      ExpressionValue result;
      result.type = ExpressionValue::RegisterOffset;
      result.reg = RSP;
      result.offset = variable->offset;
      return result;
    } break;

    case NodeType_Literal:{
      auto literal = (Literal *)node;
      ExpressionValue result;
      result.type = ExpressionValue::Constant;
      result.int_value = literal->intValue;
      return result;
    } break;

    case NodeType_BinaryOperation:{
      auto result = CodegenBinop((BinaryOperation *)node, ctx);
      return result;
    } break;

    default: {
      Assert(false);
    }break;
  }
}

//TODO(Torin) We need to look ahead in the list of statements and find the next call within the code block
//We then backtrace though the list of statements and...
//Actualy this should be done as a preprocess step on the AST where we find every Call statement within a givien 
//Codeblock and we check the arguments to see where they are declared / mutated and set a flag(index) to use the
//respective x86_64 register a similar thing can be done with return values if nessecary

static ExpressionValue
CodegenBinaryInstruction(OperationType op, ExpressionValue lhs, ExpressionValue rhs, CodegenContext *ctx){
  Assert(op == OperationType_ADD || op == OperationType_SUB);
  if(lhs.type == ExpressionValue::Register && rhs.type == ExpressionValue::Register){
    if(op == OperationType_ADD){
      codegen(ctx, add_reg_reg, lhs.reg, rhs.reg);
    } else if (op == OperationType_SUB){
      codegen(ctx, sub_reg_reg, lhs.reg, rhs.reg);
    } else {
      Assert(false);
    }

    ExpressionValue result;
    result.type = ExpressionValue::Register;
    result.reg = lhs.reg;
    return result;
  } 
  
  else if(lhs.type == ExpressionValue::RegisterOffset && rhs.type == ExpressionValue::RegisterOffset){
    //TODO(Torin) Make sure RAX is availiable
    codegen(ctx, mov_reg_regoffset, RAX, lhs.reg, lhs.offset);
    codegen(ctx, )

    Assert(false);


  }
  
  else if (lhs.type == ExpressionValue::Register && rhs.type == ExpressionValue::RegisterOffset){
    Assert(false);
  }

  Assert(false);
  return ExpressionValue {};
}

static ExpressionValue
CodegenBinop(BinaryOperation *binop, CodegenContext *ctx){
  auto lhsValue = CodegenExpression(binop->lhs, ctx);
  auto rhsValue = CodegenExpression(binop->rhs, ctx);
  OperationType operation = OperationType(binop->operation - TokenType_Plus);
  if(operation == OperationType_MUL || operation == OperationType_DIV){
    Assert(false);
  } else {
    return CodegenBinaryInstruction(operation, lhsValue, rhsValue, ctx);
  }
}


#if 0
  if(lhsValue.type == ExpressionValue::Register){
    if(operation == OperationType_MUL || operation == OperationType_DIV){
      if(lhsValue.reg != RAX) codegen(ctx, mov_reg_reg, RAX, lhsValue.reg);
      if(rhsValue.type == ExpressionValue::Register){
        codegen(ctx, mul_reg, rhsValue.reg);
      }else if(rhsValue.type == ExpressionValue::RegisterOffset) {
        codgen(ctx, mul_regoffset, rhsValue.reg, rhsValue.offset);
      }else if(rhsValue.type == ExpressionValue::Constant) {
        codegen(ctx, mul_const, rhsValue.int_value);
      }

      ExpressionValue result;
      result.type = ExpressionValue::Register;
      result.reg = RAX;
      return result;
    }

    CodegenBinaryInstruction(operation, lhsValue, rhsValue, ctx);
    ExpressionValue result = {};
    result.type = ExpressionValue::Register;
    result.reg = lhsValue.reg;
    return result;
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
#endif


#pragma clang diagnostic ignored "-Wformat-security"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

typedef float float32_t;
typedef double float64_t;


#define Assert(expr) assert(expr)
#define LogError(...) LogErrorInternal(__VA_ARGS__)
#define LogDebug(...) { printf(__VA_ARGS__); printf("\n"); }

static inline
void LogErrorInternal(const char *fmt, ...){
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}

#define SymbolMetaList \
_(Plus, "+") \
_(Minus, "-") \
_(Asterisk, "*") \
_(FowardSlash, "/") \
_(DoubleColon, "::") \
_(Colon, ":") \
_(ParenOpen, "(")\
_(ParenClose, ")")\
_(BlockOpen, "{")\
_(BlockClose, "}")\
_(Comma, ",")\

enum TokenType {
    TokenType_Invalid,
#define _(name, string) TokenType_##name,
    SymbolMetaList
#undef _
    
    TokenType_Identifier,
    TokenType_Number,
    TokenType_EndOfBuffer,
    TokenType_Count
};

static const char *TokenNameStrings[] = {
    "TokenType_Invalid",
#define _(name, string) "TokenType_" #name,
SymbolMetaList
#undef _
    "TokenType_Identifier",
    "TokenType_Number",
    "TokenType_EndOfBuffer",
    "TokenType_Count"
};

struct Token {
  TokenType type;
  const char *text;
  uint64_t length;
  uint32_t lineNumber;
  uint32_t columnNumber;
  uint32_t fileID;
};

static int
StringMatches(const char *a, size_t lengthA, const char *b) {
    for(size_t i = 0; i < lengthA; i++){
        if(a[i] != b[i]) return 0;
    }
    return 1;    
}

static int
CStringMatches(const char *src, const char *target){
  while(*src != 0) {
    if(*src != *target) return 0;
    src++; target++;
  }
  return 1;
}

#include "AST.cpp"
#include "codegen_nasm.cpp"

static Type *TypeS8;
static Type *TypeS16;
static Type *TypeS32;
static Type *TypeS64;
static Type *TypeU8;
static Type *TypeU16;
static Type *TypeU32;
static Type *TypeU64;
static Type *TypeF32;
static Type *TypeF64;

struct ParseState {
  const char *current;
  uint32_t lineNumber;
  uint32_t columnNumber;
  CodeBlock *currentBlock;
  Token token;
};

static void
LexNextToken(ParseState *state){
    Token token = {};
    token.type = TokenType_Invalid;
    token.lineNumber = state->lineNumber;
    token.columnNumber = state->columnNumber;
    
    while(isspace(*state->current)) state->current++;
    if(isdigit(*state->current)) {
        token.type = TokenType_Number;
        token.text = state->current;
        while(isdigit(*state->current)) state->current++;
        token.length = state->current - token.text;
    }
    
    else if(isalpha(*state->current)  || *state->current == '_'){
        token.type = TokenType_Identifier;
        token.text = state->current;
        while(isalnum(*state->current) || *state->current == '_') state->current++;
        token.length = state->current - token.text;
    }
    
#define _(name, string) else if (StringMatches((const char *)string, sizeof(string) - 1, state->current)) { \
        token.type = TokenType_##name; \
        token.text = state->current; \
        token.length = sizeof(string) - 1; \
        state->current += sizeof(string) - 1; \
    }
    
    SymbolMetaList
    #undef _
    
    else {
        token.type = TokenType_EndOfBuffer;
    }
       
    state->token = token;
}

static Node *ParseStatement(ParseState *state);

static inline
void PrintToken(Token token){
    printf("%.*s, %s\n", (int)token.length, token.text, TokenNameStrings[token.type] );
}

static inline
void PrintExpression(Node *expr){
  LogDebug("Expression: %s", NODE_TYPE_NAMES[expr->nodeType]);
}

static Node*
ParseExpressionLHS(ParseState *state){
  switch(state->token.type){
    case TokenType_Number: {
      Token firstToken = state->token;
      char temp[firstToken.length];
      memcpy(temp, firstToken.text, firstToken.length);
      temp[firstToken.length] = 0;
      bool isFloat = false;
      for(size_t i = 0; i < firstToken.length; i++){
        if(temp[i] == '.') { 
          isFloat = true;
          break;
        }
      }

      Literal *literal = CreateNodeFromToken(Literal, firstToken);
      if(isFloat) literal->floatValue = atof(temp); 
      else literal->intValue = atoi(temp);
      LexNextToken(state);
      return literal;
    } break;

    case TokenType_Identifier:{
      auto variable = FindVariable(state->token, state->currentBlock);
      if(variable == nullptr){
        Assert(false); //TODO
      }
      
      LexNextToken(state);
      return variable;
    } break;

    default: {
      LogError("Expected identifier or literal in expression");
    } break;
  }
  return nullptr;
}

static Node *
ParseExpression(ParseState *state){
  auto lhs = ParseExpressionLHS(state);
  if(lhs == nullptr) return nullptr;

  switch(state->token.type){
    case TokenType_Plus:
    case TokenType_Minus:
    case TokenType_Asterisk:
    case TokenType_FowardSlash: {
      auto binop = CreateNodeFromToken(BinaryOperation, state->token);
      binop->operation = state->token.type;
      binop->lhs = lhs;
      LexNextToken(state);
      binop->rhs = ParseExpression(state);
      if(binop->rhs == nullptr) return nullptr;
      return binop;
    } break;
    default: {
      return lhs;
    } break;
  }
}

static void
ParseBlock(ParseState *state){
  while(state->token.type != TokenType_EndOfBuffer && state->token.type != TokenType_BlockClose){
    auto statement = ParseStatement(state);
    if(statement->nodeType == NodeType_Procedure){
      ArrayAdd((Procedure *)statement, &state->currentBlock->procedures);
    } else {
      ArrayAdd(statement, &state->currentBlock->statements);
    }
  }

  if(state->token.type == TokenType_BlockClose){
    LexNextToken(state);
  }
}

//TODO(Torin) Handle nested parrens / decide how this should work
static void ParseArgumentList(ParseState *state){
  Assert(state->token.type == TokenType_ParenOpen);
  LexNextToken(state);
  while(state->token.type != TokenType_ParenClose &&
  state->token.type != TokenType_EndOfBuffer){
    if(state->token.type != TokenType_Identifier){
      LogError("Expected identifier starting expected variable decleration");
      return;
    }

    Token identToken = state->token;
    LexNextToken(state);
    if(state->token.type == TokenType_Colon){
      LexNextToken(state);

      if(state->token.type != TokenType_Identifier){
        LogError("Expected type identifier");
        return;
      }

      Token typeToken = state->token;

      LexNextToken(state);

      auto variable = CreateVariableFromToken(identToken);
      variable->type = FindTypeFromToken(typeToken, state->currentBlock);
      if(variable->type == nullptr){
        LogError("Failed to resolve type %.*s", (int)typeToken.length, typeToken.text);
      }

      ArrayAdd(variable, &state->currentBlock->variables);

      if(state->token.type == TokenType_ParenClose){
        break;
      }else if(state->token.type != TokenType_Comma){
        LogError("Expected comma after argument decleration");
      }

      LexNextToken(state);
    }else{
      LogError("Expected colon after identifier in variable decleration");
    }
  }
}

static inline
int CompareTokenConsumeOnTrue(TokenType type, ParseState *state){
  if(state->token.type == type){
    LexNextToken(state);
    return 1;
  }
  return 0;
}

static Node *
ParseStatement(ParseState *state){
  if(state->token.type != TokenType_Identifier){
    LogError("All statements must begin with an identifier");
  }

  Token identToken = state->token;
  LexNextToken(state);

  if(state->token.type == TokenType_Colon){
    LexNextToken(state);
    auto variable = CreateVariableFromToken(identToken);
    variable->type = TypeS64;
    variable->initalExpr = ParseExpression(state);
    variable->textLength = (variable->initalExpr->text + variable->initalExpr->textLength) - variable->text;
    ArrayAdd(variable, &state->currentBlock->variables);
    return variable;
  }
  
  else if(CompareTokenConsumeOnTrue(TokenType_DoubleColon, state)){
    auto thisBlock = state->currentBlock;
    auto procedure = CreateProcedure(identToken, state->currentBlock);
    state->currentBlock = procedure;

    //Parse procedure input list
    if(state->token.type == TokenType_ParenOpen){
      ParseArgumentList(state);
      Assert(state->token.type == TokenType_ParenClose);
      LexNextToken(state);//Eat ParenClose
      procedure->inputCount = procedure->variables.count;
    } else {
      LogError("Expected input parameter list when defining procedure");
    }

    //Parse procedure output list
    if(state->token.type == TokenType_ParenOpen){
      ParseArgumentList(state);
      Assert(state->token.type == TokenType_ParenClose);
      LexNextToken(state);
      procedure->outputCount = procedure->variables.count - procedure->inputCount;
    }
  
    //Parse procedure body  
    if(state->token.type == TokenType_BlockOpen){
      LexNextToken(state);  
      ParseBlock(state);
    } else {
      LogError("Expected block to open after procedure definition");
      return nullptr;
    }

    state->currentBlock = thisBlock;
    return procedure;
  }

  Assert(false);
  return nullptr;
}

static inline
int ParseFile(const char *filename, CodeBlock *root){
    FILE *file = fopen(filename, "rb");
    if(file == 0) { 
        LogError("Could not open file %s\n", filename);
        return 0;
    }
    
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t *buffer = (uint8_t *)malloc(fileSize + 1);
    fread(buffer, 1, fileSize, file);
    buffer[fileSize] = 0;
    fclose(file);
    
    LogDebug("Succuessfully opened %s [%d bytes]", filename, (int)fileSize);
    
    ParseState state = {};
    state.current = (const char *)buffer;
    state.currentBlock = root;

    LexNextToken(&state);


    ParseBlock(&state);
    if(state.token.type != TokenType_EndOfBuffer)
      assert(false);

    return 1;
}

int main() {
  CodeBlock root = {};

  TypeS8  = CreateInternalType("S8",  1, NodeType_IntType, &root);
  TypeS16 = CreateInternalType("S16", 2, NodeType_IntType, &root);
  TypeS32 = CreateInternalType("S32", 4, NodeType_IntType, &root);
  TypeS64 = CreateInternalType("S64", 8, NodeType_IntType, &root);

  TypeU8  = CreateInternalType("U8",  1, NodeType_IntType, &root);
  TypeU16 = CreateInternalType("U16", 2, NodeType_IntType, &root);
  TypeU32 = CreateInternalType("U32", 4, NodeType_IntType, &root);
  TypeU64 = CreateInternalType("U64", 8, NodeType_IntType, &root);

  TypeF32 = CreateInternalType("F32", 4, NodeType_FloatType, &root);
  TypeF64 = CreateInternalType("F64", 8, NodeType_FloatType, &root);

  ParseFile("test.sky", &root);
  CodegenAST("output.asm", &root);
  return 0;
}
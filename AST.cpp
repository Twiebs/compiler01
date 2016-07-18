template<typename T>
struct DynamicArray{
  size_t capacity;
  size_t count;
  T *data;

  inline T& operator[](size_t index){
    T& result = data[index];
    return result;
  }
};

template<typename T>
void ArrayAdd(const T& element, DynamicArray<T> *array){
  if(array->count + 1 > array->capacity){
    array->capacity = array->capacity + 10;
    T *newData = (T *)malloc(sizeof(T) * array->capacity);
    memcpy(newData, array->data, array->count * sizeof(T));
    if(array->data != nullptr) free(array->data);
    array->data = newData;
  }
  array->data[array->count++] = element;
}


enum NodeType {
    NodeType_BinaryOperation,
    NodeType_Literal,
    NodeType_Variable,
    NodeType_Procedure,

    //NOTE(Torin) Bad Idea(Probablay)!!!
    NodeType_IntType,
    NodeType_FloatType,
    NodeType_StructType,
};

static const char *NODE_TYPE_NAMES[] = {
    "BinaryOperation",
    "Literal",
    "Variable"
};

struct Node {
  NodeType nodeType;
  uint32_t lineNumber;
  uint32_t columnNumber;
  uint32_t fileID;
  const char *text;
  size_t textLength;
};

struct Type : Node {
  char *name;
  size_t size;
};

struct Variable : Node {
  char *name;
  Node *initalExpr;
  size_t offset;
  Type *type;
};

enum OperationType {
  OperationType_ADD,
  OperationType_SUB,
  OperationType_MUL,
  OperationType_DIV,
};

struct BinaryOperation : Node {
  TokenType operation;
  Node *lhs;
  Node *rhs;  
};

struct Literal : Node {
    union {
        double floatValue;
        int64_t intValue;
    };
};

struct Procedure;

//TODO(Torin) Decide how block nodes should really be stored after
//a reall allocation strategy is implemented!
struct CodeBlock : Node {
  CodeBlock *parent;
  DynamicArray<Node *> statements;
  DynamicArray<Variable *> variables;
  DynamicArray<Procedure *> procedures;
  DynamicArray<Type *> types;
  size_t variableMemoryRequired;
};

struct Procedure : CodeBlock {
  uint32_t inputCount;
  uint32_t outputCount;

  char *name;
};

#define IncompleteCodePath Assert(false)

#define CreateNodeFromToken(TNodeType, token) (TNodeType*)InternalAllocateNodeFromToken(NodeType_##TNodeType, sizeof(TNodeType), token)

static inline
Node *InternalAllocateNodeFromToken(NodeType nodeType, size_t nodeSize, const Token& token){
    Node *node = (Node *)malloc(nodeSize);
    memset(node, 0, nodeSize);
    node->nodeType = nodeType;
    node->lineNumber = token.lineNumber;
    node->columnNumber = token.columnNumber;
    node->fileID = token.fileID;
    node->text = token.text;
    node->textLength = token.length;
    LogDebug("CreateNode: %s [%d:%d]", NODE_TYPE_NAMES[nodeType], node->lineNumber, node->columnNumber);
    return node;
}

static inline
Variable* CreateVariableFromToken(const Token& token){
  auto result = CreateNodeFromToken(Variable, token);
  result->name = (char *)malloc(token.length + 1);
  memcpy(result->name, token.text, token.length);
  result->name[token.length] = 0;
  return result;
}

static inline
Variable *FindVariable(const Token& token, CodeBlock *block){
  for(size_t i = 0; i < block->variables.count; i++){
    auto variable = block->variables[i];
    if(CStringMatches(variable->name, token.text)) return variable;
  }
  return nullptr;
}

static inline
Type *FindTypeFromToken(const Token& token, CodeBlock *block){
  while(block != nullptr){
    for(size_t i = 0; i < block->types.count; i++){
      auto type = block->types[i];
      if(StringMatches(token.text, token.length, type->name)){
        return type;
      }
    }
    block = block->parent;
  }
  return nullptr;
}

static inline
int IsIntegerType(const Type *type){
  if(type->nodeType == NodeType_IntType) return 1;
  return 0;
}

static inline
int IsFloatingPointType(const Type *type){
  if(type->nodeType == NodeType_FloatType) return 1;
  return 0;
}

static inline
Type *CreateInternalType(const char *name, size_t size, NodeType nodeType, CodeBlock *block){
  auto type = (Type *)malloc(sizeof(Type));
  size_t nameLength = strlen(name);
  type->name = (char *)malloc(nameLength + 1);
  memcpy(type->name, name, nameLength);
  type->name[nameLength] = 0;

  type->size = size;
  type->nodeType = nodeType;
  ArrayAdd(type, &block->types);
  return type;
}

static inline
Procedure *CreateProcedure(const Token& token, CodeBlock *parent){
  auto procedure = CreateNodeFromToken(Procedure, token);
  procedure->name = (char *)malloc(token.length + 1);
  memcpy(procedure->name, token.text, token.length);
  procedure->name[token.length] = 0;
  procedure->parent = parent;
  return procedure;
}
#include <stdint.h>

uint64_t test(uint64_t a, uint64_t b){
  uint64_t result = a * b;
  return result;
}

int main(){
  volatile uint64_t a = 2;
  volatile uint64_t b = 2;
  uint64_t result = test(a, b);
  return result;
}
bits 64

global _start

start:
  add rax, [rsp + 0x10]
  add rax, [rsp + 0x20]
  add rax, [rsp + 0x30]


foo db "foo"

Syswrite:
  mov rsi, rdi
	mov rax, 1
  mov rdi, 1
  mov rdx, 1
  syscall
  ret

PrintFourBits:
  push rax
  push rdx
  push rsi

  sub rsp, 1
  cmp rdi, 0xA
  jge .PrintHexLetter

.PrintHexNumber:
  add rdi, '0'
  mov [rsp], dil
  jmp .WriteStdout
.PrintHexLetter:
  add rdi, ('A' - 10)
  mov [rsp], dil

.WriteStdout:
  mov rax, 1
  mov rdi, 1
  mov rsi, rsp
  mov rdx, 1
  syscall

  add rsp, 1
  pop rsi
  pop rdx
  pop rax
  ret

PrintByte:
  push rax
  mov rax, rdi

  shr rdi, 4
  call PrintFourBits

  mov rdi, rax
  and rdi, 0xF
  call PrintFourBits
  
  pop rax
  ret


PrintInt64:
  push rax
  push rsi
  push rdx

  push rdi
  sub rsp, 2
  mov [rsp + 0], byte '0'
  mov [rsp + 1], byte 'x'

  mov rax, 1
  mov rdi, 1
  mov rsi, rsp
  mov rdx, 2
  syscall

  add rsp, 2
  pop rax

%macro PrintByteAtOffset 1
  mov rdi, rax
  shr rdi, %1
  and rdi, 0xFF
  call PrintByte
%endmacro

  PrintByteAtOffset 56
  PrintByteAtOffset 48
  PrintByteAtOffset 40
  PrintByteAtOffset 32
  PrintByteAtOffset 24
  PrintByteAtOffset 16
  PrintByteAtOffset 8
  PrintByteAtOffset 0

  sub rsp, 1
  mov [rsp], byte 0xA ;\n
  mov rdi, rsp
  call Syswrite
  add rsp, 1
  
  pop rdx
  pop rsi
  pop rax
  ret
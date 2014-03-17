%include "os_dependent_stuff.asm"


global fhtw_new
fhtw_new:
  push rdi ; store requested size for later

; rdi is the size of the the table
; we need 3 pieces of metadata - occupancy, capacity, size of info word in bits
; and 3 arrays - keys, values, and hop info words
  inc rdi
  mov rax, 24
  mul rdi

; load arguments to mmap
  mov rsi, rax                      ; size 
  mov r10, MAP_SHARED | MAP_ANON    ; not backed by a file
  mov r8, -1                        ; file descripter is -1
  mov r9, 0                         ; no offset
  mov rax, SYSCALL_MMAP
  mov rdx, PROT_READ | PROT_WRITE   ; read/write access
  mov rdi, 0                        ; we don't care about the particular address
  
  syscall
  test rax, rax
  ;js .error ; local error label
  
; initialize metadata
  ; occupancy at [rax] has already been set to 0 by mmap
  pop r11
  mov [rax + 8], r11                ; store capacity
  dec r11
  bsr r11, r11                           
  mov [rax + 16], r11               ; store size of info word - logarithmic in capacity
  
  ret
  

global fhtw_free
fhtw_free:
  ; put size of memory to be freed in rsi
  mov r11, [rdi + 8]
  inc r11
  mov rax, 24
  mul r11

  ; arguments to munmap
  ; rdi already set to address
  mov rsi, rax
  mov rax, SYSCALL_MUNMAP

  syscall
  ret


global fhtw_set
fhtw_set:
  

global fhtw_get
fhtw_get:

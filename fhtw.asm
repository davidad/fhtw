%include "os_dependent_stuff.asm"

%macro hash_function 5
; clobbers rcx and all arguments
; %1 = key pointer
; %2 = key length
; %3 = desired return register
; %4 & 5 are scratch

  xor %3, %3

%%begin:
  cmp %2, 8
  jl %%last_bits
  crc32 %3, qword[%1]
  add %1, 8
  sub %2, 8
  jnz %%begin

  jmp %%ret

%%last_bits:
  ; zero out the higher bits of the last partial byte
  ; r11 holds the last byte
  mov %4, [%1]                  

  ; shift operator can only use cl, so put the number of bits remaining into rcx
  mov rcx, %2                    
  shl rcx, 3                      ; multiply by 8 to get bits

  ; rdx will hold the desired mask
  mov %5, 1
  shl %5, cl
  dec %5
  and %4, %5
  crc32 %3, %4
  
%%ret:

%endmacro


global fhtw_new
fhtw_new:
  push rdi ; store requested size for later

; rdi is the size of the the table
; we need 3 pieces of metadata - occupancy, capacity, size of info word in bits
; and 3 arrays - keys, values, and hop info words
; info word size does not change the amount of memory allocated; just says how many bits we use 
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
  ; rdi = table pointer
  ; rsi = key pointer
  ; rdx = key length
  ; rcx = value
  ; r8 = value length? -- used in function!


  ; make sure there is room in the table
  mov r11, [rdi]
  cmp r11, [rdi + 8]
  je .table_full

  add rdi, 24                             ; rdi refers to beginning of key array

  ; calculate hash
  mov r8, rsi                              ; save key pointer in r8
  mov r9, rcx                              ; save value in r9
  hash_function rsi, rdx, rax, r11, r10

  ; linear probe for empty space
  div qword[rdi - 16]                        ; hash value is in rax, we divide by table size.  

  shl rdx, 3                                ; get index in bytes
  add rdx, rdi                              ; rdx holds a pointer to the current element 

  mov rax, [rdi - 16]                       
  shl rax, 3
  add rax, rdi                              ; rax has the end of the key array

  xor rcx, rcx                              ; rcx will hold distance (in elements) from the original hash

  .begin_probe:
    cmp qword[rdx], 0
    je .end_probe
    add rdx, 8
    inc rcx
    
    ; if we're at the end, loop back to the beginning of key array
    cmp rax, rdx
    cmovz rdx, rdi

    jmp .begin_probe
  .end_probe:
     
  ; found first empty space (pointer in rdx, displacement in rcx)

  push r12
  push r13

  ; rdx is pointer - convert to index for hop loop
  sub rdx, rdi
  shr rdx, 3

  ; not altered by hop loop
  ; r8 - key pointer
  ; r9 - value pointer
  ; rdi - beginning of key array
  ; rax - beginning of value array (also end of key array :) )

  ; altered by hop loop
  ; rdx - index of empty space
  ; rcx - current displacement in elements
  ; r12 - seek index 
  ;       after seek loop it shows where we are swapping the empty space to
  ; r13 - seek mask - acceptable hop candidates (ie before original empty space)
  ; r10 - beginning of hop info array
  ; r11 - scratch

  mov r10, [rdi - 16]
  shl r10, 4
  add r10, rdi

  .begin_hop:
    ; check whether it's too far
    cmp rcx, [rdi - 8]
    jl .insert
    
    ; find first available empty space

    ; seek loop initialization
    ; mask starts as all ones 
    mov r13, -1
    ; make r12 point at first candidate swap position
    mov r12, rdx
    sub r12, [rdi - 8]
    inc r12
    jns .begin_seek
    ; if we've gone past the beginning of the table, wrap around
    add r12, [rdi - 16]

    .begin_seek:
      test r13, [r10 + r12 * 8]              ; can we swap something that hashes to this value
      jnz .end_seek
      inc r12
      shr r13 
      jz .fail_seek                           ; barely possible - table needs resizing
      jmp .begin_seek
    .end_seek

    mov r11, r13                            
    neg r11
    and r11, r13
    or [r10 + r12 * 8], r11                 ; set bit in hopinfoword

    ; r12 has the index of a hopinfo word that refers to a swappable element
    ; calculate swappable element

    bsf r13, [r10 + r12 * 8]
    btr [r10 + r12 * 8], r13                ; clear bit of element to be moved
    add r13, r12
    cmp r13, [rdi - 16]
    jl .continue
    ; wrap around end of table
    sub r13, [rdi - 16]
    .continue:

    ; r13 has the index of a swappable element
    ; move key
    mov r11, [rdi + r13 * 8]
    mov [rdi + rdx * 8], r11

    ; move value
    mov r11, [rax + r13 * 8]
    mov [rax + rdx * 8], r11

    jmp .begin_hop

  .end_hop

  pop r13
  pop r12
  
  ; if space is too far away, hop the space back until it is close enough

  ; when it's close enough, jump to insert

  .insert:
    mov [rdx], r8                             ; insert key
    mov rax, [rdi - 16]                        ; next 3 lines calculate value position
    shl rax, 3
    add rax, rdx
    mov [rax], r9                             ; insert value
  
  ; calculate address of bitmap
  inc qword[rdi - 24]                            ; increment occupancy of table
  
  xor rax, rax
  ret

.table_full:
  ; return error code
  mov rax, -1
  ret

.fail_seek:
  mov rax, -2
  ret



global fhtw_get
fhtw_get:
  ; table in rdi
  ; key in rsi
  ; keylen in rdx

; STUB linear probing

  add rdi, 24                               ; rdi = start of key array

  push r12
  mov r12, [rdi - 16]                         ; r12 = size of hash table

  mov r8, rsi ; key
  mov r9, rdx ; keylen
  hash_function rsi, rdx, rax, r10, r11
  mov r10, rdi ; r10 = start of key array
  
  div r12                                   ; hash value is in rax, we divide by table size.  

  ; get pointer in the key array into rdx
  shl rdx, 3                                ; get index in bits
  add rdx, r10      

  mov rax, rdx                              ; store original position in rax

  ; compute address of last value pointer
  shl r12, 3
  add r12, r10

  .begin:
    ; if we've hit an empty space the key is not valid
    cmp qword[rdx], 0
    jz .fail

    ; repe cmps compares strings one byte at a time; it expects
    ; rcx == strlen, rdi == str1, rsi == str2
    ; clobbers all registers, so we have to reset them each time
    mov rcx, r9                             
    mov rdi, r8
    mov rsi, [rdx]
    repe cmpsb
    
    ; zero flag will be set if the two strings are equal

    jz .success
    add rdx, 8

    ; if we're at the end of the table loop back to the beginning
    cmp rdx, r12
    cmovz rdx, r10
    
    ; if we have returned to our original position, table is full
    cmp rdx, rax                          
    je .fail

    jmp .begin
  
  .success:
    ; key pointer address is in rdx - get value
    mov rax, [r10 - 16]                     ; rax = number of elements in table
    shl rax, 3
    mov rax, [rax + rdx]                   ; value pointer in rax
    pop r12
    ret
    
  .fail:
    xor rax, rax
    pop r12
    ret
  

global fhtw_hash
fhtw_hash:
  hash_function rdi, rsi, rax, rdx, r11
  ret


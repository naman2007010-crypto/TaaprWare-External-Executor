; TaaprWare Hypervisor - AMD SVM Assembly Routines
; VMRUN entry/exit and context save/restore

.code

; Guest register storage (all GPRs except RAX which is in VMCB)
EXTERN g_GuestRegs:QWORD

; void AsmSvmRun(PHYSICAL_ADDRESS VmcbPa)
; Enter VM mode with the given VMCB
; RCX = Physical address of VMCB
AsmSvmRun PROC
    ; Save host state
    push rbx
    push rbp
    push rdi
    push rsi
    push r12
    push r13
    push r14
    push r15
    
    ; Save host RSP for exit
    mov rax, rsp
    push rax
    
    ; RAX = VMCB physical address (passed in RCX)
    mov rax, rcx
    
    ; Load guest registers (except RAX)
    mov rbx, g_GuestRegs
    mov rcx, [rbx + 8*1]   ; RCX
    mov rdx, [rbx + 8*2]   ; RDX
    mov rsi, [rbx + 8*4]   ; RSI
    mov rdi, [rbx + 8*5]   ; RDI
    mov rbp, [rbx + 8*6]   ; RBP
    mov r8,  [rbx + 8*7]   ; R8
    mov r9,  [rbx + 8*8]   ; R9
    mov r10, [rbx + 8*9]   ; R10
    mov r11, [rbx + 8*10]  ; R11
    mov r12, [rbx + 8*11]  ; R12
    mov r13, [rbx + 8*12]  ; R13
    mov r14, [rbx + 8*13]  ; R14
    mov r15, [rbx + 8*14]  ; R15
    mov rbx, [rbx + 8*0]   ; RBX (do last since we used it)
    
    ; VMRUN instruction - enters guest mode
    ; RAX = VMCB physical address
    vmrun rax
    
    ; We return here after #VMEXIT
    ; Save guest registers
    push rbx
    mov rbx, g_GuestRegs
    
    mov [rbx + 8*1], rcx   ; RCX
    mov [rbx + 8*2], rdx   ; RDX
    mov [rbx + 8*4], rsi   ; RSI
    mov [rbx + 8*5], rdi   ; RDI
    mov [rbx + 8*6], rbp   ; RBP
    mov [rbx + 8*7], r8    ; R8
    mov [rbx + 8*8], r9    ; R9
    mov [rbx + 8*9], r10   ; R10
    mov [rbx + 8*10], r11  ; R11
    mov [rbx + 8*11], r12  ; R12
    mov [rbx + 8*12], r13  ; R13
    mov [rbx + 8*13], r14  ; R14
    mov [rbx + 8*14], r15  ; R15
    
    pop rax                 ; Recover RBX
    mov [rbx + 8*0], rax    ; RBX
    
    ; Restore host RSP
    pop rax
    mov rsp, rax
    
    ; Restore host state
    pop r15
    pop r14
    pop r13
    pop r12
    pop rsi
    pop rdi
    pop rbp
    pop rbx
    
    ret
AsmSvmRun ENDP

; void AsmEnableSvme(void)
; Enable SVME bit in EFER MSR
AsmEnableSvme PROC
    mov ecx, 0C0000080h    ; MSR_EFER
    rdmsr                   ; Read EFER
    or eax, 1000h          ; Set SVME bit (bit 12)
    wrmsr                   ; Write EFER
    ret
AsmEnableSvme ENDP

; void AsmVmsave(PHYSICAL_ADDRESS VmcbPa)
; Save additional guest state
AsmVmsave PROC
    mov rax, rcx           ; VMCB PA in RAX
    vmsave rax
    ret
AsmVmsave ENDP

; void AsmVmload(PHYSICAL_ADDRESS VmcbPa)
; Load additional guest state
AsmVmload PROC
    mov rax, rcx           ; VMCB PA in RAX
    vmload rax
    ret
AsmVmload ENDP

; UINT64 AsmReadMsr(UINT32 Msr)
AsmReadMsr PROC
    mov ecx, ecx           ; MSR number
    rdmsr
    shl rdx, 32
    or rax, rdx
    ret
AsmReadMsr ENDP

; void AsmWriteMsr(UINT32 Msr, UINT64 Value)
AsmWriteMsr PROC
    mov eax, edx           ; Low 32 bits
    shr rdx, 32            ; High 32 bits in EDX
    wrmsr
    ret
AsmWriteMsr ENDP

END

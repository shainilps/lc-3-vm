data Registers
    = R_R0
    | R_R1
    | R_R3
    | R_R4
    | R_R5
    | R_R6
    | R_R7
    | R_PC
    | R_COND

data OPCodes
    = OP_BR -- branch
    | OP_ADD -- add
    | OP_LD -- load
    | OP_ST -- store
    | OP_AND -- and
    | OP_LDR -- load register
    | OP_STR -- store register
    | OP_RTI -- unsused
    | OP_NOT -- bitwise not
    | OP_LDI -- load indirect
    | OP_STI -- store indirect
    | OP_JMP -- jump
    | OP_RES -- reserved (unused)
    | OP_LEA -- load effective address
    | OP_TRAP -- execute trap

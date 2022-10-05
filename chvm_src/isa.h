#ifndef ISA_H
#define ISA_H

#include <stdint.h>

typedef struct {
    const uint8_t *data;
    uint16_t data_size;
} constant_block;

// Structure representing a system call.
typedef struct {
    // How many bytes of arguments this call takes.
    uint8_t args_len;     

    // Function which executes the syscall.
    // NOTE rt is a void * since the runtime representation
    // is independent to the program.
    void (*f)(void *rt); 
} sys_call;

typedef struct {
    const sys_call *table;
    uint8_t table_size;
} system_table;

typedef struct {
    const constant_block *text;
    const constant_block *ro;
    const system_table *sys_table;

    // Number of single byte registers the program
    // expects to have available. (Should be even)
    uint8_t num_registers; 
    uint16_t mem_size;
} program;

// In the Opcode names below.
// UR refers to a single byte index representing a single byte 
// register. DR refers to a single byte index representint a
// dual byte register. UC represents a single byte constant.
// DC represents a dual byte constant.

// OPCODE definitions and types.

#define P_MASK 0xF0 // Prefix mask.
#define S_MASK 0x0F // Suffix mask.

// UR, UC Operations ---------------------------------
// These are operations involve a signle byte register index
// and a single byte constant.
#define P_UR_UC             0x00

// Logic operation suffixes
#define S_OR_UC             0x00
#define S_AND_UC            0x01
#define S_XOR_UC            0x02
#define S_LSHIFT_UC         0x03
#define S_RSHIFT_UC         0x04

// Math operation suffixes
#define S_ADD_UC            0x05
#define S_SUB_UC            0x06

// Memory operation suffix
#define S_LOAD_UC           0x07

// All UR, UC Executable Operations.
#define X_OR_UC             (P_UR_UC | S_OR_UC)
#define X_AND_UC            (P_UR_UC | S_AND_UC)
#define X_XOR_UC            (P_UR_UC | S_XOR_UC)
#define X_LSHIFT_UC         (P_UR_UC | S_LSHIFT_UC) 
#define X_RSHIFT_UC         (P_UR_UC | S_RSHIFT_UC)
#define X_ADD_UC            (P_UR_UC | S_ADD_UC) 
#define X_SUB_UC            (P_UR_UC | S_SUB_UC) 
#define X_LOAD_UC           (P_UR_UC | S_LOAD_UC)

// UR, UR Operations -------------------------------
// These operations involve and indeces of 2 single
// byte registers.
#define P_UR_UR 0x10

// Logic operation suffixes
#define S_OR_UR             0x00
#define S_AND_UR            0x01
#define S_XOR_UR            0x02
#define S_LSHIFT_UR         0x03
#define S_RSHIFT_UR         0x04

// Unsigned math operation suffixes
#define S_ADD_UR            0x05
#define S_SUB_UR            0x06
#define S_MUL_UR            0x07
#define S_DIV_UR            0x08
#define S_MOD_UR            0x09

// Memory operation suffix
#define S_LOAD_UR           0x0A

// Executable UR, UR operations.
#define X_OR_UR             (P_UR_UR | S_OR_UR)
#define X_AND_UR            (P_UR_UR | S_AND_UR)
#define X_XOR_UR            (P_UR_UR | S_XOR_UR)
#define X_LSHIFT_UR         (P_UR_UR | S_LSHIFT_UR) 
#define X_RSHIFT_UR         (P_UR_UR | S_RSHIFT_UR)
#define X_ADD_UR            (P_UR_UR | S_ADD_UR) 
#define X_SUB_UR            (P_UR_UR | S_SUB_UR) 
#define X_MUL_UR            (P_UR_UR | S_MUL_UR) 
#define X_DIV_UR            (P_UR_UR | S_DIV_UR) 
#define X_MOD_UR            (P_UR_UR | S_MOD_UR) 
#define X_LOAD_UR           (P_UR_UR | S_LOAD_UR)

// DR, DC Operations -------------------------------
// These operations involve a double byte register and
// a double byte constant.
#define P_DR_DC 0x20

// Logic operation suffixes
#define S_OR_DC             0x00
#define S_AND_DC            0x01
#define S_XOR_DC            0x02
#define S_LSHIFT_DC         0x03
#define S_RSHIFT_DC         0x04

// Math operation suffixes
#define S_ADD_DC            0x05
#define S_SUB_DC            0x06

// Memory operation suffix
#define S_LOAD_DC           0x07

// All DR, DC Executable Operations.
#define X_OR_DC             (P_DR_DC | S_OR_DC)
#define X_AND_DC            (P_DR_DC | S_AND_DC)
#define X_XOR_DC            (P_DR_DC | S_XOR_DC)
#define X_LSHIFT_DC         (P_DR_DC | S_LSHIFT_DC) 
#define X_RSHIFT_DC         (P_DR_DC | S_RSHIFT_DC)
#define X_ADD_DC            (P_DR_DC | S_ADD_DC) 
#define X_SUB_DC            (P_DR_DC | S_SUB_DC) 
#define X_LOAD_DC           (P_DR_DC | S_LOAD_DC)

// DR, DR Operations -------------------------------
// These operations involve and indeces of 2 double
// byte registers.
#define P_DR_DR 0x30

// Logic operation suffixes
#define S_OR_DR             0x00
#define S_AND_DR            0x01
#define S_XOR_DR            0x02
#define S_LSHIFT_DR         0x03
#define S_RSHIFT_DR         0x04

// Unsigned math operation suffixes
#define S_ADD_DR            0x05
#define S_SUB_DR            0x06
#define S_MUL_DR            0x07
#define S_DIV_DR            0x08
#define S_MOD_DR            0x09

// Memory operation suffix
#define S_LOAD_DR           0x0A
#define S_LOADI_DR_MEM      0x0B    // Load dual byte from memory.
#define S_LOADI_DR_RO       0x0C    // Load dual byte from RO data block.
#define S_STORE_DR          0x0D    // Store a dual byte in memory.

// Executable DR, DR operations.
#define X_OR_DR             (P_DR_DR | S_OR_DR)
#define X_AND_DR            (P_DR_DR | S_AND_DR)
#define X_XOR_DR            (P_DR_DR | S_XOR_DR)
#define X_LSHIFT_DR         (P_DR_DR | S_LSHIFT_DR) 
#define X_RSHIFT_DR         (P_DR_DR | S_RSHIFT_DR)
#define X_ADD_DR            (P_DR_DR | S_ADD_DR) 
#define X_SUB_DR            (P_DR_DR | S_SUB_DR) 
#define X_MUL_DR            (P_DR_DR | S_MUL_DR) 
#define X_DIV_DR            (P_DR_DR | S_DIV_DR) 
#define X_MOD_DR            (P_DR_DR | S_MOD_DR) 
#define X_LOAD_DR           (P_DR_DR | S_LOAD_DR)
#define X_LOADI_DR_MEM      (P_DR_DR | S_LOADI_DR_MEM)
#define X_LOADI_DR_RO       (P_DR_DR | S_LOADI_DR_RO)
#define X_STORE_DR          (P_DR_DR | S_STORE_DR)

// UR, UR, UR Operations ---------------------------
#define P_UR_UR_UR 0x40

// Comparision Suffixes
#define S_EQ_UR             0x00
#define S_LT_UR             0x01
#define S_LTE_UR            0x02

#define X_EQ_UR             (P_UR_UR_UR | S_EQ_UR)
#define X_LT_UR             (P_UR_UR_UR | S_LT_UR)
#define X_LTE_UR            (P_UR_UR_UR | S_LTE_UR)

// UR, DR, DR Operations ---------------------------
#define P_UR_DR_DR 0x50

// Comparision Suffixes
#define S_EQ_DR             0x00
#define S_LT_DR             0x01
#define S_LTE_DR            0x02

#define X_EQ_DR             (P_UR_DR_DR | S_EQ_DR)
#define X_LT_DR             (P_UR_DR_DR | S_LT_DR)
#define X_LTE_DR            (P_UR_DR_DR | S_LTE_DR)

// UR Operations -----------------------------------
#define P_UR 0x60

// Stack Suffixes
#define S_POP_UR            0x00
#define S_PUSH_UR           0x01

// Logical complement Suffix
#define S_CMPL_UR           0x02

#define X_POP_UR            (P_UR | S_POP_UR)
#define X_PUSH_UR           (P_UR | S_PUSH_UR)
#define X_CMPL_UR           (P_UR | S_CMPL_UR)

// DR Operations -----------------------------------
#define P_DR 0x70

// Stack Suffixes
#define S_POP_DR            0x00
#define S_PUSH_DR           0x01
#define S_LOAD_SP           0x02
#define S_SET_SP_DR         0x03

// Logical complement Suffix
#define S_CMPL_DR           0x04

#define X_POP_DR            (P_DR | S_POP_DR)
#define X_PUSH_DR           (P_DR | S_PUSH_DR)
#define X_LOAD_SP           (P_DR | S_LOAD_SP)
#define X_SET_SP_DR         (P_DR | S_SET_SP_DR)
#define X_CMPL_DR           (P_DR | S_CMPL_DR)

// UC Operations -----------------------------------
#define P_UC 0x80

// System interface.
#define S_SYSCALL           0x00

#define X_SYSCALL           (P_UC | S_SYSCALL)

// DC Operations -----------------------------------
#define P_DC 0x90

// Control Suffixes.
#define S_JMP               0x00
#define S_CALL              0x01
#define S_SET_SP_DC         0x02

#define X_JMP               (P_DC | S_JMP)
#define X_CALL              (P_DC | S_CALL) 
#define X_SET_SP_DC         (P_DC | S_SET_SP_DC)

// UR, DR Operations -------------------------------
#define P_UR_DR 0xA0

// Memory Suffixes
#define S_LOADI_UR_MEM      0x00
#define S_LOADI_UR_RO       0x01
#define S_STORE_UR          0x02

#define X_LOADI_UR_MEM      (P_UR_DR | S_LOADI_UR_MEM)
#define X_LOADI_UR_RO       (P_UR_DR | S_LOADI_UR_RO)
#define X_STORE_UR          (P_UR_DR | S_STORE_UR) 

// UR, DC Operations -------------------------------
#define P_UR_DC 0xB0

// Control Structure Suffixes.
#define S_JMP_Z             0x00
#define S_JMP_NZ            0x01

#define X_JMP_Z             (P_UR_DC | S_JMP_Z)
#define X_JMP_NZ            (P_UR_DC | S_JMP_NZ)

// SOLO Operations ---------------------------------
#define P_SOLO 0xC0

// Control structure suffixes.
#define S_EXIT              0x00
#define S_RET               0x01

#define X_EXIT              (P_SOLO | S_EXIT)
#define X_RET               (P_SOLO | S_RET)

#endif

#ifndef VM_ISA_H
#define VM_ISA_H

#include <stdint.h>

// BCS for Byte-Code Section.
#define BCS_FUNCTION    0x00
#define BCS_DATA        0x01

// BCDT for Byte-Code Data Type.

typedef uint8_t bc_datatype;

#define BCDT_CHAR       0x00
#define BCDT_INT        0x01
#define BCDT_FLOAT      0x02
#define BCDT_PREF       0x03
#define BCDT_VREF       0x04

// A "Section" resides in the binary being read by the 
// VM.
//
// A "Record" resides in dynamic memory as the VM runs.
// Records do not reside in the collected space of the VM.
// They do not need to be accessed using a virtual address.
// They are always constant.

// Function Section Structure.
//
// BCS_FUNCTION             
// uint32_t func_id;        
//
// uint8_t name_length;
// char name[name_length];
//
// uint8_t params_length;
// data_type params[params_length]; 
// 
// uint8_t static_length;
// data_type statics[statics_length]; 
//
// uint32_t code_length;
// uint8_t code[code_length]; // Byte code.


// Function Record Structure.
//
// NOTE: function records will be stored in a hashset.

typedef struct {
    uint32_t func_id; // Primary Key.

    uint8_t name_len;
    char *name;

    uint8_t params_len;
    bc_datatype *params;

    uint8_t statics_len;
    bc_datatype *statics;

    uint32_t code_len;
    uint8_t *code;
} vm_function_record;



// Data Section.
//
// BCS_DATA
// uint32_t data_id;
//
// uint32_t data_length;
// uint8_t data[data_length];

// Data Record Structure.

typedef struct {
    uint32_t data_id; 

    uint32_t data_length;
    uint8_t *data;
} vm_data_record;


// There will be a stack of stack frames.
// Each stack frame will adhere by the following structure.
//
// NOTE: Stack frames will not reside in the garbage collected 
// space.
//
// void *parent_stack_frame; (NULL if main function)
// 




#endif


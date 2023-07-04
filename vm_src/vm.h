#ifndef VM_VM_H
#define VM_VM_H

#include "../gc_src/cs.h"

#include <stdint.h>

// Data which is stored in the collected space will be
// known as impersistent.
//
// Data which is not stored in the collected space will be
// known as persistent. 
//
// Impersistent data may be garbage collected and shifted.
// In order to use impersistent data, the user must hold
// its lock.

// I have decided that type checking and safety will not 
// be enforced at all while the VM is running.
// This will only be checked once when the binary is loaded
// into memory.

typedef struct {
    char *name;     // For debugging purposes only.

    uint32_t    rt_len;         // The number of virtual address 
                                // slots this function needs to run.
                                
    uint32_t    locals_len;     // The number of bytes of misc data
                                // this function needs to run.

    uint32_t    code_len; 
    uint8_t    *code;
} vm_func;

// Virtual Addresses that live in the stack frame must always
// be marked as reachable.

typedef struct {
    vm_func *parent_func;

    addr_book_vaddr rt_vaddr;       // The virtual address of an impersistent array
                                    // of virtual addresses.

    void *locals;   // Multipurpose data, this should never hold virtual addresses.

    uint32_t pc;    // The next instruction to execute.
    
    // NOTE:
    // For return values. To keep things flexible, when a function x callls a function y,
    // after y returns, x will have access to y's entire stack frame.
    // It is not untill x calls another function z or x itself returns that
    // y's stack frame is cut loose and no longer reachable.
} vm_stack_frame; 

// NOTE: The VM will work with 3 different sized data types.
//
// 1-byte (BYT) Single byte value, most likely used for characters and booleans.
// 4-byte (OFF) Quad byte offset value. This should only be used as offsets in indirection.
// 8-byte (NUM) Octa byte numeric value. Could hold an integer or floating point number.

// Group and subgroup should both be 4-bits only.
#define OPCODE(group, sub_group, code) (((group) << 12) | (sub_group << 8) | (code))


// NOTE: Below ..._l_offset refers to an offset in numberr of bytes
// in the locals array of a stack frame.
//
// ..._rt_offset refers to an offset in number of vaddrs in
// the reference table of an object.
//
// NOTE: Below Object or Obj refers to any data which resides in the 
// collected space of the VM and is thus subject to garbage collection.

#define DATA_GROUP 0x0

// Maybe add a group here...

// get_rt_read(off dest_l_offset)
#define OP_GET_RT_READ      OPCODE(DATA_GROUP, )

// get_rt_write(off dest_l_offset)
#define OP_GET_RT_WRITE     OPCODE(DATA_GROUP, )

// Unlock stack_frame's reference table.

// rt_unlock()
#define OP_RT_UNLOCK        OPCODE(DATA_GROUP, )

// Copy a vaddr from one object's rt into another object's rt.
//
// copy_vaddr(off src_l_offset, off src_rt_offset, 
//      off dest_l_offset, off dest_rt_offset)
#define OP_COPY_VADDR       OPCODE(DATA_GROUP, )

// Object Load Calls -------------------------------------------------------------------------------
#define OBJ_LOAD_SG 0x0

// Load data from one object's data array into the locals
// block of the stack_frame.
//
// load_data(off src_l_offset, off src_da_offset, 
//      off dest_l_offset, off size)
#define OP_LOAD_OBJ_DATA    OPCODE(DATA_GROUP, OBJ_LOAD_SG, )

// Load a singular cell from an object's data array into locals.
//
// load_...(off src_l_offset, off src_da_offset, 
//      off dest_l_offset)
#define OP_LOAD_OBJ_BYT     OPCODE(DATA_GROUP, OBJ_LOAD_SG, )
#define OP_LOAD_OBJ_OFF     OPCODE(DATA_GROUP, OBJ_LOAD_SG, )
#define OP_LOAD_OBJ_NUM     OPCODE(DATA_GROUP, OBJ_LOAD_SG, )

// Load data from an object with indirection!
// sf->locals[dest_l_offset ... + size] = 
//      sf->locals[src_l_offset]->da[sf->locals[src_o_l_offset] ... + size]
// 
// load_data_i(off src_l_offset, off src_o_l_offset,
//      off dest_l_offset, off size)
#define OP_LOAD_OBJ_DATA_I  OPCODE(DATA_GROUP, OBJ_LOAD_SG, )

// Load singular cell from an object's data array with indirection!
//
// load_..._i(off src_l_offset, off src_da_offset,
//      off dest_l_offset)
#define OP_LOAD_OBJ_BYT_I   OPCODE(DATA_GROUP, OBJ_LOAD_SG, )
#define OP_LOAD_OBJ_OFF_I   OPCODE(DATA_GROUP, OBJ_LOAD_SG, )
#define OP_LOAD_OBJ_NUM_I   OPCODE(DATA_GROUP, OBJ_LOAD_SG, )

// Object Store Calls -------------------------------------------------------------------------------
#define OBJ_STORE_SG 0x1

// Store data from the stack_frame locals block into an object's
// data array.
//
// store_data(off src_l_offset, off size,
//      off dest_l_offset, off dest_da_offset)
#define OP_STORE_OBJ_DATA   OPCODE(DATA_GROUP, OBJ_STORE_SG, )

// Store singular cell from locals into an object's data array.
//
// store_...(off_t src_l_offset, 
//      off dest_l_offset, off dest_da_offset)
#define OP_STORE_OBJ_BYT    OPCODE(DATA_GROUP, OBJ_STORE_SG, )
#define OP_STORE_OBJ_OFF    OPCODE(DATA_GROUP, OBJ_STORE_SG, )
#define OP_STORE_OBJ_NUM    OPCODE(DATA_GROUP, OBJ_STORE_SG, )

// Store data from stack frame locals into an object's data array
// with indirection.
//
// sf->locals[dest_l_offset]->da[sf->locals[dest_o_l_offset] ... + size] =
//      sf->locals[src_l_offset ... + size]
//
// store_data_i(off src_l_offset, off size,
//      off dest_l_offset, off dest_o_l_offset)
#define OP_STORE_OBJ_DATA_I OPCODE(DATA_GROUP, OBJ_STORE_SG, )

// Store singular cell from stack frame into data array of an object
// with indirection.
//
// store_..._i(off src_l_offset,
//      off dest_l_offset, off dest_o_l_offset)
#define OP_STORE_OBJ_BYT_I  OPCODE(DATA_GROUP, OBJ_STORE_SG, )
#define OP_STORE_OBJ_OFF_I  OPCODE(DATA_GROUP, OBJ_STORE_SG, )
#define OP_STORE_OBJ_NUM_I  OPCODE(DATA_GROUP, OBJ_STORE_SG, )

#endif


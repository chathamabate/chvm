
// VM Notes:
//
// We have a GC backend.
// This GC gives us objects.
//
// An object holds an array of virtual addresses.
// And also, a "data array" which is simply a block of
// memory.
//
// We need to create an architecture/instruction set for
// the virtual machine...
// Where will code lie??? 
// How will code be formatted???
// How will the assembler work??
//
// The virtual machine will interpret a single binary file.
//
// The virtual machine will also need "static" memory which
// can be used without needing locks.
//
// Maybe functions can be defined???
// A function will have a static size???
//
// A binary file will contain different sections.
// Each section will start with a single byte code defining what
// the section is.
//
// Data types will be...
//
// 8-bit character.                     (char)      0x00
// 64-bit integer.                      (int)       0x01
// 64-bit floating point number.        (float)     0x02
// object index. CONSTANT               (pref)      0x03
// 128-bit virtual address. CONSTANT    (vref)      0x04
//
// Can we pass a physical address??
// I think so?? Maybe we cannot store it though?
// 
//
// 0x00 - Function.
//
// uint32_t func_id;        // Unique ID.
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
//
//
// 0x01 - Data.


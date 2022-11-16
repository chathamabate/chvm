#ifndef ASSEMBLER_H
#define ASSEMBLER_H


// This file will describe the assembly language
// syntax for writing chvm binaries.
//
// <code_label>     ::= \.[a-zA-Z0-9_]+
// <data_label>     ::= #[a-zA-Z0-9_]+
// <const_int>      ::= [1-9][0-9]{0, 4}
// <const_hex>      ::= 0x[0-F]{1,4}
// <const_char>     ::= '[ -~]'
// <const>          ::= <const_int> | <const_hex> | <const_char>
//                  | <code_label> | <data_label>   
//
// <ur>             ::= r<const_int>
// <dr>             ::= R<const_int> 
//
// <p_math_op>      ::= \| | & | ^ | << | >> 
//                  | + | - 
// <p_math_assign>  ::= <p_math_op>= 
//
// <a_math_op>      ::= * | / | %
// <a_math_assign>  ::= <a_math_op>=
//
// <ur_uc_st>       ::= <ur> <p_math_assign> <const> 
//                  |   <ur> = <const>
//
// <ur_ur_st>       ::= <ur> <p_math_assign> <ur>
//                  |   <ur> <a_math_assign> <ur>
//                  |   <ur> = <ur>
// 
// <dr_dc_st>       ::= <dr> <p_math_assign> <const> 
//                  |   <dr> = <const>
// 
// <dr_dr_st>       ::= <dr> <p_math_assign> <dr>
//                  |   <dr> <a_math_assign> <dr>
//                  |   <dr> = mem[<dr>]
//                  |   <dr> = ro[<dr>]
//                  |   mem[<dr>] = <dr>
//
// <cmp>            ::= == | < | <=
// 
// <ur_ur_ur_st>    ::= <ur> = <ur> <cmp> <ur> 
//
// <ur_dr_dr_st>    ::= <ur> = <dr> <cmp> <dr>
//
// <ur_st>          ::= pop <ur>
//                  |   push <ur>
//                  |   ~<ur>
//
// <dr_st>          ::= pop <dr>
//                  |   push <dr>
//                  |   <dr> = sp
//                  |   sp = <dr>
//                  |   ~<dr>
//
// <uc_st>          ::= syscall <const>
//
// <dc_st>          ::= jmp <const>
//                  |   call <const>
//                  |   sp = <const>
//
// <ur_dr_st>       ::= <ur> = mem[<dr>]
//                  |   <ur> = ro[<dr>]
//                  |   mem[<dr>] = <ur> 
//
// <ur_dc_st>       ::= if !<ur> jmp <const>
//                  |   if <ur>  jmp <const>
//
// <solo_st>        ::= ret | exit
//
// <st>             ::= <ur_uc_st>
//                  |   <ur_ur_st>
//                  |   <dr_dc_st>
//                  |   <dr_dr_st>
//                  |   <ur_ur_ur_st>
//                  |   <ur_dr_dr_st>
//                  |   <ur_st>
//                  |   <dr_st>
//                  |   <uc_st>
//                  |   <dc_st>
//                  |   <ur_dr_st>
//                  |   <ur_dc_st>
//                  |   <solo_st>
//
// <str>            ::= "([ !#-~]|\\")+"
//
// <small_data>     ::= u <const> 
//                  |   d <const> 
// 
// <data>           ::= <small_data>
//                  |   <small_data> repeat <const>
//                  |   <str>
// 
// <code_block>     ::= begin <code_label> 
//                          (<st>;)+ 
//                      end
//
// <data_block>     ::= begin <data_label> 
//                          <data>
//                          (, <data>)*
//                      end
//
// <paste>          ::= paste <str>
//
// <program>        ::= (<code_block>|<data_block>|<paste>)+

#endif

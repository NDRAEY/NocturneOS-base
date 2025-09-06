#pragma once

#define CONCAT(a, b) a##b

#define __asm_reg_con_1 "q"
#define __asm_reg_con_2 "r"
#define __asm_reg_con_4 "r"
#define __asm_reg_con_8 "r"
#define __asm_reg_con(size) __asm_reg_con_##size

#define __asm_imm_con_1 "i"
#define __asm_imm_con_2 "i"
#define __asm_imm_con_4 "i"
#define __asm_imm_con_8 "e"
#define __asm_imm_con(size) __asm_imm_con_##size

#define __asm_op_suffix_1(operand) operand "b "
#define __asm_op_suffix_2(operand) operand "w "
#define __asm_op_suffix_4(operand) operand "l "
#define __asm_op_suffix_8(operand) operand "q "
#define __asm_op(operand, size) CONCAT(__asm_op_suffix_, size)(operand)

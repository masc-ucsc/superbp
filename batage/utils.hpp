#pragma once

// std::vector<int> array2vec (int a[], size_t a_size);
#define array2vec(a) (std::vector<int>(std::begin(a), std::end(a)))

#define ptr2vec(a, a_size) (std::vector<int>(a, a + (a_size)))

#define riscv_opcode(insn_raw)    (insn_raw & 0x7f)
#define riscv_rd(insn_raw)        ((insn_raw & 0xf80) >> 7)
#define riscv_rs1(insn_raw)       ((insn_raw & 0xf8000) >> 15)
#define riscv_imm_itype(insn_raw) ((insn_raw & 0xfff00000) >> 20)
// #define funct3(insn_raw)    ((insn_raw & 0x7000) >> 12)

enum class insn_t { non_cti, jump, branch, call, ret };

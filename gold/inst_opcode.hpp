//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#pragma once

enum Inst_opcode {
  iOpInvalid = 0,
  //-----------------
  iRALU,
  //-----------------
  iAALU, // Can got to BUNIT/AUNIT/SUNIT in scoore
  //-----------------
  iBALU_LBRANCH, // branch label/immediate
  iBALU_RBRANCH, // branch register
  iBALU_LJUMP,   // jump label/immediate
  iBALU_RJUMP,   // jump register
  iBALU_LCALL,   // call label/immediate
  iBALU_RCALL,   // call register (same as RBRANCH, but notify predictor)
  iBALU_RET,     // func return (same as RBRANCH, but notify predictor)
  //-----------------
  iLALU_LD,
  //-----------------
  iSALU_ST,
  iSALU_LL,
  iSALU_SC,
  iSALU_ADDR, // plain add, but it has a store address (break down st addr and data)
  //-----------------
  iCALU_FPMULT,
  iCALU_FPDIV,
  iCALU_FPALU,
  iCALU_MULT,
  iCALU_DIV,
  //-----------------
  iMAX
};

using AddrType=uint64_t;


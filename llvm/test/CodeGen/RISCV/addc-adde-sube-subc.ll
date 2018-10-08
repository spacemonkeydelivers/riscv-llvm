; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=riscv32 -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV32I %s
; RUN: llc -mtriple=riscv64 -verify-machineinstrs < %s \
; RUN:   | FileCheck -check-prefix=RV64I %s

; Ensure that the ISDOpcodes ADDC, ADDE, SUBC, SUBE are handled correctly

define i64 @addc_adde(i64 %a, i64 %b) {
; RV32I-LABEL: addc_adde:
; RV32I:       # %bb.0:
; RV32I-NEXT:    add a1, a1, a3
; RV32I-NEXT:    add a2, a0, a2
; RV32I-NEXT:    sltu a0, a2, a0
; RV32I-NEXT:    add a1, a1, a0
; RV32I-NEXT:    mv a0, a2
; RV32I-NEXT:    ret
;
; RV64I-LABEL: addc_adde:
; RV64I:       # %bb.0:
; RV64I-NEXT:    add a0, a0, a1
; RV64I-NEXT:    ret
  %1 = add i64 %a, %b
  ret i64 %1
}

define i64 @subc_sube(i64 %a, i64 %b) {
; RV32I-LABEL: subc_sube:
; RV32I:       # %bb.0:
; RV32I-NEXT:    sub a1, a1, a3
; RV32I-NEXT:    sltu a3, a0, a2
; RV32I-NEXT:    sub a1, a1, a3
; RV32I-NEXT:    sub a0, a0, a2
; RV32I-NEXT:    ret
;
; RV64I-LABEL: subc_sube:
; RV64I:       # %bb.0:
; RV64I-NEXT:    sub a0, a0, a1
; RV64I-NEXT:    ret
  %1 = sub i64 %a, %b
  ret i64 %1
}

define i128 @addc_adde128(i128 %a, i128 %b) {
; RV32I-LABEL: addc_adde128:
; RV32I:       # %bb.0:
; RV32I-NEXT:    lw      a3, 4(a2)
; RV32I-NEXT:    lw      a5, 4(a1)
; RV32I-NEXT:    add     a7, a5, a3
; RV32I-NEXT:    lw      a3, 0(a2)
; RV32I-NEXT:    lw      a4, 0(a1)
; RV32I-NEXT:    add     a6, a4, a3
; RV32I-NEXT:    sltu    a3, a6, a4
; RV32I-NEXT:    add     a4, a7, a3
; RV32I-NEXT:    beq     a4, a5, .LBB2_2
; RV32I-NEXT:  # %bb.1:
; RV32I-NEXT:    sltu    a3, a4, a5
; RV32I-NEXT:  .LBB2_2:
; RV32I-NEXT:    sw      a6, 0(a0)
; RV32I-NEXT:    sw      a4, 4(a0)
; RV32I-NEXT:    lw      a4, 12(a2)
; RV32I-NEXT:    lw      a5, 12(a1)
; RV32I-NEXT:    add     a4, a5, a4
; RV32I-NEXT:    lw      a2, 8(a2)
; RV32I-NEXT:    lw      a1, 8(a1)
; RV32I-NEXT:    add     a2, a1, a2
; RV32I-NEXT:    add     a3, a2, a3
; RV32I-NEXT:    sw      a3, 8(a0)
; RV32I-NEXT:    sltu    a3, a3, a2
; RV32I-NEXT:    sltu    a1, a2, a1
; RV32I-NEXT:    add     a1, a4, a1
; RV32I-NEXT:    add     a1, a1, a3
; RV32I-NEXT:    sw      a1, 12(a0)
; RV32I-NEXT:    ret
;
; RV64I-LABEL: addc_adde128:
; RV64I:       # %bb.0:
; RV64I-NEXT:    add a1, a1, a3
; RV64I-NEXT:    add a2, a0, a2
; RV64I-NEXT:    sltu a0, a2, a0
; RV64I-NEXT:    add a1, a1, a0
; RV64I-NEXT:    mv a0, a2
; RV64I-NEXT:    ret
  %1 = add i128 %a, %b
  ret i128 %1
}

define i128 @subc_sube128(i128 %a, i128 %b) {
; RV32I-LABEL: subc_sube128:
; RV32I:       # %bb.0:
; RV32I-NEXT:    lw      a4, 4(a2)
; RV32I-NEXT:    lw      a3, 4(a1)
; RV32I-NEXT:    lw      a7, 0(a2)
; RV32I-NEXT:    lw      t0, 0(a1)
; RV32I-NEXT:    sltu    a5, t0, a7
; RV32I-NEXT:    mv      a6, a5
; RV32I-NEXT:    beq     a3, a4, .LBB3_2
; RV32I-NEXT:  # %bb.1:
; RV32I-NEXT:    sltu    a6, a3, a4
; RV32I-NEXT:  .LBB3_2:
; RV32I-NEXT:    sub     a3, a3, a4
; RV32I-NEXT:    sub     a3, a3, a5
; RV32I-NEXT:    sub     a4, t0, a7
; RV32I-NEXT:    sw      a4, 0(a0)
; RV32I-NEXT:    sw      a3, 4(a0)
; RV32I-NEXT:    lw      a3, 8(a2)
; RV32I-NEXT:    lw      a4, 8(a1)
; RV32I-NEXT:    sub     a7, a4, a3
; RV32I-NEXT:    sub     a5, a7, a6
; RV32I-NEXT:    sw      a5, 8(a0)
; RV32I-NEXT:    sltu    a3, a4, a3
; RV32I-NEXT:    lw      a2, 12(a2)
; RV32I-NEXT:    lw      a1, 12(a1)
; RV32I-NEXT:    sub     a1, a1, a2
; RV32I-NEXT:    sub     a1, a1, a3
; RV32I-NEXT:    sltu    a2, a7, a6
; RV32I-NEXT:    sub     a1, a1, a2
; RV32I-NEXT:    sw      a1, 12(a0)
; RV32I-NEXT:    ret
;
; RV64I-LABEL: subc_sube128:
; RV64I:       # %bb.0:
; RV64I-NEXT:    sub a1, a1, a3
; RV64I-NEXT:    sltu a3, a0, a2
; RV64I-NEXT:    sub a1, a1, a3
; RV64I-NEXT:    sub a0, a0, a2
; RV64I-NEXT:    ret
  %1 = sub i128 %a, %b
  ret i128 %1
}

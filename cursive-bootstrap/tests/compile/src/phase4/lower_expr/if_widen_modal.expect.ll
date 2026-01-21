proc @demo_x3a_x3amain {
  seq {
    seq {
      call @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3ademo (%__panic)
      panic_check
    }
    block seq {
      seq {
        nop
        bind %count = 0x1
      }
      seq {
        seq {
          seq {
            read %count
            nop
            binop == %count, 0x1
          }
          panic_check
          if binop then seq {
            block nop seq {
              seq {
                read %count
                unop widen record
              }
              nop
            }
            seq {
              clear_panic
              call @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3ademo_x3a_x3aStatus (unop, %__panic)
              seq {
                load_ptr panic_flag_ptr
                load_ptr panic_code_ptr
              }
              binop && 0x0, panic_flag
              if double_panic then call @cursive_x3a_x3aruntime_x3a_x3apanic (panic_code) else nop
              unop ! panic_flag
              binop && 0x0, panic_clear
              if panic_restore then seq {
                store_ptr panic_flag_ptr = 0x1
                store_ptr panic_code_ptr = 0x0
              } else nop
              binop || 0x0, panic_flag
              if panic_flag then nop else nop
              clear_panic
              call @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3ademo_x3a_x3aStatus_x3a_x3aIdle (record, %__panic)
              seq {
                load_ptr panic_flag_ptr
                load_ptr panic_code_ptr
              }
              binop && panicking, panic_flag
              if double_panic then call @cursive_x3a_x3aruntime_x3a_x3apanic (panic_code) else nop
              unop ! panic_flag
              binop && panicking, panic_clear
              if panic_restore then seq {
                store_ptr panic_flag_ptr = 0x1
                store_ptr panic_code_ptr = panic_code_sel
              } else nop
              binop || panicking, panic_flag
              if panic_flag then nop else nop
              panic_check
            }
          } else seq {
            block nop seq {
              seq {
                seq {
                  read %count
                  nop
                }
                unop widen record
              }
              nop
            }
            seq {
              clear_panic
              call @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3ademo_x3a_x3aStatus (unop, %__panic)
              seq {
                load_ptr panic_flag_ptr
                load_ptr panic_code_ptr
              }
              binop && 0x0, panic_flag
              if double_panic then call @cursive_x3a_x3aruntime_x3a_x3apanic (panic_code) else nop
              unop ! panic_flag
              binop && 0x0, panic_clear
              if panic_restore then seq {
                store_ptr panic_flag_ptr = 0x1
                store_ptr panic_code_ptr = 0x0
              } else nop
              binop || 0x0, panic_flag
              if panic_flag then nop else nop
              clear_panic
              call @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3ademo_x3a_x3aStatus_x3a_x3aBusy (record, %__panic)
              seq {
                load_ptr panic_flag_ptr
                load_ptr panic_code_ptr
              }
              binop && panicking, panic_flag
              if double_panic then call @cursive_x3a_x3aruntime_x3a_x3apanic (panic_code) else nop
              unop ! panic_flag
              binop && panicking, panic_clear
              if panic_restore then seq {
                store_ptr panic_flag_ptr = 0x1
                store_ptr panic_code_ptr = panic_code_sel
              } else nop
              binop || panicking, panic_flag
              if panic_flag then nop else nop
              panic_check
            }
          }
        }
        bind %status = if
      }
      seq {
        seq {
          seq {
            seq {
              read %status
              move_state status
            }
            match %status {
              arm {
                pat @Idle {ticks: idle_ticks}
                body { %idle_ticks }
              }
              arm {
                pat @Busy {code: busy_code, retries: busy_retries}
                body { binop }
              }
            }
          }
          nop
        }
        panic_check
      }
      seq {
        nop
        panic_check
        ret 0x0
      }
    } nop
  }
}

proc @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3ademo {
  nop
}

proc @cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3ademo {
  nop
}

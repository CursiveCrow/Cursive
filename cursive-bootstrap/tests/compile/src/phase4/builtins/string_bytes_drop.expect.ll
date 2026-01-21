proc @demo_x3a_x3amain {
  seq {
    seq {
      call @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3ademo (%__panic)
      panic_check
    }
    block seq {
      seq {
        seq {
          block nop seq {
            seq {
              seq {
                nop
                nop
                nop
              }
              transmute tuple to string@Managed
            }
            nop
          }
          bind %s = transmute
        }
        seq {
          clear_panic
          call @cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3adrop_x5fmanaged (transmute)
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
          panic_check
        }
      }
      seq {
        seq {
          block nop seq {
            seq {
              seq {
                nop
                nop
                nop
              }
              transmute tuple to bytes@Managed
            }
            nop
          }
          bind %b = transmute
        }
        seq {
          clear_panic
          call @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3adrop_x5fmanaged (transmute)
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
          panic_check
        }
      }
      seq {
        nop
        seq {
          clear_panic
          call @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3adrop_x5fmanaged (%b)
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
          call @cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3adrop_x5fmanaged (%s)
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

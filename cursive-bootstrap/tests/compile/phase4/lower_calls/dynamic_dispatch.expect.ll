vtable @vtable_x3a_x3ademo_x3a_x3aR_x3a_x3acl_x3a_x3ademo_x3a_x3aD size=4 align=4 drop=@cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3ademo_x3a_x3aR slots=[@demo_x3a_x3aR_x3a_x3aping]

proc @demo_x3a_x3aR_x3a_x3aping {
  block seq {
    nop
    nop
    ret 0x1
  } nop
}

proc @demo_x3a_x3amain {
  seq {
    seq {
      call @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3ademo (%__panic)
      panic_check
    }
    block seq {
      seq {
        seq {
          nop
          bind %r = record
        }
        panic_check
      }
      seq {
        addr_of r
        bind %d = dyn
      }
      seq {
        seq {
          read %d
          nop
          seq {
            call_vtable %d [0] (%__panic)
            panic_check
          }
        }
        seq {
          clear_panic
          call @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3ademo_x3a_x3aR (%r, %__panic)
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
        ret dyncall_result
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

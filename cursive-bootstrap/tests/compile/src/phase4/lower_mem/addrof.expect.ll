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
          nop
        }
        bind %p = record
      }
      seq {
        bind %ptr = addr_of p
      }
      seq {
        seq {
          addr_of p
          addr_of p.x
        }
        bind %ptr_x = addr_of
      }
      seq {
        seq {
          seq {
            nop
            nop
            nop
          }
          bind %arr = array
        }
        panic_check
      }
      seq {
        seq {
          seq {
            addr_of arr
            nop
            check_index addr_of, 0x0
            panic_check
            addr_of arr[0usize]
          }
          bind %ptr_elem = addr_of arr[0usize]
        }
        panic_check
      }
      seq {
        nop
        seq {
          clear_panic
          call @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3ademo_x3a_x3aPoint (%p, %__panic)
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

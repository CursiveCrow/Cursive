proc @demo_x3a_x3amain {
  seq {
    seq {
      call @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3ademo (%__panic)
      panic_check
    }
    block seq {
      seq {
        seq {
          seq {
            nop
            nop
          }
          bind %a = array
        }
        panic_check
      }
      seq {
        seq {
          seq {
            read %a
            nop
            check_index %a, 0x0
            panic_check
          }
          bind %x = index_elem
        }
        panic_check
      }
      seq {
        seq {
          seq {
            read %a
            nop
            check_index %a, 0x1
            panic_check
          }
          bind %y = index_elem
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

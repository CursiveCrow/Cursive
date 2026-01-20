proc @demo_x3a_x3atake {
  block seq {
    nop
    panic_check
    ret 0x0
  } nop
}

proc @demo_x3a_x3aborrow {
  block seq {
    nop
    nop
    ret 0x0
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
        nop
        bind %a = 0x1
      }
      seq {
        nop
        bind %b = 0x2
      }
      seq {
        seq {
          read @demo::take
          seq {
            read %a
            move_state a
          }
          call @take (%a, %__panic)
          panic_check
        }
        panic_check
      }
      seq {
        seq {
          read @demo::borrow
          addr_of b
          call @borrow (addr_of, %__panic)
          panic_check
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

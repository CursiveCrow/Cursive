proc @demo_x3a_x3atrace {
  block seq {
    read %msg
    nop
    ret %msg
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
        nop
        bind %c = 0x3
      }
      seq {
        seq {
          seq {
            read @demo::trace
            addr_of a
            call @trace (addr_of, %__panic)
            panic_check
          }
          seq {
            read @demo::trace
            addr_of b
            call @trace (addr_of, %__panic)
            panic_check
          }
          seq {
            read @demo::trace
            addr_of c
            call @trace (addr_of, %__panic)
            panic_check
          }
        }
        bind %_t = tuple
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

proc @demo_x3a_x3atake {
  block seq {
    nop
    seq {
      call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x74726174532D70756E61656C43, "")
      call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x656E6F442D70756E61656C43, "")
      panic_check
    }
    ret 0x0
  } nop
}

proc @demo_x3a_x3aborrow {
  block seq {
    nop
    call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x7974706D452D70756E61656C43, "")
    ret 0x0
  } nop
}

proc @demo_x3a_x3amain {
  seq {
    seq {
      call @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3ademo (%__panic)
      seq {
        call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x6B6365684363696E6150, "")
        panic_check
      }
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
          seq {
            call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x6B6365684363696E6150, "")
            panic_check
          }
        }
        seq {
          call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x74726174532D70756E61656C43, "")
          call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x656E6F442D70756E61656C43, "")
          panic_check
        }
      }
      seq {
        seq {
          read @demo::borrow
          addr_of b
          call @borrow (addr_of, %__panic)
          seq {
            call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x6B6365684363696E6150, "")
            panic_check
          }
        }
        seq {
          call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x74726174532D70756E61656C43, "")
          call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x656E6F442D70756E61656C43, "")
          panic_check
        }
      }
      seq {
        nop
        seq {
          call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x74726174532D70756E61656C43, "")
          call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x656E6F442D70756E61656C43, "")
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

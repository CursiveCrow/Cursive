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
        loop infinite
        seq {
          call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x74726174532D70756E61656C43, "")
          call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x656E6F442D70756E61656C43, "")
          panic_check
        }
      }
      seq {
        nop
        bind %i = 0x0
      }
      seq {
        loop while
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

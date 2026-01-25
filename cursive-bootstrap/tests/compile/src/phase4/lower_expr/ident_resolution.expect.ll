global_const @demo_x3a_x3ag bytes=4

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
        bind %local = 0x2
      }
      seq {
        read %local
        bind %a = %local
      }
      seq {
        read @demo::g
        bind %b = @g
      }
      seq {
        seq {
          read %a
          read %b
          check_op Overflow + %a, %b
          seq {
            call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x6B6365684363696E6150, "")
            panic_check
          }
          binop + %a, %b
        }
        seq {
          call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x74726174532D70756E61656C43, "")
          call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x656E6F442D70756E61656C43, "")
          panic_check
        }
        ret binop
      }
    } nop
  }
}

proc @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3ademo {
  seq {
    nop
    bind %g = 0x1
    store @demo_x3a_x3ag = %g
    seq {
      call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x656C646E614863696E615074696E49, "")
      init_panic_handle demo [demo]
    }
  }
}

proc @cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3ademo {
  seq {
    read @demo::g
    nop
  }
}

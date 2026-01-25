global_const @demo_x3a_x3ax bytes=4

global_zero @demo_x3a_x3ay size=4

global_zero @demo_x3a_x3aa size=4

global_zero @demo_x3a_x3ab size=1

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
      nop
      call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x7974706D452D70756E61656C43, "")
      ret 0x0
    } nop
  }
}

proc @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3ademo {
  seq {
    seq {
      nop
      bind %x = 0x1
      store @demo_x3a_x3ax = %x
      seq {
        call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x656C646E614863696E615074696E49, "")
        init_panic_handle demo [demo]
      }
    }
    seq {
      seq {
        nop
        nop
        check_op Overflow + 0x1, 0x2
        seq {
          call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x6B6365684363696E6150, "")
          panic_check
        }
        binop + 0x1, 0x2
      }
      bind %y = binop
      store @demo_x3a_x3ay = %y
      seq {
        call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x656C646E614863696E615074696E49, "")
        init_panic_handle demo [demo]
      }
    }
    seq {
      seq {
        nop
        nop
      }
      seq {
        bind %a = pat_tuple_elem
        bind %b = pat_tuple_elem
      }
      seq {
        store @demo_x3a_x3aa = %a
        store @demo_x3a_x3ab = %b
      }
      seq {
        call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x656C646E614863696E615074696E49, "")
        init_panic_handle demo [demo]
      }
    }
  }
}

proc @cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3ademo {
  seq {
    seq {
      seq {
        read @demo::b
        nop
      }
      seq {
        read @demo::a
        nop
      }
    }
    seq {
      read @demo::y
      nop
    }
    seq {
      read @demo::x
      nop
    }
  }
}

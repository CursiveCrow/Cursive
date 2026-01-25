vtable @vtable_x3a_x3ademo_x3a_x3aR_x3a_x3acl_x3a_x3ademo_x3a_x3aD size=4 align=4 drop=@cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3ademo_x3a_x3aR slots=[@demo_x3a_x3aR_x3a_x3aping]

proc @demo_x3a_x3aR_x3a_x3aping {
  block seq {
    nop
    call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x7974706D452D70756E61656C43, "")
    ret 0x1
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
        bind %r = record
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
            seq {
              call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x6B6365684363696E6150, "")
              panic_check
            }
          }
        }
        seq {
          call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x74726174532D70756E61656C43, "")
          clear_panic
          call @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3ademo_x3a_x3aR (%r, %__panic)
          seq {
            load_ptr panic_flag_ptr
            load_ptr panic_code_ptr
          }
          binop && 0x0, panic_flag
          if double_panic then seq {
            call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x74726F62412D706F72442D706574532D70756E61656C43, "")
            call @cursive_x3a_x3aruntime_x3a_x3apanic (panic_code)
          } else nop
          unop ! 0x0
          binop && panic_flag, panic_not_panicking
          if cleanup_panic then call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x63696E61502D706F72442D706574532D70756E61656C43, "") else nop
          unop ! panic_flag
          if panic_clear then call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x6B4F2D706F72442D706574532D70756E61656C43, "") else nop
          binop && 0x0, panic_clear
          if panic_restore then seq {
            store_ptr panic_flag_ptr = 0x1
            store_ptr panic_code_ptr = 0x0
          } else nop
          binop || 0x0, panic_flag
          if panic_flag then nop else nop
          call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x656E6F442D70756E61656C43, "")
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

proc @demo_x3a_x3amain {
  seq {
    seq {
      call @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3ademo (%__panic)
      panic_check
    }
    block seq {
      seq {
        nop
        bind %raw = 0x0
      }
      seq {
        seq {
          block nop seq {
            seq {
              seq {
                read %raw
                read %raw
              }
              transmute tuple to $FileSystem
            }
            nop
          }
          bind %fs = transmute
        }
        panic_check
      }
      seq {
        nop
        bind %s = 0x6968
      }
      seq {
        seq {
          addr_of fs
          read %s
          call @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir (addr_of, %s)
        }
        seq {
          clear_panic
          match method_call {
            arm {
              pat __case0
              body { () }
            }
            arm {
              pat __case1
              body { () }
            }
          }
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
      }
      seq {
        seq {
          addr_of fs
          read %s
          call @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists (addr_of, %s)
        }
        bind %_e = method_call
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

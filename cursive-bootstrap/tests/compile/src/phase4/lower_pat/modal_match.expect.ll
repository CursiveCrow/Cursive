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
          unop widen record
        }
        bind %s = unop
      }
      seq {
        nop
        bind %b = record
      }
      seq {
        seq {
          seq {
            seq {
              read %s
              move_state s
            }
            match %s {
              arm {
                pat @Idle
                body { 0x0 }
              }
              arm {
                pat @Busy {code}
                body { %code }
              }
            }
          }
          nop
        }
        panic_check
      }
      seq {
        seq {
          seq {
            read %b
            move_state b
          }
          match %b {
            arm {
              pat @Busy {code}
              body { %code }
            }
          }
        }
        nop
        ret match
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

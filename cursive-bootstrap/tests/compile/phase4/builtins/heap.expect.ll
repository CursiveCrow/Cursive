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
              transmute tuple to $HeapAllocator
            }
            nop
          }
          bind %heap = transmute
        }
        panic_check
      }
      seq {
        nop
        bind %count = 0x1
      }
      seq {
        block seq {
          seq {
            seq {
              addr_of heap
              read %count
              call @cursive_x3a_x3aruntime_x3a_x3aheap_x3a_x3aalloc_x5fraw (addr_of, %count)
            }
            bind %p = method_call
          }
          seq {
            seq {
              addr_of heap
              seq {
                read %p
                read %count
              }
              call @cursive_x3a_x3aruntime_x3a_x3aheap_x3a_x3adealloc_x5fraw (addr_of, %p, %count)
            }
            panic_check
          }
        } seq {
          nop
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

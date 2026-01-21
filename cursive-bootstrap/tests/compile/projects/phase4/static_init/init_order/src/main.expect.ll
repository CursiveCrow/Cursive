global_const @alpha_x3a_x3aax bytes=4

proc @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3aalpha {
  seq {
    nop
    bind %ax = 0x1
    store @alpha_x3a_x3aax = %ax
    init_panic_handle alpha [alpha]
  }
}

proc @cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3aalpha {
  seq {
    read @alpha::ax
    nop
  }
}


global_const @beta_x3a_x3abx bytes=4

proc @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3abeta {
  seq {
    nop
    bind %bx = 0x1
    store @beta_x3a_x3abx = %bx
    init_panic_handle beta [beta]
  }
}

proc @cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3abeta {
  seq {
    read @beta::bx
    nop
  }
}


global_const @demo_x3a_x3aroot_x5fval bytes=4

proc @demo_x3a_x3amain {
  seq {
    seq {
      seq {
        call @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3aalpha (%__panic)
        panic_check
      }
      seq {
        call @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3abeta (%__panic)
        panic_check
      }
      seq {
        call @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3ademo (%__panic)
        panic_check
      }
    }
    block seq {
      nop
      nop
      ret 0x0
    } nop
  }
}

proc @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3ademo {
  seq {
    nop
    bind %root_val = 0x0
    store @demo_x3a_x3aroot_x5fval = %root_val
    init_panic_handle demo [demo]
  }
}

proc @cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3ademo {
  seq {
    read @demo::root_val
    nop
  }
}

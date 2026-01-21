proc @demo_x3a_x3aPoint_x3a_x3alen {
  block seq {
    nop
    nop
    ret 0x0
  } nop
}

proc @demo_x3a_x3aMachine_x3a_x3aIdle_x3a_x3astatus {
  block seq {
    nop
    nop
    ret 0x0
  } nop
}

proc @demo_x3a_x3aMachine_x3a_x3aIdle_x3a_x3astay {
  block seq {
    nop
    seq {
      clear_panic
      call @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3ademo_x3a_x3aMachine_x3a_x3aIdle (%self, %__panic)
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
    ret record
  } nop
}

proc @demo_x3a_x3amain {
  seq {
    seq {
      call @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3ademo (%__panic)
      panic_check
    }
    block seq {
      nop
      nop
      ret 0x0
    } nop
  }
}

proc @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3ademo {
  nop
}

proc @cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3ademo {
  nop
}

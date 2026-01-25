proc @default_x3a_x3ademo_x3a_x3aSprite_x3a_x3acl_x3a_x3ademo_x3a_x3aRenderable_x3a_x3arender {
  block seq {
    nop
    call @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit (0x7974706D452D70756E61656C43, "")
    ret 0x1
  } nop
}

vtable @vtable_x3a_x3ademo_x3a_x3aSprite_x3a_x3acl_x3a_x3ademo_x3a_x3aRenderable size=4 align=4 drop=@cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3ademo_x3a_x3aSprite slots=[@default_x3a_x3ademo_x3a_x3aSprite_x3a_x3acl_x3a_x3ademo_x3a_x3aRenderable_x3a_x3arender]

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
  nop
}

proc @cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3ademo {
  nop
}

proc @default_x3a_x3ademo_x3a_x3aSprite_x3a_x3acl_x3a_x3ademo_x3a_x3aRenderable_x3a_x3arender {
  block seq {
    nop
    nop
    ret 0x1
  } nop
}

vtable @vtable_x3a_x3ademo_x3a_x3aSprite_x3a_x3acl_x3a_x3ademo_x3a_x3aRenderable size=4 align=4 drop=@cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3ademo_x3a_x3aSprite slots=[@default_x3a_x3ademo_x3a_x3aSprite_x3a_x3acl_x3a_x3ademo_x3a_x3aRenderable_x3a_x3arender]

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

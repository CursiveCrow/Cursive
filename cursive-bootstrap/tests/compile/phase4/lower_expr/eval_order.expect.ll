bind %a = 0x1
bind %b = 0x2
bind %c = 0x3
call @trace (addr_of, %__panic)
call @trace (addr_of, %__panic)
call @trace (addr_of, %__panic)
ret 0x0

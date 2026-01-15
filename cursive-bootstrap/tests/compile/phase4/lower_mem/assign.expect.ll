bind %x = 0xA
store %x = 0x14
bind %arr = array
check_index %arr, 0x0
rec_update arr[0usize]
bind %rec = record
rec_update rec.a

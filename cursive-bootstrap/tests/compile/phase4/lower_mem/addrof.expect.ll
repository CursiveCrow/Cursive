bind %p = record
bind %ptr = addr_of p
addr_of p.x
bind %ptr_x = addr_of
bind %arr = array
bind %ptr_elem = addr_of arr[0usize]

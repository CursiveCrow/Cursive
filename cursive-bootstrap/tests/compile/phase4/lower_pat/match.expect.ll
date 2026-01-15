match 0xA {
  arm {
     pat <literal 10>
     body { 0x1 }
  }
  arm {
     pat _
     body { 0x0 }
  }
}

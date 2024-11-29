package main

/*
#cgo CXX CXXFLAGS: -std=c++17

#cgo CFLAGS: -I./include

#cgo LDFLAGS: -L${SRCDIR}/build -lletus -lssl -lcrypto

#include "Letus.h"

*/
import "C"

import "fmt"

func main() {
   var c *C.DMMTrie = C.OpenLetus()
   C.LetusPut(c)
   res := C.LetusGet(c)
   fmt.Println(res)
}

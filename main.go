package main

/*
#cgo CXX CXXFLAGS: -std=c++17

#cgo CFLAGS: -I./lib

#cgo LDFLAGS: -L${SRCDIR}/build -lletus -lssl -lcrypto

#include "Letus.h"

*/
import "C"

import "fmt"

func main() {
   c := C.OpenLetus()
   C.LetusPut(c)
   res := C.LetusGet(c)
   fmt.Println(res)
}

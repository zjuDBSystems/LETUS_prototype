package main

/*
#cgo CFLAGS: -I${SRCDIR}/../lib

#cgo LDFLAGS: -L${SRCDIR}/../build -lletus -lssl -lcrypto -lstdc++

#include "Letus.h"

*/
import "C"

import "fmt"

func main() {
   c := C.OpenLetus()
   C.LetusPut(c)
   res := C.LetusGet(c)
   fmt.Println(C.GoString(res))
   fmt.Println("Hello, world!")
}

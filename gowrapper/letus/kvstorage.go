package letus

import "fmt"
import "unsafe"

/*
#cgo CFLAGS: -I${SRCDIR}/../../lib
#cgo LDFLAGS: -L${SRCDIR}/../../build -lletus -lssl -lcrypto -lstdc++
#include "Letus.h"
*/
import "C"

type cgo_Letus C.struct_Letus


// LetusKVStroage is an implementation of KVStroage.
type LetusKVStroage struct {
	c *cgo_Letus
}

func NewLetusKVStroage() (KVStorage, error) {
	s := &LetusKVStroage{
		c: (*cgo_Letus)(C.OpenLetus()),
	}
	return s, nil
}

func (s *LetusKVStroage) Put(key []byte, value []byte) error {
	C.LetusPut(s.c, (*C.char)(unsafe.Pointer(&key[0])), (*C.char)(unsafe.Pointer(&value[0])))
	fmt.Println("Letus Put!")
	return nil
}

func (s *LetusKVStroage) Get(key []byte) ([]byte, error) {
	res := C.LetusGet(s.c, (*C.char)(unsafe.Pointer(&key[0])))
	// res := C.LetusGet(s.c)
	fmt.Println("Letus Get!")
	fmt.Println((C.GoString)(res))
	return nil, nil
}

func (s *LetusKVStroage) Delete(key []byte) error { 
	fmt.Println("Letus delete!")
	return nil 
}

func (s *LetusKVStroage) Close() error {
	fmt.Println("close Letus!")
	return nil 
}
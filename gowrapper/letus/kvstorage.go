package letus

import "fmt"
import "unsafe"

/*
#cgo CFLAGS: -I${SRCDIR}/../../lib
#cgo LDFLAGS: -L${SRCDIR}/../../build -lletus -lssl -lcrypto -lstdc++
#include "Letus.h"
#include <stdio.h>
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
    // 假设LetusGet函数返回的是一个C.char*类型的指针
    res := (*C.char)(C.LetusGet(s.c, (*C.char)(unsafe.Pointer(&key[0]))))
    if res == nil {
        return nil, fmt.Errorf("key not found")
    }
	fmt.Printf("Type of res: %T\n", res)
    // 将C.char*转换为[]byte
    resBytes := C.GoBytes(unsafe.Pointer(res), C.int(C.strlen(res)))
    return nil, nil
    // return resBytes, nil
}

func (s *LetusKVStroage) Delete(key []byte) error { 
	fmt.Println("Letus delete!")
	return nil 
}

func (s *LetusKVStroage) Close() error {
	fmt.Println("close Letus!")
	return nil 
}

func (s *LetusKVStroage) NewBatch() (Batch, error) { 
	return NewLetusBatch()
}
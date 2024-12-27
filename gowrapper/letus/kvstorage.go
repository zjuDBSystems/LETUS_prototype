package letus

import "fmt"
import "unsafe"

/*
#cgo CFLAGS: -I${SRCDIR}/../../lib
#cgo LDFLAGS: -L${SRCDIR}/../../build_release -lletus -lssl -lcrypto -lstdc++ -fsanitize=address
#include "Letus.h"
#include <stdio.h>
*/
import "C"

type cgo_Letus C.struct_Letus


// LetusKVStroage is an implementation of KVStroage.
type LetusKVStroage struct {
	c *cgo_Letus
	tid uint64
	stable_seq_no uint64
	current_seq_no uint64
}

func NewLetusKVStroage(path []byte) (KVStorage, error) {
	s := &LetusKVStroage{
		c: (*cgo_Letus)(C.OpenLetus((*C.char)(unsafe.Pointer(&path[0])))),
		tid: 0,
		stable_seq_no: 0,
		current_seq_no: 1,
	}
	return s, nil
}

func (s *LetusKVStroage) Put(key []byte, value []byte) error {
	C.LetusPut(s.c, C.uint64_t(s.tid), C.uint64_t(s.current_seq_no), (*C.char)(unsafe.Pointer(&key[0])), (*C.char)(unsafe.Pointer(&value[0])))
	fmt.Println("Letus Put!", s.tid, s.current_seq_no, string(key), string(value))
	return nil
}

func (s *LetusKVStroage) Get(key []byte) ([]byte, error) {
    value := C.LetusGet(s.c, C.uint64_t(s.tid), C.uint64_t(s.stable_seq_no), (*C.char)(unsafe.Pointer(&key[0])))
    if value == nil {
		return nil, fmt.Errorf("key not found")
    }
	fmt.Println("Letus Get!", s.tid, s.stable_seq_no, string(key), C.GoString(value))
	return []byte(C.GoString(value)), nil
}

func (s *LetusKVStroage) Delete(key []byte) error { 
	fmt.Println("Letus delete!", string(key))
	return nil 
}

func (s* LetusKVStroage) Commit(seq uint64) error { 
	fmt.Println("Letus commit!")
	C.LetusCommit(s.c, C.uint64_t(seq))
	s.stable_seq_no = seq
	s.current_seq_no = seq + 1
	return nil 
}
func (s *LetusKVStroage) Close() error {
	fmt.Println("close Letus!")
	return nil 
}

func (s *LetusKVStroage) NewBatch() (Batch, error) {
	return NewLetusBatch()
}

func (s *LetusKVStroage) SetSeqNo(seq uint64) error { 
	s.current_seq_no = seq
	return nil 
}

func (s *LetusKVStroage) GetStableSeqNo() (uint64, error) {
	return s.stable_seq_no, nil
}
func (s *LetusKVStroage) GetCurrentSeqNo() (uint64, error) {
	return s.current_seq_no, nil
}

func (s *LetusKVStroage) Proof(key []byte, seq uint64) (types.ProofPath, error){
	proofPath := make(ProofPath, 0)
	for i := uint64(0); i <= seq; i++ {
		proofPath = append(proofPath, s.Get(key))
	}
	return proofPath, nil

}
func (s *LetusKVStroage) SetEngine(engine cryptocom.Engine) {}
func (s *LetusKVStroage) FSync(seq uint64) error { return nil }
package letus

import "fmt"
import "unsafe"
import "letus/types"
/*
#cgo CFLAGS: -I${SRCDIR}/../../lib
#cgo LDFLAGS: -L${SRCDIR}/../../build_release -lletus -lssl -lcrypto -lstdc++
#include "Letus.h"
#include <stdio.h>
#include <stdlib.h>
*/
import "C"


// LetusKVStroage is an implementation of KVStroage.
type LetusKVStroage struct {
	c *C.Letus
	tid uint64
	stable_seq_no uint64
	current_seq_no uint64
}

func NewLetusKVStroage(config *LetusConfig) (KVStorage, error) {
	path := config.GetDataPath()
	s := &LetusKVStroage{
		c: C.OpenLetus(C.CString(path)),
		tid: 0,
		stable_seq_no: 0,
		current_seq_no: 1,
	}
	return s, nil
}

func (s *LetusKVStroage) Put(key []byte, value []byte) error {
	sha1key := sha1hash(key)
	C.LetusPut(s.c, C.uint64_t(s.tid), C.uint64_t(s.current_seq_no), getCPtr(sha1key), getCPtr(value))
	fmt.Printf("Letus Put! tid=%d, seq=%d, key=%s(%s), value=%s\n", s.tid, s.current_seq_no, string(key), string(sha1key), string(value))
	return nil
}

func (s *LetusKVStroage) Get(key []byte) ([]byte, error) {
	var value *C.char
	sha1key := sha1hash(key)

	if s.stable_seq_no != 0 {
		value = C.LetusGet(s.c, C.uint64_t(s.tid), C.uint64_t(s.stable_seq_no), getCPtr(sha1key))
		fmt.Printf("Letus Get! tid=%d, seq=%d, key=%s(%s), value=%s\n", s.tid, s.stable_seq_no, string(key), string(sha1key), C.GoString(value))
	} else  {
		value = C.LetusGet(s.c, C.uint64_t(s.tid), C.uint64_t(1), getCPtr(sha1key))
		fmt.Printf("Letus Get! tid=%d, seq=%d, key=%s(%s), value=%s\n", s.tid, 1, string(key), string(sha1key), C.GoString(value))
	} 
		
	if value == nil || C.GoString(value) == "" {
		return nil, fmt.Errorf("key not found")
    }
	fmt.Println("Letus Get!", s.tid, s.stable_seq_no, string(key), C.GoString(value))
	return []byte(C.GoString(value)), nil
	}
	
func (s *LetusKVStroage) Delete(key []byte) error {
	sha1key := sha1hash(key)
	C.LetusDelete(s.c, C.uint64_t(s.tid), C.uint64_t(s.current_seq_no), getCPtr(sha1key))
	fmt.Printf("Letus Delete! tid=%d, seq=%d, key=%s(%s)\n", s.tid, s.current_seq_no, string(key), string(sha1key))
	return nil 
}

func (s* LetusKVStroage) Commit(seq uint64) error { 
	fmt.Println("Letus commit! version=", seq)
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

func (s *LetusKVStroage) NewBatchWithEngine() (Batch, error) {
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
	proof_path_c := C.LetusProof(s.c, C.uint64_t(s.tid), C.uint64_t(seq), (*C.char)(unsafe.Pointer(&key[0])))
	proof_path_size := C.LetusGetProofPathSize(proof_path_c)
	proof_path := make(types.ProofPath, proof_path_size)
	for i:=0; i < int(proof_path_size); i++ {
		proof_node_size := C.LetusGetProofNodeSize(proof_path_c, C.uint64_t(i))
		proof_path[i] = &types.ProofNode{
			IsData: bool(C.LetusGetProofNodeIsData(proof_path_c, C.uint64_t(i))),
			Hash: []byte(C.GoString(C.LetusGetProofNodeHash(proof_path_c, C.uint64_t(i)))),
			Key: []byte(C.GoString(C.LetusGetProofNodeKey(proof_path_c, C.uint64_t(i)))),
			Index: int(C.LetusGetProofNodeIndex(proof_path_c, C.uint64_t(i))),
			Inodes: make(types.Inodes, proof_node_size),
		}
		for j:=0; j < int(proof_node_size); j++ {
			proof_path[i].Inodes[j] = &types.Inode{
				Hash: []byte(C.GoString(C.LetusGetINodeHash(proof_path_c, C.uint64_t(i), C.uint64_t(j)))),
				Key: []byte(C.GoString(C.LetusGetINodeKey(proof_path_c, C.uint64_t(i), C.uint64_t(j)))),
			}
		}
	}
	return proof_path, nil
}

// func (s *LetusKVStroage) SetEngine(engine cryptocom.Engine) {}
func (s *LetusKVStroage) FSync(seq uint64) error { return nil }


type LetusConfig struct {
	DataPath      string
	CheckInterval uint64
	Compress      bool
	Encrypt       bool
	BucketMode    bool
	VlogSize      uint64
	sync          bool
}


func GetDefaultConfig() *LetusConfig {
	DefaultSync := false
	DefaultEncryption := false
	DefaultCheckInterval := uint64(100)
	DefaultDataPath := "./data"
	DefaultBucketMode := false
	DefaultVlogSize := uint64(1024 * 1024)
	return &LetusConfig{
		sync:          DefaultSync,
		Encrypt:       DefaultEncryption,
		Compress:      true,
		CheckInterval: DefaultCheckInterval,
		DataPath:      DefaultDataPath,
		BucketMode:    DefaultBucketMode,
		VlogSize:      DefaultVlogSize,
	}
}

func (v LetusConfig) GetDataPath() string {
	return v.DataPath
}
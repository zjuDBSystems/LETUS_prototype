package main

import (
	"fmt"
	"letus/letus"
)

func main() {
   config := letus.GetDefaultConfig()
   db, ok := letus.NewLetusKVStroage(config)
   if ok != nil{
      panic("Failed to create LetusKVStroage")
   }
   batch, _ := db.NewBatch()
   var seq uint64
   var res []byte

   // batch operation
   batch.Put([]byte("11111"), []byte("aaaaa"))
   batch.Put([]byte("22222"), []byte("bbbbb"))
   batch.Put([]byte("33333"), []byte("ccccc"))
   seq, _ = db.GetSeqNo()
   batch.Hash(seq)
   // db get operations
   res, _ = db.Get([]byte("11111"))
   if res == nil { fmt.Println("Failed to get") }
   batch.Write(seq)
   batch.Release()
   fmt.Println(string(res))
   
   // batch operation
   batch.Put([]byte("44444"), []byte("aaaaa"))
   batch.Put([]byte("55555"), []byte("bbbbb"))
   batch.Put([]byte("66666"), []byte("ccccc"))
   ok = batch.Delete([]byte("22222"))
   if ok != nil{ fmt.Println("Failed to delete") }
   seq, _ = db.GetSeqNo()
   batch.Hash(seq)
   batch.Write(seq)
   batch.Release()
   // db get operations
   res, _ = db.Get([]byte("55555"))
   if res == nil { 
      fmt.Println("Failed to get") 
   } else { fmt.Println(string(res)) }
   res, _ = db.Get([]byte("22222"))
   if res == nil { 
      fmt.Println("Failed to get") 
   } else { fmt.Println(string(res)) }

   seq, _ = db.GetStableSeqNo()
   db.Proof([]byte("11111"), seq)
   // close db
   _ = db.Close()
   fmt.Println("Pass!")
}

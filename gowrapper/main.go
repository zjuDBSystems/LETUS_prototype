package main


import "fmt"
import "./letus"

func main() {
   config := letus.GetDefaultConfig()
   db, ok := letus.NewLetusKVStroage(config)
   if ok != nil{
      panic("Failed to create LetusKVStroage")
   }
   batch, _ := db.NewBatch()
   // batch operation
   batch.Put([]byte("11111"), []byte("aaaaa"))
   batch.Put([]byte("22222"), []byte("bbbbb"))
   batch.Put([]byte("33333"), []byte("ccccc"))
   ok = batch.Delete([]byte("22222"))
   if ok != nil{
      panic("Failed to delete")
   }
   batch.Write(db)
   // db get operations
   res, _ := db.Get([]byte("11111"))
   if res == nil{
      panic("Failed to get")
   }
   fmt.Println(string(res))
   seq, _ := db.GetStableSeqNo()
   db.Proof([]byte("11111"), seq)
   db.Delete([]byte("11111"))
   // close db
   _ = db.Close()
   fmt.Println("Pass!")
}

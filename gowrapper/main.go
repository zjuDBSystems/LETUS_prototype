package main


import "fmt"
import "./letus"

func main() {
   db, ok := letus.NewLetusKVStroage()
   if ok != nil{
      panic("Failed to create LetusKVStroage")
   }
   db.Put([]byte("123"), []byte("aaaaa"))
   _ , _ = db.Get([]byte("123"))
   db.Delete([]byte("123"))
   _ = db.Close()
   fmt.Println("Hello, world!")
}

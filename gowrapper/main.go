package main


import "fmt"
import "./letus"

func main() {
   c := letus.NewLetusKVStroage()
   c.Close()
   fmt.Println("Hello, world!")
}

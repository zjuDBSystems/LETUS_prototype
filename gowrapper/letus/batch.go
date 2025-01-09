package letus

import "fmt"

type KVPair struct {
    key  string
    value string
}

type LetusBatch struct {
	db *LetusKVStroage
	kv []KVPair
}

func NewLetusBatch(db_ *LetusKVStroage) (Batch, error) {
	b := &LetusBatch{
		kv: make([]KVPair, 0),
		db: db_
	}
	return b, nil
}

func (b *LetusBatch) Put(key, value []byte) error {
  	// 创建一个新的KVPair
	pair := KVPair{
		key:   string(key),
		value: string(value),
	}

	// 将新的KVPair添加到kv切片中
	b.kv = append(b.kv, pair)
	return nil
}

func (b *LetusBatch) Delete(key []byte) error {
	// 遍历kv切片
	for i := 0; i < len(b.kv); i++ {
		if b.kv[i].key == string(key) {
			// 如果找到匹配的键，使用切片的删除方法
			b.kv = append(b.kv[:i], b.kv[i+1:]...)
			return nil // 成功删除，返回nil
		}
	}
	return fmt.Errorf("key %v not found", key) // 键不存在，返回错误
}

// Persist batch content

func (b *LetusBatch) Write(seq interface{}) error { 
	for i := 0; i < len(b.kv); i++ {
		ok := db_.Put([]byte(b.kv[i].key), []byte(b.kv[i].value))
		if ok != nil { return ok }
	}
	// [TODO]: db.Flush()
	ok = db.Commit(seq.(uint64))
	if ok != nil { return ok }
	return nil 
}

func (b *LetusBatch) Hash(seq uint64) error {
	ok = db.Commit(seq)
	return nil
}

func (b *LetusBatch) Len() uint64 {
	return uint64(len(b.kv))
}

func (b *LetusBatch) Release() error {
	b.kv = make([]KVPair, 0)
	return nil
}
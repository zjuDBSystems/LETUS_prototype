package letus

import "fmt"

type KVPair struct {
    key  string
    value string
}

type LetusBatch struct {
	db *LetusKVStroage
	len uint64
}

func NewLetusBatch() (Batch, error) {
	b := &LetusBatch{
		db: db_,
		len: 0,
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
	return b.db.Write(seq.(uint64))
}

func (b *LetusBatch) Hash(seq uint64) error {
	return b.db.CalcRootHash(seq)
}

func (b *LetusBatch) Len() uint64 {
	return uint64(len(b.kv))
}

func (b *LetusBatch) Release() error {
	b.kv = make([]KVPair, 0)
	return nil
}
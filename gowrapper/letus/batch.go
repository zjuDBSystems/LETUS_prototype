package letus

type LetusBatch struct {
	db *LetusKVStroage
	len uint64
}

func NewLetusBatch(db *LetusKVStroage) (Batch, error) {
	b := &LetusBatch{
		db: db,
		len: 0,
	}
	return b, nil
}

func (b *LetusBatch) Put(key, value []byte) error {
	b.len += 1
	return b.db.Put(key, value)
}

func (b *LetusBatch) Delete(key []byte) error {
	b.len += 1
	return b.db.Delete(key) // 键不存在，返回错误
}

// Persist batch content
func (b *LetusBatch) Write(seq interface{}) error { 
	return b.db.Write(seq.(uint64))
}

func (b *LetusBatch) Hash(seq uint64) error {
	return b.db.CalcRootHash(seq)
}

func (b *LetusBatch) Len() uint64 {
	return b.len
}

func (b *LetusBatch) Release() error {
	b.len = 0
	return nil
}
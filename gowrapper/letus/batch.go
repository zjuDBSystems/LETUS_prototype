package letus

type LetusBatch struct {
	db *LetusKVStroage
<<<<<<< HEAD
	len uint64
=======
	kv []KVPair
>>>>>>> feat:hash before get/put
}

func NewLetusBatch(db_ *LetusKVStroage) (Batch, error) {
	b := &LetusBatch{
<<<<<<< HEAD
		db: db_,
		len: 0,
=======
		kv: make([]KVPair, 0),
		db: db_
>>>>>>> feat:hash before get/put
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
<<<<<<< HEAD
func (b *LetusBatch) Write(seq interface{}) error { 
	return b.db.Write(seq.(uint64))
}

func (b *LetusBatch) Hash(seq uint64) error {
	return b.db.CalcRootHash(seq)
=======

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
>>>>>>> feat:hash before get/put
}

func (b *LetusBatch) Len() uint64 {
	return b.len
}

func (b *LetusBatch) Release() error {
	b.len = 0
	return nil
}
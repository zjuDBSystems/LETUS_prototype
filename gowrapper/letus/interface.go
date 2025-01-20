
package letus

// import "crypto"
import "letus/types"

// KVStorage is a interface that provides managing k/v.
type KVStorage interface {
	// Put sets the value for the given key.
	Put(key []byte, value []byte) error
	// Get gets the value for the given key. It returns error.ErrNotFound if the
	// DB does not contain the key.
	Get(key []byte) ([]byte, error)
	// Delete deletes the value for the given key. It returns ErrNotFound if
	// the MemTable does not contain the key.
	Delete(key []byte) error
	// Close the storage engine.
	Close() error

	// NewBatch return a storage batch.
	NewBatch() (Batch, error)
	// NewBatchWithEngine return a storage batch.
	NewBatchWithEngine() (Batch, error)
	
	// NewIterator returns an iterator of the storage.
	// TODO: NewIterator should return error
	NewIterator(begin, end []byte) Iterator

	// get seqno of multicache, mainly for rollback, other db should return error
	GetSeqNo() (uint64, error)
	// revert according to seqno
	Revert(uint64) error

	// Commit persists batches whose seq is equal or smaller than the seq.
	// Commit only happens when at checkpoints, and all batches in an interval belong to a single
	// tempDB, so simply iterate the whole tempDB and do persist. After a tempDB is done persisting,
	// the pointer to that db and all disk files should be deleted. If everything seems fine,
	// related wals will be deleted afterwards.
	Commit(seq uint64) error

	// GetStableSeqNo return the max seq no in persist db
	GetStableSeqNo() (uint64, error)
	// Proof return proofPath for the given key.
	Proof(key []byte, seq uint64) (types.ProofPath, error)
	// SetEngine(engine cryptocom.Engine)
	// FSync fsync all data before seq.
	FSync(seq uint64) error
}

// Batch is a write batch interface for KVStorage.
type Batch interface {
	Put(key, value []byte) error
	Delete(key []byte) error
	// Persist batch content
	Write(interface{}) error
	Hash(uint64) error
	Len() uint64
	Release() error
}


type Iterator interface {
	LedgerIterator() LedgerIterator
	First() bool
	Last() bool
	Prev() bool
	Error() error
}


type LedgerIterator interface {
	//Next return true iff we can use Key() or Value() to read valid data
	Next() bool
	Key() interface{}
	Value() []byte
	Release()
	// Seek Targeting the iterator to the first key that not less than the param
	// return false iff the param is out of the right bound
	Seek(interface{}) bool
}
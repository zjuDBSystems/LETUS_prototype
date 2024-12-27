

# Hyperchain-db.interface

Hyperchain内部通过flatodb来管理所有类型的存储引擎，相应的需要实现对应接口，如ViDB实现KVStorage接口。

KV存储引擎接口：
```go
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
	//TODO: NewIterator should return error
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
	SetEngine(engine cryptocom.Engine)
	// FSync fsync all data before seq.
	FSync(seq uint64) error
}
```

Batch接口：
```go
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
```

通用型Iterator接口：

```go
// Iterator is a interface that traversing data.
type Iterator interface {
	LedgerIterator
	First() bool
	Last() bool
	Prev() bool
	Error() error
}
```

在初始化阶段，需要实现类似于**Open**的方法，方法定义如下：
```go
Open(conf VidbConfigInterface, logger Logger) (KVStorage,error)
```

```go
type VidbConfigInterface interface {
	Sync() bool
	VidbDataPath() string
	CompressEnable() bool
	GetBucketMode() bool
	GetEncrypt() bool
	GetCheckInterval() uint64
	GetVlogSize() uint64
}
```

接口的调用顺序以及逻辑如下：

+ Open() 返回 KVStorage实例对象 db
+ db.NewBatch() / db.NewBatchWithEngine() 返回一个 Batch 的实例对象 batch
+ batch.Put(key,value) 对 Batch中 Put交易数据；
+ batch.Hash(blockSeq) -> batch.Write(blockSeq)
+ 在每N个区块高度时，进行一个 db.Commit(blockSeq) 操作，用于当前数据的CheckPoint，保证数据的一致性； 



参考示例如下：

```go
// VidbConfig struct
type VidbConfig struct {
	DataPath      string
	CheckInterval uint64
	Compress      bool
	Encrypt       bool
	BucketMode    bool
	VlogSize      uint64
	sync          bool
}

// VidbDataPath func
func (v VidbConfig) VidbDataPath() string {
	return v.DataPath
}

// CompressEnable func
func (v VidbConfig) CompressEnable() bool {
	return v.Compress
}

// GetBucketMode func
func (v VidbConfig) GetBucketMode() bool {
	return v.BucketMode
}

// GetEncrypt func
func (v VidbConfig) GetEncrypt() bool {
	return v.Encrypt
}

// GetCheckInterval func
func (v VidbConfig) GetCheckInterval() uint64 {
	return v.CheckInterval
}

// Sync func
func (v VidbConfig) Sync() bool {
	return v.sync
}

// GetVlogSize get vlog size from confid
func (v VidbConfig) GetVlogSize() uint64 {
	return v.VlogSize
}

// GetDefaultConfig return default vidb config
func GetDefaultConfig() *VidbConfig {
	return &VidbConfig{
		sync:          DefaultSync,
		Encrypt:       DefaultEncryption,
		Compress:      true,
		CheckInterval: DefaultCheckInterval,
		DataPath:      DefaultDataPath,
		BucketMode:    DefaultBucketMode,
		VlogSize:      DefaultVlogSize,
	}
}

```
其中，Logger接口定义如下：
```go
logger接口：
type Logger interface {
	Debug(v ...interface{})
	Debugf(format string, v ...interface{})

	Info(v ...interface{})
	Infof(format string, v ...interface{})

	Notice(v ...interface{})
	Noticef(format string, v ...interface{})

	Warning(v ...interface{})
	Warningf(format string, v ...interface{})

	Error(v ...interface{})
	Errorf(format string, v ...interface{})

	Critical(v ...interface{})
	Criticalf(format string, v ...interface{})
}
```


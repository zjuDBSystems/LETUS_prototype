package types

// Inode struct
type Inode struct {
	Key  []byte `json:"key,omitempty"`
	Hash []byte `json:"hash,omitempty"`
}

// Inodes struct
type Inodes []*Inode

// ProofNode struct
type ProofNode struct {
	IsData bool   `json:"isData"`
	Key    []byte `json:"key,omitempty"`
	Hash   []byte `json:"hash,omitempty"`
	Inodes Inodes `json:"inodes,omitempty"`
	Index  int    `json:"index"`
}

// ProofPath struct
type ProofPath []*ProofNode
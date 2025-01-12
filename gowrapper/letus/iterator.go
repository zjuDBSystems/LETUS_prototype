package letus

import "strconv"
import "fmt"

func keyInc(s []byte) []byte{
	// increase string by 1
	uintVal, err := strconv.ParseUint(string(s), 10, 64)
	if err != nil {
		fmt.Println("strIncr: ", err)
		return nil
	}
	return []byte(strconv.FormatUint(uintVal+1, 10))
}

func keyDec(s []byte) []byte{
	// increase string by 1
	uintVal, err := strconv.ParseUint(string(s), 10, 64)
	if err != nil {
		fmt.Println("strIncr: ", err)
		return nil
	}
	return []byte(strconv.FormatUint(uintVal-1, 10))
}


func strCmp(a []byte, b []byte) (bool, error) {
	// compare two strings as unsigned integers
	uintValA, err := strconv.ParseUint(string(a), 10, 64)
	if err != nil {
		return false, fmt.Errorf("strCmp: ", err)
	}
	uintValB, err := strconv.ParseUint(string(b), 10, 64)
	if err != nil {
		return false, fmt.Errorf("strCmp: ", err)
	}
	if uintValA < uintValB {
		return true, nil
	} else {
		return false, nil
	}
}

type LetusIterator struct{
	lg LedgerIterator
}

type LetusLedgerIterator struct {
	db *LetusKVStroage
	current_key []byte
	begin_key []byte
	end_key []byte
}

func NewLetusIterator(db *LetusKVStroage, begin []byte, end []byte) Iterator{
	it := &LetusIterator{
		lg: NewLetusLedgerIterator(db, begin, end),
	}
	return it
}

func NewLetusLedgerIterator(db *LetusKVStroage, begin []byte, end []byte) LedgerIterator{
	lg := &LetusLedgerIterator{
		db: db,
		current_key: begin,
		begin_key: begin,
		end_key: end,
	}
	return lg
}

func (it *LetusIterator) LedgerIterator() LedgerIterator{
	return it.lg
}

func (it *LetusIterator) First() bool{
	return true
}
func (it *LetusIterator) Last() bool{
	return true
}
func (it *LetusIterator) Prev() bool{
	return it.First()
}

func (it *LetusIterator) Error() error{
	return nil
}


func (lg *LetusLedgerIterator) Next() bool {
	// get the next key
	if cmp, _ := strCmp(lg.current_key, lg.end_key); cmp {
		lg.current_key = keyInc(lg.current_key)
		return true
	}
	return false
}

func (lg *LetusLedgerIterator) Key() interface{} {
	return lg.current_key
}

func (lg *LetusLedgerIterator) Value() []byte{
	val, err := lg.db.Get(lg.current_key)
	if err != nil {
		fmt.Println("Value: ", err)
		return nil
	}
	return val 
}

func (lg *LetusLedgerIterator) Release() {}

func (lg *LetusLedgerIterator) Seek(key interface{}) bool{

	if cmp, _ := strCmp(key.([]byte), lg.end_key); cmp {
		return false
	} else if cmp, _ := strCmp(lg.begin_key, key.([]byte)); cmp {
		return false
	}
	lg.current_key = key.([]byte)
	return true
}



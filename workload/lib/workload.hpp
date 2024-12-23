#ifndef _WORKLOAD_HPP_
#define _WORKLOAD_HPP_

#include <string>
#include "generator.hpp"
#include "properties.hpp"
#include "workload_utilis.hpp"

// INSERT: insert a key that are **not** loaded in the loading phrase
// READ: read a key that are loaded in the loading phrase
// UPDATE: update a key that are loaded in the loading phrase
enum Operation { INSERT, READ, UPDATE, SCAN, READMODIFYWRITE };

class Workload {
 public:
  /// The name of the property for the proportion of read transactions.
  static const std::string READ_PROPORTION_PROPERTY;
  static const std::string READ_PROPORTION_DEFAULT;

  /// The name of the property for the proportion of update transactions.
  static const std::string UPDATE_PROPORTION_PROPERTY;
  static const std::string UPDATE_PROPORTION_DEFAULT;

  /// The name of the property for the proportion of insert transactions.
  static const std::string INSERT_PROPORTION_PROPERTY;
  static const std::string INSERT_PROPORTION_DEFAULT;

  /// The name of the property for the proportion of scan transactions.
  static const std::string SCAN_PROPORTION_PROPERTY;
  static const std::string SCAN_PROPORTION_DEFAULT;

  /// The name of the property for the proportion of
  /// read-modify-write transactions.
  static const std::string READMODIFYWRITE_PROPORTION_PROPERTY;
  static const std::string READMODIFYWRITE_PROPORTION_DEFAULT;

  /// The name of the property for the max scan length (number of records).
  static const std::string MAX_SCAN_LENGTH_PROPERTY;
  static const std::string MAX_SCAN_LENGTH_DEFAULT;

  static const std::string RECORD_COUNT_PROPERTY;
  static const std::string OPERATION_COUNT_PROPERTY;

  static const std::string KEY_LENGTH_PROPERTY;
  static const std::string KEY_LENGTH_DEFAULT;
  static const std::string VALUE_LENGTH_PROPERTY;
  static const std::string VALUE_LENGTH_DEFAULT;
  static const std::string BATCH_SIZE_PROPERTY;
  static const std::string BATCH_SIZE_DEFAULT;

  Workload() {}
  Workload(utils::Properties &props);
  ~Workload();
  std::string NextSequenceKey();     /// Used for loading data
  std::string NextTransactionKey();  /// Used for transactions
  std::string NextRandomValue();
  Operation NextOperation() { return op_chooser_.Next(); }
  size_t NextScanLength() { return scan_len_chooser_->Next(); }
  uint64_t GetBatchSize() { return batch_size_; }
  uint64_t GetRecordCount() { return record_count_; }
  uint64_t GetOperationCount() { return operation_count_; }

 private:
  Generator<uint64_t> *key_generator_;
  Generator<uint64_t> *key_chooser_;
  Generator<uint64_t> *scan_len_chooser_;
  DiscreteGenerator<Operation> op_chooser_;
  uint64_t record_count_;  // number of records before transactions
  uint64_t operation_count_;
  uint64_t batch_size_;
  uint64_t min_key_;
  uint64_t max_key_;
  uint64_t key_len_;
  uint64_t value_len_;
  uint64_t max_scan_len_;
};

/// The name of the property for the proportion of read transactions.
const std::string Workload::READ_PROPORTION_PROPERTY = "readproportion";
const std::string Workload::READ_PROPORTION_DEFAULT = "0.95";

/// The name of the property for the proportion of update transactions.
const std::string Workload::UPDATE_PROPORTION_PROPERTY = "updateproportion";
const std::string Workload::UPDATE_PROPORTION_DEFAULT = "0.05";

/// The name of the property for the proportion of insert transactions.
const std::string Workload::INSERT_PROPORTION_PROPERTY = "insertproportion";
const std::string Workload::INSERT_PROPORTION_DEFAULT = "0.0";

/// The name of the property for the proportion of scan transactions.
const std::string Workload::SCAN_PROPORTION_PROPERTY = "scanproportion";
const std::string Workload::SCAN_PROPORTION_DEFAULT = "0.0";

/// The name of the property for the proportion of
/// read-modify-write transactions.
const std::string Workload::READMODIFYWRITE_PROPORTION_PROPERTY =
    "readmodifywriteproportion";
const std::string Workload::READMODIFYWRITE_PROPORTION_DEFAULT = "0.0";

const string Workload::MAX_SCAN_LENGTH_PROPERTY = "maxscanlength";
const string Workload::MAX_SCAN_LENGTH_DEFAULT = "1000";

const std::string Workload::RECORD_COUNT_PROPERTY = "recordcount";
const std::string Workload::OPERATION_COUNT_PROPERTY = "operationcount";

const std::string Workload::KEY_LENGTH_PROPERTY = "keylength";
const std::string Workload::KEY_LENGTH_DEFAULT = "5";
const std::string Workload::VALUE_LENGTH_PROPERTY = "valuelength";
const std::string Workload::VALUE_LENGTH_DEFAULT = "5";
const std::string Workload::BATCH_SIZE_PROPERTY = "batchsize";
const std::string Workload::BATCH_SIZE_DEFAULT = "10";

Workload::Workload(utils::Properties &p) {
  // operations proportions
  double read_proportion = std::stod(
      p.GetProperty(READ_PROPORTION_PROPERTY, READ_PROPORTION_DEFAULT));
  double update_proportion = std::stod(
      p.GetProperty(UPDATE_PROPORTION_PROPERTY, UPDATE_PROPORTION_DEFAULT));
  double insert_proportion = std::stod(
      p.GetProperty(INSERT_PROPORTION_PROPERTY, INSERT_PROPORTION_DEFAULT));
  double scan_proportion = std::stod(
      p.GetProperty(SCAN_PROPORTION_PROPERTY, SCAN_PROPORTION_DEFAULT));
  double readmodifywrite_proportion = std::stod(p.GetProperty(
      READMODIFYWRITE_PROPORTION_PROPERTY, READMODIFYWRITE_PROPORTION_DEFAULT));

  record_count_ = std::stoi(p.GetProperty(RECORD_COUNT_PROPERTY));
  operation_count_ = std::stoi(p.GetProperty(OPERATION_COUNT_PROPERTY));
  batch_size_ =
      std::stoi(p.GetProperty(BATCH_SIZE_PROPERTY, BATCH_SIZE_DEFAULT));
  key_len_ = std::stoi(p.GetProperty(KEY_LENGTH_PROPERTY, KEY_LENGTH_DEFAULT));
  value_len_ =
      std::stoi(p.GetProperty(VALUE_LENGTH_PROPERTY, VALUE_LENGTH_DEFAULT));
  min_key_ = 0;
  max_key_ = pow(10, key_len_ + 1) - 1;
  key_chooser_ = new ZipfianGenerator(min_key_, max_key_);
  key_generator_ = new CounterGenerator(min_key_);
  scan_len_chooser_ = new UniformGenerator(1, max_scan_len_);
  if (read_proportion > 0) {
    op_chooser_.AddValue(READ, read_proportion);
  }
  if (update_proportion > 0) {
    op_chooser_.AddValue(UPDATE, update_proportion);
  }
  if (insert_proportion > 0) {
    op_chooser_.AddValue(INSERT, insert_proportion);
  }
  if (scan_proportion > 0) {
    op_chooser_.AddValue(SCAN, scan_proportion);
  }
  if (readmodifywrite_proportion > 0) {
    op_chooser_.AddValue(READMODIFYWRITE, readmodifywrite_proportion);
  }
#ifdef DEBUG
  std::cout << "record_count: " << record_count_ << std::endl;
  std::cout << "operation_count: " << operation_count_ << std::endl;
  std::cout << "batch_size: " << batch_size_ << std::endl;
  std::cout << "key_len: " << key_len_ << std::endl;
  std::cout << "value_len: " << value_len_ << std::endl;
  std::cout << "read_proportion: " << read_proportion << std::endl;
  std::cout << "update_proportion: " << update_proportion << std::endl;
  std::cout << "insert_proportion: " << insert_proportion << std::endl;
  std::cout << "scan_proportion: " << scan_proportion << std::endl;
  std::cout << "readmodifywrite_proportion: " << readmodifywrite_proportion
            << std::endl;
#endif
}

Workload::~Workload() {
  if (key_generator_) delete key_generator_;
  if (key_chooser_) delete key_chooser_;
}

std::string Workload::NextSequenceKey() {
  uint64_t key_num = key_generator_->Next();
  return utils::BuildKeyName(key_num, key_len_);
}

std::string Workload::NextTransactionKey() {
  uint64_t key_num;
  do {
    key_num = key_chooser_->Next();
  } while (key_num > key_generator_->Last());
  return utils::BuildKeyName(key_num, key_len_);
}

std::string Workload::NextRandomValue() {
  std::string val = "";
  val = val.append(value_len_, utils::RandomPrintChar());
  return val;
}

#endif  //_WORKLOAD_HPP_

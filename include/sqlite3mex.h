// SQLite3 matlab driver library.
//
// Kota Yamaguchi 2012 <kyamagu@cs.stonybrook.edu>

#ifndef __SQLITE3MEX_H__
#define __SQLITE3MEX_H__

#include "boost/unordered_map.hpp"
#include "boost/variant.hpp"
#include <deque>
#include <map>
#include <mex.h>
#include "sqlite3.h"
#include <stdint.h>
#include <string>
#include <vector>

using namespace std;

// Alias for the mex error function.
#define ERROR(...) mexErrMsgIdAndTxt("sqlite3:error", __VA_ARGS__)

namespace sqlite3mex {

// Integer type.
typedef int64_t IntegerValue;
// Float type.
typedef double FloatValue;
// Text type.
typedef string TextValue;
// Blob value class.
typedef vector<uint8_t> BlobValue;
// Null value class.
typedef struct {} NullValue;
// Value of the sqlite row. A pair of type and value.
typedef pair<int, boost::variant<IntegerValue, FloatValue, TextValue,
    BlobValue, NullValue> > Value;
// Column of the query result.
typedef struct {
  string name;         // Name of the column.
  deque<Value> values; // Values of the column.
} Column;

// SQL statement object. It manages execution and query results.
class Statement {
public:
  // Create a new statement.
  Statement();
  // Finalize this statement.
  ~Statement();
  // Prepare the statement.
  bool prepare(const string& statement, sqlite3* database);
  // Finalize the statement.
  bool finalize();
  // Execute the prepared statement.
  bool step();
  // Reset the prepared statement.
  bool reset();
  // Bind parameters.
  bool bind(const vector<const mxArray*>& params);
  // Check if the return code is ok.
  bool ok() const;
  // Check if the return code is done.
  bool done() const;
  // Check if the return code is row.
  bool row() const;
  // Return the last result code.
  int code() const;
  // Return sqlite3_stmt* object.
  sqlite3_stmt* get();
  // Column count. The statement must be in ROW code. i.e., row() == true.
  int column_count() const;
  // Data count. The statement must be in ROW code. i.e., row() == true.
  int data_count() const;
  // Column name. The statement must be in ROW code. i.e., row() == true.
  const char* column_name(int i) const;
  // Column type. The statement must be in ROW code. i.e., row() == true.
  int column_type(int i) const;
  // Column value. The statement must be in ROW code. i.e., row() == true.
  const Value& column_value(int i);

private:
  // Prepared statement.
  sqlite3_stmt* statement_;
  // Return code.
  int code_;
  // Current value.
  Value value_;
};

// Cache for the prepared statements. It looks up the prepared SQL statement
// in the hash map, and if not found, compiles a new statement. The cache
// entries are maintained by the fifo so that oldest entry is removed when
// the size exceeds the maximum limit.
class StatementCache {
public:
  // Default cache size.
  static const int DEFAULT_CACHE_SIZE = 16;

  // Create a default cache.
  StatementCache(size_t cache_size = DEFAULT_CACHE_SIZE);
  // Cache destructor.
  ~StatementCache();
  // Get the cached statement.
  Statement* get(const string& statement, sqlite3* database);

private:
  // FIFO to maintain generation of the keys.
  deque<string> fifo_;
  // Lookup table.
  boost::unordered_map<string, Statement> table_;
  // Maximum cache size.
  size_t cache_size_;
};

// Database connection.
class Database {
public:
  Database();
  ~Database();
  // Open a connection.
  bool open(const string& filename);
  // Return the last error code.
  int error_code();
  // Return the last error message.
  const char* error_message();
  // Execute SQL statement.
  bool execute(const string& statement,
               const vector<const mxArray*>& params,
               mxArray** result);
  // Set timeout when busy.
  bool busy_timeout(int milliseconds);

private:
  // Close the connection.
  void close();
  // Make columns and fill in matlab-safe column names.
  void create_columns(const Statement& stmt, vector<Column>* columns);
  // Convert vector<Column> to mxArray*.
  bool convert_columns_to_array(vector<Column>* columns, mxArray** array);
  // Convert Value object to mxArray*.
  mxArray* convert_value_to_array(const Value& value);

  // Statement cache.
  StatementCache statement_cache_;
  // SQLite3 C object.
  sqlite3* database_;
};

} // namespace sqlite3mex

#endif // __SQLITE3MEX_H__

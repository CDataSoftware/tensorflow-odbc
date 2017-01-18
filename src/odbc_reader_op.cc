/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.
   Copyright 2017 CData Software, Inc. All Rights Reserved.

Licensed under the GPL License, Version 3.0 (the "License"), see License.md 
for details.

ODBC Reader for TensorFlow is also is available for commercial licensing. 
Please, contact sales@cdata.com for details. 

==============================================================================*/

#include <memory>
#include "tensorflow/core/framework/reader_op_kernel.h"
#include "tensorflow/core/kernels/reader_base.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/strings/strcat.h"
#include "tensorflow/core/platform/env.h"

#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>

namespace tensorflow {

class ODBCReader : public ReaderBase {
 public:
  ODBCReader(const string& node_name, const string& connection_string, const string& sql_query, Env* env)
      : ReaderBase(strings::StrCat("ODBCReader '", node_name, "'")),
        connection_string_(connection_string),
        sql_query_(sql_query),
        env_(env),
        sql_env_(NULL),
        sql_dbc_(NULL),
        sql_stmt_(NULL),
        key_column_(-1),
        value_column_(-1),
        value_is_binary_(false),
        line_number_(0) {}

  Status ODBCStatus(SQLRETURN sql_return) {
    if (sql_return < 0) {
      return Status(error::INTERNAL, "ODBCReader error");
    } else
      return Status::OK();
  }

  Status FindColumn(const string& name, SQLSMALLINT column_count, SQLSMALLINT *column) {
    Status status = Status::OK();
    SQLCHAR column_name[1024];
    SQLSMALLINT name_len;
    SQLSMALLINT data_type;
    SQLULEN column_size;
    SQLSMALLINT decimal_digits;
    SQLSMALLINT nullable;

    *column = -1;
    for (SQLSMALLINT i = 1; status.ok() && (i <= column_count); i++) {
      status = ODBCStatus(SQLDescribeCol(sql_stmt_, i, 
        column_name, 1024, &name_len, &data_type, &column_size, &decimal_digits, &nullable));
      if (status.ok()) {
        if (name.compare((const char *)column_name) == 0) {
          *column = i;
          break;
        }
      }
    }
    return status;
  }
  
  Status IsColumnBinary(SQLSMALLINT column, bool &binary) {
    Status status = Status::OK();
    SQLCHAR column_name[1024];
    SQLSMALLINT name_len;
    SQLSMALLINT data_type;
    SQLULEN column_size;
    SQLSMALLINT decimal_digits;
    SQLSMALLINT nullable;

    binary = false;
    status = ODBCStatus(SQLDescribeCol(sql_stmt_, column,
      column_name, 1024, &name_len, &data_type, &column_size, &decimal_digits, &nullable));
    if (status.ok())
      binary = (data_type == SQL_BINARY);

    return status;
  }

  Status SetupColumns() {
    Status status = Status::OK();
    SQLSMALLINT column_count;
    status = ODBCStatus(SQLNumResultCols(sql_stmt_, &column_count));
    if (status.ok()) {
      if (column_count < 1) {
        status = Status(error::INTERNAL, "Query returns no columns");
      } else if (column_count == 1) {
        key_column_ = -1;
        value_column_ = 1;
      } else {
        FindColumn("key", column_count, &key_column_);
        if (key_column_ < 0)
          FindColumn("KEY", column_count, &key_column_);
        FindColumn("value", column_count, &value_column_);
        if (value_column_ < 0)
          FindColumn("VALUE", column_count, &value_column_);
        if ((key_column_ < 0) && (value_column_ < 0)) {
          key_column_ = 1;
          value_column_ = 2;
        } else if (value_column_ < 0) 
          value_column_ = 1;
        status = IsColumnBinary(value_column_, value_is_binary_);
      }
    }
    return status;
  }

  Status OnWorkStartedLocked() override {
    Status status = Status::OK();

    status = ODBCStatus(SQLAllocHandle(SQL_HANDLE_ENV, NULL, &sql_env_));
    if (status.ok()) {
      status = ODBCStatus(SQLSetEnvAttr(sql_env_, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0));
      if (status.ok()) {
        status = ODBCStatus(SQLAllocHandle(SQL_HANDLE_DBC, sql_env_, &sql_dbc_));
        if (status.ok()) {
          status = ODBCStatus(SQLDriverConnect(sql_dbc_, NULL, (SQLCHAR*)connection_string_.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE));
          if (status.ok()) {
            status = ODBCStatus(SQLAllocHandle(SQL_HANDLE_STMT, sql_dbc_, &sql_stmt_));
            if (status.ok()) {
              status = ODBCStatus(SQLExecDirect(sql_stmt_, (SQLCHAR*)sql_query_.c_str(), SQL_NTS));
              if (status.ok())
                status = SetupColumns();
            }
          }
        }
      }
    }
    if (!status.ok())
      OnWorkFinishedLocked();

    line_number_ = 0;
    return status;
  }

  Status OnWorkFinishedLocked() override {
    if (sql_stmt_ != NULL) {
      SQLFreeHandle(SQL_HANDLE_STMT, sql_stmt_);
      sql_stmt_ = NULL;
    }
    if (sql_dbc_ != NULL) {
      SQLFreeHandle(SQL_HANDLE_DBC, sql_dbc_);
      sql_dbc_ = NULL;
    }
    if (sql_env_ != NULL) {
      SQLFreeHandle(SQL_HANDLE_ENV, sql_env_);
      sql_env_ = NULL;
    }
    return Status::OK();
  }

  Status ReadValue(SQLSMALLINT column, SQLSMALLINT data_type, int buf_size, string* value) {
    Status status = Status::OK();
    SQLRETURN sql_return = SQL_SUCCESS;
    SQLLEN read_size;
    int value_pos = 0, value_size = 0;

    while (status.ok()) {
      if (value_pos + buf_size < value->size())
        value->resize(value_pos + buf_size, 0);
      sql_return = SQLGetData(sql_stmt_, column, data_type, 
        (SQLPOINTER)(value->c_str() + value_pos), buf_size, &read_size);
      if (sql_return == SQL_NO_DATA)
        break;
      else {
        status = ODBCStatus(sql_return);
        if (status.ok()) {
          value_pos += read_size;
          value_size += read_size;
        }
      }
    }

    if (status.ok()) 
      value->resize(value_size);

    return status;
  }

  Status ReadLocked(string* key, string* value, bool* produced,
                    bool* at_end) override {
    Status status = Status::OK();
    SQLRETURN sql_return = SQL_SUCCESS;

    sql_return = SQLFetch(sql_stmt_);
    if (sql_return == SQL_NO_DATA) {
      *produced = false;
      *at_end = true;
    } else {
      ++line_number_;
      status = ODBCStatus(sql_return);
      if (status.ok()) {
        if (key_column_ < 0)
          *key = strings::StrCat(current_work(), ":", line_number_);
        else
          status = ReadValue(key_column_, SQL_C_CHAR, 1024, key);
        if (status.ok()) 
          status = ReadValue(value_column_, value_is_binary_ ? SQL_C_BINARY : SQL_C_CHAR, 16 * 1024, value);
        if (status.ok())
          *produced = true;
      }
    }

	  return status;	
  }

  Status ResetLocked() override {
    line_number_ = 0;
    return ReaderBase::ResetLocked();
  }

 private:
  const string connection_string_;
  const string sql_query_;
  SQLHANDLE sql_env_;
  SQLHANDLE sql_dbc_;
  SQLHANDLE sql_stmt_;
  SQLSMALLINT key_column_;
  SQLSMALLINT value_column_;
  bool value_is_binary_;
  Env* const env_;
  int64 line_number_;
};

class ODBCReaderOp : public ReaderOpKernel {
 public:
  explicit ODBCReaderOp(OpKernelConstruction* context)
      : ReaderOpKernel(context) {
	  string connection_string, sql_query;
    //int skip_header_lines = -1;
    OP_REQUIRES_OK(context,
                   context->GetAttr("connection_string", &connection_string));
    OP_REQUIRES_OK(context,
                   context->GetAttr("sql_query", &sql_query));
    Env* env = context->env();
    SetReaderFactory([this, connection_string, sql_query, env]() {
      return new ODBCReader(name(), connection_string, sql_query, env);
    });
  }
};

REGISTER_KERNEL_BUILDER(Name("ODBCReader").Device(DEVICE_CPU),
                        ODBCReaderOp);

}  // namespace tensorflow

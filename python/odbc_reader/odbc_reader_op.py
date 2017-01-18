import tensorflow as tf

odbc_reader_module = tf.load_op_library('odbc_reader_op.so')
ODBCReader = odbc_reader_module.ODBCReader
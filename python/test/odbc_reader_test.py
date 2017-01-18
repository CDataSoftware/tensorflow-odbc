import tensorflow as tf

from platform import system
os = system()
if os == 'Windows':
  reader = tf.ODBCReader()
else:
  import odbc_readed as odbc
  reader = odbc.ODBCReader()

key, value = reader.read("DSN=TF_DSN;", "select * from tf_test;")

print(key)
print(value)
READERDIR=`pwd`

cd ..
if cd tensorflow; then git pull; else git clone https://github.com/tensorflow/tensorflow && cd tensorflow; fi
echo "The call to ./configure completely reinitializes the building envinonment; it is better for speed to comment it out"
./configure

bazel build -c opt //tensorflow/tools/pip_package:build_pip_package
mkdir -p ./tensorflow/contrib/cdata
cp -f $READERDIR/src/* ./tensorflow/contrib/cdata
bazel build -c opt //tensorflow/contrib/cdata:odbc_reader_op.so
mkdir -p $READERDIR/python/odbc_reader/bin
cp -f ./bazel-out/local-opt/bin/tensorflow/contrib/cdata/odbc_reader_op.so $READERDIR/python/odbc_reader/bin

load("//tensorflow:tensorflow.bzl", "tf_custom_op_library")

tf_custom_op_library(
    name = "odbc_reader_op.so",
    srcs = ["odbc_reader_op.cc"],
    deps = ["//tensorflow/core/kernels:reader_base"]
)

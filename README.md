# CData ODBC Reader for TensorFlow 

Welcome to the CData ODBC Reader for TensorFlow, maintained by [CData Software](http://www.cdata.com).

The ODBC Reader for [TensorFlow](https://www.tensorflow.org) allows users to connect the TensorFlow Machine Learning library with 
any SQL-92 compliant ODBC Driver.  The ODBC Reader is driver-agnostic and can retrieve data from any ODBC Driver source. This 
includes the commercial drivers available from [CData Software](http://www.cdata.com/odbc/), as well as open source and other 
standards-compliant third-party ODBC Drivers.

## Installation

On Linux and macOS users can utilize the ODBC Reader for TensorFlow in precompiled binary form. On Windows, currently, the ODBC Reader should be compiled together with TensorFlow itself, from the source code. The following are the instructions for compiling the 
source.

### Linux, macOS

#### Prerequisites

- A git client is needed to download the source code of TensorFlow.
- The unixodbc and unixodbc-dev packages must be installed in order to build the ODBC Reader for TensorFlow.
- The minimum requirements of the TensorFlow package for compilation (at least 4+GB of memory). Other prerequisites are listed on <https://www.tensorflow.org/get_started/os_setup#installing_from_sources>.  

The build.sh script will fetch or update TensorFlow from GitHub, recompile it, then compile the module with the reader for TensorFlow to use. It is recommended to compile the reader synchronously with TensorFlow, as new versions of TensorFlow can contain breaking changes. If you do not need to fetch or compile TensorFlow, you will need to modify the script accordingly.

Before running the script, ensure that you have the directories with the TensorFlow source code and the ODBC Reader next to each other. If you have a different directory layout, you can change the path to the ODBC Reader in the READERDIR variable. Also, ensure that you remove the relative paths.  

#### Installation of the compiled module:

The compiled .so module should be placed in {reader directory}/python/odbc_reader/bin directory (the build script does this for you after the module is built). After that, assuming that your current directory is the project root directory where this README file is located, execute these commands to install the Python module: 

`cd python
sudo pip install .`


### Windows

Use the following procedure to compile the ODBC Reader into TensorFlow:

 1. Download the TensorFlow source code here: <https://www.tensorflow.org/get_started/os_setup#installing_from_sources> 
 2. Copy {tensorflow-odbc}/src/odbc_reader_op.cc to {tensorflow}/tensorflow/core/user_ops/
 3. Compile the TensorFlow source code. The procedure is described in the following document: <https://github.com/tensorflow/tensorflow/blob/master/tensorflow/contrib/cmake/README.md>. 

## Usage

### Required Parameters

The ODBC Reader for TensorFlow requires two parameters to be set in order to be used: 

1. The ODBC connection string, e.g., "DSN=TF_Test;"
2. The SQL query, e.g., "select * from tf_test;"

### Working with Data in TensorFlow

All readers in TensorFlow return key=value pairs. The best way to ensure that you map the correct columns to keys and values is to use SQL aliases in requests, e.g., `key, value = reader.read("DSN=My CRM", "SELECT name AS key, photo AS value FROM personnel;")`. Here the `name` and `photo` columns become keys and values, respectively. 

The ODBC Reader for TensorFlow returns `key` as text and `value` as binary. Your code can then process the binary values as required by your business logic. 

The data returned by the ODBC Reader is mapped to key-value pairs according to the following scheme:

1. If a query returns named Key and Value columns, the content will be returned as `key` and `value` variables. 
2. If a query returns a single column, the key will be set to the row index and the value will be defined by the row values.  
3. If a query returns more than one column and no key and value columns have been named, the first column becomes `key` and the second column becomes `value`.


## License

Copyright 2017 [CData Software](http://www.cdata.com)

This software is released under the GPL 3.0 license (see LICENSE file for the license).  For commercial licensing, please contact CData Software for additional information. 

## Additional Information Sources

* [CData ODBC Reader for TensorFlow](http://www.cdata.com/labs/tensorflow/) project site on CData Labs.
* [CData Software Website](http://www.cdata.com/) containing free trials and support for CData Software's ODBC drivers.
* [Microsoft ODBC Documentation](https://msdn.microsoft.com/en-us/library/ms714177) containing the ODBC API Reference Guide.


















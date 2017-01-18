from setuptools import setup

setup(name='cdata_odbc_reader',
      version='1.0',
      description='Tensorflow ODBC reader by CData',
      url='https://github.com/CDataInc/tensorflow-odbc',
      author='CData Software, Inc.',
      author_email='sales@cdata.com',
      license='GPL',
      packages=['odbc_reader'],
      install_requires=[
          'tensorflow>=0.11',
      ],
      zip_safe=False)
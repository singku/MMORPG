pushd "%~dp0"
del as3 /s /q
mkdir as3
protoc --plugin=protoc-gen-as3=..\protoc-gen-as3-1.1.3-bin\protoc-gen-as3.bat --as3_out=as3 *.proto
del as3\initializer.as.inc /s /q
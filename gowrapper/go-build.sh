cd ../;
./build.sh;
cd gowrapper;
rm -rf ./data;
mkdir -p data;
# go mod init letus;
go build -o main.o main.go;

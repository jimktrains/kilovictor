CXX = clang
CXXFLAG = -std=c++1z -Wall -Wextra -Wpedantic -Wsign-conversion \
	  -Wold-style-cast -Wsign-promo -Wsign-promo -Wswitch-enum \
	  -Weffc++ -Wshadow -Wno-missing-braces -g \
	  -I /usr/local/include \
	  -L /usr/local/lib \
	  -lprotobuf -lstdc++ -lgrpc++

EXE=build/kilovictor.exe
CLIENT=build/kilovictor_client.exe

SERVER_HEADERS=src/DynHat.h src/message.pb.h src/message.grpc.h
CLIENT_HEADERS=src/message.pb.h src/message.grpc.h


build: ${EXE} ${CLIENT}
server: ${EXE}
client: ${CLIENT}
test: ${EXE} ${CLIENT}
	./${EXE} &
	sleep 1
	./${CLIENT}

${EXE}: build/main.o build/message.pb.o build/message.grpc.pb.o build/KVServer.o ${SERVER_HEADERS}
	${CXX} ${CXXFLAG} -o $@ $(filter-out ${SERVER_HEADERS},$^)

${CLIENT}: build/client.o build/message.pb.o build/message.grpc.pb.o  ${CLIENT_HEADERS}
	${CXX} ${CXXFLAG} -o $@ $(filter-out ${CLIENT_HEADERS},$^)

src/message.pb.cc: message.proto
	protoc message.proto --grpc_out=src --cpp_out=src --plugin=protoc-gen-grpc=`which grpc_cpp_plugin`

src/message.pb.h: message.proto
	protoc message.proto --grpc_out=src --cpp_out=src --plugin=protoc-gen-grpc=`which grpc_cpp_plugin`

src/message.grpc.cc: message.proto
	protoc message.proto --grpc_out=src --cpp_out=src --plugin=protoc-gen-grpc=`which grpc_cpp_plugin`

src/message.grpc.h: message.proto
	protoc message.proto --grpc_out=src --cpp_out=src --plugin=protoc-gen-grpc=`which grpc_cpp_plugin`

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $?

build/%.o: src/%.cc
	${CXX} ${CXXFLAG} -c -o $@ $?

clean:
	rm -f *.o ${EXE} ${CLIENT} *.s src/*.o build/*

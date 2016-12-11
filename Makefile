CXX = clang
CXXFLAG = -std=c++14 -Wall -Wextra -Wpedantic -Wsign-conversion \
	  -Wold-style-cast -Wsign-promo -Wsign-promo -Wswitch-enum \
	  -Weffc++ -Wshadow -Wno-missing-braces -g \
	  -I /usr/local/include \
	  -L /usr/local/lib \
	  -lprotobuf -lstdc++ -lgrpc++

EXE=kilovictor.exe

build: ${EXE}
test: ${EXE}
	./$?

${EXE}: src/main.o src/message.pb.o src/message.grpc.pb.o
	${CXX} ${CXXFLAG} -o $@ src/main.o src/message.pb.o src/message.grpc.pb.o

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $?

%.o: %.cc
	${CXX} ${CXXFLAG} -c -o $@ $?

clean:
	rm -f *.o {EXE} *.s

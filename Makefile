TARGET=dwarf-viewer
SRCS=			\
	main.cpp	\
	elf_parser.cpp		\
	dwarf.cpp

CFLAGS+=	-Wall -fPIC -O3	 -std=c++17

all:
	${CXX} ${CFLAGS} ${SRCS} ${LIBS} -o ${TARGET}

clean:
	rm -f ${TARGET} *.o

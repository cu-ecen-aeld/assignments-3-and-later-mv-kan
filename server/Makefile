SOURCES=main.c
CROSS_COMPILE=""
BINARY_FILE="aesdsocket"

default: $(SOURCES)
	${CROSS_COMPILE}gcc -c $(SOURCES)
	${CROSS_COMPILE}gcc -o ${BINARY_FILE} $(SOURCES:%.c=%.o)
debug: 
	${CROSS_COMPILE}gcc -ggdb3 -c $(SOURCES)
	${CROSS_COMPILE}gcc -ggdb3 -o ${BINARY_FILE} $(SOURCES:%.c=%.o)
all: default
clean:
	rm $(SOURCES:%.c=%.o)  ${BINARY_FILE} | true
SOURCES=writer.c
CROSS_COMPILE=""
BINARY_FILE="writer"
default: $(SOURCES)
	${CROSS_COMPILE}gcc -c $(SOURCES)
	${CROSS_COMPILE}gcc -o ${BINARY_FILE} $(SOURCES:%.c=%.o)
clean:
	rm $(SOURCES:%.c=%.o)  ${BINARY_FILE} | true
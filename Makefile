obj-m += grot.o

all:
	make -C linux M=grot modules

clean:
	make -C linux M=grot clean


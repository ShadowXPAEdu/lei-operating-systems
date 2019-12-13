DEPS = msgdist.h msgdist_c.h msgdist_s.h

all: server client verificador clean

server: msgdist_server.o msgdist_common.o
	gcc msgdist_server.o msgdist_common.o -lpthread -o msgdist_server

client: msgdist_client.o msgdist_common.o
	gcc msgdist_client.o msgdist_common.o -lpthread -lncurses -o msgdist_client

verificador: verificador.o
	gcc verificador.o -o verificador

clean:
	rm -f *.o

delexe:
	rm msgdist_server msgdist_client verificador


DEPS = msgdist.h msgdist_c.h msgdist_s.h

all: msgdist_client.o msgdist_common.o msgdist_server.o verificador.o
	gcc msgdist_client.o msgdist_common.o -o msgdist_client
	gcc msgdist_server.o msgdist_common.o -lpthread -o msgdist_server
	gcc verificador.o -o verificador
	rm -f *.o

server: msgdist_server.o msgdist_common.o
	gcc msgdist_server.o msgdist_common.o -lpthread -o msgdist_server
	rm -f *.o

client: msgdist_client.o msgdist_common.o
	gcc msgdist_client.o msgdist_common.o -o msgdist_client
	rm -f *.o

verificador: verificador.o
	gcc verificador.o -o verificador
	rm -f *.o

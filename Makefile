cc=gcc
headfile=/home/leirao/data-disk/plugin/

target:common.o
	$(cc) -shared -fPIC -o plugin.so plugin_client.c -I $(headfile) -g common.o
	$(cc) plugin_server.c -o plugin_server -I $(headfile) -g common.o
common.o:common.c
	$(cc) -c common.c -o common.o

clean:
	rm  -f *.so plugin_server *.o

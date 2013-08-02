DB2PATH=$(HOME)/sqllib

all: cudaudfsrv cudaudfcli

cudaudfsrv: cudaudfsrv.o
	g++ -m64 -shared -o cudaudfsrv cudaudfsrv.o -Wl,-rpath,$(DB2PATH)/lib64 -L$(DB2PATH)/lib64 -ldb2 -lpthread
	
cudaudfsrv.o: cudaudfsrv.C
	g++ -m64 -fpic -I/home/db2inst1/sqllib/include -c cudaudfsrv.C -D_REENTRANT

cudaudfcli: utilemb.sqC cudaudfcli.sqC
	./embprep utilemb
	./embprep cudaudfcli
	g++ -m64 -I$(DB2PATH)/include -c cudaudfcli.C
	g++ -Wl,-rpath,$(DB2PATH)/lib64 -I$(DB2PATH)/include -c utilemb.C
	g++ -Wl,-rpath,$(DB2PATH)/lib64 -o cudaudfcli cudaudfcli.o utilemb.o -Wl,-rpath,$(DB2PATH)/lib64 -L$(DB2PATH)/lib64 -ldb2

install: cudaudfsrv
	cp cudaudfsrv $(DB2PATH)/function
	
readcolordata: utilemb.sqC readcolordata.sqC
	./embprep utilemb kmeans
	./embprep readcolordata kmeans
	g++ -m64 -I$(DB2PATH)/include -c readcolordata.C
	g++ -Wl,-rpath,$(DB2PATH)/lib64 -I$(DB2PATH)/include -c utilemb.C
	g++ -Wl,-rpath,$(DB2PATH)/lib64 -o readcolordata readcolordata.o utilemb.o -Wl,-rpath,$(DB2PATH)/lib64 -L$(DB2PATH)/lib64 -ldb2	
	
clean:
	rm -f cudaudfcli.C readcolordata.C utilemb.C *.o *.bnd cudaudfsrv cudaudfcli readcolordata readcolordata_sample testcli
	
uninstall:
	rm -f $(DB2PATH)/function/cudaudfsrv
	
.PHONY=install
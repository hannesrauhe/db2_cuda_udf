DB2PATH=$(HOME)/sqllib

all: cudaudfsrv cudaudfcli readcolordata

cudaudfsrv: cudaudfsrv.o cuda_kmeans.o seq_kmeans.o
	#g++ -m64 -shared -o cudaudfsrv cudaudfsrv.o cuda_kmeans.o -Wl,-rpath,$(DB2PATH)/lib64 -L$(DB2PATH)/lib64 -ldb2 -lpthread
	/usr/local/cuda/bin/nvcc  -o cudaudfsrv cudaudfsrv.o cuda_kmeans.o seq_kmeans.o -L$(DB2PATH)/lib64 -ldb2 -lpthread --shared -m64 -Xlinker=-rpath,$(DB2PATH)/lib64
	
cudaudfsrv.o: cudaudfsrv.sqC
	./embprep cudaudfsrv kmeans
	g++ -m64 -fpic -I/home/db2inst1/sqllib/include -c cudaudfsrv.C -D_REENTRANT
	
seq_kmeans.o: seq_kmeans.c
	g++ -m64 -fpic -I/home/db2inst1/sqllib/include -c seq_kmeans.c
	
cuda_kmeans.o: cuda_kmeans.cu
	/usr/local/cuda/bin/nvcc -c -Xcompiler -fpic cuda_kmeans.cu

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
	g++ -m64 -I$(DB2PATH)/include -c readcolordata.C -Wno-write-strings
	g++ -Wl,-rpath,$(DB2PATH)/lib64 -I$(DB2PATH)/include -c utilemb.C -Wno-write-strings
	g++ -Wl,-rpath,$(DB2PATH)/lib64 -o readcolordata readcolordata.o utilemb.o -Wl,-rpath,$(DB2PATH)/lib64 -L$(DB2PATH)/lib64 -ldb2	
	
clean:
	rm -f cudaudfcli.C readcolordata.C utilemb.C cudaudfsrv.C *.o *.bnd cudaudfsrv cudaudfcli readcolordata readcolordata_sample testcli
	
uninstall:
	rm -f $(DB2PATH)/function/cudaudfsrv
	
.PHONY=install
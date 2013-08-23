DB2PATH=$(HOME)/sqllib

all: cudaudfsrv readcolordata

cudaudfsrv: cudaudfsrv.o cuda_kmeans.o cpu_kmeans.o
	#g++ -m64 -shared -o cudaudfsrv cudaudfsrv.o cuda_kmeans.o -Wl,-rpath,$(DB2PATH)/lib64 -L$(DB2PATH)/lib64 -ldb2 -lpthread
	/usr/local/cuda/bin/nvcc  -o cudaudfsrv cudaudfsrv.o cuda_kmeans.o cpu_kmeans.o -L/usr/lib/gcc/i686-linux-gnu/4.4/ -L$(DB2PATH)/lib64 -ldb2 -lpthread -lgomp --shared -m64 -Xlinker=-rpath,$(DB2PATH)/lib64
	
cudaudfsrv.o: cudaudfsrv.sqC
	./embprep cudaudfsrv kmeans
	g++ -m64 -fpic -I/home/db2inst1/sqllib/include -c cudaudfsrv.C -D_REENTRANT -O3
	
cpu_kmeans.o: cpu_kmeans.c
	g++ -m64 -fpic -c cpu_kmeans.c  -O3 -fopenmp
	
cuda_kmeans.o: cuda_kmeans.cu
	/usr/local/cuda/bin/nvcc -c -Xcompiler -fpic cuda_kmeans.cu -DBLOCK_SHARED_MEM_OPTIMIZATION=1 -O3
	
readcolordata: utilemb.sqC readcolordata.sqC
	./embprep utilemb kmeans
	./embprep readcolordata kmeans
	g++ -g -m64 -I$(DB2PATH)/include -c readcolordata.C -Wno-write-strings
	g++ -Wl,-rpath,$(DB2PATH)/lib64 -I$(DB2PATH)/include -c utilemb.C -Wno-write-strings
	g++ -Wl,-rpath,$(DB2PATH)/lib64 -o readcolordata readcolordata.o utilemb.o -Wl,-rpath,$(DB2PATH)/lib64 -L$(DB2PATH)/lib64 -ldb2	
	
clean:
	rm -f cudaudfcli.C readcolordata.C utilemb.C cudaudfsrv.C *.o *.bnd cudaudfsrv  readcolordata

install: cudaudfsrv
	cp cudaudfsrv $(DB2PATH)/function
	
uninstall:
	rm -f $(DB2PATH)/function/cudaudfsrv
	
.PHONY=install

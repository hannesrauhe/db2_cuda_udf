select * from TABLE(KMEANSCOLORSTATICUDF(2,'CPU'));
select * from TABLE(KMEANSCOLORUDF(2,'CPU','COLORS17695'));
--select * from TABLE(KMEANSCOLORUDF(16,'GPU','CSV:/home/db2inst1/workspace/db2CudaUDF/color17695.csv'));
--select * from TABLE(KMEANSCOLORUDF(16,'GPU','COLORS17695'));
select * from TABLE(KMEANSCOLORUDF(16,'CPU','BIN:/home/db2inst1/workspace/db2CudaUDF/color17695.bin'));
select * from TABLE(KMEANSCOLORUDF(16,'CPU','CSV:/home/db2inst1/workspace/db2CudaUDF/color17695.csv'));
select * from TABLE(KMEANSCOLORUDF(16,'CPU','COLORS17695'));
select * from TABLE(KMEANSCOLORSTATICUDF(16,'CPU'));

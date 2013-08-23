select * from TABLE(KMEANSCOLORUDF(12,'CPU','CSV:/home/db2inst1/workspace/db2CudaUDF/color17695.csv'));
select * from TABLE(KMEANSCOLORUDF(12,'OMP','CSV:/home/db2inst1/workspace/db2CudaUDF/color17695.csv'));

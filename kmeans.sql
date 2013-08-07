select * from TABLE(KMEANSCOLORUDF(2,'CPU'));
select * from TABLE(KMEANSCOLORUDF(2,'GPU'));

select * from TABLE(KMEANSCOLORUDF(4,'GPU'));
select * from TABLE(KMEANSCOLORUDF(4,'CPU'));

select * from TABLE(KMEANSCOLORUDF(8,'CPU'));
select * from TABLE(KMEANSCOLORUDF(8,'GPU'));

select * from TABLE(KMEANSCOLORUDF(16,'GPU'));
select * from TABLE(KMEANSCOLORUDF(16,'CPU'));


#!/usr/bin/python
import subprocess,os

sql_file = "kmeans_gen.sql"
result_file = "result_gen.txt"
bin_dir = "/home/db2inst1/workspace/db2CudaUDF/"
wrk_dir = os.getcwd()+"/"

sqlf = open(sql_file,"w")
q_list = []
for tables in [100,1000]:
  subprocess.call([bin_dir+"/readcolordata","-gen=%d"%tables,"-files"])
  for dev in ['GPU','CPU']:
    for numClusters in [2,4,8,16]:
      inputs = ["COLORS%d"%tables,"BIN:%sgen_%d.bin"%(wrk_dir,tables),"CSV:%sgen_%d.csv"%(wrk_dir,tables)]
      for inputtab in inputs:
        q_list.append([dev,tables,numClusters,inputtab[0:3]])
        sqlf.write("\
--#BGBLK 3\n\
select * from TABLE(KMEANSCOLORUDF(%d,'%s','%s'));\n\
--#EOBLK\n"%(numClusters,dev,inputtab))

sqlf.close()
subprocess.call(["db2batch", "-f", sql_file,"-d","kmeans","-r",result_file])

rf = open(result_file,"r")
r = rf.readlines()[-9-len(q_list):-9]
rf.close()
for q,resultl in zip(q_list,r):
  q.append(resultl.split()[6])
#  print ",".join(str(x) for x in q)+","+str(resultl.split()[6])

print q_list

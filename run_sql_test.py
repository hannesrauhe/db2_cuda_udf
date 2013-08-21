#!/usr/bin/python
import subprocess,os,sys,pprint

import argparse



def exec_statements(binf_path,csvf_path,table_name,result_csvf,num_iterations = 3,tables = 999999):    
    sql_file = "kmeans_gen.sql"
    result_txt = "result_gen.txt"
    q_list = []
    devices = ['GPU','CPU'] 
    
    print("Sanity check: comparing results for BIN and DB2 input for size %d"%tables)
    proc = subprocess.Popen(["db2", "select * from TABLE(KMEANSCOLORUDF(2,'GPU','BIN:%s')) except select * from TABLE(KMEANSCOLORUDF(2,'GPU','%s'))"%(binf_path,table_name)], stdout=subprocess.PIPE)
    num_diff_lines = proc.stdout.readlines()[-2].strip()
    print("select * from TABLE(KMEANSCOLORUDF(2,'GPU','BIN:%s')) except select * from TABLE(KMEANSCOLORUDF(2,'GPU','%s'))"%(binf_path,table_name))
    print(num_diff_lines)
    if num_diff_lines[0]!="0":
        raise Exception("Sanity check failed for size %d."%tables)
    
    
    print("Generating SQL-statements for size %d"%tables)
    sqlf = open(sql_file,"w")
    for dev in devices:
        for numClusters in [2,4,8,16]:            
            inputs = [table_name,"BIN:"+binf_path]
            if len(csvf_path):
                inputs = [table_name,"BIN:"+binf_path,"CSV:"+csvf_path]
            for inputtab in inputs:
                q_list.append([dev,tables,numClusters,inputtab[0:3]])
                sqlf.write("\
--#COMMENT select * from TABLE(KMEANSCOLORUDF(%d,'%s','%s'));\n\
--#BGBLK %d\n\
select * from TABLE(KMEANSCOLORUDF(%d,'%s','%s'));\n\
--#EOBLK\n"%(numClusters,dev,inputtab,num_iterations,numClusters,dev,inputtab))

    sqlf.close()
    
    print("Executing SQL Statements for size %d"%tables)
    subprocess.call(["db2batch", "-f", sql_file,"-d","kmeans","-r",result_txt])
    
    rf = open(result_txt,"r")
    #read x lines from result.txt where x = number of executed queries 
    r = rf.readlines()[-9-len(q_list):-9]
    rf.close()
    #split every read line and extract only the average run-time
    #append this to the query list 
    for q,resultl in zip(q_list,r):
      q.append(resultl.split()[4])
      q.append(resultl.split()[5])
      q.append(resultl.split()[6])
    
    #write to csv
    for q_r in q_list:
        result_csvf.write(",".join(str(x) for x in q_r))
        result_csvf.write("\n")
    result_csvf.flush()

result_csv = "result_gen.csv"
bin_dir = sys.path[0]
wrk_dir = os.getcwd()+"/"
num_iterations = 3
binf_path = ''
table_name = 'COLORS'
#force_generate = False
#
parser = argparse.ArgumentParser(description='Generate test data; save it in binary, CSV, and in DB2; Execute kmeans UDF on it; save performance results in CSV')
#parser.add_argument('--gen', dest='force_generate',action='store_true', help='generate new data')
parser.add_argument('--bin-file', dest='binf_path', action='store', help='execute kmeans on input given by binary file instead of generated data ')
args = parser.parse_args()

if os.path.exists(args.binf_path):
    binf_path = os.path.abspath(args.binf_path)
else:
    print("bin file %s does not exists"%args.binf_path)
    exit(1)


subprocess.call(["db2", "connect to kmeans"])
result_csvf = open(result_csv,"w")
result_csvf.write("DEV,INPUT_SIZE,CLUSTER_SIZE,INPUT_TYPE,EXECUTION_TIME_MIN,EXECUTION_TIME_MAX,EXECUTION_TIME_AVG\n")

try:
    if len(binf_path):
        #subprocess.call([bin_dir+"/readcolordata",binf_path,"-table-name=%s"%table_name])
        proc = subprocess.Popen(["db2", "select count(*) from %s"%(table_name)], stdout=subprocess.PIPE)
        table_s = int(proc.stdout.readlines()[3].strip())
        exec_statements(binf_path,"",table_name,result_csvf,num_iterations,table_s)
    else:    
        for tables in [100]:
            print("Generating needed data for size %d"%tables)
            csvf_path = "%sgen_%d.csv"%(wrk_dir,tables)
            binf_path = "%sgen_%d.bin"%(wrk_dir,tables)
            subprocess.call([bin_dir+"/readcolordata","-gen=%d"%tables,"-files","-table-name=COLORS"])
            
            exec_statements(binf_path,csvf_path,table_name,result_csvf,num_iterations)        
except:
    raise
finally:
    result_csvf.close()
    print("Results written to "+result_csv)
subprocess.call(["db2", "connect reset"])

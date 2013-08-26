#!/usr/bin/python
import subprocess,os,sys,pprint,copy

import argparse

def cmp_file_table():
    cmp_stmt = "select * from table(CMPFILETABLE('%sgen_%d.bin','COLORS'))"%(wrk_dir,table_size)
    print(cmp_stmt)
    proc = subprocess.Popen(["db2", cmp_stmt], stdout=subprocess.PIPE)
    num_diff_lines = proc.stdout.readlines()[-2].strip()
    
    if num_diff_lines[0]!="0":
        raise Exception("Sanity check failed for size %d."%table_size)

def cmp_kmeans_results(table_size,table_name):
    print("Sanity check: comparing results for BIN and DB2 input for size %d"%table_size)
    cast_stmt_part = "cast(c1 as decimal(5,3)),cast(c2 as decimal(5,3)),cast(c3 as decimal(5,3)),cast(c4 as decimal(5,3)),cast(c5 as decimal(5,3)),\
            cast(c6 as decimal(5,3)),cast(c7 as decimal(5,3)),cast(c8 as decimal(5,3)),cast(c9 as decimal(5,3))"
    bin_stmt = "select %s from TABLE(KMEANSCOLORUDF(2,'GPU','BIN:%s'))"%(cast_stmt_part,binf_path)
    db_stmt = "select %s from TABLE(KMEANSCOLORUDF(2,'GPU','%s'))"%(cast_stmt_part,table_name)
    cmp_stmt = bin_stmt+"\nexcept \n"+db_stmt
    print(cmp_stmt)
    proc = subprocess.Popen(["db2", cmp_stmt], stdout=subprocess.PIPE)
    num_diff_lines = proc.stdout.readlines()[-2].strip()
    
    if num_diff_lines[0]!="0":
        subprocess.call(["db2", bin_stmt])
        subprocess.call(["db2", db_stmt])
        raise Exception("Sanity check failed for size %d."%table_size)

def exec_statements(binf_path="",csvf_path="",table_name="",result_csvf="gen_results.csv",table_size=0,num_iterations = 3, sanity_check=True, devices = ['CPU','OMP','GPU'],buffer_size=0):    
    sql_file = "gen_kmeans.sql"
    result_txt = "gen_results.txt"
    q_list = [] 
    
    print("Generating SQL-statements for size %d and buffer %d"%(table_size,buffer_size))
    sqlf = open(sql_file,"w")
    for dev in devices:
        for numClusters in [2,4,8,16,32,64,128]: 
            if len(binf_path):
                q_list.append([dev,table_size,numClusters,'BIN'])
                sqlf.write("\
--#BGBLK %d\n\
select count(*) from TABLE(KMEANSCOLORUDF(%d,'%s','BIN:%s'));\n\
--#EOBLK\n"%(num_iterations,numClusters,dev,binf_path))
                       
            if len(csvf_path):
                q_list.append([dev,table_size,numClusters,'CSV'])
                sqlf.write("\
--#BGBLK %d\n\
select count(*) from TABLE(KMEANSCOLORUDF(%d,'%s','CSV:%s'));\n\
--#EOBLK\n"%(num_iterations,numClusters,dev,csvf_path))
                
            if len(table_name):
                q_list.append([dev,table_size,numClusters,'COL'])
                sqlf.write("\
--#BGBLK %d\n\
select count(*) from TABLE(KMEANSCOLORUDF(%d,'%s','%s'));\n\
--#EOBLK\n"%(num_iterations,numClusters,dev,table_name))
                
    sqlf.close()
        
    if 0==len(q_list):
        print "Nothing to do, give at least csv path, bin path or table name"
        return
    
    print("Executing SQL Statements for size %d, bufferpool size %d"%(table_size,buffer_size))

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
        result_csvf.write(",%d\n"%buffer_size)
    result_csvf.flush()

result_csv = "gen_results.csv"
bin_dir = sys.path[0]
wrk_dir = os.getcwd()+"/"
num_iterations = 3
binf_path = ''
table_name = 'COLORS'
sanity = True
no_generate = False
table_sizes = [10000,20000]
devs = ["CPU","GPU"]

parser = argparse.ArgumentParser(description='Generate test data; save it in binary, CSV, and in DB2; Execute kmeans UDF on it; save performance results in CSV')
parser.add_argument('--no-gen', dest='no_generate',action='store_true', help='do not generate new data')
parser.add_argument('--bin-file', dest='binf_path', action='store', help='execute kmeans on input given by binary file instead of generated data ')
parser.add_argument('--no-sanity', dest='no_sanity', action='store_true', help='do not check results')
parser.add_argument('--input-sizes', nargs="*", type=int, help='do tests on table with INPUT_SIZE records')
#parser.add_argument('--append',action='store_true', help='append results to CSV file instead of replacing it')
args = parser.parse_args()

if args.binf_path:
    if os.path.exists(args.binf_path):
        binf_path = os.path.abspath(args.binf_path)
    else:
        print("bin file %s does not exists"%args.binf_path)
        exit(1)
        
if args.no_sanity:
    sanity=False
if args.no_generate:
    no_generate = True
if args.input_sizes:
    table_sizes=args.input_sizes

subprocess.call(["db2", "connect to kmeans"])
result_csvf = open(result_csv,"w")
result_csvf.write("DEV,INPUT_SIZE,CLUSTER_SIZE,INPUT_TYPE,EXECUTION_TIME_MIN,EXECUTION_TIME_MAX,EXECUTION_TIME_AVG,BUFFERPOOL\n")

try:
    if len(binf_path):
        subprocess.call([bin_dir+"/readcolordata",binf_path,"-table-name=%s"%table_name])
        proc = subprocess.Popen(["db2", "select count(*) from %s"%(table_name)], stdout=subprocess.PIPE)
        table_size = int(proc.stdout.readlines()[3].strip())
        csvf_path=binf_path.replace(".bin", ".csv")
        if not os.path.exists(csvf_path):
            print("No CSV timing: %s does not exist"%csvf_path)
            csvf_path = ""     
    
        exec_statements(binf_path,csvf_path,"",result_csvf,table_size,devices=devs,buffer_size=0)
        for buffers in [10,100,1000,10000]:
            subprocess.call(["db2","alter bufferpool bp10 size %d"%buffers])
            exec_statements("","",table_name,result_csvf,table_size,devices=devs,buffer_size=buffers)
        
    else:    
        for table_size in table_sizes:
            csvf_path = "%sgen_%d.csv"%(wrk_dir,table_size)
            binf_path = "%sgen_%d.bin"%(wrk_dir,table_size)
            
            if not no_generate:
                print("Generating needed data for size %d"%table_size)
                subprocess.call([bin_dir+"/readcolordata","-gen=%d"%table_size,"-files","-table-name=COLORS"])
                
            exec_statements(binf_path,csvf_path,"",result_csvf,table_size,devices=devs,buffer_size=0)
            for buffers in [10,100,1000,10000]:
                subprocess.call(["db2","alter bufferpool bp10 size %d"%buffers])
                exec_statements("","",table_name,result_csvf,table_size,devices=devs,buffer_size=buffers)        
except:
    raise
finally:
    result_csvf.close()
    print("Results written to "+result_csv)
subprocess.call(["db2", "connect reset"])

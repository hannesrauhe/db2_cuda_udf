#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlca.h>
#include <sqludf.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>     /* read(), close() */


float** file_read(int   isBinaryFile,  /* flag: 0 or 1 */
                  char *filename,      /* input file name */
                  int  *numObjs,       /* no. data objects (local) */
                  int  *numCoords)     /* no. coordinates */;
float** cuda_kmeans(float **objects,      /* in: [numObjs][numCoords] */
                   int     numCoords,    /* no. features */
                   int     numObjs,      /* no. objects */
                   int     numClusters,  /* no. clusters */
                   float   threshold,    /* % objects change membership */
                   int    *membership,   /* out: [numObjs] */
                   int    *loop_iterations);

/*----< seq_kmeans() >-------------------------------------------------------*/
/* return an array of cluster centers of size [numClusters][numCoords]       */
float** seq_kmeans(float **objects,      /* in: [numObjs][numCoords] */
                   int     numCoords,    /* no. features */
                   int     numObjs,      /* no. objects */
                   int     numClusters,  /* no. clusters */
                   float   threshold,    /* % objects change membership */
                   int    *membership,   /* out: [numObjs] */
                   int    *loop_iterations);

struct kmeans_scratch
{
  float** clusters;
  int* membership;
  int numClusters, numObjs, numCoords;
  int result_pos;
};


/*
DROP Function KMEANSCOLORUDF;

CREATE FUNCTION KMEANSCOLORUDF(NUMCLUSTERS SMALLINT, DEVICE CHAR(3))
RETURNS TABLE(C1 DOUBLE, C2 DOUBLE, C3 DOUBLE, C4 DOUBLE, C5 DOUBLE, C6 DOUBLE, C7 DOUBLE, C8 DOUBLE, C9 DOUBLE)
EXTERNAL NAME 'cudaudfsrv!kmeansColorUDF'
DETERMINISTIC
NO EXTERNAL ACTION
FENCED
NOT NULL CALL
LANGUAGE C
PARAMETER STYLE DB2SQL
SCRATCHPAD
FINAL CALL
DISALLOW PARALLEL
DBINFO

select * from TABLE(KMEANSCOLORUDF(8,'CPU'));
 */
#ifdef __cplusplus
extern "C"
#endif
void SQL_API_FN kmeansColorUDF(// Return row fields
						 //SQLUDF_LOCATOR *inColorTable,
						 SQLUDF_SMALLINT *numClusters,
						 SQLUDF_CHAR *device,
						 //out:
                         SQLUDF_DOUBLE *C1,
                         SQLUDF_DOUBLE *C2,
                         SQLUDF_DOUBLE *C3,
                         SQLUDF_DOUBLE *C4,
                         SQLUDF_DOUBLE *C5,
                         SQLUDF_DOUBLE *C6,
                         SQLUDF_DOUBLE *C7,
                         SQLUDF_DOUBLE *C8,
                         SQLUDF_DOUBLE *C9,
                         // Return row field null indicators
                         SQLUDF_SMALLINT *ColorNullInd,
                         SQLUDF_SMALLINT *deviceNullInd,
                         SQLUDF_SMALLINT *C1NullInd,
                         SQLUDF_SMALLINT *C2NullInd,
                         SQLUDF_SMALLINT *C3NullInd,
                         SQLUDF_SMALLINT *C4NullInd,
                         SQLUDF_SMALLINT *C5NullInd,
                         SQLUDF_SMALLINT *C6NullInd,
                         SQLUDF_SMALLINT *C7NullInd,
                         SQLUDF_SMALLINT *C8NullInd,
                         SQLUDF_SMALLINT *C9NullInd,
                         SQLUDF_TRAIL_ARGS_ALL)
{
  struct kmeans_scratch *pScratArea;
  pScratArea = (struct kmeans_scratch *)SQLUDF_SCRAT->data;
//  const char* device = "CPU";

  // SQLUDF_CALLT, SQLUDF_SCRAT, SQLUDF_STATE and SQLUDF_MSGTX are
  // parts of SQLUDF_TRAIL_ARGS_ALL
  switch (SQLUDF_CALLT)
  {
    case SQLUDF_TF_OPEN:
    {
    	int     loop_iterations = 0;
    	float   threshold = 0.001;
    	float **objects;
		struct sqlca sqlca;

		pScratArea->numClusters = *numClusters;

		objects = file_read(1, "/home/db2inst1/workspace/db2CudaUDF/color17695.bin", &(pScratArea->numObjs), &(pScratArea->numCoords));
		if(objects==NULL || objects[0]==NULL) {
			strcpy(SQLUDF_STATE, "38999");
			strcpy(SQLUDF_MSGTX, "OPENING FILE ERROR");
		} else {
			pScratArea->membership = (int*) malloc(pScratArea->numObjs * sizeof(int));

			if(strcmp(device,"GPU") == 0) {
				pScratArea->clusters = cuda_kmeans(objects, pScratArea->numCoords, pScratArea->numObjs, pScratArea->numClusters, threshold,
								pScratArea->membership, &loop_iterations);
//				pScratArea->clusters[0][0] = cuda_test();
			} else {
				pScratArea->clusters = seq_kmeans(objects, pScratArea->numCoords, pScratArea->numObjs, pScratArea->numClusters, threshold,
						pScratArea->membership, &loop_iterations);
			}

			free(objects[0]);
			free(objects);
			if(pScratArea->clusters==NULL) {
				strcpy(SQLUDF_STATE, "38998");
				strcpy(SQLUDF_MSGTX, "KMEANS ERROR");
			}
		}
		/* Error Handling
		if(numCoords!=9) {
			printf("Wrong number of coords in file\n");
			return -1;
		}
		*/
		pScratArea->result_pos = 0;
		break;
    }
    case SQLUDF_TF_FETCH:
      if (pScratArea->result_pos >= pScratArea->numClusters)
      {
        // SQLUDF_STATE is part of SQLUDF_TRAIL_ARGS_ALL
        strcpy(SQLUDF_STATE, "02000");
        break;
      }
      // Normal call UDF: Fetch next row
      *C1 = (double)pScratArea->clusters[pScratArea->result_pos][0];
      *C2 = (double)pScratArea->clusters[pScratArea->result_pos][1];
      *C3 = (double)pScratArea->clusters[pScratArea->result_pos][2];
      *C4 = (double)pScratArea->clusters[pScratArea->result_pos][3];
      *C5 = (double)pScratArea->clusters[pScratArea->result_pos][4];
      *C6 = (double)pScratArea->clusters[pScratArea->result_pos][5];
      *C7 = (double)pScratArea->clusters[pScratArea->result_pos][6];
      *C8 = (double)pScratArea->clusters[pScratArea->result_pos][7];
      *C9 = (double)pScratArea->clusters[pScratArea->result_pos][8];
/*
    	if(pScratArea->result_pos>=1) {
    	      strcpy(SQLUDF_STATE, "02000");
    		break;
    	}
        *C1 = pScratArea->numObjs;
        *C2 = pScratArea->numClusters;
        *C3 = pScratArea->numCoords;
        *C4 = 0;
        *C5 =0;
        *C6 = 0;
        *C7 = 0;
        *C8 = 0;
        *C9 = 0;*/
      // Next row of data
      pScratArea->result_pos++;
      strcpy(SQLUDF_STATE, "00000");
      break;
    case SQLUDF_TF_CLOSE:
        free(pScratArea->membership);
        free(pScratArea->clusters[0]);
        free(pScratArea->clusters);
      break;
    case SQLUDF_TF_FINAL:
      break;
  }
} //kmeansColorUDF

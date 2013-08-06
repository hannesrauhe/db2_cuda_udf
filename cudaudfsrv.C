#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlca.h>
#include <sqludf.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>     /* read(), close() */

  #define PATH_SEP "/"


#ifdef __cplusplus
extern "C"
#endif
void SQL_API_FN ScalarCUDAUDF(SQLUDF_CHAR *inJob,
                          SQLUDF_DOUBLE *inSalary,
                          SQLUDF_DOUBLE *outNewSalary,
                          SQLUDF_SMALLINT *jobNullInd,
                          SQLUDF_SMALLINT *salaryNullInd,
                          SQLUDF_SMALLINT *newSalaryNullInd,
                          SQLUDF_TRAIL_ARGS)
{
  if (*jobNullInd == -1 || *salaryNullInd == -1)
  {
    *newSalaryNullInd = -1;
  }
  else
  {
    if (strcmp(inJob, "Mgr  ") == 0)
    {
      *outNewSalary = *inSalary * 3.20;
    }
    else if (strcmp(inJob, "Sales") == 0)
    {
      *outNewSalary = *inSalary * 0.10;
    }
    else // it is a clerk
    {
      *outNewSalary = *inSalary * 0.05;
    }
    *newSalaryNullInd = 0;
  }
} //ScalarUDF







// Scratchpad data structure
struct scratch_area
{
  int file_pos;
};

struct person
{
  const char *name;
  const char *job;
  const char *salary;
};

// Following is the data buffer for this example.
// You may keep the data in a separate text file.
// See "Application Development Guide" on how to work with
// a data file instead of a data buffer.
struct person staff[] =
{
  {"Hannes", "Mgr", "17300.00"},
  {"Wagland", "Sales", "15000.00"},
  {"Davis", "Clerk", "10000.00"},
  // Do not forget a null terminator
  {(const char *)0, (const char *)0, (const char *)0}
};


#ifdef __cplusplus
extern "C"
#endif
void SQL_API_FN TableTestUDF(// Return row fields
                         SQLUDF_DOUBLE *inSalaryFactor,
                         SQLUDF_CHAR *outName,
                         SQLUDF_CHAR *outJob, SQLUDF_DOUBLE *outSalary,
                         // Return row field null indicators
                         SQLUDF_SMALLINT *salaryFactorNullInd,
                         SQLUDF_SMALLINT *nameNullInd,
                         SQLUDF_SMALLINT *jobNullInd,
                         SQLUDF_SMALLINT *salaryNullInd,
                         SQLUDF_TRAIL_ARGS_ALL)
{
  struct scratch_area *pScratArea;
  pScratArea = (struct scratch_area *)SQLUDF_SCRAT->data;

  // SQLUDF_CALLT, SQLUDF_SCRAT, SQLUDF_STATE and SQLUDF_MSGTX are
  // parts of SQLUDF_TRAIL_ARGS_ALL
  switch (SQLUDF_CALLT)
  {
    case SQLUDF_TF_OPEN:
      pScratArea->file_pos = 0;
      break;
    case SQLUDF_TF_FETCH:
      // Normal call UDF: Fetch next row
      if (staff[pScratArea->file_pos].name == (char *)0)
      {
        // SQLUDF_STATE is part of SQLUDF_TRAIL_ARGS_ALL
        strcpy(SQLUDF_STATE, "02000");
        break;
      }
      strcpy(outName, staff[pScratArea->file_pos].name);
      strcpy(outJob, staff[pScratArea->file_pos].job);
      *nameNullInd = 0;
      *jobNullInd = 0;

      if (staff[pScratArea->file_pos].salary != (char *)0)
      {
        *outSalary = (*inSalaryFactor) *
          atof(staff[pScratArea->file_pos].salary);
        *salaryNullInd = 0;
      }

      // Next row of data
      pScratArea->file_pos++;
      break;
    case SQLUDF_TF_CLOSE:
      break;
    case SQLUDF_TF_FINAL:
      // close the file
      pScratArea->file_pos = 0;
      break;
  }
} //CUDATableUDF



/*** kmeans ***/

float** file_read(int   isBinaryFile,  /* flag: 0 or 1 */
                  char *filename,      /* input file name */
                  int  *numObjs,       /* no. data objects (local) */
                  int  *numCoords)     /* no. coordinates */
{
    float **objects;
    int     i, j, len;
    ssize_t numBytesRead;

    if (isBinaryFile) {  /* input file is in raw binary format -------------*/
        int infile;
        if ((infile = open(filename, O_RDONLY, "0600")) == -1) {
            fprintf(stderr, "Error: no such file (%s)\n", filename);
            return NULL;
        }
        numBytesRead = read(infile, numObjs,    sizeof(int));
//        assert(numBytesRead == sizeof(int));
        numBytesRead = read(infile, numCoords, sizeof(int));
//        assert(numBytesRead == sizeof(int));

        /* allocate space for objects[][] and read all objects */
        len = (*numObjs) * (*numCoords);
        objects    = (float**)malloc((*numObjs) * sizeof(float*));
//        assert(objects != NULL);
        objects[0] = (float*) malloc(len * sizeof(float));
//        assert(objects[0] != NULL);
        for (i=1; i<(*numObjs); i++)
            objects[i] = objects[i-1] + (*numCoords);

        numBytesRead = read(infile, objects[0], len*sizeof(float));
//        assert(numBytesRead == len*sizeof(float));

        close(infile);
    }

    return objects;
}

/*----< euclid_dist_2() >----------------------------------------------------*/
/* square of Euclid distance between two multi-dimensional points            */
__inline static
float euclid_dist_2(int    numdims,  /* no. dimensions */
                    float *coord1,   /* [numdims] */
                    float *coord2)   /* [numdims] */
{
    int i;
    float ans=0.0;

    for (i=0; i<numdims; i++)
        ans += (coord1[i]-coord2[i]) * (coord1[i]-coord2[i]);

    return(ans);
}

/*----< find_nearest_cluster() >---------------------------------------------*/
__inline static
int find_nearest_cluster(int     numClusters, /* no. clusters */
                         int     numCoords,   /* no. coordinates */
                         float  *object,      /* [numCoords] */
                         float **clusters)    /* [numClusters][numCoords] */
{
    int   index, i;
    float dist, min_dist;

    /* find the cluster id that has min distance to object */
    index    = 0;
    min_dist = euclid_dist_2(numCoords, object, clusters[0]);

    for (i=1; i<numClusters; i++) {
        dist = euclid_dist_2(numCoords, object, clusters[i]);
        /* no need square root */
        if (dist < min_dist) { /* find the min and its array index */
            min_dist = dist;
            index    = i;
        }
    }
    return(index);
}

/*----< seq_kmeans() >-------------------------------------------------------*/
/* return an array of cluster centers of size [numClusters][numCoords]       */
float** seq_kmeans(float **objects,      /* in: [numObjs][numCoords] */
                   int     numCoords,    /* no. features */
                   int     numObjs,      /* no. objects */
                   int     numClusters,  /* no. clusters */
                   float   threshold,    /* % objects change membership */
                   int    *membership,   /* out: [numObjs] */
                   int    *loop_iterations)
{
    int      i, j, index, loop=0;
    int     *newClusterSize; /* [numClusters]: no. objects assigned in each
                                new cluster */
    float    delta;          /* % of objects change their clusters */
    float  **clusters;       /* out: [numClusters][numCoords] */
    float  **newClusters;    /* [numClusters][numCoords] */

    /* allocate a 2D space for returning variable clusters[] (coordinates
       of cluster centers) */
    clusters    = (float**) malloc(numClusters *             sizeof(float*));
    clusters[0] = (float*)  malloc(numClusters * numCoords * sizeof(float));
    for (i=1; i<numClusters; i++)
        clusters[i] = clusters[i-1] + numCoords;

    /* pick first numClusters elements of objects[] as initial cluster centers*/
    for (i=0; i<numClusters; i++)
        for (j=0; j<numCoords; j++)
            clusters[i][j] = objects[i][j];

    /* initialize membership[] */
    for (i=0; i<numObjs; i++) membership[i] = -1;

    /* need to initialize newClusterSize and newClusters[0] to all 0 */
    newClusterSize = (int*) calloc(numClusters, sizeof(int));

    newClusters    = (float**) malloc(numClusters *            sizeof(float*));
    newClusters[0] = (float*)  calloc(numClusters * numCoords, sizeof(float));
    for (i=1; i<numClusters; i++)
        newClusters[i] = newClusters[i-1] + numCoords;

    do {
        delta = 0.0;
        for (i=0; i<numObjs; i++) {
            /* find the array index of nestest cluster center */
            index = find_nearest_cluster(numClusters, numCoords, objects[i],
                                         clusters);

            /* if membership changes, increase delta by 1 */
            if (membership[i] != index) delta += 1.0;

            /* assign the membership to object i */
            membership[i] = index;

            /* update new cluster centers : sum of objects located within */
            newClusterSize[index]++;
            for (j=0; j<numCoords; j++)
                newClusters[index][j] += objects[i][j];
        }

        /* average the sum and replace old cluster centers with newClusters */
        for (i=0; i<numClusters; i++) {
            for (j=0; j<numCoords; j++) {
                if (newClusterSize[i] > 0)
                    clusters[i][j] = newClusters[i][j] / newClusterSize[i];
                newClusters[i][j] = 0.0;   /* set back to 0 */
            }
            newClusterSize[i] = 0;   /* set back to 0 */
        }

        delta /= numObjs;
    } while (delta > threshold && loop++ < 500);

    *loop_iterations = loop + 1;

    free(newClusters[0]);
    free(newClusters);
    free(newClusterSize);

    return clusters;
}

struct kmeans_scratch
{
  float** clusters;
  int* membership;
  int numClusters, numObjs, numCoords;
  int result_pos;
};


/*
CREATE FUNCTION KMEANSCOLORUDF(TABLE LIKE COLORS AS LOCATOR)
RETURNS TABLE(C1 DOUBLE, C2 DOUBLE, C3 DOUBLE, C4 DOUBLE, C5 DOUBLE, C6 DOUBLE, C7 DOUBLE, C8 DOUBLE, C9 DOUBLE)
EXTERNAL NAME 'cudaudfsrv!kmeansColorUDF'
DETERMINISTIC
NO EXTERNAL ACTION
FENCED
NOT NULL CALL
LANGUAGE C
PARAMETER STYLE DB2SQL
NO SQL
SCRATCHPAD
FINAL CALL
DISALLOW PARALLEL
DBINFO
 */
#ifdef __cplusplus
extern "C"
#endif
void SQL_API_FN kmeansColorUDF(// Return row fields
						 SQLUDF_LOCATOR *inColorTable,
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

		objects = file_read(1, "/home/db2inst1/workspace/db2CudaUDF/colors17695.bin", &(pScratArea->numObjs), &(pScratArea->numCoords));
		pScratArea->membership = (int*) malloc(pScratArea->numObjs * sizeof(int));
		pScratArea->clusters = seq_kmeans(objects, pScratArea->numCoords, pScratArea->numObjs, pScratArea->numClusters, threshold,
	    		pScratArea->membership, &loop_iterations);

		free(objects[0]);
		free(objects);
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

      // Normal call UDF: Fetch next row
      if (pScratArea->result_pos == pScratArea->numClusters)
      {
        // SQLUDF_STATE is part of SQLUDF_TRAIL_ARGS_ALL
        strcpy(SQLUDF_STATE, "02000");
        break;
      }
      *C1 = (double)pScratArea->clusters[pScratArea->result_pos][1];
      *C2 = (double)pScratArea->clusters[pScratArea->result_pos][2];
      *C3 = (double)pScratArea->clusters[pScratArea->result_pos][3];
      *C4 = (double)pScratArea->clusters[pScratArea->result_pos][4];
      *C5 = (double)pScratArea->clusters[pScratArea->result_pos][5];
      *C6 = (double)pScratArea->clusters[pScratArea->result_pos][6];
      *C7 = (double)pScratArea->clusters[pScratArea->result_pos][7];
      *C8 = (double)pScratArea->clusters[pScratArea->result_pos][8];
      *C9 = (double)pScratArea->clusters[pScratArea->result_pos][9];

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

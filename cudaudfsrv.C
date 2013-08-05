#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlca.h>
#include <sqludf.h>
#include <sys/types.h>
#include <dirent.h>

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
    assert(clusters != NULL);
    clusters[0] = (float*)  malloc(numClusters * numCoords * sizeof(float));
    assert(clusters[0] != NULL);
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
    assert(newClusterSize != NULL);

    newClusters    = (float**) malloc(numClusters *            sizeof(float*));
    assert(newClusters != NULL);
    newClusters[0] = (float*)  calloc(numClusters * numCoords, sizeof(float));
    assert(newClusters[0] != NULL);
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



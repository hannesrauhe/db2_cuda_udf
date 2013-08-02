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

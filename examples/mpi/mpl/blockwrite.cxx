/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
   %%%%
   %%%% This program file is part of the book and course
   %%%% "Parallel Computing"
   %%%% by Victor Eijkhout, copyright 2023
   %%%%
   %%%% blockwrite.c : MPI IO example in MPL
   %%%%
   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mpi.h"

int main(int argc,char **argv) {

  /*
   * Each process has a buffer of length `nwords'
   * containing consecutive integers
   */
  int nwords = 3;
  int *output_data = (int*) malloc(nwords*sizeof(int));
  for (int iw=0; iw<nwords; iw++)
    output_data[iw] = procno*nwords+iw+1;
  
  mpl::file mpifile
    (comm_world,"blockwrite.dat",
     mpl::file::access_mode::create | mpl::file::access_mode::write_only
     );

  MPI_File_set_view
    (mpifile,procno*nwords*sizeof(int),
     MPI_INT,MPI_INT,"native",MPI_INFO_NULL);

  int nwriters = 2;
  if (procno<nwriters) {
    int offset = procno*nwords*sizeof(int);
    MPI_File_write
      (mpifile,output_data,nwords,MPI_INT,MPI_STATUS_IGNORE);
  }
  MPI_Barrier(comm);

  MPI_File_close(&mpifile);

  if (procno==0) {
    FILE *f;
    f = fopen("blockwrite.dat","r");
    for (int i=0; i<nwords*nwriters; i++) {
      int fromfile,success;
      success = fread(&fromfile,sizeof(int),1,f);
      if (success==EOF) {
  	printf("Premature end of file\n"); break;
      }
      if (fromfile!=i+1)
  	printf("Error %d:%d\n",i,fromfile);
    }
    fclose(f);
    printf("Finished\n");
  }

  MPI_Finalize();
  return 0;
}

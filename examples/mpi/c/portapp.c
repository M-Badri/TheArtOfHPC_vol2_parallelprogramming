/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
   %%%%
   %%%% This program file is part of the book and course
   %%%% "Parallel Computing"
   %%%% by Victor Eijkhout, copyright 2013-2021
   %%%%
   %%%% portapp.c : port stuff in a single executable
   %%%%
   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <mpi.h>

int main(int argc,char **argv) {

  MPI_Init(&argc,&argv);
  MPI_Comm
    comm_world = MPI_COMM_WORLD,
    comm_self = MPI_COMM_SELF;

  int world_p,world_n;
  MPI_Comm_size(comm_world,&world_n);
  MPI_Comm_rank(comm_world,&world_p);

  /*
   * Set up a communicator for all the worker ranks
   */
  //codesnippet groupworker
  MPI_Comm comm_work;
  {
    MPI_Group world_group,work_group;
    MPI_Comm_group( comm_world,&world_group );
    int manager[] = {0};
    MPI_Group_excl( world_group,1,manager,&work_group );
    MPI_Comm_create( comm_world,work_group,&comm_work );
    MPI_Group_free( &world_group ); MPI_Group_free( &work_group );
  }
  //codesnippet end

  if (world_p==0) {
    /*
     * On world process zero open a port, and
     * send its name to world process 1,
     * which is zero in the worker comm.
     */
    //codesnippet mpiportmanage
    MPI_Comm intercomm;
    char myport[MPI_MAX_PORT_NAME];
    MPI_Open_port( MPI_INFO_NULL,myport );
    int portlen = strlen(myport);
    MPI_Send( myport,portlen+1,MPI_CHAR,1,0,comm_world );
    printf("Host sent port <<%s>>\n",myport);
    MPI_Comm_accept( myport,MPI_INFO_NULL,0,comm_self,&intercomm );
    printf("host accepted connection\n");
    //codesnippet end

    /*
     * After the workers have accept the connection,
     * we can talk over the inter communicator
     */
    int work_n;
    MPI_Comm_remote_size(intercomm,&work_n);
    double work_data[work_n];
    MPI_Send( work_data,work_n,MPI_DOUBLE,
	      /* to rank zero of worker comm */ 0,0,intercomm );
    printf("Manager sent %d items over intercomm\n",work_n);
    /*
     * After we're done, close the port
     */
    MPI_Close_port(myport);

  } else {

    int work_p,work_n;
    MPI_Comm_size( comm_work,&work_n );
    MPI_Comm_rank( comm_work,&work_p );
    /*
     * In the workers communicator, rank 0
     * (which is 1 in the global)
     * receives the port name and passes it on.
     */
    //codesnippet mpiportwork
    char myport[MPI_MAX_PORT_NAME];
    if (work_p==0) {
      MPI_Recv( myport,MPI_MAX_PORT_NAME,MPI_CHAR, 
                MPI_ANY_SOURCE,0, comm_world,MPI_STATUS_IGNORE );
      printf("Worker received port <<%s>>\n",myport);
    }
    MPI_Bcast( myport,MPI_MAX_PORT_NAME,MPI_CHAR,0,comm_work );

    /*
     * The workers collective connect over the inter communicator
     */
    MPI_Comm intercomm;
    MPI_Comm_connect( myport,MPI_INFO_NULL,0,comm_work,&intercomm );
    if (work_p==0) {
      int manage_n;
      MPI_Comm_remote_size(intercomm,&manage_n);
      printf("%d workers connected to %d managers\n",work_n,manage_n);
    }
    //codesnippet end

    /*
     * The local leader receives work from the manager
     */
    if (work_p==0) {
      double work_data[work_n];
      MPI_Status work_status;
      MPI_Recv( work_data,work_n,MPI_DOUBLE, 
		/* from rank zero of manager comm */ 0,0,intercomm,&work_status );
      int work_count;
      MPI_Get_count(&work_status,MPI_DOUBLE,&work_count);
      printf("Worker zero received %d data items from manager\n",work_count);
    }
    /*
     * After we're done, close the connection
     */
    MPI_Close_port(myport);
  }

  MPI_Finalize();

  return 0;
}

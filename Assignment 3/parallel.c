
#include "parallel.h"


void Program_Message(char *txt)
/* produces a stderr text output  */

{
   int myrank;

   MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
   fprintf(stderr,"-MESSAGE- P:%2d : %s\n",myrank,txt);
   fflush(stdout);
   fflush(stderr);
}


void Programm_Sync(char *txt)
/* produces a stderr textoutput and synchronize all processes */

{
   int myrank;

   MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
   MPI_Barrier(MPI_COMM_WORLD);                             /* synchronize output */
   fprintf(stderr,"-MESSAGE- P:%2d : %s\n",myrank,txt);
   fflush(stdout);
   fflush(stderr);
   MPI_Barrier(MPI_COMM_WORLD);
}


void Programm_Stop(char *txt)
/* all processes will produce a text output, be synchronized and finished */

{
   int myrank;

   MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
   MPI_Barrier(MPI_COMM_WORLD);                           /* synchronize output */
   fprintf(stderr,"-STOP- P:%2d : %s\n",myrank,txt);
   fflush(stdout);
   fflush(stderr);
   MPI_Barrier(MPI_COMM_WORLD);
   MPI_Finalize();
   exit(1);
}


void init_parallel(int iproc,
               int jproc,
               int imax,
               int jmax,
               int *myrank,
               int *il,
               int *ir,
               int *jb,
               int *jt,
               int *l_rank,
               int *r_rank,
               int *b_rank,
               int *t_rank,
               int *omg_i,
               int *omg_j,
               int num_proc)
{

  *omg_i = (*myrank % iproc) + 1;
  *omg_j = ((*myrank+1-*omg_i)/iproc)+1;

  *il = (*omg_i-1)*(imax/iproc) + 1;
  *ir = (*omg_i!=iproc)?((*omg_i)*(imax/iproc)):imax;

  *jb = (*omg_j-1)*(jmax/jproc) + 1;
  *jt = (*omg_j!=jproc)?((*omg_j)*(jmax/jproc)):jmax;

  if(*il == 1)      *l_rank = MPI_PROC_NULL;
  else              *l_rank = *myrank - 1;

  if(*ir == imax)   *r_rank = MPI_PROC_NULL;
  else              *r_rank = *myrank + 1;


  if(*jb == 1)      *b_rank = MPI_PROC_NULL;
  else              *b_rank = *myrank - iproc;

  if(*jt == jmax)   *t_rank = MPI_PROC_NULL;
  else              *t_rank = *myrank + iproc;



printf("Thread_id: %d omg_ij: %d%d \nil: %d, ir: %d, jb: %d, jt: %d \n",*myrank,*omg_i,*omg_j, *il,*ir,*jb,*jt);

printf("l_rank: %d, r_rank: %d, b_rank: %d, t_rank: %d \n \n", *l_rank,*r_rank,*b_rank,*t_rank);
}

void pressure_comm(double **P,int il,int ir,int jb,int jt,
                  int l_rank,int r_rank,int b_rank, int t_rank,
              double *bufSend, double *bufRecv, MPI_Status *status, int chunk )
{
  int myrank;
  int x_dim = ir-il;
  int y_dim = jt-jb;
  chunk = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  //Send to left &  recieve from right
  for (int j = 1; j <=y_dim; ++j)
  {
    bufSend[j] = P[1][j];
  }
  if (l_rank != MPI_PROC_NULL)
    MPI_Send(bufSend, sizeof(double)*y_dim, MPI_DOUBLE, l_rank, chunk, MPI_COMM_WORLD);

  if (r_rank != MPI_PROC_NULL)
    MPI_Recv(bufRecv, sizeof(double)*y_dim, MPI_DOUBLE, r_rank, chunk, MPI_COMM_WORLD, status);

  for (int j = 1; j <=y_dim; ++j)
  {
    P[ir-il+1][j] = bufRecv[j];
  }

  // send to right & recieve from left
  for (int j = 1; j <=y_dim; ++j)
  {
    bufSend[j] = P[ir-il][j];
  }
  if (r_rank != MPI_PROC_NULL)
    MPI_Send(bufSend, sizeof(double)*y_dim, MPI_DOUBLE, r_rank, chunk, MPI_COMM_WORLD);

  else if (l_rank != MPI_PROC_NULL)
    MPI_Recv(bufRecv, sizeof(double)*y_dim, MPI_DOUBLE, l_rank, chunk, MPI_COMM_WORLD, status);

  for (int j = 1; j <=y_dim; ++j)
  {
    P[0][j] = bufRecv[j];
  }

  //send to top recieve from bottom
  for (int i = 1; i <=x_dim; ++i)
  {
    bufSend[i] = P[i][jt-jb];
  }
  if (t_rank != MPI_PROC_NULL)
    MPI_Send(bufSend, sizeof(double)*x_dim, MPI_DOUBLE, t_rank, chunk, MPI_COMM_WORLD);

  if (b_rank != MPI_PROC_NULL)
    MPI_Recv(bufRecv, sizeof(double)*x_dim, MPI_DOUBLE, b_rank, chunk, MPI_COMM_WORLD, status);

  for (int i = 1; i <=x_dim; ++i)
  {
    P[i][0] = bufRecv[i];
  }

  ///send to bottom recieve from top
  for (int i = 1; i <=x_dim; ++i)
  {
    bufSend[i] = P[i][1];
  }
  if (b_rank != MPI_PROC_NULL)
    MPI_Send(bufSend, sizeof(double)*x_dim, MPI_DOUBLE, b_rank, chunk, MPI_COMM_WORLD);

  if (t_rank != MPI_PROC_NULL)
    MPI_Recv(bufRecv, sizeof(double)*x_dim, MPI_DOUBLE, t_rank, chunk, MPI_COMM_WORLD, status);

  for (int i = 1; i <=x_dim; ++i)
  {
    P[i][ir-il+1] = bufRecv[i];
  }

}


void uv_comm(double **U,
            double **V,
            int il,
            int ir,
            int jb,
            int jt,
            int l_rank,
            int r_rank,
            int b_rank,
            int t_rank,
            double *bufSend,
            double *bufRecv,
            MPI_Status *status,
            int chunk)

{
  int myrank;
  int x_dim = ir-il;
  int y_dim = jt-jb;
  chunk = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

 /////////////* Velocity U *////////////
  //Send to left &  recieve from right
  for (int j = 1; j <=y_dim; ++j)
  {
    bufSend[j] = U[2][j];
  }
  if (l_rank != MPI_PROC_NULL)
  {
    MPI_Send(bufSend, sizeof(double)*y_dim, MPI_DOUBLE, l_rank, chunk, MPI_COMM_WORLD);
  }
  if (r_rank != MPI_PROC_NULL)
  {
    MPI_Recv(bufRecv, sizeof(double)*y_dim, MPI_DOUBLE, r_rank, chunk, MPI_COMM_WORLD, status);
  }
  for (int j = 1; j <=y_dim; ++j)
  {
    U[ir-il+1][j] = bufRecv[j];
  }

  // send to right & recieve from left
  for (int j = 1; j <=y_dim; ++j)
  {
    bufSend[j] = U[ir-il-1][j];
  }
  if (r_rank != MPI_PROC_NULL)
  {
    MPI_Send(bufSend, sizeof(double)*y_dim, MPI_DOUBLE, r_rank, chunk, MPI_COMM_WORLD);
  }
  if (l_rank != MPI_PROC_NULL)
  {
    MPI_Recv(bufRecv, sizeof(double)*y_dim, MPI_DOUBLE, l_rank, chunk, MPI_COMM_WORLD, status);
  }
  for (int j = 1; j <=y_dim; ++j)
  {
    U[0][j] = bufRecv[j];
  }

  //send to top recieve from bottom
  for (int i = 1; i <=x_dim+1; ++i)
  {
    bufSend[i] = U[i][jt-jb];
  }
  if (t_rank != MPI_PROC_NULL)
  {
    MPI_Send(bufSend, sizeof(double)*(x_dim+1), MPI_DOUBLE, t_rank, chunk, MPI_COMM_WORLD);
  }
  if (b_rank != MPI_PROC_NULL)
  {
    MPI_Recv(bufRecv, sizeof(double)*(x_dim+1), MPI_DOUBLE, b_rank, chunk, MPI_COMM_WORLD, status);
  }
  for (int i = 1; i <=x_dim+1; ++i)
  {
    U[i][0] = bufRecv[i];
  }

  ///send to bottom recieve from top
  for (int i = 1; i <=x_dim+1; ++i)
  {
    bufSend[i] = U[i][1];
  }
  if (b_rank != MPI_PROC_NULL)
  {
    MPI_Send(bufSend, sizeof(double)*(x_dim+1), MPI_DOUBLE, b_rank, chunk, MPI_COMM_WORLD);
      }
  if (t_rank != MPI_PROC_NULL)
  {
    MPI_Recv(bufRecv, sizeof(double)*(x_dim+1), MPI_DOUBLE, t_rank, chunk, MPI_COMM_WORLD, status);
  }
  for (int i = 1; i <=x_dim+1; ++i)
  {
    U[i][jt-jb+1] = bufRecv[i];
  }


  /////////////* Velocity V*///////////////
  //Send to left &  recieve from right
  for (int j = 1; j <=y_dim+1; ++j)
  {
    bufSend[j] = V[1][j];
  }
  if (l_rank != MPI_PROC_NULL)
  {
    MPI_Send(bufSend, sizeof(double)*(y_dim+1), MPI_DOUBLE, l_rank, chunk, MPI_COMM_WORLD);
  }
  if (r_rank != MPI_PROC_NULL)
  {
    MPI_Recv(bufRecv, sizeof(double)*(y_dim+1), MPI_DOUBLE, r_rank, chunk, MPI_COMM_WORLD, status);
  }
  for (int j = 1; j <=y_dim+1; ++j)
  {
    V[ir-il+1][j] = bufRecv[j];
  }

  // send to right & recieve from left
  for (int j = 1; j <=y_dim+1; ++j)
  {
    bufSend[j] = V[ir-il][j];
  }
  if (r_rank != MPI_PROC_NULL)
  {
    MPI_Send(bufSend, sizeof(double)*(y_dim+1), MPI_DOUBLE, r_rank, chunk, MPI_COMM_WORLD);
  }
  if (l_rank != MPI_PROC_NULL)
  {
    MPI_Recv(bufRecv, sizeof(double)*(y_dim+1), MPI_DOUBLE, l_rank, chunk, MPI_COMM_WORLD, status);
  }
  for (int j = 1; j <=y_dim+1; ++j)
  {
    V[0][j] = bufRecv[j];
  }

  //send to top recieve from bottom
  for (int i = 1; i <=x_dim+1; ++i)
  {
    bufSend[i] = V[i][jt-jb-1];
  }
  if (t_rank != MPI_PROC_NULL)
  {
    MPI_Send(bufSend, sizeof(double)*(x_dim+1), MPI_DOUBLE, t_rank, chunk, MPI_COMM_WORLD);
  }
  if (b_rank != MPI_PROC_NULL)
  {
    MPI_Recv(bufRecv, sizeof(double)*(x_dim+1), MPI_DOUBLE, b_rank, chunk, MPI_COMM_WORLD, status);
  }
  for (int i = 1; i <=x_dim+1; ++i)
  {
    V[i][0] = bufRecv[i];
  }
  ///send to bottom recieve from top
  for (int i = 1; i <=x_dim+1; ++i)
  {
    bufSend[i] = V[i][2];
  }
  if (b_rank != MPI_PROC_NULL)
  {
    MPI_Send(bufSend, sizeof(double)*(x_dim+1), MPI_DOUBLE, b_rank, chunk, MPI_COMM_WORLD);
  }
  if (t_rank != MPI_PROC_NULL)
  {
    MPI_Recv(bufRecv, sizeof(double)*(x_dim+1), MPI_DOUBLE, t_rank, chunk, MPI_COMM_WORLD, status);
  }
  for (int i = 1; i <=x_dim+1; ++i)
  {
    V[i][jt-jb+1] = bufRecv[i];
  }
}
/*
  if(l_rank != MPI_PROC_NULL)
  {
    MPI_Send(bufSend, sizeof(double)*(jt-jb), MPI_DOUBLE, l_rank, chunk, MPI_COMM_WORLD);
  }
  if(r_rank != MPI_PROC_NULL)
  {
    MPI_Recv(bufRecv, sizeof(double)*(jt-jb), MPI_DOUBLE, r_rank, chunk, MPI_COMM_WORLD, status);
  }
  if(b_rank != MPI_PROC_NULL)
  {
    MPI_Send(bufSend, sizeof(double)*(ir-il), MPI_DOUBLE, b_rank, chunk, MPI_COMM_WORLD);
  }
  if(t_rank != MPI_PROC_NULL)
  {
    MPI_Recv(bufRecv, sizeof(double)*(ir-il), MPI_DOUBLE, t_rank, chunk, MPI_COMM_WORLD, status);
  }

*/

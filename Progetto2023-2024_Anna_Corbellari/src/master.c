#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Types.h"
#include "errExit.h"

#define MAX_NO_IMPROVEMENT 1000

Point *points;
int shm_id;
int msg_queue;

void sigterm_handler(int signum) {
  // Detach from shared memory and message queue
  // detach from shared memory
  shmdt(points);

  // delete shared memory
  shmctl(shm_id, IPC_RMID, NULL);

  // delete message queue
  msgctl(msg_queue, IPC_RMID, NULL);

  printf("Ctrl-C pressed. Master terminating...\n");
  fflush(stdout);
  exit(0);
}

int main(int argc, char *argv[]) {
  // set signal handler
  signal(SIGINT, sigterm_handler);

  if (argc != 5) {
    printf("Usage: %s <K> <N> <key> <dataset>\n", argv[0]);
    return 1;
  }

  // K : number of clusters
  int K = atoi(argv[1]);
  // N : number of worker processes
  int N = atoi(argv[2]);
  // key: key for shared memory and message queue
  key_t key = atoi(argv[3]);
  // datasetFile: file containing the dataset
  char *datasetFile = argv[4];

  // open dataset file
  FILE *fp = fopen(datasetFile, "r");
  if (!fp) {
    errExit("Error opening dataset file");
    return 1;
  }

  // count number of lines in dataset file
  int lines = 0;
  char c;
  while ((c = fgetc(fp)) != EOF) {
    if (c == '\n') {
      lines++;
    }
  }
  // rewind file pointer to beginning of file
  fseek(fp, 0, SEEK_SET);

  // can't have more clusters than points
  if (lines <= K) {
    errExit("Number of clusters must be <= than number of points\n");
  }
  if (lines <= 0) {
    errExit("Number of points must be greater than 0\n");
  }

  // Create shared memory segment
  shm_id = shmget(key, sizeof(Point) * lines, IPC_CREAT | S_IRUSR | S_IWUSR);

  if (shm_id == -1) {
    errExit("master: Error creating shared memory segment");
    return 1;
  }

  // Attach to shared memory segment
  points = (Point *)shmat(shm_id, NULL, 0);
  if (points == (Point *)-1) {
    errExit("master: Error attaching to shared memory segment");
    return 1;
  }

  int i = 0;
  char line[100];
  while (fgets(line, 100, fp) != NULL) {
    char *token = strtok(line, ",");
    // convert the string to double
    points[i].x = atof(token);
    token = strtok(NULL, ",");
    points[i].y = atof(token);
    i++;
  }
  fclose(fp);

  // create message queue
  msg_queue = msgget(key, IPC_CREAT | S_IRUSR | S_IWUSR);

  if (msg_queue == -1) {
    errExit("Error creating message queue");
    return 1;
  }
  // to store the pids of the worker processes
  pid_t pids[N];

  // Generate N child processes called "worker"
  for (int i = 0; i < N; i++) {
    pids[i] = fork();
    if (pids[i] == -1) {
      errExit("Error creating child process");
    } else if (pids[i] == 0) {
      // child process
      char Kstr[10];
      char keystr[10];
      char linesstr[100];
      sprintf(Kstr, "%d", K);
      sprintf(keystr, "%d", key);
      sprintf(linesstr, "%d", lines);
      if (execl("worker", "worker", keystr, Kstr, linesstr, (char *)NULL) ==
          -1) {
        errExit("execl failed");
      }
    }
  }

  // counter for number of messages that did not improve the clustering
  int noImprov = 0;
  double min_variance = 1e100;

  // keep receiving messages from workers
  while (1) {
    // read a single message
    Message msg;
    Centroid b_centroids[K];
    if (msgrcv(msg_queue, &msg, sizeof(msg) - sizeof(long), 0, 0) ==
        -1) {  
      errExit("master: msgrc");
    }
    //printf("Received message from worker %f\n", msg.msg.variance);
   
    // update best clustering (lowest variance)
    if (msg.msg.variance < min_variance) {
      min_variance = msg.msg.variance;
      noImprov = 0;
      for (int i = 0; i < K; i++) {
        b_centroids[i].point.x = msg.msg.centroids[i].point.x;
        b_centroids[i].point.y = msg.msg.centroids[i].point.y;
      };
    } else {
      noImprov++;
    };

    // increment noImprov if the variance did not improve

    if (noImprov == MAX_NO_IMPROVEMENT) {
      // dump to file
      FILE *fp = fopen("centroids.csv", "w");
      if (fp == NULL) {
        errExit("fopen");
      }
      for (int i = 0; i < K; i++) {
        // write the centroid to file
        fprintf(fp, "%.2lf,%.2lf\n", b_centroids[i].point.x,
                b_centroids[i].point.y);  // Scrivi le coordinate
        printf("Finale: %2lf, %2lf\n", b_centroids[i].point.x,b_centroids[i].point.y);
      }
      fclose(fp);

      // send SIGINT to all workers
      for (int i = 0; i < N; i++) {
        kill(pids[i], SIGINT);
      }
      // break out of the loop
      break;
    }
  }

  // gather exit status of all worker processes
  for (int i = 0; i < N; i++) {
    wait(NULL);
  }

  // Detach from shared memory segment
  shmdt(points);

  // delete shared memory
  shmctl(shm_id, IPC_RMID, NULL);

  // Deallocate message queue
  msgctl(msg_queue, IPC_RMID, NULL);

  printf("Master terminating...\n");

  return 0;
}


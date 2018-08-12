#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

sem_t tutors, students, mutex;		//semaphores
pthread_t *student = 0;						//thread for students

int numChairs = 0;				//number of chairs in waiting room
int waiting = 0;					//number of students waiting
int sitHereNext = 0;
int serveMeNext = 0;
static int count = 0;			//check number of students left
int *seats = 0;						//array of seats
int help = 0;							//number of times student comes back for help

void tutorThread(void *arg);			//Thread function for tutors
void studentThread(void *arg);		//Thread function for students
int randtime();										//function for random time

struct arg_struct{				// struct to pass index and the remaining number of times the student will return for help
		int arg1;							//index
		int arg2;							//help
};

int main(int argc, char** argv)
{
		/*pass in number of students, tutors, chairs, and help as arguments*/
	  int numStudents = atoi(argv[1]);
	  int numTutors = atoi(argv[2]);
	  numChairs = atoi(argv[3]);
	  help = atoi(argv[4]);

		/*instantiate count, seats array*/
		count = numStudents;
		seats = malloc(sizeof(int)* numChairs);

    /*declaration for tutor thread array and student thread array*/
    pthread_t tutor[numTutors-1];
		student = malloc(sizeof(int)* numStudents);

    /*Semaphore initialization*/
    sem_init(&students,0,0);
    sem_init(&tutors,0,0);
    sem_init(&mutex,0,1);


		struct arg_struct arguments[numStudents-1];		//array of argument structures (one for each student)

		int i, status = 0;
    printf("CSMC opens\n");
    printf("Number of tutors: %d\n", numTutors);

		/*Creation of Tutor Threads*/
		for(i=0; i<numTutors; i++)
    {
       status=pthread_create(&tutor[i],NULL,(void *)tutorThread,(void*)&i);
       sleep(1);
       if(status!=0)
          perror("ERROR: creating tutor thread\n");
    }

		sleep(1);
		/*Creation of Student Threads*/
    for(i=0; i<numStudents; i++)
    {
			 arguments[i].arg1 = i;								//set arg1 to index
			 arguments[i].arg2 = help;						//set arg2 to number of help
       status=pthread_create(&student[i],NULL,(void *)studentThread,(void*)&arguments[i]);
       if(status!=0)
          perror("ERROR: creating student thread\n");
    }

		sleep(1);
		/*close the CSMC 5 seconds after count is 0*/
		while(1){
			if(count == 0){
					printf("CSMC closing in 5 seconds\n");
					sleep(5);
					break;
			}
		}
		printf("CSMC closed\n");
    exit(EXIT_SUCCESS);
		//free(seats);
		//free(student);
		//exit(EXIT_SUCCESS);
}

void studentThread(void *arg)	//student proccess
{
		struct arg_struct args  = *(struct arg_struct *)arg;
		int index = args.arg1 + 1;
		while(args.arg2 > 0)			//check if the student will come back for help again
		{
					int mySeat, T, status;
			    sem_wait(&mutex);		//Lock mutex to protect seat changes

			    if(waiting < numChairs)
			    {
							waiting++;
			        printf("Student-%d takes a seat. Waiting students = %d\n", index, waiting);
			        sitHereNext = (++sitHereNext) % numChairs;
			        mySeat = sitHereNext;
			        seats[mySeat] = index;
			        sem_post(&mutex);			//Release the seat change mutex
			        sem_post(&tutors);		//Wake up tutor
			        sem_wait(&students);	//join queue of waiting students
			        sem_wait(&mutex);			//Lock mutex to protect seat changes
			        T = seats[mySeat];
			        sem_post(&mutex);			//Release the seat change mutex
					    pthread_exit(0);
			    }
			    else if(args.arg2 >= 0)
			    {
			       sem_post(&mutex);	//Release the mutex and student leaves without seeing a tutor
			       printf("Student-%d found no empty chair and will try again later.\n",index);
	 					 args.arg2--;

						 int r = randtime();
						 sleep(r);
			    }
		}
    pthread_exit(0);
}

void tutorThread(void *arg)	//tutor proccess
{
    int index = *(int *)(arg) + 1;
    int myNext, S;
    printf("Tutor-%d[Id:%d] begins shift. \n",index,pthread_self());
    while(1)
    {
        sem_wait(&tutors);		//Join tutors
        sem_wait(&mutex);			//Lock mutex to protect seat changes

        serveMeNext = (++serveMeNext) % numChairs;	//Select next student
        myNext = serveMeNext;
        S = seats[myNext];
				int r = randtime();
				waiting--;
				printf("Tutor-%d is helping student-%d for %d seconds. Waiting students = %d\n", index, S, r, waiting);

				count--;
        //printf("Count: %d.\n",count);
				sem_post(&mutex);
        sem_post(&students);	//Call selected student

				sleep(r);
        printf("Student-%d has learned something and leaves CSMC.\n",S);
    }
}

int randtime()	//function for random time between 1 and 3
{
		 srand(time(NULL));
     int x = (rand() % 3) + 1;
     return x;
}

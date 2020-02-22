/*
* Student name: Behlul Gulmez
* Student ID : 150140113
* Date: 08/05/2018
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

#define	SHMKEY 159
#define	SEMKEY1	259
#define	SEMKEY2	359
#define	SEMKEY3	459
#define	SEMKEY4	559

void sem_signal(int semid, int val) {
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = val;
    semaphore.sem_flg = 1;
    semop(semid, &semaphore, 1);
}

void sem_wait(int semid, int val) {
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = -val;
    semaphore.sem_flg = 1;
    semop(semid, &semaphore, 1);
}

int main(int argc, char *argv[]){
	char* InputFileName;
	char c;
	char *temp;
	int employees[50];
	int i = 0, j;
	int max_water, water_level, employee_number = 0;	//max water is water from text, water level is current water, employee number is counter for employees array
	FILE *text;
	int shmid, semid1, semid2, semid3, semid4;
	int *pointer;
	int f;
	int is_worker;	//check for employee who is resposible for water or other employees, 1 means other employees
	
	InputFileName = (char *)malloc(sizeof(char)*(strlen(argv[1]) + 1));
	strcpy(InputFileName, argv[1]);
	
	text = fopen(InputFileName, "r");		//open txt which name is given as argument

	if (text == NULL){
		printf("File error");
		return 0;
	}
	temp = (char *)malloc(sizeof(char)*(50));
	while (1){
		c = fgetc(text);
		if (c == '\n'){
			temp[i] = '\0';		//when first line finish loop breaks.
			i = 0;
			break;
		}			
		else{
			temp[i] = c;		//puts chars to char array
			i++;
		}	
	}
	
	max_water = atoi(temp);		//max_water equals number which in first line of text
	water_level = atoi(temp);
	free(temp);
	
	while (1){
		c = fgetc(text);		//continues to read from second line
		if (feof(text)) {
			break;
		}			
		else{
			if(c == ' ' || c == '\t')		//if there is space or tab dont do anything but if it is not put that number to employees array as integer.
				continue;
			else{
				employees[i] = c - '0';
				i++;
			}
		}	
	}
	
	semid1 = semget(SEMKEY1, 1, IPC_CREAT|0700);		//semaphore and shared memory creatings
	semctl(semid1, 0, SETVAL, 0);
	
	semid2 = semget(SEMKEY2, 1, IPC_CREAT|0700);
	semctl(semid2, 0, SETVAL, 0);
	
	semid3 = semget(SEMKEY3, 1, IPC_CREAT|0700);
	semctl(semid3, 0, SETVAL, 0);
	
	semid4 = semget(SEMKEY4, 1, IPC_CREAT|0700);
	semctl(semid4, 0, SETVAL, 1);
	
	shmid = shmget(SHMKEY, 2 * sizeof(int), IPC_CREAT|0700);
	pointer = (int *) shmat(shmid, NULL, 0);
    *pointer = water_level;		//there are 2 variable in shared memory, current water level and employee number which means child number.
    *(pointer + 1) = employee_number;

	printf("SIMULATION BEGINS\n");
	is_worker = 1;
	
	for(j = 0 ; j < i ; j++){
		f = fork();		//creates number of employees children
		if(f == 0)
			break;
	}
	
	if(f > 0){
		is_worker = 0;		//and creates 1 more chield who is responsible for water
		f = fork();
	}
	
	if(f == 0){	
		if(is_worker == 1){		//if other employees			
			sem_wait(semid4, 1);		//starts one of the children because of semid4 is 1 but others wait because it became 0
			
			pointer = (int *) shmat(shmid, NULL, 0);
			water_level = *pointer;
			employee_number = *(pointer + 1);
			
			printf("Current water level %d cups\n", water_level);
			if(water_level > 1){
				if(employees[employee_number] == 1){		//if employee wants type 1
					printf("Employee %d wants coffee Type 1\n", employee_number);
					water_level--;	
					printf("Employee %d SERVED\n", employee_number);
				}
				else{		//if employee wants type 2
					printf("Employee %d wants coffee Type 2\n", employee_number);
					water_level = water_level - 2;
					printf("Employee %d SERVED\n", employee_number);
				}
			}
			else{
				if(water_level == 1){
					if(employees[employee_number] == 1){		//if employee wants type 1
						printf("Employee %d wants coffee Type 1\n", employee_number);
						water_level--;
						printf("Employee %d SERVED\n", employee_number);
					}
					else{		//if employee wants type 2
						printf("Employee %d wants coffee Type 2\n", employee_number);
						printf("Employee %d WAITS\n", employee_number);		//if there is not enough water, signal for water employee wait for this employee.
						
						*pointer = water_level;
						*(pointer + 1) = employee_number;		//saving datas to shared memory before signal
						shmdt(pointer);
						
						sem_signal(semid2, 1);
						sem_wait(semid3, 1);
						
						pointer = (int *) shmat(shmid, NULL, 0);		//taking datas from shared memory after come back here
						water_level = *pointer;
						employee_number = *(pointer + 1);
						
						printf("Current water level %d cups\n", water_level);
						water_level = water_level - 2;
						printf("Employee %d SERVED\n", employee_number);
					}
				}
				else{
					if(employees[employee_number] == 1){		//if employee wants type 1
						printf("Employee %d wants coffee Type 1\n", employee_number);
						printf("Employee %d WAITS\n", employee_number);		//if there is not enough water, signal for water employee wait for this employee.
						
						*pointer = water_level;
						*(pointer + 1) = employee_number;		//saving datas to shared memory before signal
						shmdt(pointer);
						
						sem_signal(semid2, 1);
						sem_wait(semid3, 1);
						
						pointer = (int *) shmat(shmid, NULL, 0);		//taking datas from shared memory after come back here
						water_level = *pointer;
						employee_number = *(pointer + 1);
						
						printf("Current water level %d cups\n", water_level);
						water_level--;
						printf("Employee %d SERVED\n", employee_number);
					}
					else{		//if employee wants type 2
						printf("Employee %d wants coffee Type 2\n", employee_number);
						printf("Employee %d WAITS\n", employee_number);		//if there is not enough water, signal for water employee wait for this employee.
						
						*pointer = water_level;
						*(pointer + 1) = employee_number;		//saving datas to shared memory before signal
						shmdt(pointer);
						
						sem_signal(semid2, 1);
						sem_wait(semid3, 1);
						
						pointer = (int *) shmat(shmid, NULL, 0);		//taking datas from shared memory after come back here
						water_level = *pointer;
						employee_number = *(pointer + 1);
						
						printf("Current water level %d cups\n", water_level);
						water_level = water_level - 2;
						printf("Employee %d SERVED\n", employee_number);
					}
				}
			}
			employee_number++;
			*pointer = water_level;
			*(pointer + 1) = employee_number;
			shmdt(pointer);
			sem_signal(semid4, 1);		//now other child can start
		}
		else{
			while(employee_number < i){		//continue until employees finish				
				sem_wait(semid2, 1);		//wait until it require
				
				pointer = (int *) shmat(shmid, NULL, 0);
				water_level = *pointer;
				employee_number = *(pointer + 1);
				
				water_level = max_water;
				if(employee_number == i)
					break;
				printf("Employee 999 wakes up and fills the coffee machine\n");
				
				*pointer = water_level;
				*(pointer + 1) = employee_number;
				shmdt(pointer);
				
				sem_signal(semid3, 1);		//other employee can continue to its work
			}		
		}
		
		sem_signal(semid1, 1);		
			
	}
	else{
		sem_wait(semid1, i);		//parents starts after children finished
		shmdt(pointer);
		printf("SIMULATION ENDS\n");
		shmctl(shmid, IPC_RMID, 0);		//clears everything we used
		semctl(semid1, 0, IPC_RMID, 0);
		semctl(semid2, 0, IPC_RMID, 0);
		semctl(semid3, 0, IPC_RMID, 0);
		semctl(semid4, 0, IPC_RMID, 0);
	}	
	exit(0);
}

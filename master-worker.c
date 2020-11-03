#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <pthread.h>

int item_to_produce, curr_buf_size;
int total_items, max_buf_size, num_workers, num_masters;
int total_consumed;
int *buffer;

pthread_mutex_t mutex; 
pthread_cond_t ccond; 
pthread_cond_t pcond;

void print_produced(int num, int master) {

	printf("Produced %d by master %d\n", num, master);
}

void print_consumed(int num, int worker) {

	printf("Consumed %d by worker %d\n", num, worker);

}


//produce items and place in buffer
//modify code below to synchronize correctly
void *generate_requests_loop(void *data)
{
	int thread_id = *((int *)data);
	while(1)
	{
		pthread_mutex_lock(&mutex);
		if(curr_buf_size == max_buf_size-1)
			pthread_cond_wait(&ccond, &mutex);
		if(item_to_produce >= total_items) 
			break;
		buffer[curr_buf_size++] = item_to_produce;
		item_to_produce++;
		print_produced(item_to_produce, thread_id);
		pthread_cond_signal(&pcond);
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}

void *consumed_loop(void *data)
{
	int thread_id = *((int *)data);
	while(1)
	{
		pthread_mutex_lock(&mutex);
		if(curr_buf_size == 0)
			pthread_cond_wait(&pcond, &mutex);
		if(total_consumed >= total_items)
			break;
		total_consumed++;
		print_consumed(buffer[curr_buf_size--], thread_id);
		pthread_cond_signal(&ccond);
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}

//write function to be run by worker threads
//ensure that the workers call the function print_consumed when they consume an item

int main(int argc, char *argv[])
{
	int *master_thread_id;
	int *worker_thread_id; 
	pthread_t *master_thread;
	pthread_t *worker_thread; 
	item_to_produce = 0;
	curr_buf_size = 0;
	total_consumed = 0;
	pthread_mutex_init(&mutex, NULL); 
	pthread_cond_init(&ccond, NULL); 
	pthread_cond_init(&pcond, NULL); 
	int i;

	if (argc < 5) {
		printf("./master-worker #total_items #max_buf_size #num_workers #masters e.g. ./exe 10000 1000 4 3\n");
		exit(1);
	}
	else {
		num_masters = atoi(argv[4]);
		num_workers = atoi(argv[3]);
		total_items = atoi(argv[1]);
		max_buf_size = atoi(argv[2]);
	}


	buffer = (int *)malloc (sizeof(int) * max_buf_size);

	//create master producer threads
	master_thread_id = (int *)malloc(sizeof(int) * num_masters);
	master_thread = (pthread_t *)malloc(sizeof(pthread_t) * num_masters);
	for (i = 0; i < num_masters; i++)
		master_thread_id[i] = i;

	for (i = 0; i < num_masters; i++)
		pthread_create(&master_thread[i], NULL, generate_requests_loop, (void *)&master_thread_id[i]);
	
	//create worker consumer threads
	worker_thread_id = (int *)malloc(sizeof(int) * num_workers);
	worker_thread = (pthread_t *)malloc(sizeof(pthread_t) * num_workers);
	for (i = 0; i < num_workers; i++)
		worker_thread_id[i] = i;

	for (i = 0; i < num_workers; i++)
		pthread_create(&worker_thread[i], NULL, consumed_loop, (void *)&worker_thread_id[i]);

	//wait for all threads to complete
	for (i = 0; i < num_masters; i++)
	{
		pthread_join(master_thread[i], NULL);
		printf("master %d joined\n", i);
	}
	for(i = 0; i < num_workers; i++)
	{
		pthread_join(worker_thread[i], NULL);
		printf("worker %d joined\n", i);
	}

	/*----Deallocating Buffers---------------------*/
	free(buffer);
	free(master_thread_id);
	free(master_thread);
	free(worker_thread_id);
	free(worker_thread);

	return 0;
}
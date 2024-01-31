#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include "pthread.h"
#include "semaphore.h"
#include <vector>
extern "C" {
	#include "hw2_output.h"
}
extern "C" {
	#include "hw2_output.c"
}

using namespace std;

int r1, c1, r3, c3;
vector<vector<int>> m1; // matrix 1
vector<vector<int>> m2; // matrix 2
vector<vector<int>> m3; // matrix 3
vector<vector<int>> m4; // matrix 4
vector<vector<int>> addition1; // addition matrix 1
vector<vector<int>> addition2; // addition matrix 2
vector<vector<int>> res; // result matrix after multiplication of addition1 and addition2
vector<vector<bool>> rescpy; // boolean version of the result matrix for if an element is computed in the result matrix

vector<int> cols; // for counting how many elements are computed in a column in the addition2 matrix

vector<pthread_t> threads;

vector<sem_t> rows; // semaphores for every row in the addition1 matrix to signal result matrix
sem_t barrier; // general semaphore for if any column is finished in the addition2 matrix

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

void Print(vector<vector<int>> &mat)
{
	for(int i = 0; i < mat.size(); i++)
	{
		for(int j = 0; j < mat[i].size(); j++)
		{
			cout << mat[i][j] << " ";
		}
		cout << "\n";
	}
}

void initMatrices()
{
	for(int i = 0; i < r1; i++) {
		vector<int> temp;
		addition1.push_back(temp);
		for(int j = 0; j < c1; j++) {
			addition1[i].push_back(0);
		}
	}

	for(int i = 0; i < r3; i++) {
		vector<int> temp;
		addition2.push_back(temp);
		for(int j = 0; j < c3; j++) {
			addition2[i].push_back(0);
		}
	}

	for(int i = 0; i < r1; i++) {
		vector<int> temp;
		res.push_back(temp);
		for(int j = 0; j < c3; j++) {
			res[i].push_back(0);
		}
	}
	
	for(int i = 0; i < c3; i++) {
		cols.push_back(0);
	}
	
	for(int i = 0; i < r1; i++) {
		vector<bool> temp;
		rescpy.push_back(temp);
		for(int j = 0; j < c3; j++) {
			rescpy[i].push_back(false);
		}
	}
}

void *addFirstMatrices(void *ptr) {
	int row = *((int*)ptr);
	delete (int*)ptr;
	
	for(int j = 0; j < c1; j++) {
		addition1[row][j] = m1[row][j] + m2[row][j];
		hw2_write_output(0, row + 1, j + 1, addition1[row][j]);
	}
	
	sem_post(&rows[row]);
	return 0;
}

void *addSecondMatrices(void* ptr) {
	int row = *((int*)ptr);
	delete (int*)ptr;

	for(int j = 0; j < c3; j++) {
		addition2[row][j] = m3[row][j] + m4[row][j];
		hw2_write_output(1, row + 1, j + 1, addition2[row][j]);
		pthread_mutex_lock(&mutex1);
		cols[j]++;
		if(cols[j] == r3) {
			for(int i = 0; i < r1; i++) {
				sem_post(&barrier);
			}
		}
		pthread_mutex_unlock(&mutex1);
	}
	return 0;
}

void *multiplyMatrices(void* ptr) {
	int row = *((int*)ptr);
	delete (int*)ptr;
	
	sem_wait(&rows[row]);
	
	bool isRowFinished = false;
	
	while(!isRowFinished)
	{
		sem_wait(&barrier);
		bool flag = false;

		for(int j = 0; j < c3; j++) {
			if(cols[j] == r3) {
				if(!rescpy[row][j]) {
					flag = true;
					for(int i = 0; i < c1; i++) {
						res[row][j] += addition1[row][i] * addition2[i][j];
					}
					hw2_write_output(2, row + 1, j + 1, res[row][j]);
					rescpy[row][j] = true;
					break;
				}
			}
		}
		
		if(!flag) sem_post(&barrier);
		
		for(int i = 0; i < c3; i++) {
			if(!rescpy[row][i]) break;
			if(i == c3 - 1) isRowFinished = true;
		}
	}

	return 0;
}

int main() {

	hw2_init_output();	

	cin >> r1 >> c1;
	for(int i = 0; i < r1; i++) {
		vector<int> v1;
		m1.push_back(v1);
		for(int j = 0; j < c1; j++) { // input for matrix 1
			int val;
			cin >> val;
			m1[i].push_back(val);
		}
	}
	
	cin >> r1 >> c1;
	for(int i = 0; i < r1; i++) {
		vector<int> v2;
		m2.push_back(v2);
		for(int j = 0; j < c1; j++) { // input for matrix 2
			int val;
			cin >> val;
			m2[i].push_back(val);
		}
	}
	
	cin >> r3 >> c3;
	for(int i = 0; i < r3; i++) {
		vector<int> v3;
		m3.push_back(v3);
		for(int j = 0; j < c3; j++) { // input for matrix 3
			int val;
			cin >> val;
			m3[i].push_back(val);
		}
	}
	
	cin >> r3 >> c3;
	for(int i = 0; i < r3; i++) {
		vector<int> v4;
		m4.push_back(v4);
		for(int j = 0; j < c3; j++) { // input for matrix 4
			int val;
			cin >> val;
			m4[i].push_back(val);
		}
	}
	
	initMatrices(); // initialize every other matrix
	
	sem_init(&barrier, 0, 0); // initialize general column finish semaphore in addition2 matrix
	
	for(int i = 0; i < r1; i++) {
		sem_t semaphore;
		sem_init(&semaphore, 0, 0); // initialize semaphores for if a row finished in the addition1 matrix
		rows.push_back(semaphore);
	}
	
	for(int i = 0; i < r1; i++) {
		pthread_t thread1;
		int* arg = new int;
		*arg = i;
		pthread_create(&thread1, NULL, addFirstMatrices, (void*) arg); // initialize threads for adding first 2 matrices
		threads.push_back(thread1);
	}
	
	for(int i = 0; i < r3; i++) {
		pthread_t thread2;
		int* arg = new int;
		*arg = i;
		pthread_create(&thread2, NULL, addSecondMatrices, (void*) arg); // initialize threads for adding second 2 matrices
		threads.push_back(thread2);
	}	
	
	for(int i = 0; i < r1; i++) {
		pthread_t thread3;
		int* arg = new int;
		*arg = i;
		pthread_create(&thread3, NULL, multiplyMatrices, (void*) arg); // initialize threads for multiplying addition1 and addition2 matrices
		threads.push_back(thread3);
	}
	
	for(int i = 0; i < threads.size(); i++) { // join threads
		pthread_join(threads[i], NULL); 
	}

	Print(res);

	return 0;
}

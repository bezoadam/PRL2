#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>

using namespace std;

void printVector(vector<int> vec) {
	for(int i = 0; i < vec.size(); i++)
	{
		if(vec[i] == -1) break;
		cout << vec[i] << " ";
	}
	cout << endl;
}

int main(int argc, char ** argv) {
	int numprocs, myId;
	vector<int> values;
	cout << "test";

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);       // zjistíme, kolik procesů běží
	MPI_Comm_rank(MPI_COMM_WORLD, &myId);           // zjistíme id svého procesu

	char input[]= "numbers";                          //jmeno souboru
	int number;                                     //hodnota pri nacitani souboru
	fstream fin;                                    //cteni ze souboru
	fin.open(input, ios::in);

	cout << "Pocet procesu: " << numprocs << endl;

	while(fin.good()){
		number= fin.get();
		if(!fin.good()) break;                      //nacte i eof, takze vyskocim
		values.push_back(number);

	}
	fin.close();
	// printVector(values);

	int size = values.size() / numprocs;
	int *sub_array = (int *) malloc(size * sizeof(int));
	int *original_array = &values[0];
	MPI_Scatter(original_array, size, MPI_INT, sub_array, size, MPI_INT, 0, MPI_COMM_WORLD);

	vector<int> finalValues(sub_array, sub_array + size);
	printVector(finalValues);

	MPI_Finalize();
	return 0;
}
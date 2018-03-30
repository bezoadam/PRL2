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
	int numprocs;
	vector<int> values;
	cout << "test";

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);       // zjistíme, kolik procesů běží

// cteni vygenerovanych cisel
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
	printVector(values);

	MPI_Finalize();
	return 0;
}
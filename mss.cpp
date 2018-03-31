#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm> 
#include <math.h>

#define TAG 0

using namespace std;

void printVector(vector<int> vec) {
	for(int i = 0; i < vec.size(); i++)
	{
		if(vec[i] == -1) break;
		cout << vec[i] << " ";
	}
	cout << endl;
}

int getNextId(int numprocs, int currentId) {
	return (currentId == numprocs - 1) ? 0 : currentId + 1;
}

int getPreviousId(int numprocs, int currentId) {
	return (currentId == 0) ? numprocs - 1 : currentId - 1;
}

int main(int argc, char ** argv) {
	int numprocs, myId;
	vector<int> values;
	MPI_Status status;            //struct- obsahuje kod- source, tag, error
	char input[]= "numbers";                          //jmeno souboru

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);       // zjistíme, kolik procesů běží
	MPI_Comm_rank(MPI_COMM_WORLD, &myId);           // zjistíme id svého procesu

	cout << "Pocet procesu: " << numprocs << endl;

	int local_counts[numprocs], offsets[numprocs];
	ifstream fileNumbers (input, ios::in|ios::binary|ios::ate);
	if (fileNumbers.is_open()) {
		int size = fileNumbers.tellg();
		int remainder = size % numprocs;
		int sum = 0;
		for (int i = 0; i < numprocs; i++) {
		    local_counts[i] = size / numprocs;
		    if (remainder > 0) {
		        local_counts[i] += 1;
		        remainder--;
		    }
		    offsets[i] = sum;
		    sum += local_counts[i];
		}	

	} else {
		cerr << "Nepodarilo sa otvorit subor";
		return 1;
	}

	int localArray[local_counts[myId]];

	if (myId == 0) {
		int number;                                     //hodnota pri nacitani souboru
		fstream fin;                                    //cteni ze souboru
		fin.open(input, ios::in);
		while(fin.good()){
			number= fin.get();
			if(!fin.good()) break;                      //nacte i eof, takze vyskocim
			values.push_back(number);

		}
		fin.close();
		printVector(values);
	}

	int *original_array = &values[0];
    MPI_Scatterv(original_array, local_counts, offsets, MPI_INT, localArray, local_counts[myId], MPI_INT, TAG, MPI_COMM_WORLD);

	vector<int> finalValues(localArray, localArray + local_counts[myId]);
	// cout << "\nBefore: \n";
	// printVector(finalValues);
	printf("\nAfter in %d: \n", myId);
	sort(finalValues.begin(), finalValues.end());
	printVector(finalValues);

	vector<int> receivedValues;

	for(int j = 1; j <= numprocs / 2; j++) {
		if (myId % 2 != 0) {
			//lichy odosielam
			int nextId = getNextId(numprocs, myId);
			MPI_Send(&finalValues[0], finalValues.size(), MPI_INT, nextId, TAG, MPI_COMM_WORLD);
			printf("Sent from id: %d \n", myId);
			printVector(finalValues);

			finalValues.resize(local_counts[nextId]);
			MPI_Recv(&finalValues[0], local_counts[nextId], MPI_INT, nextId, TAG, MPI_COMM_WORLD, &status);			
		} else {
			// prijmam od sudeho v lichom
			int previousId = getPreviousId(numprocs, myId);
			receivedValues.resize(local_counts[previousId]);
			MPI_Recv(&receivedValues[0], local_counts[previousId], MPI_INT, previousId, TAG, MPI_COMM_WORLD, &status);
			printf("Received in id: %d from %d \n", myId, previousId);
			printVector(receivedValues);

			vector<int> mergedVector;
			mergedVector.resize(finalValues.size() + receivedValues.size());
			merge(finalValues.begin(), finalValues.end(), receivedValues.begin(), receivedValues.end(), mergedVector.begin());
			printf("Merged in id: %d from %d \n", myId, previousId);
			printVector(mergedVector);

			vector<int> splittedLower(mergedVector.begin(), mergedVector.begin() + local_counts[previousId]);
			vector<int> splittedHigher(mergedVector.begin() + local_counts[previousId], mergedVector.end());

			MPI_Send(&splittedLower[0], splittedLower.size(), MPI_INT, previousId, TAG, MPI_COMM_WORLD);
			finalValues = splittedHigher;
		}

		if (myId % 2 == 0) {
			//sudy odosielam
			int nextId = getNextId(numprocs, myId);
			MPI_Send(&finalValues[0], finalValues.size(), MPI_INT, nextId, TAG, MPI_COMM_WORLD);
			printf("Sent from id: %d \n", myId);
			printVector(finalValues);

			finalValues.resize(local_counts[nextId]);
			MPI_Recv(&finalValues[0], local_counts[nextId], MPI_INT, nextId, TAG, MPI_COMM_WORLD, &status);	

		} else {
			//prijmam od licheho v sudom
			int previousId = getPreviousId(numprocs, myId);
			receivedValues.resize(local_counts[previousId]);
			MPI_Recv(&receivedValues[0], local_counts[previousId], MPI_INT, previousId, TAG, MPI_COMM_WORLD, &status);
			printf("Received in id: %d from %d \n", myId, previousId);
			printVector(receivedValues);

			vector<int> mergedVector;
			mergedVector.resize(finalValues.size() + receivedValues.size());
			merge(finalValues.begin(), finalValues.end(), receivedValues.begin(), receivedValues.end(), mergedVector.begin());
			printf("Merged in id: %d from %d \n", myId, previousId);
			printVector(mergedVector);

			vector<int> splittedLower(mergedVector.begin(), mergedVector.begin() + local_counts[previousId]);
			vector<int> splittedHigher(mergedVector.begin() + local_counts[previousId], mergedVector.end());

			MPI_Send(&splittedLower[0], splittedLower.size(), MPI_INT, previousId, TAG, MPI_COMM_WORLD);
			finalValues = splittedHigher;
		}
	}

	MPI_Gatherv(&finalValues[0], local_counts[myId], MPI_INT, original_array, local_counts, offsets, MPI_INT, TAG, MPI_COMM_WORLD);

	if (myId == 0) {
		for (int i = 0; i<12; i++) {
			printf(" %d", original_array[i]);
		}
		cout << '\n';
	}
	// vector<int> sortedVector(original_array, original_array + sizeof original_array  / sizeof original_array[0]);
	// printf("Sorted vector: \n");


	MPI_Finalize();
	return 0;
}
#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm> 
#include <math.h>

#define TAG 0
// #define DEBUG 1

using namespace std;

/**
    Vypis vektora.

    @param vector<int> Vektor integerov.
    @return void
*/
void printVector(vector<int> vec) {
	for(int i = 0; i < vec.size(); i++)
	{
		if(vec[i] == -1) break;
		cout << vec[i] << " ";
	}
	cout << endl;
}

/**
    Zistenie nasledujuceho id procesoru

    @param numprocs Celkovy pocet procesorov
    @param currentId Cislo aktualneho procesoru
    @return int
*/
int getNextId(int numprocs, int currentId) {
	return (currentId == numprocs - 1) ? 0 : currentId + 1;
}

/**
    Zistenie predchadzajuceho id procesoru

    @param numprocs Celkovy pocet procesorov
    @param currentId Cislo aktualneho procesoru
    @return int
*/
int getPreviousId(int numprocs, int currentId) {
	return (currentId == 0) ? numprocs - 1 : currentId - 1;
}

int main(int argc, char ** argv) {
	int numprocs, myId;
	vector<int> values;
	MPI_Status status;	//struct- obsahuje kod- source, tag, error
	char input[]= "numbers";	//meno suboru
	double startTime, endTime;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);       // zistime pocet procesorov
	MPI_Comm_rank(MPI_COMM_WORLD, &myId);           // zistime id svojho procesoru

#ifdef DEBUG
	cout << "Pocet procesu: " << numprocs << endl;
#endif

	// Vypocet rozdelenia vstupnych hodnot medzi vsetky procesory.
	int localCounts[numprocs], offsets[numprocs]; // Kazdy procesor ma vlastne polia
	ifstream fileNumbers (input, ios::in|ios::binary|ios::ate);
	if (fileNumbers.is_open()) {
		int size = fileNumbers.tellg();
		int remainder = size % numprocs;
		int sum = 0;
		for (int i = 0; i < numprocs; i++) {
		    localCounts[i] = size / numprocs;
		    if (remainder > 0) {
		        localCounts[i] += 1;
		        remainder--;
		    }
		    offsets[i] = sum;
		    sum += localCounts[i];
		}	

	} else {
		cerr << "Nepodarilo sa otvorit subor";
		return 1;
	}

	// Kazdy procesor si vytvori potrebnu velkost pola.
	int localArray[localCounts[myId]];

	if (myId == 0) {
		int number; 	//hodnota zo suboru
		fstream fin; 	//citania zo suboru
		fin.open(input, ios::in);
		while(fin.good()){
			number= fin.get();
			if(!fin.good()) break;		//necitame eof
			values.push_back(number);
		}
		fin.close();
		printVector(values);
	}

	startTime = MPI_Wtime();	//pociatocny cas
	int *originalArray = &values[0];

	// Rozdistribuovanie hodnot medzi vsetky procesory na zaklade vypocitanych offsetov.
	// originalArray - vsetky radene prvky
	// localCounts - pocet prvkov kolko treba poslat kazdemu procesoru
	// offsets - offset, podla ktoreho treba brat prvky pre kazdy procesor z originalArray
	// localCounts[myId] - pocet prvkov daneho procesora
    MPI_Scatterv(originalArray, localCounts, offsets, MPI_INT, localArray, localCounts[myId], MPI_INT, TAG, MPI_COMM_WORLD);

	vector<int> finalValues(localArray, localArray + localCounts[myId]);
	
	sort(finalValues.begin(), finalValues.end());
	
#ifdef DEBUG
	printf("\nAfter in %d: \n", myId);
	printVector(finalValues);
#endif

	vector<int> receivedValues;		//ziskane hodnoty od susedneho procesoru

	for(int j = 1; j <= numprocs / 2; j++) {
		if (myId % 2 != 0) {
			//odosielam data z licheho procesoru
			int nextId = getNextId(numprocs, myId);
			MPI_Send(&finalValues[0], finalValues.size(), MPI_INT, nextId, TAG, MPI_COMM_WORLD);

#ifdef DEBUG
			printf("Sent from id: %d \n", myId);
			printVector(finalValues);
#endif

			//prijmam spracovane data zo susedneho procesoru
			MPI_Recv(&finalValues[0], finalValues.size(), MPI_INT, nextId, TAG, MPI_COMM_WORLD, &status);			
		} else {
			//v sudom procesora prijmam data od susedneho procesoru
			int previousId = getPreviousId(numprocs, myId);
			receivedValues.resize(localCounts[previousId]);
			MPI_Recv(&receivedValues[0], localCounts[previousId], MPI_INT, previousId, TAG, MPI_COMM_WORLD, &status);

#ifdef DEBUG
			printf("Received in id: %d from %d \n", myId, previousId);
			printVector(receivedValues);
#endif

			vector<int> mergedVector;
			mergedVector.resize(finalValues.size() + receivedValues.size());
			//Spojim dve zoradane postupnosti do postupnosti o dvojnasobnej dlzke
			merge(finalValues.begin(), finalValues.end(), receivedValues.begin(), receivedValues.end(), mergedVector.begin());

#ifdef DEBUG
			printf("Merged in id: %d from %d \n", myId, previousId);
			printVector(mergedVector);
#endif

			if (myId == 0) { //ak sa nachadzam v prvom procesore, mensia cast zostane tu, vacsiu cast vratim spat na posledny procesor
				vector<int> splittedLower(mergedVector.begin(), mergedVector.begin() + localCounts[myId]);
				vector<int> splittedHigher(mergedVector.begin() + localCounts[myId], mergedVector.end());

#ifdef DEBUG
				printf("Splitted lower in id: %d from %d \n", myId, previousId);
				printVector(splittedLower);

				printf("Splitted higher in id: %d from %d \n", myId, previousId);
				printVector(splittedHigher);
#endif

				MPI_Send(&splittedHigher[0], splittedHigher.size(), MPI_INT, previousId, TAG, MPI_COMM_WORLD);
				finalValues = splittedLower;
			} else { //ak sa nenachadzam v prvom procesore, mensia cast sa vrati povodnemu, vacsia zostava tu
				vector<int> splittedLower(mergedVector.begin(), mergedVector.begin() + localCounts[previousId]);
				vector<int> splittedHigher(mergedVector.begin() + localCounts[previousId], mergedVector.end());

#ifdef DEBUG
				printf("Splitted lower in id: %d from %d \n", myId, previousId);
				printVector(splittedLower);

				printf("Splitted higher in id: %d from %d \n", myId, previousId);
				printVector(splittedHigher);
#endif

				MPI_Send(&splittedLower[0], splittedLower.size(), MPI_INT, previousId, TAG, MPI_COMM_WORLD);
				finalValues = splittedHigher;
			}

		}

		if (myId % 2 == 0) {
			//odosielam data zo sudeho procesoru
			int nextId = getNextId(numprocs, myId);
			MPI_Send(&finalValues[0], finalValues.size(), MPI_INT, nextId, TAG, MPI_COMM_WORLD);

#ifdef DEBUG
			printf("Sent from id: %d \n", myId);
			printVector(finalValues);
#endif

			//prijmam spracovane data zo susedneho procesoru
			MPI_Recv(&finalValues[0], finalValues.size(), MPI_INT, nextId, TAG, MPI_COMM_WORLD, &status);	
		} else {
			//v lichom procesore prijmam data
			int previousId = getPreviousId(numprocs, myId);
			receivedValues.resize(localCounts[previousId]);
			MPI_Recv(&receivedValues[0], localCounts[previousId], MPI_INT, previousId, TAG, MPI_COMM_WORLD, &status);

#ifdef DEBUG
			printf("Received in id: %d from %d \n", myId, previousId);
			printVector(receivedValues);
#endif

			vector<int> mergedVector;
			mergedVector.resize(finalValues.size() + receivedValues.size());
			//Spojim dve zoradane postupnosti do postupnosti o dvojnasobnej dlzke
			merge(finalValues.begin(), finalValues.end(), receivedValues.begin(), receivedValues.end(), mergedVector.begin());

#ifdef DEBUG
			printf("Merged in id: %d from %d \n", myId, previousId);
			printVector(mergedVector);
#endif
			vector<int> splittedLower(mergedVector.begin(), mergedVector.begin() + localCounts[previousId]);
			vector<int> splittedHigher(mergedVector.begin() + localCounts[previousId], mergedVector.end());

#ifdef DEBUG
			printf("Splitted lower in id: %d from %d \n", myId, previousId);
			printVector(splittedLower);

			printf("Splitted higher in id: %d from %d \n", myId, previousId);
			printVector(splittedHigher);
#endif
			MPI_Send(&splittedLower[0], splittedLower.size(), MPI_INT, previousId, TAG, MPI_COMM_WORLD);
			finalValues = splittedHigher;
		}
	}

	//finalValues - pole prvkov procesora
	//localCounts[myId] - pocet prvkov daneho procesora
	//localCounts - pole poctu prvkov kolko treba prijat od kazdeho procesora
	//offsets - offset, ktory urcuje miesto v originalArray kde sa ulozia prvky z kazdeho procesoru
	MPI_Gatherv(&finalValues[0], localCounts[myId], MPI_INT, originalArray, localCounts, offsets, MPI_INT, TAG, MPI_COMM_WORLD);

	if (myId == 0) {
		endTime = MPI_Wtime();
		for (int i = 0; i<values.size(); i++) {
			printf("%d\n", originalArray[i]);
		}
#ifdef DEBUG
		cerr << endTime-startTime;
#endif
	}

	MPI_Finalize();
	return 0;
}
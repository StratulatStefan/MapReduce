#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <stack>
#include <queue>
#include <fstream>
#include <chrono>
#include "GeneralPurposeHeader.h"
#include "mpi.h"

#define numberOfMasters 1
#define numberOfWorkers 1
#define masterRank 0
#define MAP 0
#define REDUCE 1
#define FREE 2
#define STOP -1


enum ProcessType { MASTER, WORKER };
enum MapReduceStep {MAP_STEP, REDUCE_STEP};


extern int mpiProcessesCount;
extern int mpiProcessRank;
extern ProcessType mpiProcessType;
extern char* inputDirectory;
extern char* outputDirectory;



namespace MR_MPI_Init {
	/* Functie care realizeaza initializarea MPI si returneza rank-ul procesului
	curent si numarul de procese, sub forma unei perechi (pair).*/
	pair<int, int> MpiInit(int argc, char* argv[]);

	/* Functie care realizeaza finalizarea MPI.*/
	void MpiSafeQuit(int status_code);
}

namespace MR_MPI_Validation {
	/* Functie care afiseaza identitatea procesului curent (Tipul de proces si rank-ul).*/
	void CheckProcessIdentity();

	/* Functie care verifica daca numarul de procese furnizat ca parametru
	la linia de comanda este potrivit.*/
	bool ValidateProcessesNumber(int size);
}

namespace MR_MPI_Display {
	/* Functie care afiseaza un mesaj de eroare. Mesajul va fi afisat de un singur
proces, identificat prin processRank. Asadar, daca procesul curent nu este acelasi
cu processRank, nu va putea afisa mesajul.*/
	void DisplayErrorMessage(const char* message, int processRank);
}

namespace MR_MPI_MASTER {
	/* Functie care realizeaza distribuirea sarcinilor coresp. maparii catre workeri.*/
	vector<pair<int, char*>> DistributeMapWork(vector<string> filenames);

	/* Functie care realizeaza distribuirea sarcinilor coresp. reducerii catre workeri.*/
	vector<pair<int, char*>> DistributeReduceWork(vector<string> filenames);

	/* Functie care trimite un mesaj unui proces si asteapta un raspuns.
	Aceasta functie va fi apelata in interiorul unui thread, intrucat nu se doreste
	blocarea programului cu asteptarea confirmarii pt mesajul curent.
	De asemenea, functia va modifica vectorul status astfel incat threadul principal
	sa detecteze finalizarea executiei de catre threadul care va apela aceasta functie.*/
	void SendAndWaitResponse(vector<pair<int, char*>>* status, deque<int>* queue, char* message, int destinationRank);

	/* Functie care verifica daca pasul de mapare s-a realizat cu succes. Se are in vedere ca rezultatul
	etapei de mapare sa aiba aceeasi dimensiune cu nr. de fisiere date ca intrare si ca fisierul sa existe. */
	bool MapStepSuccessCheck(vector<pair<int, char*>> status, vector<string> filenames);

	/* Functie care extrage numele fisierelor din rezultatul maparii. */
	vector<string> ExtractFilenamesFromMapResult(vector<pair<int, char*>> map_results);

}

namespace MR_MPI_WORKER {
	/* Functie caracteristica workerului, folosita in bucla de receptie a mesajelor.
	Acesta poate primi 4 tipuri de mesaje : MAP, REDUCE, free and STOP */
	int InterpretMessage(char message[BUFFER_SIZE]);

	/* Functie care creeaza calea de iesire pentru fisierul dat.
	input  : <director_intrare>/ana.txt
	output : <director_iesire>/ana.txt
	*/
	char* CreateOuputPath(char* filename);

	/* Functie care creeaza calea pentru directorul auxiliar in care vor fi salvate
	fisierele intermediate rezultate in urma procesului de mapare.
	input  : <director_intrare>/ana.txt
	output : intermediar/ana.txt
	*/
	char* CreateAuxPath(char* filename);

	/* Functie care sorteaza rezultatul matching-ului in ordine alfabetica, dupa cuvant */
	void AlphabeticalSort(deque<pair<string, int>>* matching);


	/* Functie care realizeaza etapa de mapare */
	void WorkerMapStep(char message[BUFFER_SIZE]);

	/* Functie care realizeaza etapa de mapare */
	void WorkerReduceStep(char message[BUFFER_SIZE]);

	/* Functie care executa maparea efectiva. Mai precis, se numara de cate ori apare fiecare cuvant intr-un fisier.*/
	deque<pair<string, int>> CountMapOccurences(deque<string> tokens);

	/* Functie care executa maparea efectiva. Mai precis, se numara de cate ori apare fiecare cuvant intr-un fisier.*/
	deque<pair<string, deque<pair<string, int>>>> CountReduceOccurences(deque<string> tokens);

	/* Functie care transforma dictionarul de matching (cuvant-contor) in formatul
	specific pentru scrierea in fisier.*/
	vector<string> ParseToMapFileFormat(string filename, deque<pair<string, int>> matching);

	/* Functie care transforma dictionarul de matching (cuvant-{fisier:contor}) in formatul
	specific pentru scrierea in fisier.*/
	vector<string> ParseToReduceFileFormat(deque<pair<string, deque<pair<string, int>>>> reduce_result);

	/* Functie care realizeaza procesarea textului, astfel incat sa se elimine caracterele redundante. */
	string TextPreprocessing(string text);

	/* Functie care transforma mesajul de reducere in componentele individuale necesare */
	deque<string> GetItemsForReduceProcess(string message);

	/* Functie care determina candidatii valizi pentru reducere, pe baza unui criteriu */
	deque<string> SelectValidsForReduce(deque<string> candidates, string criteria);
}



using namespace std;
using namespace MR_MPI_Init;
using namespace MR_MPI_Validation;
using namespace MR_MPI_Display;
using namespace MR_MPI_MASTER;
using namespace MR_MPI_WORKER;
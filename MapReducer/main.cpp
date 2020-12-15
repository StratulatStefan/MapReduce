#include "MapReduce.h"


int main(int argc, char* argv[]) {
	MPI_Status status;
	pair<int, int> mpiProcessParams = MpiInit(argc, argv);
	mpiProcessesCount = mpiProcessParams.first;
	mpiProcessRank = mpiProcessParams.second;

	if (!ValidateProcessesNumber(mpiProcessesCount)) {
		char invalidProcessNumber[] =
			"Invalid processes number.\n You should provide at least number of (Masters + Workers) processes";

		DisplayErrorMessage(invalidProcessNumber, masterRank);
		MpiSafeQuit(-1);
	}
	cout <<"\n---------------------------------------------------\n";
	
	CheckProcessIdentity();
	
	try {
		if (ValidateConsoleArgs(argc, argv))
			cout << ">>> Command line arguments are valid.\n";
	}
	catch (const char exception[]) {
		DisplayErrorMessage(exception, masterRank);
		MpiSafeQuit(-1);
	}

	if (mpiProcessType == MASTER) {
		auto startTime = chrono::high_resolution_clock::now();
		inputDirectory = GetInputDirectory(argv);
		outputDirectory = GetOutputDirectory(argv);

		vector<string> filenames = GetFilenamesFromDirectory(inputDirectory);
		vector<pair<int, char*>> map_result = DistributeMapWork(filenames);
		if (!MapStepSuccessCheck(map_result, filenames)) {
			char error[BUFFER_SIZE] = "Eroare la procesul de mapare!\n";
			DisplayErrorMessage(error, masterRank);
			MpiSafeQuit(-1);
		}
		cout << "\n\nProcesul de mapare a fost realizat cu succes!\n";
		auto mapStopTime = chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = mapStopTime - startTime;
		cout << "[**************************************]\n";
		cout << "[************** MAP time : " << elapsed.count() << " sec **************]\n";
		cout << "[**************************************]\n";

		filenames = ExtractFilenamesFromMapResult(map_result);
		vector<pair<int, char*>> reduce_result = DistributeReduceWork(filenames);
		auto stopTime = chrono::high_resolution_clock::now();
		elapsed = stopTime - mapStopTime;
		cout << "[**************************************]\n";
		cout << "[************** REDUCE time : " << elapsed.count() << " sec **************]\n";
		cout << "[**************************************]\n";

		elapsed = stopTime - startTime;
		cout << "[**************************************]\n";
		cout << "[************** TOTAL time : " << elapsed.count() << " sec **************]\n";
		cout << "[**************************************]\n";

	}
	else if (mpiProcessType == WORKER) {
		char message[BUFFER_SIZE];
		while (1) {
			MPI_Recv(message, BUFFER_SIZE, MPI_CHAR, masterRank, 99, MPI_COMM_WORLD, &status);
			if (InterpretMessage(message) == MAP)
				WorkerMapStep(message);
			if (InterpretMessage(message) == REDUCE)
				WorkerReduceStep(message);
			if (InterpretMessage(message) == FREE)
				MPI_Send(message, BUFFER_SIZE, MPI_CHAR, masterRank, 99, MPI_COMM_WORLD);
			if (InterpretMessage(message) == STOP)
				break;
		}
	}

	cout << "\n---------------------------------------------------\n";
	MpiSafeQuit(0);
}
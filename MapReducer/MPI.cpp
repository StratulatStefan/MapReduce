#include "MapReduce.h"

int mpiProcessesCount;
int mpiProcessRank;
ProcessType mpiProcessType;
char* inputDirectory;
char* outputDirectory;

namespace MR_MPI_Init {
	pair<int, int> MpiInit(int argc, char* argv[]) {
		int rank, size;
		int provided;
		MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		MPI_Comm_size(MPI_COMM_WORLD, &size);
		return pair<int, int>{ size, rank };
	}

	void MpiSafeQuit(int status_code) {
		MPI_Finalize();
		exit(status_code);
	}
}


namespace MR_MPI_Display {
	void DisplayErrorMessage(const char* message, int processRank) {
		if (mpiProcessRank == processRank) {
			cout << "[Error] >>> " << message << "\n\n";
		}
	}
}


namespace MR_MPI_Validation {
	void CheckProcessIdentity() {
		mpiProcessType = (mpiProcessRank == masterRank) ? MASTER : WORKER;
		if (mpiProcessType == MASTER) {
			cout << "[MASTER]\n";
		}
		else {
			cout << "[WORKER " << mpiProcessRank << "]\n";
		}
	}

	bool ValidateProcessesNumber(int size) {
		/* Se impune sa avem cel putin NumberOfMasters + NumberOfWorkers procese */
		if (size < numberOfMasters + numberOfWorkers) {
			return false;
		}
		return true;
	}
}


namespace MR_MPI_MASTER {
	void SendAndWaitResponse(vector<pair<int, char*>>* status, deque<int>* queue, char* message, int destinationRank) {
		std::cout << "[MASTER] >>> Trimitem carte worker-ul " << destinationRank << " : " << message << std::endl;
		MPI_Send(message, BUFFER_SIZE, MPI_CHAR, destinationRank, 99, MPI_COMM_WORLD);
		MPI_Status mpiStatus;
		char msg[BUFFER_SIZE];
		MPI_Recv(&msg, BUFFER_SIZE, MPI_CHAR, destinationRank, 99, MPI_COMM_WORLD, &mpiStatus);

		string result(msg);
		std::cout << "[MASTER] >>> Am primit raspuns de la worker-ul " << destinationRank << " : " << msg << std::endl;
		status->push_back(pair<int, char*>{destinationRank, CharArrayDeepCopy(msg)});
		queue->push_back(destinationRank);
	}

	vector<pair<int, char*>> DistributeMapWork(vector<string> filenames) {
		vector<thread> threadPool = vector<thread>();
		vector<pair<int, char*>> statuss = vector<pair<int, char*>>();
		stack<string> filenamesStack = stack<string>();
		deque<int> workerRanksQueue = deque<int>();

		for (string filename : filenames)
			filenamesStack.push(filename);

		for (int i = 0; i < mpiProcessesCount; i++)
			if (i != masterRank)
				workerRanksQueue.push_back(i);
		
		while (filenamesStack.size() > 0) {
			while (workerRanksQueue.size() == 0);

			string filename = filenamesStack.top(); 
			filenamesStack.pop();

			int workerRank = workerRanksQueue.front();
			workerRanksQueue.pop_front();

			filename = "[MAP] " + string(filename);
			char* charArrayFileName = StringToCharArray(filename);
			threadPool.push_back(thread(SendAndWaitResponse, &statuss, &workerRanksQueue, charArrayFileName, workerRank));
		}

		for (thread& thr : threadPool) {
			thr.join();
		}

		while (workerRanksQueue.size() > 0) {
			int workerRank = workerRanksQueue.front();
			workerRanksQueue.pop_front();
			char free[BUFFER_SIZE] = "[FREE]";
			MPI_Send(free, BUFFER_SIZE, MPI_CHAR, workerRank, 99, MPI_COMM_WORLD);
			MPI_Status status;
			MPI_Recv(free, BUFFER_SIZE, MPI_CHAR, workerRank, 99, MPI_COMM_WORLD, &status);
		}

		/*char stop[BUFFER_SIZE] = "[STOP]";
		for (int workerRank = 0; workerRank < mpiProcessesCount; workerRank++) {
			if (workerRank == masterRank)
				continue;
			MPI_Send(stop, BUFFER_SIZE, MPI_CHAR, workerRank, 99, MPI_COMM_WORLD);
		}*/

		return statuss;
	}

	vector<pair<int, char*>> DistributeReduceWork(vector<string> filenames) {
		vector<thread> threadPool = vector<thread>();
		vector<pair<int, char*>> statuss = vector<pair<int, char*>>();
		stack<string> operationsStack = stack<string>();
		deque<int> workerRanksQueue = deque<int>();
		string operation_type = "[REDUCE] ";

		for (char character = 'a'; character <= 'z'; character++) {
			string operation = string(1, character) + string(" ");
			for (string filename : filenames)
				operation += string(1, '|') + filename;
			operation += string(1, '|');
			operationsStack.push(operation);
		}

		
		for (int i = 0; i < mpiProcessesCount; i++)
			if (i != masterRank)
				workerRanksQueue.push_back(i);

		while (operationsStack.size() > 0) {
			while (workerRanksQueue.size() == 0);

			string operation = operationsStack.top();
			operationsStack.pop();

			int workerRank = workerRanksQueue.front();
			workerRanksQueue.pop_front();

			operation = operation_type + string(operation);
			char* charArrayOperation = StringToCharArray(operation);
			threadPool.push_back(thread(SendAndWaitResponse, &statuss, &workerRanksQueue, charArrayOperation, workerRank));
		}

		for (thread& thr : threadPool) {
			thr.join();
		}

		while (workerRanksQueue.size() > 0) {
			int workerRank = workerRanksQueue.front();
			workerRanksQueue.pop_front();
			char free[BUFFER_SIZE] = "[FREE]";
			MPI_Send(free, BUFFER_SIZE, MPI_CHAR, workerRank, 99, MPI_COMM_WORLD);
			MPI_Status status;
			MPI_Recv(free, BUFFER_SIZE, MPI_CHAR, workerRank, 99, MPI_COMM_WORLD, &status);
		}


		char stop[BUFFER_SIZE] = "[STOP]";
		for (int workerRank = 0; workerRank < mpiProcessesCount; workerRank++) {
			if (workerRank == masterRank)
				continue;
			MPI_Send(stop, BUFFER_SIZE, MPI_CHAR, workerRank, 99, MPI_COMM_WORLD);
		}

		return statuss;
	}

	bool MapStepSuccessCheck(vector<pair<int, char*>> status, vector<string> filenames) {
		if (status.size() != filenames.size())
			return false;

		for (pair<int, char*> pereche : status) {
			if (!FileExists(pereche.second)) {
				cout << ">>> Fisierul " << pereche.second << "nu exista! Eroare la procesul " << pereche.first <<endl;
				return false;
			}
		}
		return true;
	}

	vector<string> ExtractFilenamesFromMapResult(vector<pair<int, char*>> map_results) {
		vector<string> filenames = vector<string>();
		for (pair<int, char*> map_result : map_results)
			filenames.push_back(string(map_result.second));
		return filenames;
	}
}


namespace MR_MPI_WORKER {
	int InterpretMessage(char message[BUFFER_SIZE]) {
		string msg = message;
		if (msg.find("FREE") != std::string::npos)
			return FREE;
		if (msg.find("MAP") != std::string::npos)
			return MAP;
		if (msg.find("STOP") != std::string::npos)
			return STOP;
		if (msg.find("REDUCE") != std::string::npos)
			return REDUCE;
		return -2;
	}
	
	char* CreateOuputPath(char* filename) {
		string message = string(filename);
		string delimiter = "\\";
		string fname = string("output") +  message.substr(message.find(delimiter), message.size());
		return StringToCharArray(fname);
	}

	char* CreateAuxPath(char* filename) {
		string message = string(filename);
		string delimiter = "\\";
		string fname = string("intermediar\\aux_") + message.substr(message.find(delimiter) + 1, message.size());
		return StringToCharArray(fname);
	}

	deque<pair<string, int>> CountMapOccurences(deque<string> tokens) {
		deque<pair<string, int>> stringsMap = deque<pair<string, int>>();
		while (tokens.size() > 0) {
			string candidate = tokens.front();
			tokens.pop_front();
			if (candidate.compare(string("0")) == 0)
				continue;
			int counter = 1;
			for (int index = 0; index < tokens.size(); index++) {
				if (candidate.compare(tokens[index]) == 0 && tokens[index].compare(string("0")) != 0) {
					counter += 1;
					tokens[index] = string("0");
				}
			}
			if(candidate.size() > 1 || candidate.compare("a") == 0 )
				stringsMap.push_back(pair<string, int>{ candidate, counter });
		}
		return stringsMap;
	}

	deque<pair<string, deque<pair<string, int>>>> CountReduceOccurences(deque<string> tokens) {
		deque<pair<string, deque<pair<string, int>>>> match = deque<pair<string, deque<pair<string, int>>>>();
		deque<pair<string, pair<string, int>>> tokensParsed = deque<pair<string, pair<string, int>>>();
		size_t cursor; 
		string word;
		for (string token : tokens) {
			token = token.substr(1);
			token.pop_back();
			cursor = token.find(" ");
			string word = token.substr(0, cursor);
			
			cursor = token.find("{");
			token = token.substr(cursor + 1);
			token.pop_back();
			cursor = token.find(":");
			string filename = token.substr(0, cursor - 1);
			
			token = token.substr(cursor + 2);
			int contor = stoi(token);
			tokensParsed.push_back(pair{ word, pair{filename, contor} });
		}

		while (tokensParsed.size() > 0) {
			pair<string, pair<string, int>> candidate = tokensParsed.front();
			tokensParsed.pop_front();
			if (candidate.first.compare(string("0")) == 0)
				continue;
			deque<pair<string, int>> friends = deque<pair<string, int>>();
			friends.push_back(candidate.second);
			for (int index = 0; index < tokensParsed.size(); index++) {
				if (candidate.first.compare(tokensParsed[index].first) == 0 && tokensParsed[index].first.compare(string("0")) != 0) {
					friends.push_back(tokensParsed[index].second);
					tokensParsed[index].first = string("0");
				}
			}
			match.push_back(pair{ candidate.first, friends });
		}
		return match;
	}

	vector<string> ParseToMapFileFormat(string filename, deque<pair<string, int>> matching) {
		vector<string> fileformat = vector<string>();
		for (pair<string, int> match : matching) {
			fileformat.push_back(string("<") + match.first + string(" , ")
				     + string("{") + filename + string(" : ") + to_string(match.second)
					 + string("}")
			         + string(">"));
		}
		return fileformat;
	}

	vector<string> ParseToReduceFileFormat(deque<pair<string, deque<pair<string, int>>>> reduce_result) {
		vector<string> fileformat = vector<string>();
		for (pair<string, deque<pair<string, int>>> match : reduce_result) {
			string format = string("<");
			format += match.first + string(" , "); 
			for (pair<string, int> file : match.second)
				format += string("{") + file.first + string(" : ") + to_string(file.second) + string("}");
			format += string(">");
			fileformat.push_back(format);
		}
		return fileformat;
	}

	void AlphabeticalSort(deque<pair<string, int>> *matching) {
		int i, j;
		for (i = 0; i < (*matching).size(); i++) {
			for (j = 0; j < (*matching).size() - i - 1; j++) {
				if ((*matching)[j].first.compare((*matching)[j+1].first) > 0) {
					pair<string, int> temp = (*matching)[j];
					(*matching)[j] = (*matching)[j + 1];
					(*matching)[j + 1] = temp;
				}
			}
		}
	}

	void WorkerMapStep(char message[BUFFER_SIZE]) {
		cout << ">>> Faza de mapare <<<\n";
		string filename = string(message);
		filename = filename.substr(filename.find(" ") + 1, filename.size());
		string fileContent = ReadFromFile(filename);
		fileContent = TextPreprocessing(fileContent);

		deque<string> individualItems = SplitString(fileContent, " ");
		deque<pair<string, int>> matching = CountMapOccurences(individualItems);
		AlphabeticalSort(&matching);
		vector<string> fileformat = ParseToMapFileFormat(filename, matching);

		char* fname = CreateAuxPath(message);
		WriteToFile(fname, fileformat);
		MPI_Send(fname, BUFFER_SIZE, MPI_CHAR, masterRank, 99, MPI_COMM_WORLD);
		
		cout << ">>> Mapare realizata cu succes <<<\n\n\n";
	}

	void WorkerReduceStep(char message[BUFFER_SIZE]) {
		cout << "\n\n>>> Faza de reducere <<<\n";
		deque<string> items = GetItemsForReduceProcess(message);
		string searchCriteria = items.front();
		deque<string> valids = deque<string>();
		items.pop_front();
		for (string filename : items) {
			string fileContent = ReadFromFile(filename);
			deque<string> individualItems = SplitString(fileContent, "\n");
			deque<string> validCandidates = SelectValidsForReduce(individualItems, searchCriteria);
			for (string valid : validCandidates)
				valids.push_back(valid);
		}
		deque<pair<string, deque<pair<string, int>>>> matching = CountReduceOccurences(valids);
		
		vector<string> fileformat = ParseToReduceFileFormat(matching);

		char* fname = StringToCharArray("output\\" + searchCriteria + ".txt");
		WriteToFile(fname, fileformat);
		MPI_Send(fname, BUFFER_SIZE, MPI_CHAR, masterRank, 99, MPI_COMM_WORLD);

		cout << ">>> Reducere realizata cu succes <<<\n\n\n";
	}

	string TextPreprocessing(string text) {
		transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
		text.erase(remove(text.begin(), text.end(), '.'), text.end());
		text.erase(remove(text.begin(), text.end(), ','), text.end());
		text.erase(remove(text.begin(), text.end(), '('), text.end());
		text.erase(remove(text.begin(), text.end(), ')'), text.end());
		text.erase(remove(text.begin(), text.end(), '['), text.end());
		text.erase(remove(text.begin(), text.end(), ']'), text.end());
		text.erase(remove(text.begin(), text.end(), '{'), text.end());
		text.erase(remove(text.begin(), text.end(), '}'), text.end());
		text.erase(remove(text.begin(), text.end(), '?'), text.end());
		text.erase(remove(text.begin(), text.end(), '\"'), text.end());
		text.erase(remove(text.begin(), text.end(), '\''), text.end());
		text.erase(remove(text.begin(), text.end(), '*'), text.end());
		text.erase(remove(text.begin(), text.end(), '!'), text.end());
		text.erase(remove(text.begin(), text.end(), '&'), text.end());
		text.erase(remove(text.begin(), text.end(), '_'), text.end());
		text.erase(remove(text.begin(), text.end(), '#'), text.end());
		text.erase(remove(text.begin(), text.end(), '='), text.end());
		text.erase(remove(text.begin(), text.end(), '+'), text.end());
		text = regex_replace(text, regex("\\-{1,}"), " ");
		text = regex_replace(text, regex("\\s{2,}"), " ");
		return regex_replace(text, regex("\n"), " ");
	}

	deque<string> GetItemsForReduceProcess(string message) {
		deque<string> items = deque<string>();
		size_t spaceOccurence = message.find(" ");
		message = message.substr(spaceOccurence + 1);

		string individualCharacter = message.substr(0, 1);
		items.push_back(individualCharacter);

		spaceOccurence = message.find(" ");
		message = message.substr(spaceOccurence + 2);
		message.pop_back();
		cout << message << endl;

		deque<string> filenames = SplitString(message, string(1,'|'));
		for (string fname : filenames)
			items.push_back(fname);

		return items;
	}

	deque<string> SelectValidsForReduce(deque<string> candidates, string criteria) {
		deque<string> valids = deque<string>();
		size_t spacepos;
		for (string candidate : candidates) {
			string aux = candidate.substr(1);
			spacepos = aux.find(" ");
			aux = aux.substr(0, spacepos);
			if (aux.find(criteria) == 0)
				valids.push_back(candidate);
		}
		return valids;
	}



}
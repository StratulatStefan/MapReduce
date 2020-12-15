#include "GeneralPurposeHeader.h"

namespace MR_GeneralPurpose_Validation {
	bool ValidateConsoleArgs(int argc, char* argv[]) {
		using namespace MR_GeneralPurpose_Validation;
		/* Trebuie sa avem 3 argumente la linia de comanda.*/
		if (argc < 3) {
			const char exception[] = "Invalid args number. We recommend : <program.exe>  <input_dir> <output dir>";
			throw exception;
		}
		char* inputDirectory = argv[1];
		char* outputDirectory = argv[2];

		/* Verificam daca cele doua directoare exista.*/
		if (DirectoryExists(inputDirectory)) {
			/* Verificam daca toate fisierele din directorul input sunt fisiere text */
			if (!DirectoryContainsTxtFiles(inputDirectory))
				return false;
		}
		else
			return false;

		if (!DirectoryExists(outputDirectory))
			return false;

		return true;
	}

	bool DirectoryContainsTxtFiles(const char* directory) {
		vector<string> filenames = MR_GeneralPurpose_OSFiles::GetFilenamesFromDirectory(directory);
		for (string filename : filenames) {
			if (filename.find(".txt") == std::string::npos) {
				char errorMessage[BUFFER_SIZE];
				sprintf(errorMessage, "The file %s is not a txt file!", filename.c_str());
				throw errorMessage;
			}
		}
		return true;
	}

	bool DirectoryExists(const char* directory) {
		DWORD fileType = GetFileAttributesA(directory);
		if (fileType == INVALID_FILE_ATTRIBUTES) {
			char errorMessage[BUFFER_SIZE];
			sprintf(errorMessage, "The directory %s not found!", directory);
			throw errorMessage;
		}
		if (fileType & FILE_ATTRIBUTE_DIRECTORY)
			return true;
		char errorMessage[BUFFER_SIZE];
		sprintf(errorMessage, "The provided name (%s) is not a directory name!", directory);
		throw errorMessage;
	}

	bool FileExists(char* filename) {
		ifstream candidate(filename);
		return candidate.good();
	}
}


namespace MR_GeneralPurpose_Copy {
	char* CharArrayDeepCopy(char array[BUFFER_SIZE]) {
		char* copy = new char[BUFFER_SIZE];
		for (int i = 0; i < BUFFER_SIZE; i++)
			copy[i] = array[i];
		return copy;
	}
}


namespace MR_GeneralPurpose_OSFiles {
	vector<string> GetFilenamesFromDirectory(const char* directory) {
		vector<string> filenames = vector<string>();
		for (const auto& file : std::filesystem::directory_iterator(directory)) {
			string filename = file.path().string();
			filenames.push_back(filename);
		}
		return filenames;
	}

	char* GetInputDirectory(char* argv[]) {
		return argv[1];
	}

	char* GetOutputDirectory(char* argv[]) {
		return argv[2];
	}

	string ReadFromFile(string filename) {
		string fileContent = string();
		ifstream inputFile(filename);
		char c;
		while (inputFile.get(c))
			fileContent = fileContent + c;
		return fileContent;
	}

	void WriteToFile(char* filename, vector<string> content) {
		ofstream outputFile(filename);
		for(int index = 0; index < content.size(); index++){
			outputFile << content[index];
			if(index != content.size() - 1)
				outputFile << "\n";
		}
		outputFile.close();
	}

}


namespace MR_GeneralPurpose_Conversion {
	char* StringToCharArray(string str) {
		char* result = new char[BUFFER_SIZE];
		for (int i = 0; i < BUFFER_SIZE; i++) {
			if (i < str.size())
				result[i] = str[i];
			else
				result[i] = '\0';
		}
		return result;
	}

	deque<string> SplitString(string candidate, string delimiter) {
		deque<string> items = deque<string>();
		size_t* positions = new size_t[candidate.size()];
		positions[0] = -1;
		size_t position = candidate.find(delimiter, 0);
		int index = 1;
		while (position != string::npos) {
			positions[index++] = position;
			position = candidate.find(delimiter, position + 1);
		}
		size_t length = 0;
		for (int indexPozitie = 0; indexPozitie < index; indexPozitie++) {
			if (indexPozitie == index - 1)
				length = candidate.size();
			else
				length = positions[indexPozitie + 1];
			length -= positions[indexPozitie];
			items.push_back(candidate.substr(positions[indexPozitie] + 1, length - 1));
		}
		if (items.size() == 1) {
			if (items.front().compare("") == 0) {
				items.pop_front();
			}
		}
		return items;
	}
}


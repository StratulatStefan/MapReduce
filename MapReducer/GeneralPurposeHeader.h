#pragma once
#include <stdio.h>
#include <iostream>
#include <windows.h>
#include <vector>
#include <regex>
#include <filesystem>
#include <fstream>
#include <string>
#include <stack>
#include <queue>
using namespace std;

#define BUFFER_SIZE 200

namespace MR_GeneralPurpose_Validation {
	/* Functie care valideaza daca parametrii furnizati la linia de comanda sunt corecti.
	Se impune furnizarea a doua argumente, anume doua nume de directoare, unul de input si
	unul de output.
	Se impune ca directorul de input sa contina doar fisiere cu extensia .txt.*/
	bool ValidateConsoleArgs(int argc, char* argv[]);

	/* Functie care verifica daca un director exista */
	bool DirectoryExists(const char* directory);

	/* Functie care verifica daca un director contine doar fisiere text */
	bool DirectoryContainsTxtFiles(const char* directory);

	/* Functie care verifica daca un fisier exista */
	bool FileExists(char* filename);
}


namespace MR_GeneralPurpose_OSFiles {
	/* Functie care returneaza toate numele fisierelor dintr-un director */
	vector<string> GetFilenamesFromDirectory(const char* directory);

	/* Functie care returneaza numele directorului de intrare, dintre argumentele furnizate
	la linia de comanda.*/
	char* GetInputDirectory(char* argv[]);

	/* Functie care returneaza numele directorului de iesire, dintre argumentele furnizate
	la linia de comanda.*/
	char* GetOutputDirectory(char* argv[]);

	/* Functie care citeste dintr-un fisier */
	string ReadFromFile(string filename);

	/* Functie care scrie intr-un fisier */
	void WriteToFile(char* filename, vector<string> content);
}


namespace MR_GeneralPurpose_Conversion {
	/* Functie care transforma un string intr-un char array */
	char* StringToCharArray(string str);

	/* Functie care face split-ul unui string pe baza unui character*/
	deque<string> SplitString(string candidate, string delimiter);
}


namespace MR_GeneralPurpose_Copy {
	/* Functie care face deep copy unui char array */
	char* CharArrayDeepCopy(char array[BUFFER_SIZE]);
}


using namespace MR_GeneralPurpose_Validation;
using namespace MR_GeneralPurpose_OSFiles;
using namespace MR_GeneralPurpose_Conversion;
using namespace MR_GeneralPurpose_Copy;
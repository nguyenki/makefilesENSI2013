#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include "sdmake.h"


string inputFile="";

int main( int argc, char* argv[])
{
	cout <<"Distributed MakeFile"<<"\n";
	int parseResult = getParameterCommandLine(argc,argv);
	if (parseResult!=1) {
		return -1;
	}
}


Rule *findRuleByName(const string &rule) {
	Rule* r = rules[rule];
	if (r==NULL) {
		return rules[rule] = new Rule(rule);
	} else {
		return r;
	}
}

void addDependency(Rule *rule, const string &dependencyName) {
	Rule *newDependency = rules[rule];
	rule->dependences.push_back(newDependency);
	newDependency->dependants.push_back(rule);
}

void parse(string &nameInputFile) {
	char* line[LINE_LENGTH];
	string ruleName="";
	Rule *rule;
	stringstream line_temp;	

	ifstream inFile;
	inFile.open(nameInputFile.c_str(), ifstream::in);
	if (inFile.bad()) {
		cerr <<"Bad bit is set in Makefile"<< endl;
		exit(BAD_FILE);
	}
	while (!inFile.eof()) {
		inFile.getline(line, LINE_LENGTH);
		line_temp << line;
		if (strlen(line)==0) // Laisse tomber la ligne vide
			continue;
		if (line[0]=='#') // Saute la ligne de commentaire
			continue;
		if (line[0]!='\t') { // Si la ligne ne commence pas avec un tabulation. Alors, c'est un regle
			line_temp >> ruleName;
			// Si ce n'est pas une ligne qui a un cible suivant par un : ==> ignore le.
			if (ruleName.length() <2 && ruleName[ruleName.size()]!=':')
				continue;
			ruleName = ruleName.substr(0, ruleName.size()-1);
			rule = findRuleByName(ruleName);
			string dependency;
			while (line_temp >> dependency) {
				addDependency(rule, dependency);
  			}
		} else {
			rule->command.push_back(line);
		}
	}
	inFile.close();
}

int getParameterCommandLine(int argc, char* argv[]) {
	if (argc<3) {
		cout <<"Usage: ./sdmake -nkt Makefile"<<endl;
		exit(0);
	} else {
		if (argv[1] != (string("-nkt")).c_str()) {
			cout << argv[1] <<" parameter doesn't exist\n"<<endl;
			cout <<"Usage: ./sdmake -nkt Makefile"<<endl;
			return -1;
		} else {
			string temp = argv[2];
			if (temp=="Makefile") {
				inputFile = temp;
				return 1;
			} else {
				return -1;
			}
		}
	}
}



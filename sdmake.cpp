#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <map>
#include "sdmake.h"


//string inputFile=PREMIER;
string inputFile="";
int main( int argc, char* argv[])
{
	cout <<"Distributed MakeFile"<<"\n";
	int parseResult = getParameterCommandLine(argc,argv);
	if (parseResult!=1) {
		return -1;
	}
//	openFile(inputFile);

	parse(inputFile);
	printAllRule(rules);

}


Rule *findRuleByName(const string &rule) {
	Rule* r = rules[rule];
	if (r==NULL) {
		cptRule++;
		Rule* newR = new Rule(rule);
		setIdRule(newR,cptRule);
		rules[rule] = newR;
		return newR;
	} else {
		return r;
	}
}

void addDependency(Rule *rule, const string &dependencyName) {
	Rule *newDependency = findRuleByName(dependencyName);
	rule->dependences.push_back(newDependency);
	newDependency->dependants.push_back(rule);
}

void parse(string &nameInputFile) {
	char line[LINE_LENGTH];
	string ruleName;
	Rule *rule;

	ifstream inFile;
	inFile.open(nameInputFile.c_str());
	if (inFile.bad()) {
		cerr <<"Bad bit is set in Makefile"<< endl;
		exit(BAD_FILE);
	}
	while (!inFile.eof()) {
		inFile.getline(line,LINE_LENGTH);
		stringstream line_temp(line);
		if (strlen(line)==0) // Laisse tomber la ligne vide
			continue;
		if (line[0]=='#') // Saute la ligne de commentaire
			continue;
		if (line[0]!= '\t') { // Si la ligne ne commence pas avec un tabulation. Alors, c'est un regle
			ruleName = "";
			line_temp >> ruleName;
			// Si ce n'est pas une ligne qui a un cible suivant par un : ==> ignore le.
			if (ruleName.length() <2 && ruleName[ruleName.length()]!=':')
				continue;
			ruleName = ruleName.substr(0, ruleName.size()-1);
			rule = findRuleByName(ruleName);
			string dependency;
			while (line_temp >> dependency) {
				addDependency(rule, dependency);
				cout <<"Dependecy of "<< rule->name<<" : "<< dependency <<"\n" << endl;
  			}
		} else {
			rule->command.push_back(line);
		}
	}
	inFile.close();
}

int getParameterCommandLine(int argc, char* argv[]) {
	string para;
	string path;
	if (argc<3) {
		cout <<"Usage: ./sdmake -tc Makefile"<<endl;
		exit(0);
	} else {
		para = argv[1];
		if (para != "-tc") {
			cout << argv[1] <<" parameter doesn't exist\n"<<endl;
			cout <<"Usage: ./sdmake -tc Makefile"<<endl;
			return -1;
		} else {
			string temp = argv[2];
			if (temp=="Makefile") {
//				std::ostringstream oss;
//				oss << inputFile << temp.c_str();
//				inputFile = oss.str();
				inputFile=temp;
				return 1;
			} else {
				return -1;
			}
		}
	}
}


/*
Methode pour tester les fonctions dans le program
*/

void printAllCommand(const vector<string> &cmd) {
	for (vector<string>::const_iterator c = cmd.begin(); c!=cmd.end(); ++c) {
		cout << (*c) <<endl;
	}
}

void printARule(Rule* rule) {
	cout <<"Id rule:   " << rule->idRule << "  Name:"<< rule->name <<"\n Command:" << endl;
	printAllCommand(rule->command);
	cout <<"\n" <<endl;
}

void printAllRule(map<string,Rule*> rules) {
	for (map<string,Rule*>::const_iterator r = rules.begin(); r!= rules.end(); ++r) {
		printARule(((*r).second));
	}
}


void executeCommand(Rule* rule) {
        vector<string> cmds = rule->command;
        for (vector<string>::const_iterator it = cmds.begin(); it!= 
cmds.end();$
                string cmd = (*it);
                cout<< "Running command:" << cmd << "\n" <<endl;
                system(cmd.c_str());
        }
}


void openFile(const string &path) {
	char line[LINE_LENGTH];
	std::ostringstream oss;
	oss << path.c_str() << "Makefile";
	ifstream inFile;
        inFile.open((oss.str()).c_str());
	while (!inFile.eof()) {
		inFile.getline(line, LINE_LENGTH);
	}
	cout << "SUCCEED" <<endl;
	inFile.close();
}

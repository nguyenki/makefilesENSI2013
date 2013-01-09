#ifndef __SDMAKE_H

#define __SDMAKE_H

#include <cstdlib>

#include <cstring>
#include <vector>
#include <iostream>
#include <list>
#include <map>

#include <mpi.h>

#define BAD_FILE -1
#define LINE_LENGTH 32687

using namespace std;

const string CORRECT_INPUT = "sdmake makefile_name";

/* Struct Rule pour presenter une ligne (une regle) du MakeFile
 * Une regle depend aux autre regles et a aussi des dependances
*/
struct Rule {
	int idRule;
	string name;
	vector<int> command; // La ligne de la commande suivant une regle
	bool isExecuted;
	bool isFinished;
	list<Rule*> dependences;
	list<Rule*> dependants;
	Rule (string ruleName): isExecuted(false), isFinished(false), name(ruleName) { }
};

map<string, Rule*> rules;
vector<Rule*> tasks;

// Chercher une regle using son nom. Retourner un nouveau regle si'il n'existe pas
Rule *findRuleByName(const string &rule);

// Ajouter un regle dependant d'un autre regle
void addDependency(Rule* rule, const string &dependencyName);

// Parser un fichier Makefile simple
void parse(string &nameInputFile);

// Lire la commande entree par l'utilisateur
int getParameterCommandLine(int argc, char* argv[]);

#endif

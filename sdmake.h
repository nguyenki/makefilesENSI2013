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
#define LINE_LENGTH 1024

using namespace std;

const string MATRIX = "TestPrograms/matrix/";
const string PREMIER = "/TestPrograms/premier/";
const string BLENDER_249 = "/TestPrograms/blender_2.49/";
const string BLENDER_259 = "/TestPrograms/blender_2.59/";
const string TEST = "/TestPrograms/test/";
int cptRule = 0;

/* Struct Rule pour presenter une ligne (une regle) du MakeFile
 * Une regle depend aux autre regles et a aussi des dependances
*/
struct Rule {
	int idRule;
	string name;
	vector<string> command; // La ligne de la commande suivant une regle
	bool isExecuted;
	bool isFinished;
	list<Rule*> dependences;
	list<Rule*> dependants;
	Rule (string ruleName): isExecuted(false), isFinished(false), name(ruleName) { }
};

void setIdRule(Rule* rule, int id) {
	rule->idRule = id;
}

map<string, Rule*> rules;
vector<Rule*> tasks;

// Chercher une regle using son nom. Retourner un nouveau regle si'il n'existe pas
Rule *findRuleByName(const string &rule);

// Ajouter un regle dependant d'un autre regle
void addDependency(Rule* rule, const string &dependencyName);

// Exécuter une commande
void executeCommand(Rule* rule);

// Parser un fichier Makefile simple
void parse(string &nameInputFile);

// Lire la commande entree par l'utilisateur
int getParameterCommandLine(int argc, char* argv[]);

void printAllCommand(const vector<string> &cmd);

void printARule(Rule* rule);

void printAllRule(map<string,Rule*> rules);

void openFile(const string &path);
#endif

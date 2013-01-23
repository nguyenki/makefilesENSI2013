#ifndef __SDMAKE_H

#define __SDMAKE_H

#include <cstdlib>

#include <cstring>
#include <vector>
#include <iostream>
#include <list>
#include <map>

#include <mpi.h>

#define BAD_FILE -2
#define LINE_LENGTH 1024
#define MASTER 0
#define REQUEST_TAG 11
#define FINISHED_TAG 8
#define DIE_TAG -1
#define NEED_FILE 89
#define SENT_FILE 23

using namespace std;

const string MATRIX = "TestPrograms/matrix/";
const string PREMIER = "/TestPrograms/premier/";
const string BLENDER_249 = "/TestPrograms/blender_2.49/";
const string BLENDER_259 = "/TestPrograms/blender_2.59/";
const string TEST = "/TestPrograms/test/";
int cptRule = 0;


string target; // Le premier cible a executer
int tasksTodo;

list<int> myTasks;

/* Struct Rule pour presenter une ligne (une regle) du MakeFile
 * Une regle depend aux autre regles et a aussi des dependances
 */
struct Rule {
	int idRule;
	string name;
	vector<string> command; // La ligne de la commande suivant une regle
	bool isExecute;
	bool isFinished;
	list<string> dpNames; // Enregistre les noms des fichiers necessaires
	list<Rule*> dependences; // Les regles que cette regle depend
	list<Rule*> dependants;  // Les regles qui depandent de cette regle
	Rule (string ruleName): isExecute(false), isFinished(false), name(ruleName) { }
};

void setIdRule(Rule* rule, int id) {
	rule->idRule = id;
}

map<string, Rule*> rules;
vector<Rule*> tasks; // Enregistrer l'ordre d'execution de tous les regles dans le Makefile

// Chercher une regle using son nom. Retourner un nouveau regle si'il n'existe pas
Rule *findRuleByName(const string &rule);

// Ajouter un regle dependant d'un autre regle
void addDependency(Rule* rule, const string &dependencyName);

// Exécuter une commande
void executeCommand(Rule* rule);

// Executer les tasks d'une machine
void executeAllMyTasks();

MPI_Status receiveMessages();

bool isAllDependantFilesExist(Rule* rule);

void sendFile(const string &fileName, const string &hostname);

bool isFileExist(const string &fileName);

void deleteFile(const string &fileName);

string getCurrentDirectory();

vector<string> getAllFileNameInCurrentDir();

// Masquer que la tache est fini
void maskTaskAsFinished(Rule* rule);

// Parser un fichier Makefile simple
void parse(string &nameInputFile);

// Lire la commande entree par l'utilisateur
int getParameterCommandLine(int argc, char* argv[]);

void printAllCommand(const vector<string> &cmd);

void printARule(Rule* rule);

void printAllRule(map<string,Rule*> rules);

void openFile(const string &path);

void master();

void worker();

void getTaskTodoFromRule(Rule* rule);

#endif



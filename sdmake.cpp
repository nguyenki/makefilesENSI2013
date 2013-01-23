#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <map>
#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include <mpi.h>
#include "sdmake.h"

using namespace std;

//string inputFile=PREMIER;
string inputFile="";
int myRank;
int nbM;
char processName[MPI_MAX_PROCESSOR_NAME];
char masterName[MPI_MAX_PROCESSOR_NAME];

int main( int argc, char* argv[])
{
	cout <<"Distributed MakeFile"<<"\n";
	int parseResult = getParameterCommandLine(argc,argv);
	int len;
	if (parseResult!=1) {
		return -1;
	}
	cout<<"Last target:"<<target<<endl;
	/****************************************************
	 * Routine principale
	 ****************************************************/
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	parse(inputFile);
	getTaskTodoFromRule(rules[target]);
	tasksTodo = tasks.size();
	MPI_Comm_size(MPI_COMM_WORLD, &nbM);
	MPI_Get_processor_name(processName, &len);
	// Distribuer les taches
	for(int i=myRank;i<tasks.size();i+=nbM) {
		myTasks.push_back(i);
	}

/*	if (myRank==MASTER) {
		master();
	} else {
		worker();
	}
*/

//	deleteFile("kim"); // OK
//	cout <<"delete file test"<< isFileExist("sendtest")<<endl;
//	sendFile("cogang","nguyenki@ensisun.imag.fr"); OK
	MPI_Finalize();

	return 0;
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
	rule->dpNames.push_back(dependencyName);
	newDependency->dependants.push_back(rule);
}


void executeAllMyTasks() {
	MPI_Request request;
	list<int> tasks_done;
	int done_task;
	for (list<int>::const_iterator it = myTasks.begin(); it!=myTasks.end();++it) {
		bool notHaveDependencies = tasks[(*it)]->dependences.size()==0;
		if (notHaveDependencies) {
			// Execute la commande
			// Verifier si les fichiers dependants sont deja fournis par le maitre
			bool isAbleToExecuteCmd = isAllDependantFilesExist(tasks[*it]);
			if (isAbleToExecuteCmd) {
				executeCommand(tasks[(*it)]);
				// Envoyer a broadcast message a tous le monde
				done_task = (*it);
				for (int i=0;i<nbM;i++) {
					if (i!=myRank) {
						MPI_Isend(&done_task, 1, MPI_INT, i, FINISHED_TAG, MPI_COMM_WORLD, &request);
					}
				}
				tasks_done.push_back(*it);
			}
		} else {
			// TODO: Demander le fichier necessaire
			
		}
	}
	for(list<int>::const_iterator it = tasks_done.begin(); it!=tasks_done.end();++it) {
		myTasks.remove(*it);
	}
}


/*************************************
Worker tasks
**************************************/

void worker() {
	MPI_Status status;
	int msg;
	while (1) {
		// Recevoir un message du maitre
		status = receiveMessages();
		// Verifier le tag du message recu
		if (status.MPI_TAG == DIE_TAG) {
			return;
		}
		// Faire les taches
		executeAllMyTasks();
	}
}

/***********************
Master tasks
***********************/
void master() {
	int idTask;

	while (tasksTodo>0) {
		// Recevoir message des worker
		receiveMessages();
		// Faire les taches
		executeAllMyTasks();
	}

	// Envoyer un broadcast pour informer que tous les taches sont finis
	for (int i=1;i<nbM;i++) {
		MPI_Send(0, 0, MPI_INT, i, DIE_TAG, MPI_COMM_WORLD);
	}

}


MPI_Status receiveMessages() {
	// Recevoir la demande de tache
	MPI_Status st;
	char* fileName;
	MPI_Recv(&fileName, 1, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
	int src = st.MPI_SOURCE;
	if (st.MPI_TAG == NEED_FILE) {
		sendFile(fileName,processName);
	}

	// Recevoir les autres demandes
	MPI_Status status;
	int msg;
	MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	int origin = status.MPI_SOURCE;
	switch(status.MPI_TAG) {
		case FINISHED_TAG: {
			maskTaskAsFinished(tasks[msg]);
			break;
		}
		case SENT_FILE: {
			
		}
			
	}

	return status;
}
void parse(string &nameInputFile) {
	char line[LINE_LENGTH];
	string ruleName;
	Rule *rule;
	bool getLastTarget = true;

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

			if(getLastTarget) {
				target = ruleName;
				getLastTarget = false;
			}

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
		cout <<"Usage: ./sdmake -nkt Makefile"<<endl;
		exit(0);
	} else {
		para = argv[1];
		if (para != "-nkt") {
			cout << argv[1] <<" parameter doesn't exist\n"<<endl;
			cout <<"Usage: ./sdmake -tc Makefile"<<endl;
			return -1;
		} else {
			string temp = argv[2];
			if (temp=="Makefile") {
				inputFile=temp;
				return 1;
			} else {
				return -1;
			}
		}
	}
}

void getTaskTodoFromRule(Rule* rule) {
	rule->isExecute = true;
	bool existDependences = rule->dependences.size()>0;
	if (existDependences) {
		for(list<Rule*>::const_iterator it=rule->dependences.begin(); it!=rule->dependences.end();++it) {
			getTaskTodoFromRule(*it);
		}
	}
	rule->idRule = tasks.size(); // id Regle est assigne par priorite
	tasks.push_back(rule);
}


bool isAllDependantFilesExist(Rule* rule) {
	bool allDependantFileExist = true;
	for(list<string>::const_iterator it = rule->dpNames.begin(); it!=rule->dpNames.end();++it) {
		bool fileExist = isFileExist(*it);
		allDependantFileExist = allDependantFileExist && fileExist;
	}
	return allDependantFileExist;
}

bool isFileExist(const string &fileName) {
	vector<string> allFiles = getAllFileNameInCurrentDir();
	bool exist = false;
	for (vector<string>::const_iterator it = allFiles.begin(); it!=allFiles.end(); ++it) {
		if (*it==fileName) {
			exist = true;
		}
	}
	return exist;
}

// TODO: need to review
void sendDemandFile(const string &fileName) {
	MPI_Request request;
	MPI_Isend(&myRank, 1, MPI_INT, MASTER, NEED_FILE, MPI_COMM_WORLD, &request);
}

void deleteFile(const string &fileName) {
	string cmd = "rm "+fileName;
	system(cmd.c_str());
	if (isFileExist(fileName)) {
		cout <<"File: "<<fileName <<"  is deleted" <<endl;
	} else {
		cout << "Unable to delete file: "<<fileName<<endl;
	}
}

void sendFile(const string &fileName, const string &hostname) {
	cout <<"Current directory:"<<getCurrentDirectory()<<endl;
	string cmd = "scp "+fileName+" "+hostname+":~";
	system(cmd.c_str());
	if (myRank!=MASTER) {
		deleteFile(fileName);
	}
}



string getCurrentDirectory() {
        char result[ PATH_MAX ];
        ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
        std::string proName = std::string( result, (count > 0) ? count : 0 );
        size_t lastSlash = proName.find_last_of("\\/");
        return proName.substr(0,lastSlash)+"/"; 
}

vector<string> getAllFileNameInCurrentDir() {
	vector<string> archives = vector<string>();
	string dirPath = getCurrentDirectory();
	DIR *dp;
	struct dirent *drnt;
	dp = opendir(dirPath.c_str());
	if (dp == NULL) {
		cout << "Error(" << errno << ") opening" << dirPath << endl;
	} else {
		while ((drnt=readdir(dp)) != NULL) {
			archives.push_back(drnt->d_name);
		}
                closedir(dp);
        }
        return archives;
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

void maskTaskAsFinished(Rule* rule) {
	rule->isFinished = true;

	for (list<Rule*>::const_iterator it =(rule->dependants).begin(); it!=(rule->dependants).end(); ++it) {
		if ((*it)->isExecute) {
			(*it)->dependences.remove(rule);
		}
	}
	tasksTodo-=1;
}


void executeCommand(Rule* rule) {
        vector<string> cmds = rule->command;
        for (vector<string>::const_iterator it = cmds.begin(); it!= cmds.end(); ++it) {
                string cmd = (*it);
                cout<< "Running command:" << cmd << "\n" <<endl;
                system(cmd.c_str());
        }
	while(1) {
		if (isFileExist(rule->name)) {
			 break;
			cout << "FILE: " << rule->name << " existed"<<endl;
		}
		cout << "WARNING: File" << rule->name << " has not existed yet" <<endl;
	}
	// TODO:
	sendFile(rule->name,"");
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

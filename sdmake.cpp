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
	cout <<"Running in machine:" << processName << "  with rank: " << myRank <<endl;


	if (myRank==MASTER) {
		master();
	} else {
		worker();
	}



//	deleteFile("kim"); // OK
//	cout <<"delete file test"<< isFileExist("sendtest")<<endl;
//	sendFile("cogang","nguyenki@ensisun.imag.fr"); OK

/*

	getAllHostName(myHostName);
	cout << "MASTER HOST NAME:" << getMasterName(myHostName) <<endl;

	cout << "Host:" << getHostName(1) <<endl;
*/
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


int getTaskTodo() {
	int todo = -1;
	int indice = tasks.size() - 1;
	bool foundTask = false;
	for (vector<Rule*>::reverse_iterator it=tasks.rbegin(); it!=tasks.rend(); ++it) {
		if (foundTask == false) {
			if ((*it)->isExecute==false) {
				todo = indice;
				tasks[indice]->isExecute = true;
				foundTask = true;
			}
		}
		indice--;
	}
	return todo;
}


// ANOTHER IDEA
void executeTask(int taskToExecute) {
	bool notHaveDependencies = tasks[taskToExecute]->dependences.size()==0;
	if (notHaveDependencies) {
		// Execute la commande
		// Verifier si les fichiers dependants sont deja fournis par le maitre
		bool isAbleToExecuteCmd = isAllDependantFilesExist(tasks[taskToExecute]);
		if (isAbleToExecuteCmd) {
			executeCommand(tasks[taskToExecute]);
		} else {
			// Denader le fichier necessaire
			MPI_Send(&taskToExecute, 1, MPI_INT, MASTER, NEED_FILE, MPI_COMM_WORLD);
		}
	}
}



/*************************************
Worker tasks
**************************************/

void worker() {
	cout << "ENTERING ESCLAVE: " << processName << "WITH RANK:" << myRank <<endl;
	MPI_Status status;
	int work;
	int result;
	while (1) {
		cout << "In the while loop of WORKER:  " << myRank <<endl;

		// Recevoir des taches a faire
		MPI_Recv(&work, 1, MPI_INT, MASTER, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

		// S'il recoit work = -1 alors, tous les taches sont deja envoye
		// Il doit attendre les nouvelles taches
		if (work==-1) {
			cout << "BYE BYE FROM ESCLAVE:"  << myRank <<endl;
			return;
		}

		// Verifier le tag du message recu
		if (status.MPI_TAG == DIE_TAG) {
			cout << "RECEIVED DIE TAG FROM MASTER" <<endl;
			return;
		}

		if (status.MPI_TAG == SENT_FILE) {
			cout << "Receive file" << tasks[work]->name << "from master" <<endl;
		}
		
		if (status.MPI_TAG == NOT_EXIST_TAG) {
			cout << "File not exist yet" <<endl;
		}
		
		if (status.MPI_TAG == 123) {
			return;
		}

		if (status.MPI_TAG==WORK_TAG) {
			tasksTodo = work;
		}

		if (tasks[tasksTodo]->isFinished!=true) {
		bool allDependeciesExist = isAllDependantFilesExist(tasks[tasksTodo]);
		if (allDependeciesExist) {
			// Faire les taches a faire
			cout << "RUNNING CMD IN WORKER WITH ID:" << myRank <<endl;
			executeCommand(tasks[tasksTodo]);
		}
		}

	}
}

/***********************
Master tasks
***********************/
void master() {
	cout << "ENTERING MASTER..........................." <<endl;
	int work;
	int result; // The result received from the master
	int source;
	int nbHost;
	int taTodo = 0;
	MPI_Status status;

	MPI_Comm_size(MPI_COMM_WORLD,&nbHost);


	// Send a task to each worker
	for (int i=1;i<nbHost;i++) {
		work = getTaskTodo();
		cout << "DISTRIBUTED TASKS " << tasks[work]->name << " TO " << i   <<endl;
		MPI_Send(&work, 1, MPI_INT, i, WORK_TAG, MPI_COMM_WORLD);
	}

	work = getTaskTodo();
	while (work!=-1) {
		MPI_Recv(&result, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,&status);
		source = status.MPI_SOURCE;
		switch(status.MPI_TAG) {
			case FINISHED_TAG: {
				maskTaskAsFinished(tasks[result]);
				// Envoyer nouveau tache pour esclave
				if (taTodo!=-1) {
					MPI_Send(&work, 1, MPI_INT, source, WORK_TAG, MPI_COMM_WORLD);
					cout << "MASTER SEND TASK: " << tasks[work]->name << "  TO   " << source <<endl;
				}
				taTodo = getTaskTodo();
				if (taTodo!=-1) {
					work = taTodo;
				}
				break;
			}

			case NEED_FILE: {
				cout << "ENTERING NEED FILE..................: FROM:" << source << ":RESULT:" << tasks[result]->name <<endl;
				bool fileExist = isFileExist(tasks[result]->name);

					if (fileExist) {
						//sendFile((*it)->name,getHostName(source));
						cout << "FILE:" << tasks[result]->name << "EXISTE" <<endl;
						MPI_Send(&result, 1, MPI_INT, source, SENT_FILE, MPI_COMM_WORLD);
					} else {
						cout << "FILE:" << tasks[result]->name << " DOESN'T EXISTE" <<endl;
						MPI_Send(&result, 1, MPI_INT, source, NOT_EXIST_TAG, MPI_COMM_WORLD);
					}
				
				break;
			}
		}
		cout << "TASK NUMBER LEFT :" << tasksTodo <<endl;
		if (tasksTodo==0) {
			cout << "ALL TASK DONE ....................." <<endl;
			work=-1;
		}

	}

	// Tous les taches sont faites. Alors, on recoit tous les derieres resultats d'esclave
//	for (int i=1; i<nbHost;i++) {
//		MPI_Recv(&result, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,&status);
//	}

	// Envoyer un broadcast pour informer que tous les taches sont finis
	int all_done = 123;
	for (int i=1;i<nbHost;i++) {
		cout << "SEND DIE TASK TO RANK:" << i <<endl;
		MPI_Send(0, 0, MPI_INT, i, DIE_TAG, MPI_COMM_WORLD);
	}

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

			if (target=="") {
				target = ruleName;
			}

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
				if (argc==4) {
					target = argv[3];
				}
				return 1;
			} else {
				return -1;
			}
		}
	}
}

void getTaskTodoFromRule(Rule* rule) {
	rule->isExecute = false;
	bool existDependences = rule->dependences.size()>0;
	if (existDependences) {
		for(list<Rule*>::const_iterator it=rule->dependences.begin(); it!=rule->dependences.end();++it) {
			getTaskTodoFromRule(*it);
		}
	}
	rule->idRule = tasks.size(); // id Regle est assigne par priorite
	//cout <<"Add tasks to list:" << rule->name <<endl;
	tasks.push_back(rule);
}


bool isAllDependantFilesExist(Rule* rule) {
	bool allDependantFileExist = true;
	for(list<string>::const_iterator it = rule->dpNames.begin(); it!=rule->dpNames.end();++it) {
		bool fileExist = isFileExist(*it);
		if (fileExist == false) {
			cout << "--------FILE NOT EXIST-----" << *it << "RANK:" << myRank <<endl;
			sendDemandFile(*it); 
		} else {
			cout << "--------FILE  EXISTED-----" << *it << "RANK:" << myRank  <<endl;
			
		}
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
	int ind=0;
	int i=0;;
	for(vector<Rule*>::const_iterator it=tasks.begin();it!=tasks.end();++it) {
		if ((*it)->name==fileName) {
			ind = i;
		}
		i++;
	}
	MPI_Request request;
	MPI_Isend(&ind, 1, MPI_INT, MASTER, NEED_FILE, MPI_COMM_WORLD, &request);
}

void deleteFile(const string &fileName) {
	string cmd = "rm "+fileName;
	system(cmd.c_str());
	if (!isFileExist(fileName)) {
		cout <<"File: "<<fileName <<"  is deleted" <<endl;
	} else {
		cout << "Unable to delete file: "<<fileName<<endl;
	}
}

void sendFile(const string &fileName, const string &hostname) {
	MPI_Request request;
	cout <<"Current directory:"<<getCurrentDirectory()<<endl;
	string cmd = "scp "+fileName+" "+hostname+":~";
	system(cmd.c_str());
	if (myRank!=MASTER) {
		deleteFile(fileName);
	} else { // BUG
		MPI_Isend(&myRank,1, MPI_INT, MASTER, SENT_FILE, MPI_COMM_WORLD, &request);
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
	cout << "MASK TASK:" << rule->name << "  AS FINISHED"  <<endl;
	for (list<Rule*>::const_iterator it =(rule->dependants).begin(); it!=(rule->dependants).end(); ++it) {
		(*it)->dependences.remove(rule);
	}
	tasksTodo-=1;
}


void executeCommand(Rule* rule) {
        vector<string> cmds = rule->command;
        for (vector<string>::const_iterator it = cmds.begin(); it!= cmds.end(); ++it) {
                string cmd = (*it);
                cout<< "Running command:" << cmd << " in rank:" << myRank<< "\n" <<endl;
	        system(cmd.c_str());
        }

	rule->isFinished = true;
//	while(1) {
//		if (!isFileExist(rule->name)) {
//			 break;
//			cout << "FILE: " << rule->name << " existed"<<endl;
//		}
//		cout << "WARNING: File  " << rule->name << " has not existed yet" <<endl;
//	}
	
//	sendFile(rule->name, getHostName(0));
	int result = rule->idRule;
	MPI_Send(&result, 1, MPI_INT, MASTER, FINISHED_TAG, MPI_COMM_WORLD);
	cout << "ESCLAVE:" << myRank << "FINISHED TASK:" << tasks[result]->name  << endl;
}

void printAllTasks(vector<Rule*> tasks) {
	for (vector<Rule*>::const_iterator it=tasks.begin(); it!=tasks.end();++it) {
		printARule((*it));
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

string getMasterName(const string &hostFileName) {
	char line[LINE_LENGTH];	
	string masterName="";
	ifstream inFile;
	inFile.open(myHostName.c_str());
	while (!inFile.eof()) {
		inFile.getline(line, LINE_LENGTH);
		stringstream line_tmp(line);
		if (strlen(line)==0) // Laisse tomber la ligne vide
			continue;
		if (line[0]=='#') // Saute la ligne de commentaire
			continue;
		line_tmp >> masterName;
		while (masterName == "") {
			line_tmp >> masterName;
		}
		if (masterName =="") {
			cout << "myHost File is empty" <<endl;		
		} else {
			break;
		}
	}

	return masterName;
}

void getAllHostName(const string &hostFileName) {
	char line[LINE_LENGTH];	
	string host = "";
	ifstream inFile;
	inFile.open(myHostName.c_str());
	while (!inFile.eof()) {
		inFile.getline(line, LINE_LENGTH);
		stringstream line_tmp(line);
		if (strlen(line)==0) // Laisse tomber la ligne vide
			continue;
		if (line[0]=='#') // Saute la ligne de commentaire
			continue;
		line_tmp >> host;
		if (host!="") {
			allHostNames.push_back(host);	
		}		
	}
	if (allHostNames.size()==0) {
		cout << "NO HOST FOUND IN THE MYHOST FILE" <<endl;
	}
}

string getHostName(int rank) {
	return allHostNames[rank];
}




/*****************************************************************************
*************** VOL DE TRAVAIL APRES******************************************/



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
			MPI_Isend(&myRank,1, MPI_INT, MASTER, NEED_FILE, MPI_COMM_WORLD, &request);
		}
	}
	for(list<int>::const_iterator it = tasks_done.begin(); it!=tasks_done.end();++it) {
		myTasks.remove(*it);
	}
}


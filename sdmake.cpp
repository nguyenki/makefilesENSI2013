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

string inputFile="";
string premierDir = "/tmp/makefile/";

int myRank;
int nbM;
char processName[MPI_MAX_PROCESSOR_NAME];
char masterName[MPI_MAX_PROCESSOR_NAME];
double startTime;
double endTime;

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
	startTime = MPI_Wtime();
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	parse(inputFile);
	getTaskTodoFromRule(rules[target]);


	tasksTodo = tasks.size();
	MPI_Comm_size(MPI_COMM_WORLD, &nbM);
	MPI_Get_processor_name(processName, &len);
	cout <<"Running in machine:" << processName << "  with rank: " << myRank <<endl;
	getAllHostName(myHostName);

	cout << getCurrentDirectory() <<endl;

	if (myRank==MASTER) {
		master();

	} else {
		worker();
	}


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
			cout << "RECEIVE TASK:" << tasks[work]->name <<endl;
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
		if (work!=-1) {
			cout << "DISTRIBUTED TASKS " << tasks[work]->name << " TO " << i   <<endl;
		}
		MPI_Send(&work, 1, MPI_INT, i, WORK_TAG, MPI_COMM_WORLD);
	}

	work = getTaskTodo();
	bool exit = false;
	while (!exit) {
		MPI_Recv(&result, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,&status);
		source = status.MPI_SOURCE;
		switch(status.MPI_TAG) {
			case FINISHED_TAG: {
				maskTaskAsFinished(tasks[result]);
				// Envoyer nouveau tache pour esclave
				if (taTodo!=-1 && work!=-1) {
					MPI_Send(&work, 1, MPI_INT, source, WORK_TAG, MPI_COMM_WORLD);
					cout << "MASTER SENDTASK: " << tasks[work]->name << "  TO   " << source <<endl;
				}
				taTodo = getTaskTodo();
				if (taTodo!=-1) {
					work = taTodo;
				}
				break;
			}

			case NEED_FILE: {
				cout << "ENTERING NEED FILE..................: FROM:" << source << ":RESULT:" << tasks[result]->name <<endl;
				bool isAllFileExist = isAllDependantFilesExist(tasks[result]);
					if (isAllFileExist) {
						for(list<string>::const_iterator it = tasks[result]->dpNames.begin(); it!= tasks[result]->dpNames.end();++it) {
							cout << "PREPARING TO SEND FILE..................................." <<endl;
							sendFile(*it,getHostName(source));
							cout << "SENT  FILE:" << (*it) << "TO HOST:" << getHostName(source) <<endl;
						}
						MPI_Send(&result, 1, MPI_INT, source, SENT_FILE, MPI_COMM_WORLD);
					} else {
						cout << "ALL FILE NEEDED:" << tasks[result]->name << " DOESN'T EXISTE YET" <<endl;
						MPI_Send(&result, 1, MPI_INT, source, NOT_EXIST_TAG, MPI_COMM_WORLD);
					}
				break;
			}
		}
		cout << "TASK NUMBER LEFT :" << tasksTodo <<endl;
		if (tasksTodo==0) {
			cout << "ALL TASK DONE ....................." <<endl;
			exit = true;
		}

	}

	// Tous les taches sont faites. Alors, on recoit tous les derieres resultats d'esclave
	MPI_Request re;
	for (int i=1; i<nbHost;i++) {
		MPI_Irecv(&result, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,&re);
	}


        endTime = MPI_Wtime();
        if (myRank==MASTER) {
		stringstream result;
		result << (endTime-startTime);
                cout << "RUNNING TIME:" << endTime-startTime <<endl;
		write_result(result.str());
	}

	// Envoyer un broadcast pour informer que tous les taches sont finis
        int all_done = 123;
        MPI_Request rq;
        for (int i=1;i<nbHost;i++) {
                cout << "SEND DIE TASK TO RANK:" << i <<endl;
                MPI_Isend(&all_done, 1, MPI_INT, i, DIE_TAG, MPI_COMM_WORLD, &rq);
        }


	MPI_Abort(MPI_COMM_WORLD, 911);
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

			if (ruleName[ruleName.length()-1]==':') {
				ruleName = ruleName.substr(0, ruleName.size()-1);
			}

			if (target=="") {
				target = ruleName;
			}

			rule = findRuleByName(ruleName);
			stringstream p;
			p  << ':';
			string point;
			p >> point;
			string dependency;
			while (line_temp >> dependency) {
				if (dependency!=point && dependency[dependency.length()-1]!='\t') {
					addDependency(rule, dependency);
				}
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
		cout <<"Usage: ./sdmake tc Makefile"<<endl;
		exit(0);
	} else {
		para = argv[1];
		if (para != "-tc") {
			cout << argv[1] <<" parameter doesn't exist\n"<<endl;
			cout <<"Usage: ./sdmake -tc Makefile dir_name [target]"<<endl;
			return -1;
		} else {
			string temp = argv[2];
			if (temp=="Makefile") {
				inputFile=temp;
				if (argc==4) {
					string dir_name = argv[3];
					premierDir+=dir_name;
					if (argc==5) {
						target = argv[3];
					}
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

		allDependantFileExist = allDependantFileExist && fileExist;
	}
	if (!allDependantFileExist && myRank!=MASTER) {
		sendDemandFile(rule->name);
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
		cout <<"File: "<<fileName <<"  is deleted" << "    RANK:" << myRank <<endl;
	} else {
		cout << "Unable to delete file: "<<fileName << "   IN RANK" << myRank<<endl;
	}
}

void sendFile(const string &fileName, const string &hostname) {
	MPI_Request request;
	string currentDirectory = getCurrentDirectory();
	cout <<"Current directory:"<<currentDirectory<<endl;
	string cmd = "scp "+fileName+" "+hostname+":"+premierDir;
	cout << "SENT FILE" << fileName << "TO:" << hostname <<endl;
	system(cmd.c_str());
	if (myRank!=MASTER) {
		if (isFileExist(fileName)) {
			deleteFile(fileName);
		}
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
	string dirPath = premierDir;

	DIR *dp;
	struct dirent *drnt;
	dp = opendir(dirPath.c_str());
	if (dp == NULL) {
		cout << "Error(" << errno << ") opening" << dirPath << endl;
	} else {
		while ((drnt=readdir(dp)) != NULL) {
			cout << "DEBUG:" << drnt->d_name << "	IN RANK	" << myRank <<endl;
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

	sendFile(rule->name, getHostName(0));
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

void write_result(const string &text) {
    std::ofstream log_file("time_execution.txt", std::ios_base::out | std::ios_base::app);
    log_file << text << endl;
}





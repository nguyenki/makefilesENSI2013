NGUYEN KIM THUAT - NGUYEN DANG CHUAN
ENSIMAG - ISI3A
-----------------------------------------------------------
Rapport/
	Rapportsystemedistribue.pdf // Rapport du projet
	TimeExecution.xlsx // Donnees bruts des tests de performance



----------------------------------------- MANUEL D'INSTALLATION ------------------------------------------

Configurer le PATH: 
 Lancer cette commande avant de faire le test ou l�ajoute dans le fichier .bashrc
 Supposons que make_dir est la place ou vous deposez le programme make distribu�
 Dans notre cas:  make_dir=/home/nguyend/partage/sysdis/makefilesENSI2013
	
 PATH=make_dir/TestPrograms/matrix:make_dir/TestProrams/premier:make_dir:$PATH


Ce qu�on fait ci-dessus est pour objectif d�ajouter tous les fichiers ex�cutables n�cessaires dans le PATH partage. Ce qui nous permet de lancer le programme dans le r�pertoire /tmp/makefile de chaque hote sans ajouter le  ./

On ajoute aussi une autre fois le chemin vers le r�pertoire matrix parce qu�il existe aussi les deux programmes  split et multiply dans les machines TX. Comme �a, on force l�utilisation des fichiers ex�cutables plac�s dans le r�pertoire matrix.   

Lancer le script deploy.h: 
	./deploy.sh

	Ex: voir le fichier deployTrace.txt
	Le script va tout d�abord cr�er le r�pertoire /tmp/makefile dans chaque machine h�te. Il va ensuite copier tous les programmes de test dans ces r�pertoires. On copie �galement le fichier myhosts dans le r�pertoire de chaque programme de test.

Structure du r�pertoire /tmp/makefile sur chaque h�te:
	/makefile
	blender_2.49/
		�........
		myhosts
	blender 2.59/
		�........
myhosts
	premier/
		�........
		myhosts
	matrix/
		�........
		myhosts

	Lorsque l�environnement est pr�t. Il est suffit d�aller dans la  machine maitre (le premier h�te dans le fichier myhosts) et lancer la commande:

		mpirun -x PATH -hostfile myhost sdmake -nkt Makefile > resultat.txt

	Pour compiler le programme, il faut aller dans le r�pertoire ou vous d�posez le programme et lancer la commande ci-dessous:

mpic++ sdmake.cpp -o sdmake

Notes: s�il y a des erreurs pendant l'ex�cution, il est suffit de relancer la commande mpirun ci-dessus. Si tous va bien, vous pouvez voir le temps d'ex�cution dans le fichier temps_execution.txt qui est g�n�r� � la fin de calcul dans la machine ma�tre.

#!/bin/bash

cp myhosts TestPrograms/matrix/
cp myhosts TestPrograms/blender_2.49/
cp myhosts TestPrograms/blender_2.59/
cp myhosts TestPrograms/test/

cd TestPrograms/
tar -zcvf matrix.tar.gz matrix/
tar -zcvf blender_2.49.tar.gz blender_2.49/
tar -zcvf blender_2.59.tar.gz blender_2.59/
tar -zcvf test.tar.gz test/
cd ..

while read line
do 
	echo "$line"
	ssh -f "$line" 'cd /tmp;if [ ! -d makefile ]; then 
		mkdir makefile 
	fi'
	scp TestPrograms/*.tar.gz "$line":/tmp/makefile > resultat.txt
	ssh -f "$line" 'cd /tmp/makefile;tar -zxvf matrix.tar.gz > resultat.txt;tar -zxvf blender_2.49.tar.gz > resultat.txt; tar -zxvf blender_2.59.tar.gz > resultat.txt; tar -zxvf test.tar.gz > resultat.txt'
	ssh -f "$line" 'cd /tmp/makefile; rm *.tar.gz; rm resultat.txt'
done <myhosts

rm resultat.txt
cd TestPrograms/
rm *tar.gz
cd ..

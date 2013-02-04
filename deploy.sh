#!/bin/bash

cp myhosts TestPrograms/matrix/
cp myhosts TestPrograms/blender_2.59/
cp myhosts TestPrograms/blender_2.49/
cp myhosts TestPrograms/test/
cd TestPrograms/
tar -zcvf matrix.tar.gz matrix/
tar -zcvf blender_2.49.tar.gz blender_2.49/
tar -zcvf blender_2.59.tar.gz blender_2.59/
tar -zcvf test.tar.gz test/


while read line
do
	echo "$line"
		
done < myhosts

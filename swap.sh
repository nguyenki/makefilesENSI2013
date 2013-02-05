#!/bin/bash

while read line
do
	ssh -f "$line" 'cd /tmp/makefile/premier; rm *txt'


done < myhosts

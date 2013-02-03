list.txt: list1.txt list2.txt list3.txt list4.txt
	cp list1.txt list.txt ; cat list2.txt >> list.txt ; cat list3.txt >> list.txt ; cat list4.txt >> list.txt ;

list1.txt:
	premier 2 11 > list1.txt

list2.txt:
	premier 2 51 > list2.txt

list3.txt:
	premier 7 35 > list3.txt

list4.txt:
	premier 17 300 > list4.txt



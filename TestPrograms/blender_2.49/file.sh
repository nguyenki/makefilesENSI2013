file=$1
 
[ $# -eq 0 ] && { echo "Usage: $0 filename"; exit 999; }
 
while [ ! -f $file ];
do
	echo "*************************** $1  NOT EXIST  *****************"
done
echo " $1 TON TAI"

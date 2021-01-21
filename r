rm -f restarts
while true
do
	./mybmm -c mybmm.conf -d 1
	date >> restarts
done

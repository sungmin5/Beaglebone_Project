#! /bin/bash
#test cases

#check bad argument
echo "checking bad argument..."
./lab4b --trash &> /dev/null;
if [[ $? -ne 1 ]]
then
	echo "Failed"
else
	echo "Success"
fi

#check arguments and whether logs the STDIN inputs
./lab4b --log=logfile --scale=C --period=2 <<-EOF
SCALE=F
SCALE=C
STOP
START
LOG
OFF
EOF

echo "checking logfile creation..."
if [[ -f logfile ]]
then
	echo "Success"
else
	echo "Failed"
fi

echo "checking proper stdin input commands..."
for command in SCALE=F SCALE=C STOP START LOG OFF
do
	grep $command logfile &> /dev/null
	if [[ $? -ne 0 ]]
	then
		echo "Failed for $command"
	else
		echo "Success for $command"
	fi
done

#check time and temperature format
echo "checking time and temperature format..."
grep '[0-9][0-9]:[0-9][0-9]:[0-9][0-9] [0-9][0-9].[0-9]' logfile &> /dev/null
if [[ $? == 0 ]]
then
	echo "Success"
else
	echo "Failed"
fi


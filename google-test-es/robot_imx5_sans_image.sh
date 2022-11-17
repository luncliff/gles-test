#!/bin/bash

if [ -z `which diff` ]; then

	echo "robot error: diff not found"
	exit 255
fi

if [ -z `which bc` ]; then

	echo "robot error: bc not found"
	exit 254
fi

TEST_ES=test_imx5_sans_image
TEST_ES_RUN=(
	'-frames 100 -bitness "5 6 5 0" -screen "1024 512 60" -app "image_dim 1024 512"'
	'-frames 100 -bitness "8 8 8 0" -screen "1024 512 60" -app "image_dim 1024 512"'
)
TEST_ES_FPS=(
	16.5
	15.6
)

N_RUNS=${#TEST_ES_RUN[@]}

for (( i=0; i < $N_RUNS; i++)); do

	LOG=$TEST_ES"_"$i"_"$(date +%Y%m%d).log
	ERR=$TEST_ES"_"$i"_"$(date +%Y%m%d).err

	echo "./"$TEST_ES ${TEST_ES_RUN[$i]} > $LOG
	eval "./"$TEST_ES ${TEST_ES_RUN[$i]} >> $LOG 2> $ERR

	if (( 0 != $? )); then

		echo "robot error: test run $i failed; command line:"
		echo "./"$TEST_ES ${TEST_ES_RUN[$i]}
		exit 253
	fi

	FPS=$(grep ^'average FPS:' $LOG | grep -o -E -e '[[:digit:]]+[.]?[[:digit:]]*')

	if (( 0 != $? )); then

		echo "robot error: test run $i failed to yield fps; check log"
		exit 252
	fi

	echo -e "test $i: \c"

	if (( 1 == $(echo "$FPS < ${TEST_ES_FPS[$i]}" | bc) )); then

		echo low performance \($(echo "scale=3; $FPS / ${TEST_ES_FPS[$i]}" | bc)\)
	else
		echo ok
	fi
done

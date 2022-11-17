#!/bin/bash

if [ -z `which diff` ]; then

	echo "robot error: diff not found"
	exit 255
fi

if [ -z `which bc` ]; then

	echo "robot error: bc not found"
	exit 254
fi

TEST_ES=test_imx5_matmul
TEST_ES_RUN=(
	'-frames 100 -drawcalls 16 -bitness "8 8 8 8"'
	'-frames 100 -drawcalls 16 -bitness "8 8 8 8" -app linear_map'
)
TEST_ES_FPS=(
	1.956
	1.959
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

	RUN_GRAB=$TEST_ES"_"$i"_"$(date +%Y%m%d).raw
	RUN_REF=$TEST_ES"_"$i"_ref.raw"

	eval "./"$TEST_ES ${TEST_ES_RUN[$i]} -frames 1 -grab_frame \"0 $RUN_GRAB\" > /dev/null 2>&1

	if (( 0 != $? )); then

		echo "robot error: test run $i failed to yield conformance sample"
		exit 251
	fi

	echo -e "test $i: \c"

	RUN_SUCCESS=1

	if (( 1 == $(echo "$FPS < ${TEST_ES_FPS[$i]}" | bc) )); then

		echo low performance \($(echo "scale=3; $FPS / ${TEST_ES_FPS[$i]}" | bc)\)
		RUN_SUCCESS=0
	fi

	if [[ `diff $RUN_GRAB $RUN_REF` ]]; then

		echo non-conformant
		RUN_SUCCESS=0
	fi

	if (( 1 == $RUN_SUCCESS )); then

		echo ok
	fi
done

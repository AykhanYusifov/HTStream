#!/usr/bin/env bash
set -euo pipefail
export BUILDDIR=$1
echo $0

if [[ "$OSTYPE" == "darwin"* ]]; then
    findarg="-perm +111"
else
    findarg=-executable
fi

## note you must run this from the regression dir.
## first arg is build dir second is either test or regen

fastqr1=./test_r1.fastq.gz
fastqr2=./test_r2.fastq.gz
fastq_rawr1=./test_r1.fastq
fastq_rawr2=./test_r2.fastq

gunzip -kf $fastqr1
gunzip -kf $fastqr2

regenerate() {
    for prog in `find $BUILDDIR -maxdepth 2 -name "hts*" -type f $findarg -not -name "*_test"`; do
        out=${prog##*/}
        echo running $prog output to $out
        $prog -1 $fastqr1 -2 $fastqr2 -t $out -F
        $prog -1 $fastq_rawr1 -2 $fastq_rawr2 -t $out -F -u

        if [ ${prog##*/} == 'hts_SuperDeduper' ]
        then
           mv $out.tab6.gz $out.tmp && gzip -dc $out.tmp | sort | gzip > $out.tab6.gz
           mv $out.tab6 $out.tmp && cat $out.tmp | sort > $out.tab6
           rm $out.tmp
        fi
    done
}

testrun() {
    for prog in `find $BUILDDIR  -maxdepth 2 -name "hts*" -type f $findarg -not -name "*_test"`; do
        out=${prog##*/}.test
        echo running $prog output to $out
        $prog -1 $fastqr1 -2 $fastqr2 -t $out -F
        $prog -1 $fastq_rawr1 -2 $fastq_rawr2 -t $out -F -u

        if [ ${prog##*/} == 'hts_SuperDeduper' ]
        then
            echo sorting superDeduper because its output is non-deterministic
            cp $out.tab6.gz $out.tmp && gzip -dc  $out.tmp | sort | gzip > $out.tab6.gz
            echo "cp $out.tab6 $out.tmp && cat $out.tmp | sort > $out.tab6"
            cp $out.tab6 $out.tmp && cat $out.tmp | sort > $out.tab6
            rm $out.tmp

            orig=${out%%.*}

            echo diff $out.tab6 $orig.tab6
            diff $out.tab6 $orig.tab6
            echo zdiff $out.tab6.gz $orig.tab6.gz
            diff <(gzip -dc $out.tab6.gz) <(gzip -dc $orig.tab6.gz)
            rm $out.tab6 $out.tab6.gz
        else
            echo zdiff $out.tab6.gz ${out%%.*}.tab6.gz
            diff <(gzip -dc $out.tab6.gz) <(gzip -dc ${out%%.*}.tab6.gz)
            echo diff $out.tab6 ${out%%.*}.tab6
            diff $out.tab6 ${out%%.*}.tab6
            rm $out.tab6.gz $out.tab6
        fi
    done
}

if [ $2 == 'test' ]
then
    testrun
elif [ $2 == 'regen' ]
then
    regenerate
fi

rm $fastq_rawr1 $fastq_rawr2
rm stats.log

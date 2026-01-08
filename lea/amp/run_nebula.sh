#!/bin/bash

ZIP_FILE=$1
TARGET=$2               # small-bench / bench / single
QUEUE=${3:-lock_free}   #  single
THREADS=${4:-32}        #  single
MAX_TIME=${5:-1}        #  single
REPS=${6:-10}           #  single
CONFIG_RECIPE=${7:-thread} #  single
LOG_FILE=nebula.log
DATA_DIR=data

function copy_to_nebula {
    scp $ZIP_FILE nebula:~/test/$ZIP_FILE
}

function copy_from_nebula {
    mkdir -p $DATA_DIR
    scp -r 'nebula:~/test/uut/data/*' $DATA_DIR
}

function clean_test_dir {
    ssh nebula "mkdir -p test && cd test && rm -rf *"
}

function run_on_nebula {
    ssh nebula "\
        cd test &&
        unzip -u $ZIP_FILE -d uut &&
        cd uut &&
        make clean &&
        make all &&
        $(case $TARGET in
            small-bench)
                echo 'make small-bench'
                ;;
            bench)
                echo 'echo Large benchmark skipped on Nebula due to 10min limit'
                ;;
            single)
                echo 'mkdir -p data && ./build/queue \
                    --type '$QUEUE' \
                    --n_threads '$THREADS' \
                    --max_time '$MAX_TIME' \
                    --repetitions '$REPS' \
                    --config_recipe '$CONFIG_RECIPE' \
                    --print_header >> data/single.csv'
            ;;
            *)
                echo "echo Unknown TARGET: $TARGET"
                ;;
        esac)" | tee $LOG_FILE
}

if [ ! $# -ge 2 ]; then
    echo "Usage: $0 <zip-file> <target> [queue] [threads] [max_time] [repetitions] [config_recipe]"
    exit 1
fi

clean_test_dir
copy_to_nebula
run_on_nebula
copy_from_nebula

# Small benchmark
#bash run_nebula.sh project.zip small-bench

# Large benchmark (wird nur Info ausgeben)
#bash run_nebula.sh project.zip bench

# Einzelne Konfiguration testen
#bash run_nebula.sh project.zip single lock_free 32 1 10 thread


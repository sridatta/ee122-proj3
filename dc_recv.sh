#!/bin/bash

echo "Gathering data on receiver machine."

ITERATIONS=3

echo "Task 1: "
for i in $(seq 1 $ITERATIONS)
do
    echo "Iteration $i"
    for R in $(seq 5 5 20)
    do
        echo "R = $R"
        ./router 10 64 5432 5433 2>&1 > /dev/null &
        ./receiver 5433 > task_1_iter_${i}_R_${R}.csv
    done
done

echo "Task 2: "
for i in $(seq 1 $ITERATIONS)
do
    echo "Iteration $i"
    for R in $(seq 5 5 20)
    do
        echo "R = $R"
        ./router 10 64 5432 5433 5434 5435 50 2>&1 > /dev/null &
        ./receiver 5433 > task2_iter_${i}_R_${R}_recv_1.csv &
        ./receiver 5435 > task2_iter_${i}_R_${R}_recv_2.csv
    done
done

echo "Task 3.1: "
for i in $(seq 1 $ITERATIONS)
do
    echo "Iteration $i"
    for R in $(seq 5 5 20)
    do
        echo "R = $R"
        ./router 10 64 5432 5433 5434 5435 50 yes 2>&1 > /dev/null &
        ./receiver 5433 > task31_iter_${i}_R_${R}_recv_1.csv &
        ./receiver 5435 > task31_iter_${i}_R_${R}_recv_2.csv
    done
done

echo "Task 3.2: "
for i in $(seq 1 $ITERATIONS)
do
    echo "Iteration $i"
    for R in $(seq 5 5 20)
    do
        echo "R = $R"
        for B in $(seq 32 32 128) 
        do
            ./router 10 $B 5432 5433 5434 5435 50 yes 2>&1 > /dev/null &
            ./receiver 5433 > task32_iter_${i}_R_${R}_B_${B}_recv_1.csv &
            ./receiver 5435 > task32_iter_${i}_R_${R}_B_${B}_recv_2.csv
         done
    done
done

echo "Task 3.3: " 
for i in $(seq 1 $ITERATIONS)
do
    echo "Iteration $i"
    for L in $(seq 5 5 20)
    do
        echo "L = $L"
        ./router $L 64 5432 5433 5434 5435 50 yes 2>&1 > /dev/null &
        ./receiver 5433 > task33_iter_${i}_B_${B}_recv_1.csv &
        ./receiver 5435 > task33_iter_${i}_B_${B}_recv_2.csv
    done
done

echo "Task 3.4: "
for i in $(seq 1 $ITERATIONS)
do
    echo "Iteration $i"
    for L in $(seq 5 5 20) 
    do
        echo "L = $L"
        for B in $(seq 32 32 128) 
        do
            ./router $L $B 5432 5433 5434 5435 50 yes 2>&1 > /dev/null &
            ./receiver 5433 > task34_iter_${i}_L_${L}_B_${B}_recv_1.csv &
            ./receiver 5435 > task34_iter_${i}_L_${L}_B_${B}_recv_2.csv
         done
    done
done


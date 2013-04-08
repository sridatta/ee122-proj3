#!/bin/bash

echo "Gathering data on receiver machine."

ITERATIONS=3

echo "Task 1: "
for i in {1..$ITERATIONS} do
    echo "Iteration $i"
    for R in {5..20..5} do
        echo "R = $R"
        ./router 10 64 5432 5433 2>&1 > /dev/null &
        ./receiver 5433 > task_1_iter_$i_R_$R.csv
    done
done

echo "Task 2: "
for i in {1..$ITERATIONS} do
    echo "Iteration $i"
    for R in {5..20..5} do
        echo "R = $R"
        ./router 10 64 5432 5433 5434 5435 50 2>&1 > /dev/null &
        ./receiver 5433 > task2_iter_$i_R_$R_recv_1.csv &
        ./receiver 5435 > task2_iter_$i_R_$R_recv_2.csv
    done
done

echo "Task 3.1: "
for i in {1..$ITERATIONS} do
    echo "Iteration $i"
    for R in {5..20..5} do
        echo "R = $R"
        ./router 10 64 5432 5433 5434 5435 50 yes 2>&1 > /dev/null &
        ./receiver 5433 > task31_iter_$i_R_$R_recv_1.csv &
        ./receiver 5435 > task31_iter_$i_R_$R_recv_2.csv
    done
done

echo "Task 3.2: "
for i in {1..$ITERATIONS} do
    echo "Iteration $i"
    for R in {5..20..5} do
        echo "R = $R"
        for B in {32..128..32} do
            ./router 10 $B 5432 5433 5434 5435 50 yes 2>&1 > /dev/null &
            ./receiver 5433 > task32_iter_$i_R_$R_B_$B_recv_1.csv &
            ./receiver 5435 > task32_iter_$i_R_$R_B_$B_recv_2.csv
         done
    done
done

echo "Task 3.3: " 
for i in {1..$ITERATIONS} do
    echo "Iteration $i"
    for L in {5..20..5} do
        echo "L = $L"
        ./router $L 64 5432 5433 5434 5435 50 yes 2>&1 > /dev/null &
        ./receiver 5433 > task33_iter_$i_B_$B_recv_1.csv &
        ./receiver 5435 > task33_iter_$i_B_$B_recv_2.csv
    done
done

echo "Task 3.4: "
for i in {1..$ITERATIONS} do
    echo "Iteration $i"
    for L in {5..20..5} do
        echo "L = $L"
        for B in {32..128..32} do
            ./router $L $B 5432 5433 5434 5435 50 yes 2>&1 > /dev/null &
            ./receiver 5433 > task34_iter_$i_L_$L_B_$B_recv_1.csv &
            ./receiver 5435 > task34_iter_$i_L_$L_B_$B_recv_2.csv
         done
    done
done


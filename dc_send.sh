#!/bin/bash

echo "Gathering data on sender machine."

ITERATIONS=3

echo "Task 1: "
for i in {1..$ITERATIONS} do
    echo "Iteration $i"
    for R in {5..20..5} do
        echo "R = $R"
        ./sender $1 5432 $R A
        sleep 2
    done
done

echo "Task 2: "
for i in {1..$ITERATIONS} do
    echo "Iteration $i"
    for R in {5..20..5} do
        echo "R = $R"
        ./sender $1 5432 $R A &
        ./sender $1 5434 $R B
        sleep 2
    done
done

echo "Task 3.1: "
for i in {1..$ITERATIONS} do
    echo "Iteration $i"
    for R in {5..20..5} do
        echo "R = $R"
        ./sender $1 5432 $R A &
        ./sender $1 5434 $R B
        sleep 2
    done
done

echo "Task 3.2: "
for i in {1..$ITERATIONS} do
    echo "Iteration $i"
    for R in {5..20..5} do
        echo "R = $R"
        for B in {32..128..32} do
            ./sender $1 5432 $R A &
            ./sender $1 5434 $R B
            sleep 2
         done
    done
done

R = 12
echo "Task 3.3: " 
for i in {1..$ITERATIONS} do
    echo "Iteration $i"
    for L in {5..20..5} do
        echo "R = 12"
         ./sender $1 5432 $R A &
         ./sender $1 5434 $R B
         sleep 2
    done
done

echo "Task 3.4: "
for i in {1..$ITERATIONS} do
    echo "Iteration $i"
    for L in {5..20..5} do
        echo "R = $R"
        for B in {32..128..32} do
         ./sender $1 5432 $R A &
         ./sender $1 5434 $R B
         sleep 2
        done
    done
done


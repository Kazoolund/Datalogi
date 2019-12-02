#!/bin/sh

for testname in tests/*; do
    ./master $testname | tee test_results/`basename $testname`run1
    ./master $testname | tee test_results/`basename $testname`run2
    ./master $testname | tee test_results/`basename $testname`run3
    ./master $testname | tee test_results/`basename $testname`run4
    ./master $testname | tee test_results/`basename $testname`run5
done

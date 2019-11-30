#!/bin/sh

for testname in tests/*; do
    ./master $testname | tee test_results/`basename $testname`
done

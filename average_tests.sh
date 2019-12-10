#!/bin/bash

for testname in tests/*; do
    awk -f average.awk test_results/`basename $testname`run* | tee average_test_results/`basename $testname`
done

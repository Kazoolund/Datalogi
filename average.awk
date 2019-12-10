BEGIN {
    total_runtime = 0
    total_waste_time = 0
    worker_count = 0
    runs = ARGC - 1
}

{
    if (FNR == 7) {
	worker_count = $2
	total_time_line = 13 + (2 * worker_count)
    }
    
    if (FNR == total_time_line)
	total_runtime += $6

    for (i = 1; i <= worker_count; i++) {
	if (FNR == 17 + (2 * worker_count) + (2 * i))
	    total_waste_time += $8
    }
}

END {
    split(FILENAME, filename, "run")
    print "Average for " filename[1] " (" runs " runs)"
    print "\tAverage total runtime " (total_runtime / runs)
    print "\tAverage wasted time " (total_waste_time / (runs * worker_count))
}

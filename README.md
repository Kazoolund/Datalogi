# P1 program for group A313

To compile the project, type `make`. Note that the program requires a unix like system
such as Linux or macOS. If it has to be run on windows, cygwin is needed.

After 'make' is run, the two binaries 'master' and 'worker' are produced.

The master program is the only program the user has to run, and it expects a settings file
as a commandline argument.

Many example setting files can be found in the tests folder.

To run all the tests from the tests folder and save their results, run 'make tests'. This
will run './master test-file' five times for each test file, and save the result into the
test_results folder.

To get the average of each of the five runs, run 'make average'. Note that this requires
awk to be installed on the system.

If the 'make tests' or 'make average' commands don't work, try to make the directories
they expect before running the commands. This can be done with 'mkdir test_results' and
'mkdir average_test_results'
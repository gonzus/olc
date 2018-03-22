A C implementation of Google's Open Location Code

Based on the project's C++ implementation, this is pure C, aiming for
efficiency and simplicity.

Try the following:

    # run a simple example
    make && ./example

    # run tests based on CSV data files (copied from the original project)
    make && ./test_csv

    # that last command outputs a lot; this only shows failing tests
    make && ./test_csv | egrep BAD

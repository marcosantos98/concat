#! /usr/bin/python3

import glob
import subprocess
from genericpath import exists
import sys

concat = "./main"

def check_concat():
    return exists(concat)

def program_has_record(filename):
    return exists(filename + ".out")

def run_program(filename):
    if not program_has_record(filename):
        return False
    run_output = subprocess.run(
            [concat, filename], 
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE, 
            check=False, 
            text=True)
    record_output = open(filename + ".out").read()
    if run_output.stdout == record_output:
        print(f"> Test {filename} \u001b[32mpassed.\u001b[0m")
    else:
        print(f"> Test {filename} \u001b[31mfailed.\u001b[0m")
        print("\u001b[32m=== Expected: ===\u001b[0m")
        print(record_output)
        print("\u001b[31m==== Actual ====\u001b[0m")
        print(run_output.stdout)
    return True

def record_program(filename):
    run_output = subprocess.run(
            ["./main", filename], 
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE, 
            check=False, 
            text=True)
    with open(filename + ".out", "w") as file:
        file.write(run_output.stdout)


if __name__ == "__main__":
    if not check_concat():
        print("Compile concat before running tests\n")

    argc = len(sys.argv)

    if argc == 1:
        print("Running all tests:")
        tests = glob.glob("./tests/*.cc")
        for test in tests:
            if not run_program(test):
                print(f"\u001b[33mTest {test} doesn't have a record\u001b[0m")
                print(f"\u001b[33m> ./test.py r {test}\u001b[0m")
        examples = glob.glob("./examples/*.cc")

        print("Running all examples:")
        for example in examples:
            if not run_program(example):
                print(f"\u001b[33mExample {example} doesn't have a record\u001b[0m")

    if argc >= 2:
        arg = sys.argv[1]
        if arg == "s":
            if argc >= 3:
                filename = sys.argv[2]
                if not run_program(filename):
                    print(f"Run of single program {filename} doesn't have a record")
            else:
                print("Usage: test.py s <filename>. Not enought arguments")
        elif arg == "r":
            if argc >= 3:
                filename = sys.argv[2]
                record_program(filename)
            else:
                print("Usage test.py r <filename>. Not enought arguments")

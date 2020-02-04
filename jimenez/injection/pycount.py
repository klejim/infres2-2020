import json
import os
import sys


def parse_text(text):
    letters = {}
    for c in text:
        if c.isalpha() and c in letters.keys():
            letters[c] += 1
        else: letters[c] = 1
    return letters


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Missing arguments\nUsage : pycount [file]")
        sys.exit(1)
    filename = sys.argv[1]
    conf = {}
    try:
        with open("conf.json", "r") as file:
            conf = json.loads(file.read())
    except IOError as err:
        print("Error loading config file : {}".format(err))
    try: 
        with open(filename, "r") as file:
            n_lines = None
            try:
                n_lines = int(conf["n_lines"] if "n_lines" in conf else '')
            except (ValueError, TypeError) as err:
                print("Warning : error reading config, n_lines is set to unlimited")
            letters = parse_text(" ".join(file.readlines(n_lines)))
            print(json.dumps(letters, indent=4))
    except IOError as err:
        print("Error : {}".format(err))
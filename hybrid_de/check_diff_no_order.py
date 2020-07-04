import sys

def check_diff(file1,file2):
    check = {}
    for file in [file1,file2]:
        with open(file,'r') as f:
            check[file] = []
            for line in f:
                check[file].append(line.rstrip())
    diff = set(check[file1]) - set(check[file2])
    for line in diff:
        print(line)

if __name__ == "__main__":
    check_diff(sys.argv[1], sys.argv[2])
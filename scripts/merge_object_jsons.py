import json, os

'''
Script that iterates through a directory & merges all the JSONs in it into a single file
'''

dir_path = 'res/json/objects'
out_path = 'res/json/objects.json'

level1 = os.listdir(dir_path)

data = []

entry_count = 0
file_count = 0

# print(filenames, os.getcwd())
for name in level1:
    fullpath = os.path.join(dir_path, name)

    if os.path.isdir(fullpath):
        filepaths = [os.path.join(fullpath, x) for x in os.listdir(fullpath)]
    else:
        filepaths = [fullpath]

    for path in filepaths:
        with open(path, 'r') as f:
            entry = json.load(f)
        file_count += 1
        
        if type(entry) == list:
            data += entry
            entry_count += len(entry)
        else:
            data.append(entry)
            entry_count += 1


with open(out_path, 'w') as f:
    json.dump(data, f)
print(f"Done (merged {file_count} files, {entry_count} entries)...")

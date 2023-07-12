import json, os

dir_path = 'spritesheets'
out_path = 'spritesheets.json'

dir_names = os.listdir(dir_path)

name_mapping = {
    'oc': 'orc',
    'hu': 'human'
}

data = []

for dir_name in dir_names:
    filenames = os.listdir(os.path.join(dir_path, dir_name))

    for fname in filenames:
        fpath = os.path.join(dir_path, dir_name, fname)
        with open(fpath, 'r') as f:
            entry = json.load(f)
        prefix = dir_name if (dir_name not in name_mapping) else name_mapping[dir_name]
        entry['name'] = "/".join([prefix, *fname.split('.')[:-1]])
        data.append(entry)


with open(out_path, 'w') as f:
    json.dump(data, f)
print("Done.")

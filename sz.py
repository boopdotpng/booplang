import os

script_dir = os.path.abspath(os.path.dirname(__file__))
line_counts = {}

for directory in ['src', 'include']:
    full_dir = os.path.join(script_dir, directory)
    for root, _, files in os.walk(full_dir):
        for file in files:
            if not (file.endswith('.c') or file.endswith('.h')):
                continue
            path = os.path.join(root, file)
            try:
                with open(path, 'r', encoding='utf-8') as f:
                    lc = sum(1 for _ in f)
                    if lc:
                        rel_path = os.path.relpath(path, script_dir)
                        line_counts[rel_path] = lc
            except UnicodeDecodeError:
                continue

# sorted and formatted output
header = f"{'Lines':>8} | File"
print(header)
print("-" * len(header))
for file, count in sorted(line_counts.items(), key=lambda x: x[1], reverse=True):
    print(f"{count:8} | {file}")
print("-" * len(header))
print(f"{sum(line_counts.values()):8} | total")

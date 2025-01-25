import os

line_counts = {}

for directory in ['src', 'include']:
    for root, _, files in os.walk(directory):
        for file in files:
            path = os.path.join(root, file)
            try:
                with open(path, 'r', encoding='utf-8') as f:
                    lc = sum(1 for _ in f)
                    if lc != 0:
                        line_counts[path] = lc
            except:
                print(f"Warning: Couldn't read {path}")

for file, count in sorted(line_counts.items(), key=lambda x: x[1], reverse=True):
    print(f"{count:6d} | {file}")
    
print("-" * 30)
print(f"{sum(line_counts.values()):6d} | total")

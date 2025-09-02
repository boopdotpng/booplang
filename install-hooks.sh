#!/bin/sh

HOOKS_DIR=".git/hooks"
HOOK_FILE="$HOOKS_DIR/pre-commit"

mkdir -p "$HOOKS_DIR"

cat > "$HOOK_FILE" << 'EOF'
#!/bin/sh
echo "Running clang-format on all source files..."

if command -v clang-format >/dev/null 2>&1; then
    find src/ -name '*.c' -o -name '*.h' | xargs clang-format -i
else
    echo "clang-format not found, skipping formatting."
fi
EOF

chmod +x "$HOOK_FILE"

echo "pre-commit hook installed!"

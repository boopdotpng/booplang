#!/bin/sh

HOOKS_DIR=".git/hooks"
HOOK_FILE="$HOOKS_DIR/pre-commit"

mkdir -p "$HOOKS_DIR"

cat > "$HOOK_FILE" << 'EOF'
#!/bin/sh
echo "Running clang-format on staged files..."

if command -v clang-format >/dev/null 2>&1; then
    # Get staged .c and .h files and format them
    git diff --name-only --cached -- '*.c' '*.h' | xargs -r clang-format -i
    
    # Re-add formatted files to staging
    git diff --name-only --cached -- '*.c' '*.h' | xargs -r git add
else
    echo "clang-format not found, skipping formatting."
fi
EOF

chmod +x "$HOOK_FILE"

echo "pre-commit hook installed!"
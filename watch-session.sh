#!/usr/bin/env bash
# watch-session.sh — tail the latest Claude session and show what it's doing
#
# Usage:
#   ./watch-session.sh              # watches latest session file
#   ./watch-session.sh <session-id> # watches a specific session

PROJECT_DIR="$HOME/.claude/projects/-Users-hlimas-Development-github-marko-tmbundle-tree-sitter-marko"

if [[ -n "${1:-}" ]]; then
  FILE="$PROJECT_DIR/$1.jsonl"
else
  FILE=$(ls -t "$PROJECT_DIR"/*.jsonl | head -1)
fi

if [[ ! -f "$FILE" ]]; then
  echo "Session file not found: $FILE"
  exit 1
fi

echo "Watching: $FILE"
echo ""

tail -f "$FILE" | while IFS= read -r line; do
  type=$(echo "$line" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('type',''))" 2>/dev/null)

  if [[ "$type" == "assistant" ]]; then
    echo "$line" | python3 -c "
import sys, json
d = json.load(sys.stdin)
msg = d.get('message', {})
for block in msg.get('content', []):
    if block['type'] == 'tool_use':
        tool = block['name']
        inp = block.get('input', {})
        detail = inp.get('command') or inp.get('file_path') or inp.get('pattern') or ''
        print(f'  [{tool}] {str(detail)[:120]}')
    elif block['type'] == 'text':
        text = block['text'].strip()
        if text:
            print(f'  {text[:120]}')
" 2>/dev/null
  fi
done

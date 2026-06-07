# Project Todo List — REDIRECT

> ⚠️ This file is a navigation stub. **Do NOT edit it.**
> The live project backlog lives on the `todo_list` branch, accessed via a git worktree.

---

## Accessing the live todo list

The live file is at: **`.agents/todo/todo.md`**

### Normal case (worktree already set up)

```bash
# Read
cat .agents/todo/todo.md

# Write — always commit from within the worktree directory
cd .agents/todo
# edit todo.md
git add todo.md
git commit -m "todo: <description>"
git push origin todo_list
cd -
```

### Fresh clone (worktree not yet set up)

```bash
git worktree add .agents/todo todo_list
```

Then proceed as above.

---

See `.agents/workflows/todo.md` for the full `/todo` workflow (add, update, review).

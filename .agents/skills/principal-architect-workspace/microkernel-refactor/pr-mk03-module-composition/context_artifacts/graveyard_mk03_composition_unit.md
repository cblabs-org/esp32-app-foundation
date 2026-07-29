# Graveyard: Thin ModuleComposition Unit

**Attempted Concept:** Creating a framework-owned `ModuleComposition.cpp` that reads enable flags and registers modules.
**Why it was abandoned:** It obscured the composition order and re-introduced framework ownership of optional modules in a disguised form. The "honest main.cpp" approach was preferred as it enforces the discipline of treating framework optionals identically to user app modules.

# Graveyard: App Tab DOM Race Conditions

## Failed Attempt: Synchronous Initialization
**What we tried:**
We placed synchronous DOM queries at the top level of `appconfig.js`:
```javascript
var container = document.getElementById('tab-app');
if (!container) return;
container.innerHTML = '...';
```

**Why it failed:**
The `loadScripts()` array in `app.js` loads the files sequentially and executes them *before* the asynchronous `/api/tabs` endpoint completes and triggers `buildBottomNav()`. Therefore, `document.getElementById('tab-app')` returned `null`, and the script silently aborted. When the user later clicked the tab, the container was created but remained blank because the initialization logic had already exited.

**The Fix:**
Wrap the DOM initialization in an event listener bound to `tab-changed`.

## Failed Attempt: C++ virtual method definitions in .cpp
**What we tried:**
We accidentally placed the inline implementation of `int getSchemaVersion() const override { return 2; }` into `U8G2DisplayModule.cpp` outside of a class scope block.

**Why it failed:**
The compiler flagged `virt-specifiers in 'getSchemaVersion' not allowed outside a class definition` because `override` is illegal on out-of-line method definitions, and the method lacked the `U8G2DisplayModule::` scope resolution anyway.

**The Fix:**
Removed the duplicate code block from `.cpp` and relied solely on the correct inline definition within `U8G2DisplayModule.hpp`.

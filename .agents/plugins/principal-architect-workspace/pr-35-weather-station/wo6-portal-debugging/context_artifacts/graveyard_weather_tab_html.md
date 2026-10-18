# Graveyard: Raw HTML Fragments (`tab.html`)

## Dead End
Attempting to serve a raw `tab.html` fragment from a Module's API endpoint (e.g. `/api/weather/tab.html`) does not work. 

## Why it failed
The core `app.js` script does not dynamically `fetch()` tab contents from the server over HTTP on click. It simply flips the CSS visibility of pre-rendered divs (`.active` class). 
Therefore, if a module only serves `tab.html`, the browser never downloads it and the tab remains permanently blank. The SPA architecture relies entirely on `<script src="...">` tags loaded during initialization.

## Resolution
Removed `tab.html` serving from `WeatherModule`. Swapped to serving `/weather.js`, which leverages the existing Javascript execution path to inject HTML exactly when the user navigates to the tab.

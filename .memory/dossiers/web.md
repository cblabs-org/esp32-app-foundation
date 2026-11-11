# web

## Invariants & contracts

### [docseed-7925611b] **Delivering a tab script (canonical): the gzip+PROGMEM pipeline.** A tab's JavaScript is served over an HTTP route your module registers in `registerRoutes()`. The **canonical** way to deliver it — for framework modules *and consuming apps* — is the framework's gzip+PROGMEM asset pipeline, the same one used for `app.js` and friends: Put the script in a `web/` directory: `modules/<Name>/web...
`invariant · kind: semantic · anchors: web · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

A Data Flow Tracker and Reference Monitor for WebKit and JavaScriptCore
=======================================================================

This repo is a heavily-patched version of WebKit from around May 2012,
implementing a data flow tracker and reference monitor as part of my (Andrew
Bloomgarden) senior undergrad thesis for Dartmouth College's computer science
program. I'll update this with a link to the Technical Report filed with
Dartmouth when it's up.

Abstract
--------

Browser security revolves around the same-origin policy, but it does not defend
against all attacks as evidenced by the prevalence of cross-site scripting
attacks. Rather than solve that attack in particular, I have opted for a more
general solution. I have modified WebKit to allow data flow tracking via labels
and to allow security-sensitive operations to be allowed or denied from
JavaScript.

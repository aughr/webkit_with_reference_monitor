if (this.importScripts) {
    importScripts('../../../fast/js/resources/js-test-pre.js');
    importScripts('shared.js');
}

description("Test IndexedDB's openCursor.");

function emptyCursorSuccess()
{
    debug("Empty cursor opened successfully.")
    // FIXME: check that we can iterate the cursor.
    finishJSTest();
}

function openEmptyCursor()
{
    debug("Opening an empty cursor.");
    keyRange = IDBKeyRange.lowerBound("InexistentKey");
    request = evalAndLog("objectStore.openCursor(keyRange)");
    request.onsuccess = emptyCursorSuccess;
    request.onerror = unexpectedErrorCallback;
}

function cursorSuccess()
{
    debug("Cursor opened successfully.")
    // FIXME: check that we can iterate the cursor.
    shouldBe("event.target.result.direction", "0");
    shouldBe("event.target.result.key", "'myKey'");
    shouldBe("event.target.result.value", "'myValue'");
    debug("");
    try {
        debug("Passing an invalid key into .continue({}).");
        event.target.result.continue({});
        testFailed("No exception thrown");
    } catch (e) {
        testPassed("Caught exception: " + e.toString());
    }
    debug("");
    openEmptyCursor();
}

function openCursor()
{
    debug("Opening cursor");
    keyRange = IDBKeyRange.lowerBound("myKey");
    request = evalAndLog("event.target.source.openCursor(keyRange)");
    request.onsuccess = cursorSuccess;
    request.onerror = unexpectedErrorCallback;
}

function setVersionSuccess()
{
    debug("setVersionSuccess():");
    self.trans = evalAndLog("trans = event.target.result");
    shouldBeTrue("trans !== null");
    trans.onabort = unexpectedAbortCallback;

    deleteAllObjectStores(db);

    var objectStore = evalAndLog("objectStore = db.createObjectStore('test')");
    request = evalAndLog("objectStore.add('myValue', 'myKey')");
    request.onsuccess = openCursor;
    request.onerror = unexpectedErrorCallback;
}

function openSuccess()
{
    var db = evalAndLog("db = event.target.result");

    request = evalAndLog("db.setVersion('new version')");
    request.onsuccess = setVersionSuccess;
    request.onerror = unexpectedErrorCallback;
}

function test()
{
    removeVendorPrefixes();
    request = evalAndLog("indexedDB.open('open-cursor')");
    request.onsuccess = openSuccess;
    request.onerror = unexpectedErrorCallback;
}

test();
if (this.importScripts) {
    importScripts('../../../fast/js/resources/js-test-pre.js');
    importScripts('shared.js');
}

description("Verify that you can put the same data in 2 different databases without uniqueness constraints firing.");

testCount = 0;
function test()
{
    if (testCount++ == 0)
        request = evalAndLog("indexedDB.open('duplicates-1')");
    else
        request = evalAndLog("indexedDB.open('duplicates-2')");
    request.onsuccess = setVersion;
    request.onerror = unexpectedErrorCallback;
}

function setVersion()
{
    db = evalAndLog("db = event.target.result");

    request = evalAndLog("db.setVersion('new version')");
    request.onsuccess = deleteExisting;
    request.onerror = unexpectedErrorCallback;
}

function deleteExisting()
{
    debug("setVersionSuccess():");
    self.trans = evalAndLog("trans = event.target.result");
    shouldBeTrue("trans !== null");
    trans.onabort = unexpectedAbortCallback;

    deleteAllObjectStores(db);

    self.store = evalAndLog("db.createObjectStore('storeName', null)");
    self.indexObject = evalAndLog("store.createIndex('indexName', 'x')");
    addData();
}

function addData()
{
    request = evalAndLog("store.add({x: 'value', y: 'zzz'}, 'key')");
    request.onsuccess = addMore;
    request.onerror = unexpectedErrorCallback;
}

function addMore()
{
    request = evalAndLog("event.target.source.add({x: 'value2', y: 'zzz2'}, 'key2')");
    request.onsuccess = getData;
    request.onerror = unexpectedErrorCallback;
}

function getData()
{
    request = evalAndLog("indexObject.getKey('value')");
    request.onsuccess = getObjectData;
    request.onerror = unexpectedErrorCallback;
}

function getObjectData()
{
    shouldBeEqualToString("event.target.result", "key");

    request = evalAndLog("indexObject.get('value')");
    request.onsuccess = getDataFail;
    request.onerror = unexpectedErrorCallback;
}

function getDataFail()
{
    shouldBeEqualToString("event.target.result.x", "value");
    shouldBeEqualToString("event.target.result.y", "zzz");

    request = evalAndLog("indexObject.getKey('does not exist')");
    request.onsuccess = getObjectDataFail;
    request.onerror = unexpectedErrorCallback;
}

function getObjectDataFail()
{
    shouldBe("event.target.result", "undefined");

    request = evalAndLog("indexObject.get('does not exist')");
    request.onsuccess = openKeyCursor;
    request.onerror = unexpectedErrorCallback;
}

function openKeyCursor()
{
    shouldBe("event.target.result", "undefined");

    self.request = evalAndLog("indexObject.openKeyCursor()");
    request.onsuccess = cursor1Continue;
    request.onerror = unexpectedErrorCallback;
}

function cursor1Continue()
{
    shouldBeFalse("event.target.result === null");
    shouldBeEqualToString("event.target.result.key", "value");
    shouldBeEqualToString("event.target.result.primaryKey", "key");

    // We re-use the last request object.
    evalAndLog("event.target.result.continue()");
    self.request.onsuccess = cursor1Continue2;
}

function cursor1Continue2()
{
    shouldBeFalse("event.target.result === null");
    shouldBeEqualToString("event.target.result.key", "value2");
    shouldBeEqualToString("event.target.result.primaryKey", "key2");

    // We re-use the last request object.
    evalAndLog("event.target.result.continue()");
    self.request.onsuccess = openObjectCursor;
}

function openObjectCursor()
{
    shouldBeTrue("event.target.result === null");

    self.request = evalAndLog("indexObject.openCursor()");
    request.onsuccess = cursor2Continue;
    request.onerror = unexpectedErrorCallback;
}

function cursor2Continue()
{
    shouldBeFalse("event.target.result === null");
    shouldBeEqualToString("event.target.result.key", "value");
    shouldBeEqualToString("event.target.result.value.x", "value");
    shouldBeEqualToString("event.target.result.value.y", "zzz");

    // We re-use the last request object.
    evalAndLog("event.target.result.continue()");
    self.request.onsuccess = cursor2Continue2;
}

function cursor2Continue2()
{
    shouldBeFalse("event.target.result === null");
    shouldBeEqualToString("event.target.result.key", "value2");
    shouldBeEqualToString("event.target.result.value.x", "value2");    
    shouldBeEqualToString("event.target.result.value.y", "zzz2");

    // We re-use the last request object.
    evalAndLog("event.target.result.continue()");
    self.request.onsuccess = last;
}

function last()
{
    shouldBeTrue("event.target.result === null");

    if (testCount == 1)
        test();
    else
        finishJSTest();
}

removeVendorPrefixes();
test();
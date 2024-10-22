if (this.importScripts) {
    importScripts('../../../fast/js/resources/js-test-pre.js');
    importScripts('shared.js');
}

description("Test IndexedDB keyPaths");

function test()
{
    removeVendorPrefixes();

    name = self.location.pathname;
    description = "My Test Database";
    request = evalAndLog("indexedDB.open(name, description)");
    request.onsuccess = openSuccess;
    request.onerror = unexpectedErrorCallback;
}

function openSuccess()
{
    db = evalAndLog("db = event.target.result");

    request = evalAndLog("request = db.setVersion('1')");

    testData = [{ name: "simple identifier",
                  value: {id:10},
                  keyPath: "id",
                  key: 10 },
                { name: "simple identifiers",
                  value: {id1:10, id2:20},
                  keyPath: "id1",
                  key: 10 },
                { name: "nested identifiers",
                  value: {outer:{inner:10}},
                  keyPath: "outer.inner",
                  key: 10 },
                { name: "nested identifiers with distractions",
                  value: {outer:{inner:10}, inner:{outer:20}},
                  keyPath: "outer.inner",
                  key: 10 },
    ];
    nextToOpen = 0;
    request.onsuccess = createAndPopulateObjectStore;
    request.onerror = unexpectedErrorCallback;
}

function createAndPopulateObjectStore()
{
    debug("");
    debug("testing " + testData[nextToOpen].name);

    deleteAllObjectStores(db);

    objectStore = evalAndLog("objectStore = db.createObjectStore(testData[nextToOpen].name, {keyPath: testData[nextToOpen].keyPath});");
    result = evalAndLog("result = objectStore.add(testData[nextToOpen].value);");
    result.onerror = unexpectedErrorCallback;
    result.onsuccess = openCursor;
}

function openCursor()
{
    result = evalAndLog("result = objectStore.openCursor();");
    result.onerror = unexpectedErrorCallback;
    result.onsuccess = checkCursor;
}

function checkCursor()
{
    cursor = evalAndLog("cursor = event.target.result;");
    if (cursor) {
        shouldBe("cursor.key", "testData[nextToOpen].key");
    } else {
        testFailed("cursor is null");
    }
    if (++nextToOpen < testData.length) {
        createAndPopulateObjectStore();
    } else {
        finishJSTest();
    }
}

test();
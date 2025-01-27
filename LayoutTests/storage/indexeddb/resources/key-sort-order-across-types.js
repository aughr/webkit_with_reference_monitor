if (this.importScripts) {
    importScripts('../../../fast/js/resources/js-test-pre.js');
    importScripts('shared.js');
}

description("Test IndexedDB key comparison");

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
    request.onsuccess = addKey1;
    request.onerror = unexpectedErrorCallback;
}

function addKey1()
{
    deleteAllObjectStores(db);

    objectStore = evalAndLog("db.createObjectStore('foo');");
    request = evalAndLog("request = objectStore.add([], Infinity);");
    request.onsuccess = addKey2;
    request.onerror = unexpectedErrorCallback;
}

function addKey2()
{
    request = evalAndLog("request = objectStore.add([], -Infinity);");
    request.onsuccess = addKey3;
    request.onerror = unexpectedErrorCallback;
}

function addKey3()
{
    request = evalAndLog("request = objectStore.add([], 1.0);");
    request.onsuccess = addKey4;
    request.onerror = unexpectedErrorCallback;
}

function addKey4()
{
    request = evalAndLog("request = objectStore.add([], '');");
    request.onsuccess = addKey5;
    request.onerror = unexpectedErrorCallback;
}

function addKey5()
{
    request = evalAndLog("request = objectStore.add([], '1.0');");
    request.onsuccess = openACursor;
    request.onerror = unexpectedErrorCallback;
}

function openACursor()
{
    keyIndex = 0;
    sortedKeys = evalAndLog("sortedKey = [-Infinity, 1.0, Infinity, '', '1.0'];");
    request = evalAndLog("request = objectStore.openCursor();");
    request.onerror = unexpectedErrorCallback;
    request.onsuccess = function (event) {
        cursor = evalAndLog("cursor = event.target.result;");
        if (cursor) {
            shouldBe("cursor.key", "sortedKeys[keyIndex]");
            evalAndLog("cursor.continue();");
            evalAndLog("keyIndex++;");
        }
        else {
            finishJSTest();
        }
    }
}

test();
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
    date1 = evalAndLog("date1 = new Date(1000);");
    request = evalAndLog("request = objectStore.add([], date1);");
    request.onsuccess = addKey2;
    request.onerror = unexpectedErrorCallback;
}

function addKey2()
{
    date0 = evalAndLog("date0 = new Date(0);");
    request = evalAndLog("request = objectStore.add([], date0);");
    request.onsuccess = addKey3;
    request.onerror = unexpectedErrorCallback;
}

function addKey3()
{
    date3 = evalAndLog("date3 = new Date(3000);");
    request = evalAndLog("request = objectStore.add([], date3);");
    request.onsuccess = addKey4;
    request.onerror = unexpectedErrorCallback;
}

function addKey4()
{
    date2 = evalAndLog("date2 = new Date(2000);");
    request = evalAndLog("request = objectStore.add([], date2);");
    request.onsuccess = openACursor;
    request.onerror = unexpectedErrorCallback;
}

function openACursor()
{
    keyIndex = 0;
    sortedKeys = evalAndLog("sortedKeys = [date0.toString(), date1.toString(), date2.toString(), date3.toString()];");
    request = evalAndLog("request = objectStore.openCursor();");
    request.onerror = unexpectedErrorCallback;
    request.onsuccess = function (event) {
        cursor = evalAndLog("cursor = event.target.result;");
        if (cursor) {
            shouldBe("cursor.key.toString()", "sortedKeys[keyIndex]");
            evalAndLog("cursor.continue();");
            evalAndLog("keyIndex++;");
        }
        else {
            finishJSTest();
        }
    }
}

test();
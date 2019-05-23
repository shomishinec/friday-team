const types = {
    loginRequest: "loginRequest",
    logintResponse: "loginResponse",
    loadDataRequest: "loadDataRequest",
    loadDataResponse: "loadDataReponse",
    setDataRequest: "setDataRequest",
    setDataResponse: "setDataResponse",
    datalockedByAnotherUser: "datalockedByAnotherUser",
    dataHasModified: "dataHasModified",
    dataFreeByAnotherUser: "dataFreeByAnotherUser"
}

if (module && module.exports) {
    module.exports = types;
}

if (window) {
    window.types = types;
}
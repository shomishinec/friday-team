const types = {
    loginRequest: "loginRequest",
    loginResponse: "loginResponse",
    loadDataRequest: "loadDataRequest",
    loadDataResponse: "loadDataReponse",
    setDataRequest: "setDataRequest",
    setDataResponse: "setDataResponse",
    datalockedByAnotherUser: "datalockedByAnotherUser",
    dataHasModified: "dataHasModified",
    dataFreeByAnotherUser: "dataFreeByAnotherUser"
}

if (typeof window !== "undefined") {
    window.types = types;
} else {
    module.exports = types;
}
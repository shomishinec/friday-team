const types = {
    loginRequest: "loginRequest",
    loginResponse: "loginResponse",
    loadDataRequest: "loadDataRequest",
    loadDataResponse: "loadDataReponse",
    setDataRequest: "setDataRequest",
    setDataResponse: "setDataResponse",
    lockDataRequest: "lockDataRequest",
    lockDataResponse: "lockDataResponse",
    dataLocked: "dataLocked",
    dataFree: "dataFree",
    dataHasModified: "dataHasModified",
}

if (typeof window !== "undefined") {
    window.types = types;
} else {
    module.exports = types;
}
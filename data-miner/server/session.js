const crypto = require("crypto");
const config = require("./config");
const sessions = {};
const sessionExpirators = {};

const startExpiration = (login) => {
    if (sessionExpirators[login]) {
        clearTimeout(sessionExpirators[login]);
    }
    sessionExpirators[login] = setTimeout(() => {
        stop(login)
    }, config.sessionExpirationTimeout);
};

const start = (login) => {
    sessions[login] = crypto.createHash("md5").update(`${login}${Date.now().toString()}`).digest("hex");
    startExpiration(login);
    return sessions[login];
};

const stop = (login) => {
    delete sessions[login];
    clearTimeout(sessionExpirators[login]);
    delete sessionExpirators[login];
};

const check = (auth) => {
    if (sessions[auth.login] === auth.key) {
        startExpiration(auth.login);
        return true;
    }
    return false;
};

module.exports = {
    start,
    stop,
    check,
};
const Classes = {};

Classes.Auth = class Auth {
    constructor(login, key) {
        this.login = login || "";
        this.key = key || "";
    }

    static fromDto(dto) {
        dto = dto || {};
        return new Classes.Auth(dto.login, dto.key)
    }
}

Classes.Request = class Request {
    constructor(auth, data) {
        this.auth = auth;
        this.data = data;
    }
    static fromDto(dto, map) {
        dto = dto || {};
        return new Classes.Request(dto.auth, map && dto.data ? Array.isArray(dto.data) ? dto.data.map(map) : map(dto.data) : null);
    }
}

Classes.Response = class Response {
    constructor(isSuccess, data, error) {
        this.isSuccess = isSuccess;
        this.error = error;
        this.data = data;
    }

    static fromDto(dto, map) {
        dto = dto || {};
        return new Classes.Response(dto.isSuccess, map && dto.data ? Array.isArray(dto.data) ? dto.data.map(map) : map(dto.data) : null, dto.error);
    }
}

Classes.User = class User {
    constructor(login, password) {
        this.login = login || "";
        this.password = password || "";
    }

    static fromDto(dto) {
        dto = dto || {};
        return new Classes.User(dto.login, dto.password)
    }
}

Classes.DataItemFrameAssumption = class DataItemFrameAssumption {
    constructor(login, assumption) {
        this.login = login || "";
        this.assumption = assumption || "";
    }

    static fromDto(dto) {
        dto = dto || {};
        return new Classes.DataItemFrameAssumption(dto.login, dto.assumption)
    }
}

Classes.DataItemFrame = class DataItemFrame {
    constructor(id, assumptions) {
        this.id = id || 0;
        this.assumptions = assumptions || [];
    }

    static fromDto(dto) {
        dto = dto || {};
        return new Classes.DataItemFrame(dto.id, dto.assumptions ? dto.assumptions.map(Classes.DataItemFrameAssumption.fromDto) : null)
    }
}

Classes.DataItem = class DataItem {
    constructor(id, duration, bytesCount, isBisy, spectrogramUrl, waveGraphUrl, frames) {
        this.id = id || 0;
        this.duration = duration || 0;
        this.bytesCount = bytesCount || 0;
        this.isBisy = isBisy || false;
        this.spectrogramUrl = spectrogramUrl || "";
        this.waveGraphUrl = waveGraphUrl || "";
        this.frames = frames || [];
    }

    static fromDto(dto) {
        dto = dto || {};
        return new Classes.DataItem(dto.id, dto.duration, dto.bytesCount, dto.isBisy, dto.spectrogramUrl, dto.waveGraphUrl, dto.frames ? dto.frames.map(Classes.DataItemFrame.fromDto) : null)
    }
}

Classes.DB = class DB {
    constructor(users, dataItems) {
        this.users = users || [];
        this.dataItems = dataItems || [];
    }

    static fromDto(dto) {
        dto = dto || {};
        return new Classes.DB(dto.users ? dto.users.map(Classes.User.fromDto) : null, dto.dataItems ? dto.dataItems.map(Classes.DataItem.fromDto) : null)
    }
}

if (typeof window !== "undefined") {
    window.classes = Classes;
} else {
    module.exports = Classes;
}
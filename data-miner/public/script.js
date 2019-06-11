
$(function () {
    this.db = ko.observableArray();

    ko.components.register("login-form", {
        viewModel: function (params) {
            this.login = ko.observable();
            this.password = ko.observable();
            this.onSubmit = () => {

            };
        },
        template: ""
    });

    ko.components.register("data-list", {
        viewModel: function (params) {
            this.data = ko.observableArray();
        },
        template: ""
    });

    ko.applyBindings();

    const parse = (dto, Class) => {
        return classes.Response.fromDto(JSON.parse(dto), Class ? Class.fromDto : null);
    }

    const req = (auth, model) => {
        return new window.classes.Request(auth, model);
    }

    const loginRequest = (userName, password) => { }

    const onLoginResponse = () => { }

    const loadDataRequest = () => { }

    const loadDataResponse = () => { };

    const setDataRequest = () => { };

    const setDataResponse = () => { };

    const datalockedByAnotherUser = () => { };

    const dataHasModified = () => { };

    const dataFreeByAnotherUser = () => { };

    const socket = io();

    socket.emit(window.types.loginRequest, req(null, new classes.User("grandead", "welcome_grandead")));
    socket.on(window.types.loginResponse, (dto) => {
        const auth = parse(dto, classes.Auth).data;
        socket.emit(window.types.loadDataRequest, req(auth));
    });
    socket.on(window.types.loadDataResponse, (dto) => {
        const data = parse(dto, classes.DataItem).data;
    });
});




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

    socket.emit(window.types.loadDataRequest, new window.classes.Request());
});



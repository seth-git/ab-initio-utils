(function(appName) {
  "use strict";
  angular
    .module(appName, [])
    .controller('Controller', Controller);

  var pages = [
    {
      key: 'intro',
      title: 'Introduction'
    },
    {
      key: 'input',
      title: 'Input Files'
    },
    {
      key: 'install',
      title: 'Installation Instructions'
    }
  ];

  Controller.$inject = ['$window','$timeout'];
  function Controller($window, $timeout) {
    var vm = this;

    vm.toggleShow = toggleShow;

    vm.pages = pages;
    toggleShow(vm.pages[0]);

    function toggleShow(page) {
      page.show = !page.show;
      if (page.show) {
        vm.pages.forEach(function (pg) {
          if (pg !== page)
            pg.show = false;
        });
        $timeout(function() {
          $window.location.href = '#' + page.key;
        });
      }
    }
  }
}('ab-initio-utils'));



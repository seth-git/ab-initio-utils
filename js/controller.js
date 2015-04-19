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
			key: 'install',
			title: 'Installation Instructions'
		}
	];

  function Controller() {
    var vm = this;

		vm.toggleShow = toggleShow;

		vm.pages = pages;
		toggleShow(vm.pages[vm.pages.length-1]);

		function toggleShow(page) {
			page.show = !page.show;
			if (page.show)
				vm.pages.forEach(function (pg) {
					if (pg !== page)
						pg.show = false;
				});
		}
  }
}('ab-initio-utils'));



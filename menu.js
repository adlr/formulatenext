let menuOpen = function() { console.log('menuOpen'); }
let menuClose = function() { console.log('menuClose'); }
let menuSave = function() { console.log('menuSave'); }
let menuSaveAs = function() { console.log('menuSaveAs'); }
let menuPrint = function() { console.log('menuPrint'); }
let menuUndo = function() { console.log('menuUndo'); }
let menuRedo = function() { console.log('menuRedo'); }
let menuCut = function() { console.log('menuCut'); }
let menuCopy = function() { console.log('menuCopy'); }
let menuPaste = function() { console.log('menuPaste'); }
let menuZoomOut = function() { console.log('menuZoomOut'); }
let menuZoomIn = function() { console.log('menuZoomIn'); }
let menuView100 = function() { console.log('menuView100'); }

let menus = [
  {name: "File", children: [
    {name: "Open...", key: "o", action: menuOpen},
    {name: "-"},
    {name: "Close", key: "w", action: menuClose},
    {name: "Save", key: "s", action: menuSave},
    {name: "Save As..", key: "S", action: menuSaveAs},
    {name: "-"},
    {name: "Print...", key: "P", action: menuPrint}
  ]},
  {name: "Edit", children: [
    {name: "Undo", key: "z", action: menuUndo},
    {name: "Redo", key: "Z", action: menuRedo},
    {name: "-"},
    {name: "Cut", key: "x", action: menuCut},
    {name: "Copy", key: "c", action: menuCopy},
    {name: "Paste", key: "v", action: menuPaste},
  ]},
  {name: "View", children: [
    {name: "Zoom", children: [
      {name: "Out", key: "-", action: menuZoomOut},
      {name: "In", key: "=", action: menuZoomIn},
      {name: "100%", key: "0", action: menuView100}
    ]}
  ]}
];

let populateSubmenu = function(items, parent) {
  let container = document.createElement('div');
  container.classList.add('submenucontainer');
  parent.appendChild(container);
  for (let i = 0; i < items.length; i++) {
    let item = items[i];
    let div = document.createElement("div");
    div.innerHTML = item.name;
    if (item.name == "-") {
      div.classList.add('menuspacer');
    } else {
      div.classList.add('menuitem');
      div.addEventListener('click', function() {
	console.log('clicked');
      });
    }
    container.appendChild(div);
    if (item.hasOwnProperty('children')) {
      populateSubmenu(item.children, div);
    }
  }
}

let populateDom = function(items, container) {
  for (let i = 0; i < items.length; i++) {
    let item = items[i];
    let div = document.createElement("div");
    div.innerHTML = item.name;
    div.classList.add('rootmenuitem');
    div.addEventListener('click', function() {
      console.log('clicked');
    });
    container.appendChild(div);
    if (item.hasOwnProperty('children')) {
      populateSubmenu(item.children, div);
    }
  }
}

document.addEventListener('DOMContentLoaded', function() {
  // create dom elements
  populateDom(menus, document.getElementById('menubar'));
}, false);


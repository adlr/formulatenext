'use strict';

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
    {name: "Save As...", key: "S", action: menuSaveAs},
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
    ]},
    {name: "Zoom2", children: [
      {name: "Out", key: "-", action: menuZoomOut},
      {name: "In", children: [
        {name: "Out", key: "-", action: menuZoomOut},
        {name: "In", key: "=", action: menuZoomIn},
        {name: "100%", key: "0", action: menuView100}
      ]},
      {name: "100%", key: "0", action: menuView100}
    ]}
  ]}
];

let enabledKeyShortcuts = {
};

class MenuItem {
  constructor(str, shortcut) {
    this.str = str;
    this.shortcut = shortcut;
    this.enabled = true;
    this.child = null;
    this.div = document.createElement('div');
    this.div.classList.add('menuitem');
    this.leftDiv = document.createElement('div');
    this.leftDiv.classList.add('menuitemname');
    this.div.appendChild(this.leftDiv);
    this.rightDiv = document.createElement('div');
    this.rightDiv.classList.add('menuitemright');
    this.div.appendChild(this.rightDiv);
    this.updateDom();
  }
  setEnabled(en) {
    this.enabled = en;
  }
  isSpacer() {
    return this.str === '-';
  }
  updateDom() {
    if (this.isSpacer()) {
      this.div.classList
    }
    this.leftDiv.innerHTML = this.str;
    if (this.shortcut)
      this.rightDiv.innerHTML = shortcutKeyToStr(this.shortcut);
    else if (this.child)
      this.rightDiv.innerHTML = '&#9658;';
    else
      this.rightDiv.innerHTML = '';
  }
  setContainer(container) {
    if (this.child)
      this.child.div.remove();
    this.child = container;
    this.div.appendChild(this.child.div);
  }
}

class MenuContainer {
  constructor() {
    this.items = [];
    this.div = document.createElement('div');
    this.div.classList.add('submenucontainer');
    this.div.addEventListener('click', ev => { ev.stopPropagation(); });
  }
  addMenuItem(item) {
    this.items.push(item);
    this.div.appendChild(item.div);
  }
}

class MenuBarLabel {
  constructor(str) {
    this.str = str;
    this.div = document.createElement('div');
    this.div.classList.add('rootmenuitem');
    this.div.innerHTML = str;
    this.container = null;
    this.handleClick = null;
    this.domClickHandler = ev => {
      console.log('label got a click');
      this.handleClick(this);
      ev.stopPropagation();
    };
    this.handleHover = null;
    this.domEnterHoverHandler = ev => { this.onHover(ev, true); };
    this.domLeaveHoverHandler = ev => { this.onHover(ev, false); };
  }
  setOnClick(handleClick) {
    if (this.handleClick != handleClick) {
      if (handleClick)
        this.div.addEventListener('click', this.domClickHandler);
      else
        this.div.removeEventListener('click', this.domClickHandler);
    }
    this.handleClick = handleClick;
  }
  onHover(ev, on) {
    if (this.handleHover)
      this.handleHover(this, on);
  }
  setOnHover(handleHover) {
    if (this.handleHover != handleHover) {
      if (handleHover) {
        this.div.addEventListener('mouseenter', this.domEnterHoverHandler);
        this.div.addEventListener('mouseenter', this.domLeaveHoverHandler);
      } else {
        this.div.removeEventListener('mouseenter', this.domEnterHoverHandler);
        this.div.removeEventListener('mouseenter', this.domLeaveHoverHandler);
      }
    }
    this.handleHover = handleHover;
  }
  setContainer(container) {
    if (this.container) {
      this.container.div.remove();
    }
    this.container = container;
    this.div.appendChild(this.container.div);
  }
  setChildVisible(vis) {
    if (vis) {
      this.div.classList.add('rootmenuitemactive');
    } else {
      this.div.classList.remove('rootmenuitemactive');
    }
    this.container.div.style.display = vis ? 'block' : 'none';
  }
}

class MenuBar {
  constructor(div) {
    this.div = div;
    this.div.classList.add('menubar');
    this.labels = [];
    this.menuShowing = null;
    this.docClick = ev => {
      console.log('doc click');
      this.hideMenu(); };
    document.addEventListener('click', this.docClick);
    this.div.addEventListener('click', ev => { ev.stopPropagation(); });
  }
  addLabel(label) {
    this.labels.push(label);
    this.div.appendChild(label.div);
    label.setOnClick(label => { this.labelClicked(label); });
    label.setOnHover(this.labelHover.bind(this));
  }
  showMenu(label) {
    this.menuShowing = label;
    for (let i = 0; i < this.labels.length; i++) {
      this.labels[i].setChildVisible(this.labels[i] === label);
    }
  }
  hideMenu() {
    if (!this.menuShowing)
      return;
    this.menuShowing.setChildVisible(false);
    this.menuShowing = null;
  }
  labelClicked(label) {
    if (this.menuShowing == label) {
      this.menuShowing = null;
      label.setChildVisible(false);
    } else {
      this.showMenu(label);
    }
  }
  labelHover(label, on) {
    if (!this.menuShowing || !on)
      return;
    // hovering on a menu, with a menu showing. Show what we're hoving on.
    this.showMenu(label);
  }
}

const populate2 = function(items, container) {
  for (let i = 0; i < items.length; i++) {
    const initem = items[i];
    console.log('handling ' + initem.name);
    let item = null;
    if (container instanceof MenuBar) {
      item = new MenuBarLabel(initem.name);
      container.addLabel(item);
    } else {
      item = new MenuItem(initem.name,
                          initem.hasOwnProperty('key') ? initem.key : null);
      container.addMenuItem(item);
    }
    if (initem.children) {
      let subcontainer = new MenuContainer();
      item.setContainer(subcontainer);
      populate2(initem.children, subcontainer);
    }
  }
}

let shortcutKeyToStr = function(key) {
  const code = key.charCodeAt(0);
  if (code > 64 && code < 91) {  // upper-case
    return "Ctrl+Shift+" + key.toUpperCase();
  } else {  // assume lower-case
    return "Ctrl+" + key.toUpperCase();
  }
};

let populateSubmenu = function(items, container) {
  let timerRunning = false;
  let timer = null;

  for (let i = 0; i < items.length; i++) {
    let item = items[i];
    let div = document.createElement("div");
    if (item.name == "-") {
      div.classList.add('menuspacer');
    } else {
      let left = document.createElement('div');
      left.classList.add('menuitemname');
      left.innerHTML = item.name;
      div.appendChild(left);
      if (item.children || item.key) {
        let right = document.createElement('div');
        right.classList.add('menuitemright');
        if (item.children) {
          right.innerHTML = '&#9658;';
        } else {
          right.innerHTML = shortcutKeyToStr(item.key);
        }
        div.appendChild(right);
      }
      div.classList.add('menuitem');
      div.addEventListener('click', function() {
	console.log('clicked');
      });
    }
    container.appendChild(div);
    if (item.hasOwnProperty('children')) {
      let container = document.createElement('div');
      container.classList.add('subsubmenucontainer');
      div.appendChild(container);
      var temp = function(){
        let timerRunning = false;
        let timer = -1;
        div.addEventListener('mouseenter', function() {
          if (timerRunning) {
            clearTimeout(timer);
            timerRunning = false;
          }
          timer = setTimeout(function() {
            container.style.display = 'block';
          }, 500);
          timerRunning = true;
        });
        div.addEventListener('mouseleave', function() {
          if (timerRunning) {
            clearTimeout(timer);
            timerRunning = false;
          }
          timer = setTimeout(function() {
            container.style.display = 'none';
          }, 500);
          timerRunning = true;
        });
      };
      temp();
      populateSubmenu(item.children, container);
    }
  }
}

let populateDom = function(items, rootcontainer) {
  let submenuvisible = -1;
  let labels = [];
  let submenus = [];
  const entermenuname = function(index) {
    if (submenuvisible == -1 || submenuvisible == index)
      return;
    submenus[submenuvisible].style.display = 'none';
    labels[submenuvisible].classList.remove('rootmenuitemactive');
    submenuvisible = index;
    submenus[submenuvisible].style.display = 'block';
    labels[submenuvisible].classList.add('rootmenuitemactive');
  }
  const submenuclick = function(index) {
    if (submenuvisible == -1 || submenuvisible != index) {
      if (submenuvisible != -1) {
        submenus[submenuvisible].style.display = 'none';
        labels[submenuvisible].classList.remove('rootmenuitemactive');
      }
      submenuvisible = index;
      submenus[submenuvisible].style.display = 'block';
      labels[submenuvisible].classList.add('rootmenuitemactive');
    } else {
      submenus[submenuvisible].style.display = 'none';
      labels[submenuvisible].classList.remove('rootmenuitemactive');
      submenuvisible = -1;
    }
  }
  for (let i = 0; i < items.length; i++) {
    let item = items[i];
    let div = document.createElement("div");
    div.innerHTML = item.name;
    div.classList.add('rootmenuitem');
    rootcontainer.appendChild(div);
    div.addEventListener('mouseenter', function() {
      entermenuname(i);
    });
    let container = document.createElement('div');
    container.classList.add('submenucontainer');
    div.appendChild(container);
    labels.push(div);
    submenus.push(container);
    populateSubmenu(item.children, container);
    div.addEventListener('click', function(ev) {
      submenuclick(i);
    });
    div.addEventListener('mousedown', function(ev) {
      ev.preventDefault();
    });
  }
}

const handleKeyPress = function(ev) {
  if (ev.metaKey || ev.altKey || !ev.ctrlKey)
    return;
  console.log(ev.type + " " + ev.bubbles + " " + ev.key);
  const shortcut = ev.shiftKey ? ev.key.toUpperCase() : ev.key;
  if (enabledKeyShortcuts.hasOwnProperty(shortcut)) {
    ev.preventDefault();
    enabledKeyShortcuts[shortcut]();
  }
};

document.addEventListener('DOMContentLoaded', function() {
  // create dom elements
  //populateDom(menus, document.getElementById('menubar'));
  let menuBar = new MenuBar(document.getElementById('menubar'));
  populate2(menus, menuBar);
  console.log(menuBar);
  let clicker = document.getElementById('mybutton');
  let clicknum = 0;
  clicker.addEventListener('click', function(ev) {
    clicknum++;
    document.getElementById('number').innerHTML = clicknum;
  });

  document.addEventListener('keydown', handleKeyPress);
}, false);


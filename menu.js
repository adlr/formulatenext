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

let shortcutKeyToStr = function(key) {
  const code = key.charCodeAt(0);
  if (code > 64 && code < 91) {  // upper-case
    return "Ctrl+Shift+" + key.toUpperCase();
  } else {  // assume lower-case
    return "Ctrl+" + key.toUpperCase();
  }
}

let populateSubmenu = function(items, container) {
  let submenuvisible = -1;
  let timerRunning = false;
  let timer = null;
  const onenter = function(childtoshow) {
    if (timerRunning) {
      clearTimeout(timer);
      timerRunning = false;
    }
    timer = setTimeout(function() {
      childtoshow.style.display = 'block';
    }, 500);
    timerRunning = true;
  };
  const onexit = function(childtohide) {
    if (timerRunning) {
      clearTimeout(timer);
      timerRunning = false;
    }
    timer = setTimeout(function() {
      childtohide.style.display = 'none';
    }, 500);
    timerRunning = true;
  }

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

document.addEventListener('DOMContentLoaded', function() {
  // create dom elements
  populateDom(menus, document.getElementById('menubar'));
}, false);


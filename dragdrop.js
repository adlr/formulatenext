let g_data = "";

document.addEventListener('DOMContentLoaded', function() {
  let source = document.getElementById('source');
  source.setAttribute('draggable', true);
  source.addEventListener('dragstart', function(ev) {
    console.log('dragstart');
    ev.dataTransfer.setData('application/pdf', g_data);

    // var url = URL.createObjectURL(new File([g_data], 'temp.pdf'));
    // console.log('url: ' + url);
    // ev.dataTransfer.setData("DownloadURL", "application/pdf:file_name.txt:" + url);
  });

  let sink = document.getElementById('sink');
  sink.addEventListener('dragover', function(ev) {
    console.log('ondragover');
    ev.preventDefault();
  });
  sink.addEventListener('drop', function(ev) {
    console.log('ondrop');
    ev.preventDefault();
    if (ev.dataTransfer.items) {
      for (var i = 0; i < ev.dataTransfer.items.length; i++) {
	console.log(ev.dataTransfer.items[i]);
	if (ev.dataTransfer.items[i].kind === 'file' &&
	   ev.dataTransfer.items[i].type === 'application/pdf') {
	  let file = ev.dataTransfer.items[i].getAsFile();
	  let reader = new FileReader();
	  reader.onload = function(el) {
	    let g_data = new Uint8Array(reader.result);
	    console.log('got a file of length ' + g_data.length);
	  };
	  reader.readAsArrayBuffer(file);
	} else if (ev.dataTransfer.items[i].kind === 'string' &&
		   ev.dataTransfer.items[i].type === 'application/pdf') {
	  console.log('got a string');
	  g_data = ev.dataTransfer.getData('application/pdf');
	  console.log(g_data);
	}
      }
    }


    // var data = ev.dataTransfer.getData("application/pdf");
    // console.log(data);
    // if (data) {
    //   g_data = data;
    // }
  });
}, false);

